// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Enemy_AI/Enemy_Base.h"
#include "Enemy_Minion.generated.h"

// Enemy_Minion의 사망을 알리는 델리게이트 (자기 자신을 매개변수로 전달)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMinionDeath, AEnemy_Minion*, DeadMinion);

/**
 * 
 */
UCLASS()
class KHU_GEB_API AEnemy_Minion : public AEnemy_Base
{
	GENERATED_BODY()
	
public:
	/** 미니언이 사망했을 때 발행되는 델리게이트 */
	UPROPERTY(BlueprintAssignable, Category = "Enemy Events")
	FOnMinionDeath OnMinionDeath;

protected:
	virtual void BeginPlay() override;

	virtual void OnDeath() override;

private:
	/** 사망 후 풀로 반환되기까지의 대기 시간 (초) */
	UPROPERTY(EditDefaultsOnly, Category = "Death", meta = (AllowPrivateAccess = "true"))
	float DeathDelay = 0.5f;

	/** 지연된 사망 처리를 실행하는 함수 */
	void ExecuteDelayedDeath();

	/** 타이머 핸들 */
	FTimerHandle DeathTimerHandle;
};
