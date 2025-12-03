// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Enemy_AI/Enemy_Base.h"
#include "Enemy_Tanker.generated.h"

/**
 * 
 */
UCLASS()
class KHU_GEB_API AEnemy_Tanker : public AEnemy_Base
{
	GENERATED_BODY()

public:
	AEnemy_Tanker();

	virtual void ActivateSkill() override;
	virtual void Tick(float DeltaTime) override;

	// TakeDamage를 오버라이드하여 Guard 스킬의 배리어 처리
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, 
		AController* EventInstigator, AActor* DamageCauser) override;

protected:
	virtual void BeginPlay() override;

private:
	/** Guard 스킬이 활성화되어 있는지 추적 */
	bool bIsGuardSkillActive = false;

	/** Guard 스킬 참조를 캐싱 */
	UPROPERTY()
	class USkill_Guard* CachedGuardSkill = nullptr;
};
