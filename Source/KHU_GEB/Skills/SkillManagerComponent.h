// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SkillDefinition.h"
#include "SkillManagerComponent.generated.h"

class USkillBase;
class USkillSet;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class KHU_GEB_API USkillManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	USkillManagerComponent();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TMap<ESkillSlot, TObjectPtr<USkillBase>> Equipped;

private:
	bool bIsRangeAiming = false;
	TWeakObjectPtr<class USkill_Range> ActiveRangeSkill;
	TWeakObjectPtr<AActor> SavedRangeLockOnTarget;

	bool bIsSwiftStriking = false;
	TWeakObjectPtr<class USkill_Swift> ActiveSwiftSkill;

	bool bIsGuardSkillActiveForForm = false;
	TWeakObjectPtr<class USkill_Guard> ActiveGuardSkill;

	bool bIsSpecialSkillActiveForForm = false;
	TWeakObjectPtr<class USkill_Special> ActiveSpecialSkill;

public:
	void OnRangeAimingStarted(USkill_Range* Skill);
	void OnRangeAimingEnded(USkill_Range* Skill);
	bool IsRangeAiming() const { return bIsRangeAiming; }
	TWeakObjectPtr<class USkill_Range> GetActiveRangeSkill() { return ActiveRangeSkill; }

	void OnSwiftStrikeStarted(class USkill_Swift* Skill);
	void OnSwiftStrikeEnded(class USkill_Swift* Skill);

	void OnGuardSkillStarted(class USkill_Guard* Skill);
	void OnGuardSkillEnded(class USkill_Guard* Skill);

	void OnSpecialSkillStarted(class USkill_Special* Skill);
	void OnSpecialSkillEnded(class USkill_Special* Skill);

	bool IsFormChangeLocked() const;
	
public:
	UFUNCTION(BlueprintCallable)
	void EquipFromSkillSet(USkillSet* Set);

	UFUNCTION(BlueprintCallable)
	void ClearAll();

	// 입력 바인딩에서 호출
	UFUNCTION(BlueprintCallable) bool TryActivate(ESkillSlot Slot);
	UFUNCTION(BlueprintCallable) bool TryStop(ESkillSlot Slot);
};
