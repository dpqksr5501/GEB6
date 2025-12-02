// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "EnemyState.h"
#include "Skills/SkillDefinition.h"
#include "TSkill.generated.h"

/**
 * AI가 스킬 몽타주를 재생하고 스킬을 활성화하는 태스크입니다.
 * TAttack과 유사한 구조로, DefaultFormDef에서 SkillMontage를 가져와 재생합니다.
 */
UCLASS()
class KHU_GEB_API UTSkill : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UTSkill();

	/** 블랙보드 키 - 마지막 액션 시간 */
	UPROPERTY(EditAnywhere, Category = "Skill")
	FBlackboardKeySelector CooldownKey;

	/** 이 스킬 시 활성화할 스킬 슬롯 (None = 스킬 없음) */
	UPROPERTY(EditAnywhere, Category = "Skill")
	ESkillSlot SkillSlotToActivate = ESkillSlot::Active;

protected:
	/** 태스크가 시작될 때 호출됩니다. 스킬 몽타주 재생을 시작합니다. */
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	/** 태스크가 실행 중인 동안 매 틱 호출됩니다. 몽타주 종료를 감지합니다. */
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	/** 태스크가 중단될 때 호출됩니다. */
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	UPROPERTY()
	UBlackboardComponent* BlackboardComp;

	/** 현재 재생 중인 스킬 몽타주 */
	UPROPERTY()
	UAnimMontage* CurrentMontage;
};