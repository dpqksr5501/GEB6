// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "EnemyAnimIntance.generated.h"

UCLASS()
class KHU_GEB_API UEnemyAnimIntance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category = "Animation")
	void SetIsJumping(bool bNewIsJumping);

	UFUNCTION(BlueprintCallable, Category = "Animation")
	void ResetSpaceActionInput();

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	class ACharacter* OwnerCharacter;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	class UCharacterMovementComponent* MovementComponent;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float GroundSpeed;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float VerticalSpeed;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bIsFalling;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bIsJumping;	

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bWasInAir;

	/** Guard ÆûÀÇ SpaceAction (µµ¹ß/²ø¾î´ç±è) ÀÔ·Â ÇÃ·¡±× */
	UPROPERTY(BlueprintReadWrite, Category = "Action")
	bool bSpaceActionInput;
};
