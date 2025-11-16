// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Decorators/BTDecorator_BlackboardBase.h"
#include "IsGreaterThan.generated.h"

/**
 * 블랙보드의 Float 값 (BlackboardKey)이 'CompareValue'보다 크면 true를 반환합니다.
 * (Inverse 체크 시 '작거나 같다' (<=)로 동작합니다.)
 */
UCLASS()
class KHU_GEB_API UIsGreaterThan : public UBTDecorator_BlackboardBase
{
	GENERATED_BODY()

public:
	UIsGreaterThan();

protected:
	/** 비교할 기준이 되는 값. */
	UPROPERTY(EditAnywhere, Category = "Condition")
	float CompareValue;

	/** 실제 조건 검사를 수행하는 함수입니다. */
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;

	/** 에디터에서 노드 이름을 더 알기 쉽게 표시하기 위한 함수입니다. */
	virtual FString GetStaticDescription() const override;

	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
	virtual void OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	virtual EBlackboardNotificationResult OnBlackboardKeyValueChange(const UBlackboardComponent& Blackboard, FBlackboard::FKey ChangedKeyID) override;
};
