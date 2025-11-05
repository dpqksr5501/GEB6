// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PlayerEnums.h"
#include "FormStatsData.h"
#include "PlayerStatsComponent.generated.h"

UENUM(BlueprintType)
enum class EUpgradableStatType : uint8 { Attack, Defense, MoveSpeed, SkillCooldown };

USTRUCT(BlueprintType)
struct FFormStatProgress
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 AttackLevel = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 DefenseLevel = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 MoveSpeedLevel = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 SkillCooldownLevel = 0;
};

USTRUCT(BlueprintType)
struct FFormStatState
{
	GENERATED_BODY()

	// 밸런스 곡선 (DataAsset)
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TSoftObjectPtr<UFormStatsData> StatsData;

	// 업그레이드 진행도
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FFormStatProgress Progress;

	// 실효값 캐시(계산 결과)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float Attack = 0.f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float Defense = 0.f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float MoveSpeed = 600.f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float SkillCooldown = 5.f;
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class KHU_GEB_API UPlayerStatsComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UPlayerStatsComponent();

	// 폼별 스탯 상태(업그레이드 포함)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Forms")
	TMap<EPlayerForm, FFormStatState> FormStats;

	// 체력(공통)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health") float MaxHealth = 100.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health") float Health = 100.f;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	// virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
	// 재계산/업그레이드/조회
	UFUNCTION(BlueprintCallable) void RecalcForForm(EPlayerForm Form);
	UFUNCTION(BlueprintCallable) void ApplyUpgrade(EPlayerForm Form, EUpgradableStatType Stat, int32 Levels = 1);

	UFUNCTION(BlueprintPure) float GetAttack(EPlayerForm Form) const;
	UFUNCTION(BlueprintPure) float GetDefense(EPlayerForm Form) const;
	UFUNCTION(BlueprintPure) float GetMoveSpeed(EPlayerForm Form) const;
	UFUNCTION(BlueprintPure) float GetSkillCooldown(EPlayerForm Form) const;
};