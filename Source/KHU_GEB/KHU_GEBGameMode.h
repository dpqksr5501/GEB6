// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "KHU_GEBGameMode.generated.h"

/**
 *  Simple GameMode for a third person game
 */
UCLASS(abstract)
class AKHU_GEBGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	
	/** Constructor */
	AKHU_GEBGameMode();
};



