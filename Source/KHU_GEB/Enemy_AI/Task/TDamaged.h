// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "EnemyState.h"
#include "TDamaged.generated.h"

class UAnimMontage;

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

	// BT 노드에서 몽타주를 선택할 수 있도록 UPROPERTY로 선언	
	UPROPERTY(EditAnywhere, Category = "Animation")
	UAnimMontage* DamagedMontage;
};
