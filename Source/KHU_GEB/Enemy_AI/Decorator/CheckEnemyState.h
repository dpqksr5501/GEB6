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
	/** 비헤이비어 트리 에디터에서 비교하고자 하는 상태 */
	UPROPERTY(EditAnywhere, Category = "AI")
	EEnemyState DesiredState;

	// FBlackboardKeySelector 변수는 UBTDecorator_BlackboardBase로부터 상속받아 이미 존재합니다.

	/** 조건 검사 실행 함수 */
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;

	/** Observer 기능을 위한 함수들 */
	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
	virtual void OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual EBlackboardNotificationResult OnBlackboardKeyValueChange(const UBlackboardComponent& Blackboard, FBlackboard::FKey ChangedKeyID) override;

	/** 에디터에서 노드 이름을 더 알기 쉽게 표시하기 위한 함수입니다. */
	virtual FString GetStaticDescription() const override;
};
