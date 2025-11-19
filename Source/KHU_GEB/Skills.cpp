// Fill out your copyright notice in the Description page of Project Settings.


#include "Skills.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "GameFramework/Actor.h"
#include "Components/SkeletalMeshComponent.h"
//몽타주 재생에 필요한 헤더파일들입니다.
#include "GameFramework/Character.h"
#include "Animation/AnimInstance.h"
#include "FormManagerComponent.h"
#include "FormDefinition.h"


/*=============================Base=============================*/
/** Base 폼 스킬: 몽타주 재생 + State 변경 */
void USkill_Base::ActivateSkill()
{
    Super::ActivateSkill(); // 로그 출력

    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    if (!OwnerCharacter) return;

    //폼 정의(DA)에서 SkillMontage 가져오기
    UFormManagerComponent* FormManager = OwnerCharacter->FindComponentByClass<UFormManagerComponent>();
    if (!FormManager) return; // [수정] State 복구 로직 삭제

    const UFormDefinition* CurrentDef = FormManager->FindDef(FormManager->CurrentForm);
    if (!CurrentDef || !CurrentDef->SkillMontage)
    {
        UE_LOG(LogTemp, Warning, TEXT("[USkill_Base] SkillMontage is NOT assigned..."));

        return;
    }

    //몽타주 재생
    UAnimInstance* AnimInstance = OwnerCharacter->GetMesh()->GetAnimInstance();
    if (AnimInstance && !AnimInstance->Montage_IsPlaying(CurrentDef->SkillMontage))
    {
        AnimInstance->Montage_Play(CurrentDef->SkillMontage);
    }
}

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


/*=============================Guard=============================*/
// Skill_Shield.cpp
void USkill_Guard::ActivateSkill()
{
    //원래 기존 코드 입니다.
    //if (AActor* Owner = GetOwner())
    //{
    //    Owner->SetCanBeDamaged(false);   // 임시 면역
    //    if (SkillNS)
    //    {
    //        SpawnedNS = UNiagaraFunctionLibrary::SpawnSystemAttached(
    //            SkillNS, Owner->GetRootComponent(), NAME_None,
    //            FVector::ZeroVector, FRotator::ZeroRotator,
    //            EAttachLocation::SnapToTarget, true);
    //    }
    //    Super::ActivateSkill();
    //    GetWorld()->GetTimerManager().SetTimer(DurationHandle, this, &USkill_Guard::StopSkill, Duration, false);
    //}


    AActor* Owner = GetOwner();
    if (!Owner) return;

    // 1. [Guard] 무적 활성화 (C++ 로직)
    Owner->SetCanBeDamaged(false);


    // 3. [Guard] 이펙트 재생 (기존 코드)
    if (SkillNS)
    {
        SpawnedNS = UNiagaraFunctionLibrary::SpawnSystemAttached(
            SkillNS, Owner->GetRootComponent(), NAME_None,
            FVector::ZeroVector, FRotator::ZeroRotator,
            EAttachLocation::SnapToTarget, true);
    }
    Super::ActivateSkill(); // 로그 출력

    // 4. [Guard] 3초(Duration) 뒤 StopSkill 호출 (기존 코드)
    GetWorld()->GetTimerManager().SetTimer(DurationHandle, this, &USkill_Guard::StopSkill, Duration, false);
}

void USkill_Guard::StopSkill()
{
    //기존 코드입니다.
    /*if (AActor* Owner = GetOwner()) Owner->SetCanBeDamaged(true);
    if (SpawnedNS) { SpawnedNS->Deactivate(); SpawnedNS = nullptr; }
    GetWorld()->GetTimerManager().ClearTimer(DurationHandle);

    Super::StopSkill();*/

    AActor* Owner = GetOwner();
    if (Owner)
    {
        // 1. [Guard] 무적 해제
        Owner->SetCanBeDamaged(true);
    }

    // 3.이펙트 및 타이머 정리 (기존 코드)
    if (SpawnedNS) { SpawnedNS->Deactivate(); SpawnedNS = nullptr; }
    GetWorld()->GetTimerManager().ClearTimer(DurationHandle);

    Super::StopSkill();
}

/*=============================Special=============================*/