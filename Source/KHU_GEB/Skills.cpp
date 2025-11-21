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
// Skill_Shield.cpp
void USkill_Guard::ActivateSkill()
{
    if (AActor* Owner = GetOwner())
    {
        Owner->SetCanBeDamaged(false);   // 임시 면역
        if (SkillNS)
        {
            SpawnedNS = UNiagaraFunctionLibrary::SpawnSystemAttached(
                SkillNS, Owner->GetRootComponent(), NAME_None,
                FVector::ZeroVector, FRotator::ZeroRotator,
                EAttachLocation::SnapToTarget, true);
        }
        Super::ActivateSkill();
        GetWorld()->GetTimerManager().SetTimer(DurationHandle, this, &USkill_Guard::StopSkill, Duration, false);
    }
}

void USkill_Guard::StopSkill()
{
    if (AActor* Owner = GetOwner()) Owner->SetCanBeDamaged(true);
    if (SpawnedNS) { SpawnedNS->Deactivate(); SpawnedNS = nullptr; }
    GetWorld()->GetTimerManager().ClearTimer(DurationHandle);

    Super::StopSkill();
}

/*=============================Special=============================*/