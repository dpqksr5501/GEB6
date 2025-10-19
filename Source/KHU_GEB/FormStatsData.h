// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "FormStatsData.generated.h"

/**
 ���� DataAsset
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
	// �� �ĺ���(�����/������ ����)
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FName FormId;

	// ü�� ����: ���ݷ�/����/�̼�/��ų��Ÿ��
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats") FStatCurve Attack;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats") FStatCurve Defense;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats") FStatCurve MoveSpeed;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats") FStatCurve SkillCooldown;
};