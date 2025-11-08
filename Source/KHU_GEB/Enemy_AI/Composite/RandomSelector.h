// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTCompositeNode.h"
#include "BehaviorTree/Composites/BTComposite_Selector.h"
#include "RandomSelector.generated.h"

/**
 * 
 */
UCLASS()
class KHU_GEB_API URandomSelector : public UBTComposite_Selector
{
	GENERATED_BODY()
public:
	URandomSelector(const FObjectInitializer& ObjectInitializer);

	virtual void InitializeMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryInit::Type InitType) const override;
	virtual void CleanupMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryClear::Type CleanupType) const override;
	virtual int32 GetNextChildHandler(struct FBehaviorTreeSearchData& SearchData, int32 PrevChild, EBTNodeResult::Type LastResult) const override;

private:
	mutable TArray<int32> ExecutedChildren;
	mutable int32 LastSuccessfulChildIdx;
};
