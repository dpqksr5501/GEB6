// Fill out your copyright notice in the Description page of Project Settings.


#include "Skills/Skill_Swift.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Engine/OverlapResult.h"
#include "Kismet/GameplayStatics.h"
#include "KHU_GEBCharacter.h"
#include "SkillManagerComponent.h"

void USkill_Swift::InitializeFromDefinition(const USkillDefinition* Def)
{
    Super::InitializeFromDefinition(Def);

    if (Params.Damage > 0.f) { DamagePerSample = Params.Damage; }
    if (Params.Range > 0.f) { DashDistance = Params.Range; }
}

bool USkill_Swift::CanActivate() const
{
    // 지금은 별도 제약 없이 기본 조건만 사용
    return Super::CanActivate();
}

void USkill_Swift::ActivateSkill()
{
    UWorld* World = GetWorld();
    AActor* Owner = GetOwner();
    if (!World || !Owner) return;

    if (!CanActivate())
    {
        UE_LOG(LogTemp, Warning, TEXT("[Skill_Swift] ActivateSkill blocked (CanActivate=false)"));
        return;
    }

    //애니메이션 몽타주 재생
    PlayFormSkillMontage();
    // 쿨타임 시작 + 마나 1회 소모.
    Super::ActivateSkill();

    if (USkillManagerComponent* Manager = GetSkillManager())
    {
        Manager->OnSwiftStrikeStarted(this);
    }

    // ----- 시작/끝 위치 계산 -----
    const FVector StartLocation = Owner->GetActorLocation();

    FVector Forward = Owner->GetActorForwardVector();
    Forward.Z = 0.f;
    if (!Forward.Normalize()) { Forward = FVector::ForwardVector; }

    const FVector EndLocation = StartLocation + Forward * DashDistance;

    // ----- 큰 직육면체(Oriented Box) 안의 적에게 데미지 -----
    const FVector Segment = EndLocation - StartLocation;
    const float Distance = Segment.Size();

    SwiftTargets.Reset();
    CurrentHitIndex = 0;

    // ----- 경로 상의 적(ACharacter) 수집용 박스 계산 -----
    if (Distance > KINDA_SMALL_NUMBER)
    {
        const FVector Direction = Segment / Distance;
        const FVector BoxCenter = StartLocation + 0.5f * Segment;
        const FQuat   BoxRotation = FRotationMatrix::MakeFromX(Direction).ToQuat();

        const float ExtraForwardPadding = 50.f;
        const FVector HalfExtent(
            Distance * 0.5f + ExtraForwardPadding,
            BoxHalfWidth,
            BoxHalfHeight);

        TArray<FOverlapResult> Overlaps;
        FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(SkillSwift), false, Owner);
        FCollisionObjectQueryParams ObjParams;
        ObjParams.AddObjectTypesToQuery(ECC_Pawn);

        const bool bAnyHit = World->OverlapMultiByObjectType(
            Overlaps,
            BoxCenter,
            BoxRotation,
            ObjParams,
            FCollisionShape::MakeBox(HalfExtent),
            QueryParams);

        if (bAnyHit)
        {
            for (const FOverlapResult& O : Overlaps)
            {
                AActor* Other = O.GetActor();
                if (!Other || Other == Owner) continue;

                // 이미 추가된 타겟이면 스킵 (중복 제거)
                if (SwiftTargets.Contains(Other)) continue;

                SwiftTargets.Add(Other);
            }
        }

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
        DrawDebugBox(World, BoxCenter, HalfExtent, BoxRotation, FColor::Cyan, false, 0.2f);
#endif
    }

    // ----- 실제 점멸 이동 (기존 코드 그대로) -----
    if (ACharacter* OwnerChar = Cast<ACharacter>(Owner))
    {
        if (UCapsuleComponent* Capsule = OwnerChar->GetCapsuleComponent())
        {
            const ECollisionResponse OriginalPawnResponse =
                Capsule->GetCollisionResponseToChannel(ECC_Pawn);

            Capsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);

            FHitResult Hit;
            Owner->SetActorLocation(EndLocation, true, &Hit, ETeleportType::TeleportPhysics);

            Capsule->SetCollisionResponseToChannel(ECC_Pawn, OriginalPawnResponse);
        }
        else { Owner->SetActorLocation(EndLocation); }
    }
    else { Owner->SetActorLocation(EndLocation); }

    // ----- 수집된 타겟들을 DamageSamples번에 걸쳐 때리기 -----
    if (SwiftTargets.Num() > 0 && DamageSamples > 0)
    {
        const float Interval = FMath::Max(HitInterval, KINDA_SMALL_NUMBER);
        World->GetTimerManager().SetTimer(
            SwiftDamageTimerHandle,
            this,
            &USkill_Swift::HandleSwiftDamageTick,
            Interval,
            true);

        UE_LOG(LogTemp, Log,
            TEXT("[Skill_Swift] Start multi hit: Targets=%d, Hits=%d, Interval=%.2f"),
            SwiftTargets.Num(), DamageSamples, Interval);
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("[Skill_Swift] No targets found for multi hit."));

        if (USkillManagerComponent* Manager = GetSkillManager())
        {
            Manager->OnSwiftStrikeEnded(this);
        }
    }
}

