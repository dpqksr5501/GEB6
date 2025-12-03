// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "EnemyState.h"
#include "TAttack.generated.h"

class UWeaponComponent;

/**
 * AI가 WeaponComponent를 사용하여 단순한 공격을 수행하는 태스크입니다.
 * DefaultFormDef의 첫 번째 공격 몽타주를 재생하고 WeaponComponent로 히트박스를 관리합니다.
 */
UCLASS()
class KHU_GEB_API UTAttack : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UTAttack();

	/** 블랙보드 키 설정 */
	UPROPERTY(EditAnywhere, Category = "Attack")
	FBlackboardKeySelector LastActionTimeKey;

protected:
	/** 태스크가 시작될 때 호출됩니다. 공격 몽타주를 재생합니다. */
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	/** 태스크가 실행 중인 동안 매 틱 호출됩니다. 몽타주 종료를 확인합니다. */
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	/** 태스크가 중단될 때 호출됩니다. */
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	// Notify 처리 함수들 추가
	UFUNCTION()
	void OnNotifyBeginReceived(FName NotifyName, const FBranchingPointNotifyPayload& Payload);

	UFUNCTION()
	void OnNotifyEndReceived(FName NotifyName, const FBranchingPointNotifyPayload& Payload);

private:
	UPROPERTY()
	UBlackboardComponent* BlackboardComp;

	/** 현재 재생 중인 몽타주 */
	UPROPERTY()
	UAnimMontage* CurrentMontage;

	/** WeaponComponent 참조 (캐시용) */
	UPROPERTY()
	UWeaponComponent* CachedWeaponComp;
};