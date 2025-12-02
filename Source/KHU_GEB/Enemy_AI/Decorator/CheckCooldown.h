// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Decorators/BTDecorator_BlackboardBase.h"
#include "CheckCooldown.generated.h"

/**
 * 블랙보드에 저장된 마지막 실행 시간과 현재 시간을 비교하여
 * 설정된 쿨타임(CompareValue)이 지났는지 확인하는 데코레이터입니다.
 * 쿨타임이 지났으면 true(통과)를 반환합니다.
 */
UCLASS()
class KHU_GEB_API UCheckCooldown : public UBTDecorator_BlackboardBase
{
	GENERATED_BODY()

public:
	UCheckCooldown();

	/** 비교할 쿨타임 시간 (초) */
	UPROPERTY(EditAnywhere, Category = "Cooldown")
	float CompareValue;

protected:
	/** 데코레이터의 조건 로직을 계산합니다. */
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
};