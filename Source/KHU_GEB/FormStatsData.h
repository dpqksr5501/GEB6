// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "FormStatsData.generated.h"

/**
 스탯 DataAsset
 */

USTRUCT(BlueprintType)
struct FStatCurve
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float Base = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float PerLevel = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int MaxLevel = 3;
};

UCLASS(BlueprintType)
class KHU_GEB_API UFormStatsData : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	// 폼 식별용(디버그/에디터 편의)
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FName FormId;

	// 체력 제외: 공격력/방어력/이속/스킬쿨타임
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats") FStatCurve Attack;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats") FStatCurve Defense;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats") FStatCurve MoveSpeed;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats") FStatCurve SkillCooldown;
};