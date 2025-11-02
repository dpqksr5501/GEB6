// Fill out your copyright notice in the Description page of Project Settings.


#include "MonsterAnimInstanceBase.h"
#include "Animation/AnimInstance.h"
#include "MonsterBase.h"
#include "Kismet/KismetMathLibrary.h"

void UMonsterAnimInstanceBase::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	OwningMonster = Cast<AMonsterBase>(TryGetPawnOwner());
	if (OwningMonster) {
		LastYaw = OwningMonster->GetActorRotation().Yaw;
	}
	YawDeltaSpeed = 0.f;
}

void UMonsterAnimInstanceBase::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (OwningMonster)
	{
		Speed = OwningMonster->GetVelocity().Size();
		CharacterState = OwningMonster->GetCharacterState();

        if (DeltaSeconds > 0.f) // 0으로 나누기 방지
        {
            FRotator CurrentRotation = OwningMonster->GetActorRotation();
            // 두 각도 사이의 최단 거리 각도 차이를 구합니다.
            const float DeltaYaw = FMath::FindDeltaAngleDegrees(LastYaw, CurrentRotation.Yaw);

            // 초당 회전 속도 (DeltaYaw / DeltaSeconds)
            const float TargetYawDeltaSpeed = DeltaYaw / DeltaSeconds;

            // 값을 부드럽게 보간(Interp)하여 급격한 변화를 막습니다.
            YawDeltaSpeed = FMath::FInterpTo(YawDeltaSpeed, TargetYawDeltaSpeed, DeltaSeconds, 6.0f); // 6.0f는 보간 속도(조정 필요)

            LastYaw = CurrentRotation.Yaw; // 다음 프레임을 위해 현재 Yaw 값 저장

            // 1. 카메라(컨트롤러)가 바라보는 회전 값
            const FRotator ControlRotation = OwningMonster->GetControlRotation();
            // 2. 캐릭터 메시(액터)가 실제 향하고 있는 회전 값
            const FRotator ActorRotation = OwningMonster->GetActorRotation();

            // 3. 두 회전 값의 차이를 계산 (NormalizedDeltaRotator로 -180~180 범위의 최단 각도 차이를 구함)
            const FRotator DeltaRotation = UKismetMathLibrary::NormalizedDeltaRotator(ControlRotation, ActorRotation);

            // 4. 목표 값 (Pitch, Yaw)
            const float TargetAimPitch = DeltaRotation.Pitch;
            const float TargetAimYaw = DeltaRotation.Yaw;

            // 5. 부드럽게 값을 보간(Interp)하여 AimPitch, AimYaw에 저장
            // (6.0f는 보간 속도이며, 테스트하며 조절하세요)
            AimPitch = FMath::FInterpTo(AimPitch, TargetAimPitch, DeltaSeconds, 6.0f);
            AimYaw = FMath::FInterpTo(AimYaw, TargetAimYaw, DeltaSeconds, 6.0f);
        }

        // 캐릭터가 멈춰있으면 기울임도 서서히 0으로 복귀
        if (Speed < 10.f)
        {
            YawDeltaSpeed = FMath::FInterpTo(YawDeltaSpeed, 0.f, DeltaSeconds, 4.0f);
        }
	}
    else
    {
        // ... (기존 OwningMonster가 없을 때의 로직) ...
        AimPitch = 0.f;
        AimYaw = 0.f;
    }

   
}