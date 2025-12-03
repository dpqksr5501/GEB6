// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "EnemyState.h"
#include "TWait.generated.h"

/**
 * 
 */
UCLASS()
class KHU_GEB_API UTWait : public UBTTaskNode
{
	GENERATED_BODY()
	UTWait();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	UPROPERTY()
	UBlackboardComponent* BlackboardComp;
};
