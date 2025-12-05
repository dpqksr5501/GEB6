// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Enemy_AI/Enemy_Base.h"
#include "Enemy_Dragon.generated.h"

class AFireballProjectile; 

/**
 * 
 */
UCLASS()
class KHU_GEB_API AEnemy_Dragon : public AEnemy_Base
{
	GENERATED_BODY()

public:
	AEnemy_Dragon();

	virtual void ActivateSkill() override;

	virtual void ActivateUltimate() override;

	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, 
		AController* EventInstigator, AActor* DamageCauser) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dragon|Skill")
	TSubclassOf<AFireballProjectile> FireballClass;

protected:
	virtual void BeginPlay() override;
};
