// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "EnemyState.h"
#include "TAttack.generated.h"

/**
 * AI가 지정된 몽타주를 재생하도록 하는 태스크입니다.
 * 몽타주 재생이 완료되면 성공을 반환합니다.
 */
UCLASS()
class KHU_GEB_API UTAttack : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UTAttack();

	/** 비헤이비어 트리 에디터에서 선택할 공격 몽타주입니다. */
	UPROPERTY(EditAnywhere, Category = "Attack Montage")
	UAnimMontage* AttackMontage;

	UPROPERTY(EditAnywhere, Category = "Attack Type")
	FBlackboardKeySelector LastActionTimeKey;

protected:
	/** 태스크가 시작될 때 호출됩니다. 몽타주 재생을 시작합니다. */
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	/** 태스크가 실행 중인 동안 매 틱 호출됩니다. 몽타주 종료를 감지합니다. */
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	UPROPERTY()
	UBlackboardComponent* BlackboardComp;

};