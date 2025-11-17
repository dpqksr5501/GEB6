// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "Enemy_AI/EnemyState.h"
#include "TMoveTo.generated.h"

class AAIController;
class UBlackboardComponent;
class APawn;
class UCharacterMovementComponent;

/**
 * 
 */
UCLASS()
class KHU_GEB_API UTMoveTo : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UTMoveTo();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

protected:
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector TargetLocationKey;

	UPROPERTY(EditAnywhere, Category = "Movement")
	float StopDistance = 100.0f;

	UPROPERTY(EditAnywhere, Category = "Movement")
	float WalkSpeed = 300.0f;

private:
	AAIController* AIController;
	UBlackboardComponent* BlackboardComp;
	APawn* ControlledPawn;
	UCharacterMovementComponent* MovementComponent;
	float CurrentDistance;
};
