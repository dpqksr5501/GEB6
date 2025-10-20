// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "PlayerEnums.generated.h"

/**
 변신 형태 구분
 */

UENUM(BlueprintType)
enum class EPlayerForm : uint8
{
    Base    UMETA(DisplayName = "Base"),
    Range   UMETA(DisplayName = "Range"),
    Speed   UMETA(DisplayName = "Speed"),
    Defense UMETA(DisplayName = "Defense"),
    Debuff  UMETA(DisplayName = "Debuff")
};