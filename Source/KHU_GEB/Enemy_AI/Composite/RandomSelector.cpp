// Fill out your copyright notice in the Description page of Project Settings.


#include "RandomSelector.h"

URandomSelector::URandomSelector(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer), LastSuccessfulChildIdx(INDEX_NONE)
{
	NodeName = "Selector Random";
}

void URandomSelector::InitializeMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryInit::Type InitType) const
{
	InitializeNodeMemory<FBTCompositeMemory>(NodeMemory, InitType);
	ExecutedChildren.Empty();
	LastSuccessfulChildIdx = INDEX_NONE;
}

void URandomSelector::CleanupMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryClear::Type CleanupType) const
{
	CleanupNodeMemory<FBTCompositeMemory>(NodeMemory, CleanupType);
	ExecutedChildren.Empty();
	LastSuccessfulChildIdx = INDEX_NONE;
}

int32 URandomSelector::GetNextChildHandler(FBehaviorTreeSearchData& SearchData, int32 PrevChild, EBTNodeResult::Type LastResult) const
{
	int32 NextChildIdx = BTSpecialChild::ReturnToParent;

	if (PrevChild == BTSpecialChild::NotInitialized)
	{
		// First execution, choose a random child
		NextChildIdx = FMath::RandRange(0, GetChildrenNum() - 1);
		if (NextChildIdx == LastSuccessfulChildIdx && GetChildrenNum() > 1)
		{
			// Avoid the last successful child being chosen first if possible
			NextChildIdx = (NextChildIdx + 1) % GetChildrenNum();
		}
	}
	else
	{
		// Handle last result
		if (LastResult == EBTNodeResult::Succeeded)
		{
			// If last child succeeded, return success and reset
			LastSuccessfulChildIdx = PrevChild;
			ExecutedChildren.Empty();
			return BTSpecialChild::ReturnToParent;
		}
		else if (LastResult == EBTNodeResult::Failed)
		{
			// If last child failed, mark it as executed
			ExecutedChildren.Add(PrevChild);

			// If all children have failed, return failure
			if (ExecutedChildren.Num() >= GetChildrenNum())
			{
				ExecutedChildren.Empty();
				LastSuccessfulChildIdx = INDEX_NONE;
				return BTSpecialChild::ReturnToParent;
			}

			// Choose a new random child that has not been executed yet
			TArray<int32> AvailableChildren;
			for (int32 ChildIdx = 0; ChildIdx < GetChildrenNum(); ++ChildIdx)
			{
				if (!ExecutedChildren.Contains(ChildIdx))
				{
					AvailableChildren.Add(ChildIdx);
				}
			}

			// Choose a random child index from available children
			NextChildIdx = AvailableChildren[FMath::RandRange(0, AvailableChildren.Num() - 1)];
		}
	}

	return NextChildIdx;
}