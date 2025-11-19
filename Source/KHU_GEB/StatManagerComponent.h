// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FormDefinition.h"
#include "FormStatData.h"
#include "StatManagerComponent.generated.h"

class UFormStatData;
class UFormSet;

USTRUCT(BlueprintType)
struct FFormRuntimeStats
{
	GENERATED_BODY();

	// 실제 적용 중인 공격력 / 방어력 / 이동속도 / 레벨
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	float Attack = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	float Defense = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	float WalkSpeed = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	float SprintSpeed = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	float Acceleration = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	int32 Level = 1;

	// ——— 파라미터(틀): 레벨당 공격/방어 ———
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	float AttackPerLevel = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	float DefensePerLevel = 0.f;

	// ==== 킬 카운트 ====
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats|Exp")
	int32 MinionKills = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats|Exp")
	bool bFirstMinionKillGrantsLevel = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats|Exp")
	int32 MinionKillsPerExtraLevel = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats|Exp")
	bool bKilledElite = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats|Exp")
	bool bKilledBoss = false;

	// StatData 로부터 초기화
	void InitializeFromData(const UFormStatData* Data);

	// 레벨이 바뀌었을 때 실제 공격/방어 다시 계산
	void RecalculateDerivedStats();
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class KHU_GEB_API UStatManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UStatManagerComponent();

	// 폼별 런타임 스탯
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Stats")
	TMap<EFormType, FFormRuntimeStats> RuntimeStats;

	// FormSet + 각 FormDefinition.StatData 를 이용해서 초기화
	UFUNCTION(BlueprintCallable, Category = "Stats")
	void InitializeFromFormSet(const UFormSet* FormSet);

	// 조회
	const FFormRuntimeStats* GetStats(EFormType FormType) const;

	FFormRuntimeStats* GetStatsMutable(EFormType FormType);

	// ==== 경험치 / 킬 관련 ====

	// Minion을 잡았을 때 호출
	UFUNCTION(BlueprintCallable, Category = "Stats|Exp")
	void AddMinionKill(EFormType FormType);

	// Elite를 잡았을 때 (한 번만 true로 세팅)
	UFUNCTION(BlueprintCallable, Category = "Stats|Exp")
	void MarkEliteKilled(EFormType FormType);

	// Boss를 잡았을 때 (한 번만 true로 세팅)
	UFUNCTION(BlueprintCallable, Category = "Stats|Exp")
	void MarkBossKilled(EFormType FormType);

	// 강제로 레벨 조정하고 싶을 때 (예: 퀘스트 보상 등)
	UFUNCTION(BlueprintCallable, Category = "Stats|Level")
	void AddLevel(EFormType FormType, int32 Amount = 1);
};
