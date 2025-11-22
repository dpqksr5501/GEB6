// Fill out your copyright notice in the Description page of Project Settings.


#include "Skills.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "GameFramework/Actor.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/Character.h"
#include "ManaComponent.h"
#include "DrawDebugHelpers.h"

/*=============================Base=============================*/


/*=============================Range=============================*/


/*=============================Swift=============================*/

void USkill_Swift::InitializeFromDefinition(const USkillDefinition* Def)
{
    Params = Def ? Def->Params : FSkillParams{};

    // SkillDefinition.Params.Range가 있으면 대쉬 거리로 사용
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

    // 쿨타임 시작 + 마나 1회 소모.
    Super::ActivateSkill();

    // ----- 시작/끝 위치 계산 -----
    const FVector StartLocation = Owner->GetActorLocation();

    FVector Forward = Owner->GetActorForwardVector();
    Forward.Z = 0.f;
    if (!Forward.Normalize())
    {
        Forward = FVector::ForwardVector;
    }

    const FVector EndLocation = StartLocation + Forward * DashDistance;

    // ----- 큰 직육면체(Oriented Box) 안의 적에게 데미지 -----
    const FVector Segment = EndLocation - StartLocation;
    const float Distance = Segment.Size();

    if (Distance > KINDA_SMALL_NUMBER)
    {
        const FVector Direction = Segment / Distance;
        const FVector BoxCenter = StartLocation + 0.5f * Segment;
        const FQuat   BoxRotation = FRotationMatrix::MakeFromX(Direction).ToQuat();

        // X: 진행 방향, Y: 좌우, Z: 상하
        const float ExtraForwardPadding = 50.f; // 살짝 여유
        const FVector HalfExtent(Distance * 0.5f + ExtraForwardPadding,
            BoxHalfWidth,
            BoxHalfHeight);

        const int32 NumSamples = FMath::Max(DamageSamples, 1);

        for (int32 i = 0; i < NumSamples; ++i)
        {
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
                    if (!Other || Other == Owner)
                    {
                        continue;
                    }

                    // ACharacter를 상속받는 적에게 데미지
                    if (Cast<ACharacter>(Other))
                    {
                        UGameplayStatics::ApplyDamage(
                            Other,
                            Params.Damage,                         // SkillDefinition의 Damage
                            Owner->GetInstigatorController(),
                            Owner,
                            nullptr);
                    }
                }
            }

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
            // 디버그: 만들어지는 직육면체 확인
            DrawDebugBox(World, BoxCenter, HalfExtent, BoxRotation, FColor::Cyan, false, 0.2f);
#endif
        }
    }

    // ----- 실제 점멸 이동: 적(ACharacter)을 통과하도록 Pawn 충돌 무시 -----
    if (ACharacter* OwnerChar = Cast<ACharacter>(Owner))
    {
        if (UCapsuleComponent* Capsule = OwnerChar->GetCapsuleComponent())
        {
            const ECollisionResponse OriginalPawnResponse =
                Capsule->GetCollisionResponseToChannel(ECC_Pawn);

            // 적(=Pawn)과의 충돌은 무시 (통과)
            Capsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);

            FHitResult Hit;
            Owner->SetActorLocation(EndLocation, true, &Hit, ETeleportType::TeleportPhysics);

            // 원래 설정 복원
            Capsule->SetCollisionResponseToChannel(ECC_Pawn, OriginalPawnResponse);
        }
        else
        {
            // 캡슐이 없으면 그냥 스윕 없이 텔레포트
            Owner->SetActorLocation(EndLocation, false, nullptr, ETeleportType::TeleportPhysics);
        }
    }
    else
    {
        // 캐릭터가 아니면 간단히 이동
        Owner->SetActorLocation(EndLocation, false, nullptr, ETeleportType::TeleportPhysics);
    }

    // 한 번에 끝나는 스킬이라 바로 종료
    StopSkill();
}

void USkill_Swift::StopSkill()
{
    Super::StopSkill();
}

/*=============================Guard=============================*/

void USkill_Guard::InitializeFromDefinition(const USkillDefinition* Def)
{
    // 공통 파라미터( Damage, Range, ManaCost, Cooldown ) 로드
    Super::InitializeFromDefinition(Def);

    // Range를 기본 폭발 반경으로 사용 (원하면 에디터에서 덮어쓰기)
    if (Params.Range > 0.f) { ExplosionRadius = Params.Range; }
}

bool USkill_Guard::CanActivate() const
{
    if (bIsActive)
    {
        UE_LOG(LogTemp, Verbose, TEXT("[Skill_Guard] CanActivate? false (already active)"));
        return false;
    }

    UManaComponent* Mana = GetManaComponent();
    if (!Mana)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Skill_Guard] CanActivate? false (no ManaComponent)"));
        return false;
    }

    const float Current = Mana->GetCurrentMana();
    const float Target = 100.f;
    const float Tolerance = 0.1f; // float 오차 방지용

    if (FMath::Abs(Current - Target) > Tolerance)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[Skill_Guard] CanActivate? false (Mana must be 100. Current=%.1f)"),
            Current);
        return false;
    }

    // 쿨타임은 지금은 사용하지 않음 (필요하면 여기서 Now < NextAvailableTime 체크 추가)
    return true;
}

