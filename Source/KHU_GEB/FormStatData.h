// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "FormStatData.generated.h"

UCLASS()
class KHU_GEB_API UFormStatData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// 레벨 1당 공격력
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats")
	float AttackPerLevel = 10.f;

	// 레벨 1당 방어력
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats")
	float DefensePerLevel = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats", meta = (ClampMin = "0.0"))
	float WalkSpeed = 600.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats", meta = (ClampMin = "0.0"))
	float SprintSpeed = 900.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats", meta = (ClampMin = "0.0"))
	float Acceleration = 2048.f;

	// 시작 레벨
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats", meta=(ClampMin="0"))
	int32 StartLevel = 0;

	// ---- 경험치 / 레벨업 규칙 ----

	// 첫 Minion을 잡으면 레벨을 1 올릴지 여부 (true면 MinionKills == 1일 때 레벨 +1)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats|Exp")
	bool bFirstMinionKillGrantsLevel = true;

	// 추가 레벨업을 위한 Minion 처치 수 (예: 5면, 5/10/15... 마리째마다 레벨 +1)
	// 0 이하이면 이 규칙은 사용하지 않음
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats|Exp", meta = (ClampMin = "0"))
	int32 MinionKillsPerExtraLevel = 5;
};