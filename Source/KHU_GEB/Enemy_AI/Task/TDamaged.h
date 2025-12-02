// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "EnemyState.h"
#include "TDamaged.generated.h"

/**
 * 
 */
UCLASS()
class KHU_GEB_API UTDamaged : public UBTTaskNode
{
	GENERATED_BODY()
	UTDamaged();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	UPROPERTY()
	UBlackboardComponent* BlackboardComp;

	// 현재 재생 중인 몽타주를 추적
	UPROPERTY(Transient)
	class UAnimMontage* CurrentMontage;
};
