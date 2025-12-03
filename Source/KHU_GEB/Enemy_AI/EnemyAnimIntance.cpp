// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemy_AI/EnemyAnimIntance.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"

void UEnemyAnimIntance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	OwnerCharacter = Cast<ACharacter>(TryGetPawnOwner());
	if (OwnerCharacter)
	{
		MovementComponent = OwnerCharacter->GetCharacterMovement();
	}

	GroundSpeed = 0.0f;
	VerticalSpeed = 0.0f;
	bIsFalling = false;
	bIsJumping = false;
	bWasInAir = false;
	bSpaceActionInput = false;
	bIsRangeGliding = false;
}

void UEnemyAnimIntance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!OwnerCharacter || !MovementComponent)
	{
		return;
	}

	FVector Velocity = OwnerCharacter->GetVelocity();
	GroundSpeed = Velocity.Size2D();
	VerticalSpeed = Velocity.Z;

	bIsFalling = MovementComponent->IsFalling();

	// bWasInAir는 착지 감지용으로만 사용 (Landing 상태 진입 트리거)
	if (bIsFalling)
	{
		bWasInAir = true;
	}
	else if (bWasInAir)
	{
		// 착지 감지는 하지만 bIsJumping은 TJump에서만 관리
		bWasInAir = false;
	}
}

void UEnemyAnimIntance::SetIsJumping(bool bNewIsJumping)
{
	bIsJumping = bNewIsJumping;
}

void UEnemyAnimIntance::ResetSpaceActionInput()
{
	bSpaceActionInput = false;
}

