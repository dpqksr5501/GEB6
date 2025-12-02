// Fill out your copyright notice in the Description page of Project Settings.


#include "Skills/Skill_Swift.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Engine/OverlapResult.h"
#include "KHU_GEBCharacter.h"

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

    if (AKHU_GEBCharacter* PlayerChar = Cast<AKHU_GEBCharacter>(Owner))
    {
        PlayerChar->OnSwiftStrikeStarted(this);
    }

    // ----- 시작/끝 위치 계산 -----
    const FVector StartLocation = Owner->GetActorLocation();

    FVector Forward = Owner->GetActorForwardVector();
    Forward.Z = 0.f;
    if (!Forward.Normalize()) { Forward = FVector::ForwardVector; }

    const FVector EndLocation = StartLocation + Forward * DashDistance;
    // 여기서 DashDistance가 0이네?

    // ----- 큰 직육면체(Oriented Box) 안의 적에게 데미지 -----
   // ----- 경로 상의 적(ACharacter) 수집용 박스 계산 -----
    const FVector Segment = EndLocation - StartLocation;
    const float Distance = Segment.Size();

    SwiftTargets.Reset();
    CurrentHitIndex = 0;
    // 여기서 Distance가 0이라.. 이유가 뭘까?

    if (Distance > KINDA_SMALL_NUMBER)
    {
        const FVector Direction = Segment / Distance;
        const FVector BoxCenter = StartLocation + 0.5f * Segment;
        const FQuat   BoxRotation = FRotationMatrix::MakeFromX(Direction).ToQuat();

        // X: 진행 방향, Y: 좌우, Z: 상하
        const float ExtraForwardPadding = 50.f; // 살짝 여유
        const FVector HalfExtent(
            Distance * 0.5f + ExtraForwardPadding,
            BoxHalfWidth,
            BoxHalfHeight);

        TArray<FOverlapResult> Overlaps;
        FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(SkillSwift), false, Owner);
        FCollisionObjectQueryParams ObjParams;
        ObjParams.AddObjectTypesToQuery(ECC_Pawn); // Pawn (적들)만 확인

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

                if (ACharacter* OtherChar = Cast<ACharacter>(Other))
                {
                    SwiftTargets.AddUnique(OtherChar);
                }
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

        if (AKHU_GEBCharacter* PlayerChar = Cast<AKHU_GEBCharacter>(Owner))
        {
            PlayerChar->OnSwiftStrikeEnded(this);
        }
    }
}

void USkill_Swift::StopSkill()
{
    UWorld* World = GetWorld();
    AActor* Owner = GetOwner();

    if (World)
    {
        World->GetTimerManager().ClearTimer(SwiftDamageTimerHandle);
    }

    if (AKHU_GEBCharacter* PlayerChar = Cast<AKHU_GEBCharacter>(Owner))
    {
        PlayerChar->OnSwiftStrikeEnded(this);
    }

    Super::StopSkill();
}

void USkill_Swift::HandleSwiftDamageTick()
{
    UWorld* World = GetWorld();
    AActor* Owner = GetOwner();
    if (!World || !Owner)
    {
        World->GetTimerManager().ClearTimer(SwiftDamageTimerHandle);

        if (AKHU_GEBCharacter* PlayerChar = Cast<AKHU_GEBCharacter>(Owner))
        {
            PlayerChar->OnSwiftStrikeEnded(this);
        }

        return;
    }

    ++CurrentHitIndex;

    // Hit 인덱스가 DamageSamples를 넘으면 종료
    if (CurrentHitIndex > DamageSamples)
    {
        World->GetTimerManager().ClearTimer(SwiftDamageTimerHandle);

        if (AKHU_GEBCharacter* PlayerChar = Cast<AKHU_GEBCharacter>(Owner))
        {
            PlayerChar->OnSwiftStrikeEnded(this);
        }

        UE_LOG(LogTemp, Log, TEXT("[Skill_Swift] Multi hit finished."));
        return;
    }

    // 타겟 목록을 돌면서 한 번씩 데미지
    for (int32 i = SwiftTargets.Num() - 1; i >= 0; --i)
    {
        ACharacter* TargetChar = SwiftTargets[i].Get();
        if (!TargetChar)
        {
            SwiftTargets.RemoveAtSwap(i);
            continue;
        }

        // 고정 피해, 방어력 무시, 1타씩 직접 넣기
        DealSkillDamage(
            TargetChar,
            DamagePerSample,
            /*bIgnoreDefense=*/true,
            /*bPeriodic=*/false,
            /*HitCount=*/1); // 우리 쪽에서 10번 반복 호출하므로 HitCount는 1로

        // Hit 이펙트
        if (HitNS)
        {
            UNiagaraFunctionLibrary::SpawnSystemAtLocation(
                World,
                HitNS,
                TargetChar->GetActorLocation(),
                TargetChar->GetActorRotation());
        }
    }

    // 더 이상 유효 타겟이 없으면 타이머 정리
    if (SwiftTargets.Num() == 0)
    {
        World->GetTimerManager().ClearTimer(SwiftDamageTimerHandle);

        if (AKHU_GEBCharacter* PlayerChar = Cast<AKHU_GEBCharacter>(Owner))
        {
            PlayerChar->OnSwiftStrikeEnded(this);
        }

        UE_LOG(LogTemp, Log, TEXT("[Skill_Swift] Multi hit stopped (no more targets)."));
    }
}
