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

protected:
	virtual void BeginPlay() override;
};
