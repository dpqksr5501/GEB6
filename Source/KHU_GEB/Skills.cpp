// Fill out your copyright notice in the Description page of Project Settings.


#include "Skills.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "GameFramework/Actor.h"
#include "Components/SkeletalMeshComponent.h"

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
        // 채널 시작
        GetWorld()->GetTimerManager().SetTimer(TickHandle, this, &USkill_Range::TickBreath, TickInterval, true);
        GetWorld()->GetTimerManager().SetTimer(DurationHandle, this, &USkill_Range::StopSkill, MaxDuration, false);

        Super::ActivateSkill();
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
        GetWorld()->GetTimerManager().SetTimer(DurationHandle, this, &USkill_Guard::StopSkill, Duration, false);
    }
}

void USkill_Guard::StopSkill()
{
    if (AActor* Owner = GetOwner()) Owner->SetCanBeDamaged(true);
    if (SpawnedNS) { SpawnedNS->Deactivate(); SpawnedNS = nullptr; }
    GetWorld()->GetTimerManager().ClearTimer(DurationHandle);
}

/*=============================Special=============================*/