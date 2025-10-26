// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PlayerEnums.h"
#include "PlayerStateBundle.generated.h"

/**
 변신 시 스텟 이관
 */

USTRUCT(BlueprintType)
struct FActiveEffect
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName EffectId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float TimeRemaining = 0.f;
};

USTRUCT(BlueprintType)
struct FFormStatProgressSave
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) EPlayerForm Form = EPlayerForm::Base;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 AttackLevel = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 DefenseLevel = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 MoveSpeedLevel = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 SkillCooldownLevel = 0;
};

USTRUCT(BlueprintType)
struct FPlayerStateBundle
{
    GENERATED_BODY()

    // 체력(공통)
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float MaxHP = 100.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float HP = 100.f;

    // 예시: 탄약/버프 등 (원하면 확장)
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Ammo = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 ReserveAmmo = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FActiveEffect> ActiveEffects;

    // 폼 업그레이드 진행도들
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FFormStatProgressSave> FormProgresses;
};