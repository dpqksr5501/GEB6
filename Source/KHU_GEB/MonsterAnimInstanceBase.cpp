// Fill out your copyright notice in the Description page of Project Settings.

#include "MonsterAnimInstanceBase.h"
#include "Animation/AnimInstance.h"
#include "MonsterBase.h"
#include "Kismet/KismetMathLibrary.h" // NormalizedDeltaRotator 같은 수학 함수를 위해서
#include "MyAnimDataProvider.h" // 1. 인터페이스 헤더 포함
#include "GameFramework/CharacterMovementComponent.h"

// 애니메이션 인스턴스가 처음 생성될 때 한 번 호출됩니다. (BeginPlay와 유사)
void UMonsterAnimInstanceBase::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation(); // 부모 클래스의 초기화 함수 호출

	// 이 애니메이션 인스턴스를 소유한 폰(캐릭터)을 가져와서 AMonsterBase 타입으로 변환합니다.
	APawn* OwnerPawn = TryGetPawnOwner();
	if (OwnerPawn)
	{
		// TScriptInterface는 APawn*에서 자동 캐스팅됩니다.
		// OwnerPawn이 IMyAnimDataProvider를 구현했다면 유효한 값이 됩니다.
		OwningDataProvider = OwnerPawn;
		LastYaw = OwnerPawn->GetActorRotation().Yaw;
	}
	// Yaw 회전 속도를 0으로 초기화합니다.
	YawDeltaSpeed = 0.f;
	Speed = 0.f;
	bJumpInput_Anim = false;
	bIsFalling = false;
}

// 매 프레임마다 호출되어 애니메이션 변수들을 업데이트합니다. (Tick과 유사)
void UMonsterAnimInstanceBase::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds); // 부모 클래스의 업데이트 함수 호출

    APawn* OwnerPawn = TryGetPawnOwner(); // YawDeltaSpeed 계산을 위해 Pawn 레퍼런스 유지

    //OwningDataProvider (인터페이스)가 유효한지 확인합니다.
    if (OwningDataProvider)
    {
        // 4. 인터페이스 함수를 통해 데이터를 가져옵니다.
        // Execute_ 접두사를 사용하거나 -> 연산자를 직접 사용합니다. (TScriptInterface 방식)
        Speed = IMyAnimDataProvider::Execute_GetAnimSpeed(OwningDataProvider.GetObject());
        CharacterState = IMyAnimDataProvider::Execute_GetAnimCharacterState(OwningDataProvider.GetObject());
        bIsFalling = IMyAnimDataProvider::Execute_GetAnimIsFalling(OwningDataProvider.GetObject());
        // 'true'를 전달하여 bJumpInput/bPlayerWantsToJump 값을 소모(리셋)시킵니다.
        bJumpInput_Anim = IMyAnimDataProvider::Execute_GetAnimJumpInput(OwningDataProvider.GetObject(), true);
    }
    else if (OwnerPawn) // OwnerPawn은 APawn* 타입입니다.
    {
        // (안전장치) 인터페이스가 없으면, ACharacter인지 확인하고 기본값을 가져옵니다.

        // GetVelocity()는 AActor의 함수이므로 APawn*에서도 안전하게 호출 가능합니다.
        Speed = OwnerPawn->GetVelocity().Size();

        // GetCharacterMovement()는 ACharacter의 함수이므로 캐스팅이 필요합니다.
        if (ACharacter* OwnerCharacter = Cast<ACharacter>(OwnerPawn))
        {
            // 캐스팅이 성공했다면 (이 Pawn이 Character가 맞다면)
            if (UCharacterMovementComponent* MovementComp = OwnerCharacter->GetCharacterMovement())
            {
                bIsFalling = MovementComp->IsFalling();
            }
        }
    }

    //YawDeltaSpeed 계산은 Pawn 레퍼런스를 직접 사용합니다.
    if (OwnerPawn)
    {
        // ... (YawDeltaSpeed 계산 로직) ...
        if (DeltaSeconds > 0.f)
        {
            const float DeltaYaw = FMath::FindDeltaAngleDegrees(LastYaw, OwnerPawn->GetActorRotation().Yaw);
            const float TargetYawDeltaSpeed = DeltaYaw / DeltaSeconds;
            YawDeltaSpeed = FMath::FInterpTo(YawDeltaSpeed, TargetYawDeltaSpeed, DeltaSeconds, 6.0f);
            LastYaw = OwnerPawn->GetActorRotation().Yaw;
        }
        if (Speed < 10.f)
        {
            YawDeltaSpeed = FMath::FInterpTo(YawDeltaSpeed, 0.f, DeltaSeconds, 4.0f);
        }

        // ... (AimOffset 계산 로직 - 필요시 OwnerPawn의 GetControlRotation() 사용) ...
    }
    else
    {
        // OwningPawn이 없을 때의 초기화
        Speed = 0.f;
        YawDeltaSpeed = 0.f;
        bIsFalling = false;
        bJumpInput_Anim = false;
        AimYaw = 0.f;
        AimPitch = 0.f;
    }


}