#pragma once

#include "CoreMinimal.h"
#include "EnemyState.generated.h"

UENUM(BlueprintType)
enum class EEnemyState : uint8
{
	EES_Idle		UMETA(DisplayName = "Idle"),
	EES_Moving		UMETA(DisplayName = "Moving"),
	EES_Groggy		UMETA(DisplayName = "Groggy"),
	EES_Casting		UMETA(DisplayName = "Casting"),
	EES_Attacking	UMETA(DisplayName = "Attacking"),
	EES_Damaged		UMETA(DisplayName = "Damaged"),
	EES_Dead		UMETA(DisplayName = "Dead")
};

UENUM(BlueprintType)
enum class EEnemyType : uint8 {
	EET_Normal		UMETA(DisplayName = "Normal"),
	EET_Elite		UMETA(DisplayName = "Elite"),
	EET_Boss		UMETA(DisplayName = "Boss")
};