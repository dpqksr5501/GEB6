// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillManagerComponent.h"
#include "SkillBase.h"

// Sets default values for this component's properties
USkillManagerComponent::USkillManagerComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	// PrimaryComponentTick.bCanEverTick = true;

	// ...
}
/*
// Called when the game starts
void USkillManagerComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}

// Called every frame
void USkillManagerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}
*/
void USkillManagerComponent::EquipFromSkillSet(USkillSet* Set)
{
	ClearAll();
	if (!Set) { UE_LOG(LogTemp, Warning, TEXT("[SkillManager] Equip: Set is null")); return; }
	for (auto& It : Set->Skills)
	{
		ESkillSlot Slot = It.Key;
		if (const USkillDefinition* Def = It.Value)
		{
			if (Def->SkillClass)
			{
				USkillBase* Comp = NewObject<USkillBase>(GetOwner(), Def->SkillClass);
				if (Comp)
				{
					Comp->RegisterComponent();
					Comp->Slot = Slot;
					Comp->InitializeFromDefinition(Def);
					Equipped.Add(Slot, Comp);

					UE_LOG(LogTemp, Log, TEXT("[SkillManager] Equipped Slot=%d -> %s"),
						(int32)Slot, *Comp->GetClass()->GetName());
				}
			}
		}
	}
}

void USkillManagerComponent::ClearAll()
{
	for (auto& It : Equipped)
		if (It.Value) It.Value->DestroyComponent();
	Equipped.Empty();
}

bool USkillManagerComponent::TryActivate(ESkillSlot Slot)
{
	if (USkillBase* S = Equipped.FindRef(Slot))
	{
		UE_LOG(LogTemp, Log, TEXT("[SkillManager] TryActivate Slot=%d, Skill=%s"),
			(int32)Slot, *GetNameSafe(S));
		if (S->CanActivate()) { S->ActivateSkill(); return true; }
		UE_LOG(LogTemp, Warning, TEXT("[SkillManager] CanActivate=false for %s"), *GetNameSafe(S));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[SkillManager] No skill equipped for Slot=%d"), (int32)Slot);
	}
	return false;
}

bool USkillManagerComponent::TryStop(ESkillSlot Slot)
{
	if (USkillBase* S = Equipped.FindRef(Slot))
	{
		UE_LOG(LogTemp, Log, TEXT("[SkillManager] TryStop Slot=%d, Skill=%s"),
			(int32)Slot, *GetNameSafe(S));
		S->StopSkill();
		return true;
	}
	UE_LOG(LogTemp, Warning, TEXT("[SkillManager] Stop failed. Slot=%d not equipped"), (int32)Slot);
	return false;
}
