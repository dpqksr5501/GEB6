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
#include "DrawDebugHelpers.h"

/*=============================Base=============================*/


/*=============================Range=============================*/
void USkill_Range::ActivateSkill()
{
    if (AActor* Owner = GetOwner())
    {
        // FX: 메쉬의 입 소켓에 부착
        if (SkillNS)
        {
            if (USkeletalMeshComponent* Skel = Owner->FindComponentByClass<USkeletalMeshComponent>())
            {
                SpawnedNS = UNiagaraFunctionLibrary::SpawnSystemAttached(
                    SkillNS, Skel, MouthSocket, FVector::ZeroVector, FRotator::ZeroRotator,
                    EAttachLocation::SnapToTarget, true);
            }
        }
        Super::ActivateSkill();

        // 채널 시작
        GetWorld()->GetTimerManager().SetTimer(TickHandle, this, &USkill_Range::TickBreath, TickInterval, true);
        GetWorld()->GetTimerManager().SetTimer(DurationHandle, this, &USkill_Range::StopSkill, MaxDuration, false);
    }
}

void USkill_Range::StopSkill()
{
    GetWorld()->GetTimerManager().ClearTimer(TickHandle);
    GetWorld()->GetTimerManager().ClearTimer(DurationHandle);
    if (SpawnedNS) { SpawnedNS->Deactivate(); SpawnedNS = nullptr; }

    Super::StopSkill();
}

void USkill_Range::TickBreath()
{
    // 콘 트레이스: 전방으로 여러 라인/스피어 트레이스 or Overlap
    AActor* Owner = GetOwner();
    if (!Owner) return;

    const FVector Start = Owner->GetActorLocation();
    const FVector Fwd = Owner->GetActorForwardVector();
    const float Range = Params.Range; //  :contentReference[oaicite:6]{index=6}

    // 간단하게: 원뿔 안의 액터들 필터링 (의사코드)
    TArray<FHitResult> Hits;
    // ... 라인트레이스 여러 번 or SphereOverlapActors 후 각도 필터
    for (const FHitResult& H : Hits)
    {
        if (AActor* HitA = H.GetActor())
        {
            const FVector Dir = (HitA->GetActorLocation() - Start).GetSafeNormal();
            const float Angle = FMath::RadiansToDegrees(acosf(FVector::DotProduct(Fwd, Dir)));
            if (Angle <= ConeAngleDeg)
            {
                // DamagePerTick = Params.Damage * TickInterval (혹은 고정값)
                // 적 데미지 처리(임시): UGameplayStatics::ApplyDamage(HitA, DamagePerTick, ...)
            }
        }
    }
}

/*=============================Swift=============================*/

void USkill_Swift::InitializeFromDefinition(const USkillDefinition* Def)
{
    Params = Def ? Def->Params : FSkillParams{};

    // SkillDefinition.Params.Range가 있으면 대쉬 거리로 사용
    if (Params.Range > 0.f)
    {
        DashDistance = Params.Range;
    }
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
    if (!World || !Owner)
    {
        return;
    }

    if (!CanActivate())
    {
        UE_LOG(LogTemp, Warning, TEXT("[Skill_Swift] ActivateSkill blocked (CanActivate=false)"));
        return;
    }

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

void USkill_Guard::ActivateSkill()
{
    UWorld* World = GetWorld();
    AActor* Owner = GetOwner();
    if (!World || !Owner)
    {
        return;
    }

    if (!CanActivate())
    {
        UE_LOG(LogTemp, Warning, TEXT("[Skill_Guard] ActivateSkill blocked (CanActivate=false)"));
        return;
    }

    Super::ActivateSkill();

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

void USkill_Guard::StopSkill()
{
    // 이미 꺼져 있으면 한 번 더 호출돼도 아무 것도 안 함
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

    // 4. 보호막이 모두 없어지기 전에(=소진되기 전에) 우클릭을 해제했다면 반격 데미지
    if (!bEndedByDepletion && ConsumedShields > 0 && World && Owner)
    {
        const float TotalDamage = Params.Damage * ConsumedShields;
        if (TotalDamage > 0.f)
        {
            TArray<AActor*> IgnoreActors;
            IgnoreActors.Add(Owner);

            UE_LOG(LogTemp, Log, TEXT("[Skill_Guard] Releasing: Consumed=%d, TotalDamage=%.1f"),
                ConsumedShields, TotalDamage);

            UGameplayStatics::ApplyRadialDamage(
                World,
                TotalDamage,                          // 보호막이 막은 히트 수 × Params.Damage
                Owner->GetActorLocation(),
                ExplosionRadius,
                nullptr,                              // 기본 DamageType
                IgnoreActors,
                Owner,
                Owner->GetInstigatorController(),
                true                                  // Do full damage
            );
        }
    }
    else
    {
        if (bEndedByDepletion)
        {
            UE_LOG(LogTemp, Log, TEXT("[Skill_Guard] Shields depleted. Skill ended without retaliation."));
        }
        else
        {
            UE_LOG(LogTemp, Log, TEXT("[Skill_Guard] Stopped with no shields consumed."));
        }
    }

    // 상태 리셋
    bIsActive = false;
    RemainingShields = 0;
    ConsumedShields = 0;
    bEndedByDepletion = false;

    Super::StopSkill();
}

bool USkill_Guard::HandleIncomingDamage(float Damage,
    const UDamageType* DamageType,
    AController* InstigatedBy,
    AActor* DamageCauser)
{
    // 스킬이 켜져 있지 않거나, 이미 보호막이 없는 경우엔 그냥 데미지를 먹게 둔다.
    if (!bIsActive || RemainingShields <= 0)
    {
        return false;
    }

    // 한 번의 데미지를 한 장의 보호막으로 완전히 막음
    RemainingShields = FMath::Max(RemainingShields - 1, 0);
    ConsumedShields++;

    UE_LOG(LogTemp, Log, TEXT("[Skill_Guard] Damage absorbed. RemainingShields=%d, Consumed=%d"),
        RemainingShields, ConsumedShields);

    // 마지막 보호막까지 사용했다면, 조건 3: 스킬만 해제되고 반격 데미지는 없음
    if (RemainingShields <= 0)
    {
        bEndedByDepletion = true;
        StopSkill(); // 여기서 bIsActive가 false로 바뀌고 상태 초기화됨(반격 없음)
    }

    // true를 반환하면 캐릭터 쪽에서 실제 체력 감소 처리를 하지 않도록 할 것임
    return true;
}

/*=============================Special=============================*/