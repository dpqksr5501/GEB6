// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "EnemyState.h"
#include "TGroggy.generated.h"

/**
 *
 */
UCLASS()
class KHU_GEB_API UTGroggy : public UBTTaskNode
{
	GENERATED_BODY()
	UTGroggy();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	UPROPERTY()
	UBlackboardComponent* BlackboardComp;
};
