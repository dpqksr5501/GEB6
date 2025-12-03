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

	/** Guard 폼의 SpaceAction (도발/끌어당김) 입력 플래그 */
	UPROPERTY(BlueprintReadWrite, Category = "Action")
	bool bSpaceActionInput;

	/** Range 폼의 글라이딩(활강) 상태 플래그 */
	UPROPERTY(BlueprintReadWrite, Category = "Movement")
	bool bIsRangeGliding;
};
