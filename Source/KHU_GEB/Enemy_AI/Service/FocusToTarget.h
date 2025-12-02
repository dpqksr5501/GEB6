// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Services/BTService_BlackboardBase.h"
#include "FocusToTarget.generated.h"

/**
 * 
 */
UCLASS()
class KHU_GEB_API UFocusToTarget : public UBTService_BlackboardBase
{
	GENERATED_BODY()
public:
	UFocusToTarget();

	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
};