void USkill_Swift::StopSkill()
{
    // Swift는 입력 해제와 상관없이 남은 타수를 다 소진하도록
    // 여기서는 타이머를 건드리지 않습니다.
    // (타이머 종료와 OnSwiftStrikeEnded 호출은 HandleSwiftDamageTick에서만 처리)

    Super::StopSkill();
}

void USkill_Swift::HandleSwiftDamageTick()
{
    UWorld* World = GetWorld();
    AActor* Owner = GetOwner();
    if (!World || !Owner)
    {
        World->GetTimerManager().ClearTimer(SwiftDamageTimerHandle);

        if (USkillManagerComponent* Manager = GetSkillManager())
        {
            Manager->OnSwiftStrikeEnded(this);
        }

        return;
    }

    ++CurrentHitIndex;

    // Hit 인덱스가 DamageSamples(=10)를 넘으면 종료
    if (CurrentHitIndex > DamageSamples)
    {
        World->GetTimerManager().ClearTimer(SwiftDamageTimerHandle);

        if (USkillManagerComponent* Manager = GetSkillManager())
        {
            Manager->OnSwiftStrikeEnded(this);
        }

        UE_LOG(LogTemp, Log, TEXT("[Skill_Swift] Multi hit finished."));
        return;
    }

    // 타겟 목록을 돌면서 한 번씩 데미지 넣기
    for (int32 i = SwiftTargets.Num() - 1; i >= 0; --i)
    {
        AActor* TargetActor = SwiftTargets[i].Get();
        if (!TargetActor)
        {
            SwiftTargets.RemoveAtSwap(i);
            continue;
        }

        if (DamagePerSample <= 0.f) continue;

        // InstigatorController 계산
        AController* InstigatorController = nullptr;
        if (APawn* PawnOwner = Cast<APawn>(Owner))
        {
            InstigatorController = PawnOwner->GetController();
        }

        // ApplyDamage 파이프라인
        UGameplayStatics::ApplyDamage(
            TargetActor,
            DamagePerSample,
            InstigatorController,
            Owner,
            UDamageType::StaticClass());

        UE_LOG(LogTemp, Log,
            TEXT("[Skill_Swift] Tick %d -> Hit %s for %.1f"),
            CurrentHitIndex,
            *GetNameSafe(TargetActor),
            DamagePerSample);

        // Actor 기준으로 이펙트
        if (HitNS)
        {
            UNiagaraFunctionLibrary::SpawnSystemAtLocation(
                World,
                HitNS,
                TargetActor->GetActorLocation(),
                TargetActor->GetActorRotation());
        }
    }

    // 더 이상 유효 타겟이 없으면 타이머 정리
    if (SwiftTargets.Num() == 0)
    {
        World->GetTimerManager().ClearTimer(SwiftDamageTimerHandle);

        if (USkillManagerComponent* Manager = GetSkillManager())
        {
            Manager->OnSwiftStrikeEnded(this);
        }

        UE_LOG(LogTemp, Log, TEXT("[Skill_Swift] Multi hit stopped (no more targets)."));
    }
}
