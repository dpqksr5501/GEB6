// Fill out your copyright notice in the Description page of Project Settings.


#include "Skills/SkillManagerComponent.h"
#include "Skills/Skill_Base.h"
#include "Skills/Skill_Range.h"
#include "Skills/Skill_Swift.h"
#include "Skills/Skill_Guard.h"
#include "Skills/Skill_Special.h"
#include "KHU_GEBCharacter.h"
#include "LockOnComponent.h"

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

void USkillManagerComponent::OnRangeAimingStarted(USkill_Range* Skill)
{
	bIsRangeAiming = true;
	ActiveRangeSkill = Skill;

	// 스킬 시작 시점에 현재 락온 타겟을 저장
	auto Owner = GetOwner();
	if (auto OwnerChar = Cast<ACharacter>(Owner))
	{
		if (auto PlayerChar = Cast<AKHU_GEBCharacter>(OwnerChar))
		{
			if (PlayerChar->LockOnComp)
			{
				SavedRangeLockOnTarget = PlayerChar->LockOnComp->GetCurrentTarget();
			}
			else
			{
				SavedRangeLockOnTarget = nullptr;
			}
		}
	}
}

void USkillManagerComponent::OnRangeAimingEnded(USkill_Range* Skill)
{
	if (ActiveRangeSkill.Get() == Skill)
	{
		ActiveRangeSkill = nullptr;
		bIsRangeAiming = false;
	}

	// 저장해 두었던 락온 타겟이 아직 유효하면 다시 락온
	auto Owner = GetOwner();
	if (auto OwnerChar = Cast<ACharacter>(Owner))
	{
		if (auto PlayerChar = Cast<AKHU_GEBCharacter>(OwnerChar))
		{
			if (PlayerChar->LockOnComp)
			{
				if (SavedRangeLockOnTarget.IsValid())
				{
					PlayerChar->LockOnComp->LockOnToTarget(SavedRangeLockOnTarget.Get());
				}
			}
		}
	}

	// 한 번 쓰고 나면 정리
	SavedRangeLockOnTarget = nullptr;
}

void USkillManagerComponent::OnSwiftStrikeStarted(USkill_Swift* Skill)
{
	ActiveSwiftSkill = Skill;
	bIsSwiftStriking = true;
}

void USkillManagerComponent::OnSwiftStrikeEnded(USkill_Swift* Skill)
{
	if (ActiveSwiftSkill.Get() == Skill)
	{
		ActiveSwiftSkill = nullptr;
		bIsSwiftStriking = false;
	}
}

void USkillManagerComponent::OnGuardSkillStarted(USkill_Guard* Skill)
{
	ActiveGuardSkill = Skill;
	bIsGuardSkillActiveForForm = true;
}

void USkillManagerComponent::OnGuardSkillEnded(USkill_Guard* Skill)
{
	if (ActiveGuardSkill.Get() == Skill)
	{
		ActiveGuardSkill = nullptr;
		bIsGuardSkillActiveForForm = false;
	}
}

void USkillManagerComponent::OnSpecialSkillStarted(USkill_Special* Skill)
{
	ActiveSpecialSkill = Skill;
	bIsSpecialSkillActiveForForm = true;
}

void USkillManagerComponent::OnSpecialSkillEnded(USkill_Special* Skill)
{
	if (ActiveSpecialSkill.Get() == Skill)
	{
		ActiveSpecialSkill = nullptr;
		bIsSpecialSkillActiveForForm = false;
	}
}

bool USkillManagerComponent::IsFormChangeLocked() const
{
	// Range 조준 중
	if (bIsRangeAiming) return true;

	// Swift 다단히트 중
	if (bIsSwiftStriking && ActiveSwiftSkill.IsValid()) return true;

	// Guard 보호막 유지 중
	if (bIsGuardSkillActiveForForm && ActiveGuardSkill.IsValid()) return true;

	// Special 흑안개 유지 중
	if (bIsSpecialSkillActiveForForm && ActiveSpecialSkill.IsValid()) return true;

	return false;
}
