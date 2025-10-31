// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Decorators/BTDecorator_BlackboardBase.h"
#include "EnemyState.h"
#include "CheckEnemyState.generated.h"

/**
 * 
 */
UCLASS()
class KHU_GEB_API UCheckEnemyState : public UBTDecorator_BlackboardBase
{
	GENERATED_BODY()
public:
	UCheckEnemyState();

protected:
	/** �����̺�� Ʈ�� �����Ϳ��� ���ϰ��� �ϴ� ���� */
	UPROPERTY(EditAnywhere, Category = "AI")
	EEnemyState DesiredState;

	// FBlackboardKeySelector ������ UBTDecorator_BlackboardBase�κ��� ��ӹ޾� �̹� �����մϴ�.

	/** ���� �˻� ���� �Լ� */
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
};