void USkill_Guard::ActivateSkill()
{
    // 이미 켜져 있으면 다시 초기화하지 않고 무시
    if (bIsActive)
    {
        UE_LOG(LogTemp, Verbose,
            TEXT("[Skill_Guard] ActivateSkill called while already active. Ignoring."));
        return;
    }

    UWorld* World = GetWorld();
    AActor* Owner = GetOwner();
    if (!World || !Owner) return;

    bIsActive = true;
    bEndedByDepletion = false;

    RemainingShields = FMath::Max(MaxShields, 0);
    ConsumedShields = 0;

    // 보호막 이펙트 켜기
    if (SkillNS && !SpawnedNS)
    {
        SpawnedNS = UNiagaraFunctionLibrary::SpawnSystemAttached(
            SkillNS,
            Owner->GetRootComponent(),
            NAME_None,
            FVector::ZeroVector,
            FRotator::ZeroRotator,
            EAttachLocation::SnapToTarget,
            true);
    }

    UE_LOG(LogTemp, Log, TEXT("[Skill_Guard] Activated: MaxShields=%d"), RemainingShields);
}

bool USkill_Guard::HandleIncomingDamage(
    float Damage,
    const UDamageType* DamageType,
    AController* InstigatedBy,
    AActor* DamageCauser)
{
    // 스킬이 꺼져 있거나, 남은 배리어가 없으면 그냥 맞게 둔다.
    if (!bIsActive || RemainingShields <= 0)
    {
        UE_LOG(LogTemp, Verbose,
            TEXT("[Skill_Guard] HandleIncomingDamage: no shield (Active=%d, Remaining=%d)"),
            bIsActive ? 1 : 0, RemainingShields);
        return false;
    }

    // 배리어 1개 소모
    RemainingShields = FMath::Max(RemainingShields - 1, 0);
    ConsumedShields++;

    UE_LOG(LogTemp, Log,
        TEXT("[Skill_Guard] Damage absorbed. RemainingShields=%d, Consumed=%d"),
        RemainingShields, ConsumedShields);

    // 여기서는 배리어가 0이 되어도 스킬을 자동 종료하지 않습니다.
    // (요구사항: 스킬 종료는 "우클릭 해제" 시점만 담당)

    // true를 반환하면 Character::HandleAnyDamage 쪽에서 체력 감소를 하지 않음 :contentReference[oaicite:4]{index=4}
    return true;
}

void USkill_Guard::StopSkill()
{
    // 이미 꺼져 있으면 무시
    if (!bIsActive)
    {
        Super::StopSkill();
        return;
    }

    UWorld* World = GetWorld();
    AActor* Owner = GetOwner();

    // 이펙트 끄기
    if (SpawnedNS)
    {
        SpawnedNS->Deactivate();
        SpawnedNS = nullptr;
    }

    // --- 3-1. 마나를 0으로 설정 ---
    if (UManaComponent* Mana = GetManaComponent())
    {
        const float Current = Mana->GetCurrentMana();
        if (Current > 0.f)
        {
            // 현재 마나만큼 한 번에 소모 → 0이 됨
            Mana->ConsumeMana(Current);
        }
        UE_LOG(LogTemp, Log,
            TEXT("[Skill_Guard] Mana set to 0 on skill end (was %.1f)"), Current);
    }

    // --- 3-2. 배리어가 소모된 만큼 광역 대미지 ---
    if (World && Owner && ConsumedShields > 0 && Params.Damage > 0.f)
    {
        const float TotalDamage = Params.Damage * ConsumedShields;

        TArray<AActor*> IgnoreActors;
        IgnoreActors.Add(Owner);

        UE_LOG(LogTemp, Log,
            TEXT("[Skill_Guard] Stop: ConsumedShields=%d, TotalDamage=%.1f"),
            ConsumedShields, TotalDamage);

        UGameplayStatics::ApplyRadialDamage(
            World,
            TotalDamage,
            Owner->GetActorLocation(),
            ExplosionRadius,
            nullptr,            // Default DamageType
            IgnoreActors,
            Owner,
            Owner->GetInstigatorController(),
            true);              // Do full damage
    }
    else
    {
        UE_LOG(LogTemp, Log,
            TEXT("[Skill_Guard] Stop: No shields consumed or no damage value. No explosion."));
    }

    // 상태 리셋
    bIsActive = false;
    RemainingShields = 0;
    ConsumedShields = 0;
    bEndedByDepletion = false;

    Super::StopSkill();
}

/*=============================Special=============================*/