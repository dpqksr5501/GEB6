// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Enemy_AI/Enemy_Base.h"
#include "Enemy_Special.generated.h"

/**
 * 
 */
UCLASS()
class KHU_GEB_API AEnemy_Special : public AEnemy_Base
{
	GENERATED_BODY()

public:
	AEnemy_Special();

	virtual void BeginPlay() override;
	virtual void ActivateSkill() override;
	virtual void ActivateUltimate() override;

	// Special Ultimate에 사용할 Orb 클래스 (BP에서 설정)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|Ultimate")
	TSubclassOf<AActor> SpecialOrbClass;
};
