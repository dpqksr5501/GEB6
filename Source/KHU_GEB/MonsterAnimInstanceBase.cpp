// Fill out your copyright notice in the Description page of Project Settings.

#include "MonsterAnimInstanceBase.h"
#include "Animation/AnimInstance.h"
#include "MonsterBase.h"
#include "Kismet/KismetMathLibrary.h" // NormalizedDeltaRotator 같은 수학 함수를 위해서
#include "GameFramework/CharacterMovementComponent.h"

// 애니메이션 인스턴스가 처음 생성될 때 한 번 호출됩니다. (BeginPlay와 유사)
void UMonsterAnimInstanceBase::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation(); // 부모 클래스의 초기화 함수 호출

	// 이 애니메이션 인스턴스를 소유한 폰(캐릭터)을 가져와서 AMonsterBase 타입으로 변환합니다.
	OwningMonster = Cast<AMonsterBase>(TryGetPawnOwner());

		if (OwningMonster)
		{
			// Yaw 회전값 계산을 위해 현재 캐릭터의 Yaw 값을 저장합니다.
			LastYaw = OwningMonster->GetActorRotation().Yaw;
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

	// OwningMonster가 유효한지(null이 아닌지) 확인합니다.
	if (OwningMonster)
	{
		// 몬스터의 현재 속도(Velocity)의 크기를 구해 Speed 변수에 저장합니다. [cite: 3775]
		Speed = OwningMonster->GetVelocity().Size();
		// 몬스터의 현재 ECharacterState (Idle, Attacking 등)를 가져와 CharacterState 변수에 저장합니다. [cite: 3775]
		CharacterState = OwningMonster->GetCharacterState();

		UCharacterMovementComponent* MovementComp = OwningMonster->GetCharacterMovement();
		if (MovementComp)
		{
			// IsFalling()이 점프와 추락을 모두 감지합니다.
			bIsFalling = MovementComp->IsFalling();
		}
		bJumpInput_Anim = OwningMonster->bJumpInput;

		OwningMonster->bJumpInput = false;

		// --- Yaw 회전 속도 (Turn-in-Place) 계산 ---
		// DeltaSeconds가 0보다 커서 나누기 오류가 발생하지 않는지 확인합니다.
		if (DeltaSeconds > 0.f)
		{
			// 현재 캐릭터의 월드 회전값을 가져옵니다.
			FRotator CurrentRotation = OwningMonster->GetActorRotation();

			// 지난 프레임의 Yaw와 현재 프레임의 Yaw 사이의 변화량(각도)을 계산합니다.
			// (FindDeltaAngleDegrees는 -180~180 사이의 최단 각도를 반환해 줍니다.)
			const float DeltaYaw = FMath::FindDeltaAngleDegrees(LastYaw, CurrentRotation.Yaw);

			// 현재 프레임의 순간적인 Yaw 회전 속도를 계산합니다. (변화량 / 시간)
			const float TargetYawDeltaSpeed = DeltaYaw / DeltaSeconds;

			// 현재 YawDeltaSpeed 값을 목표 속도(TargetYawDeltaSpeed)로 부드럽게 보간(Interp)합니다.
			// (애니메이션이 뚝뚝 끊기지 않고 부드럽게 전환되도록 함)
			YawDeltaSpeed = FMath::FInterpTo(YawDeltaSpeed, TargetYawDeltaSpeed, DeltaSeconds, 6.0f);

			// 다음 프레임 계산을 위해 현재 Yaw 값을 LastYaw에 저장합니다.
			LastYaw = CurrentRotation.Yaw;


			//// --- 조준 각도 (Aim Offset) 계산 ---
			//// 컨트롤러(카메라/플레이어 시점)의 회전값을 가져옵니다.
			//const FRotator ControlRotation = OwningMonster->GetControlRotation();

			//// 실제 캐릭터(액터)의 회전값을 가져옵니다.
			//const FRotator ActorRotation = OwningMonster->GetActorRotation();

			//// 컨트롤러가 바라보는 방향과 액터가 바라보는 방향 사이의 차이(델타)를 정규화하여 계산합니다.
			//const FRotator DeltaRotation = UKismetMathLibrary::NormalizedDeltaRotator(ControlRotation, ActorRotation);

			//// 이 차이값에서 Pitch(상하)와 Yaw(좌우) 값을 추출합니다.
			//const float TargetAimPitch = DeltaRotation.Pitch;
			//const float TargetAimYaw = DeltaRotation.Yaw;

			//// AimPitch와 AimYaw 값을 목표 각도로 부드럽게 보간(Interp)합니다.
			//// (이 변수들은 ABP의 'Aim Offset' 블렌드 스페이스에서 상체 조준에 사용됩니다.)
			//AimPitch = FMath::FInterpTo(AimPitch, TargetAimPitch, DeltaSeconds, 6.0f);
			//AimYaw = FMath::FInterpTo(AimYaw, TargetAimYaw, DeltaSeconds, 6.0f);
		}


		// --- 회전 속도 감속 ---
		// 캐릭터의 속도가 거의 0이라면 (서 있다면)
		if (Speed < 10.f)
		{
			// YawDeltaSpeed 값을 0으로 부드럽게 되돌립니다.
			// (제자리에 서 있을 때 마우스를 돌리다가 멈추면, 회전 애니메이션도 스르륵 멈추도록 함)
			YawDeltaSpeed = FMath::FInterpTo(YawDeltaSpeed, 0.f, DeltaSeconds, 4.0f);
		}
	}
	else
	{
		
		// OwningMonster가 유효하지 않으면(예: 게임 시작 중), 모든 값을 0으로 초기화합니다.
		bIsFalling = false;
		bJumpInput_Anim = false;
		AimPitch = 0.f;
		AimYaw = 0.f;
	}


}