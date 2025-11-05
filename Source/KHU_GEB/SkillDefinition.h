// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SkillDefinition.generated.h"

UENUM(BlueprintType)
enum class ESkillSlot : uint8 { Passive, Active };

USTRUCT(BlueprintType)
struct FSkillParams
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float Damage = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float Cooldown = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float Range = 600.f;
	// 필요 시 InstancedStruct로 확장 가능
};

UCLASS(BlueprintType)
class KHU_GEB_API USkillDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<class USkillBase> SkillClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FSkillParams Params;
};

UCLASS(BlueprintType)
class KHU_GEB_API USkillSet : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TMap<ESkillSlot, TObjectPtr<USkillDefinition>> Skills;
};

