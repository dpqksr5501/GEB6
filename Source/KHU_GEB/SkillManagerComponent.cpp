// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillManagerComponent.h"
#include "SkillBase.h"

USkillManagerComponent::USkillManagerComponent() {}

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

		// 1) 공통 체크 (쿨타임 + 마나)
		if (!S->CanActivate())
		{
			UE_LOG(LogTemp, Warning, TEXT("[SkillManager] CanActivate = false for %s"),
				*GetNameSafe(S));
			return false;
		}

		// 2) 실제 발동 (여기서 Super::ActivateSkill 내부에서 비용 처리가 됨)
		S->ActivateSkill();
		return true;
	}

	UE_LOG(LogTemp, Warning, TEXT("[SkillManager] No skill equipped for Slot=%d"),
		(int32)Slot);
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
