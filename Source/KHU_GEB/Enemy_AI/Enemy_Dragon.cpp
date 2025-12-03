// Fill out your copyright notice in the Description page of Project Settings.

#include "Enemy_AI/Enemy_Dragon.h"
#include "Skills/Skill_Range.h"
#include "Skills/FireballProjectile.h"
#include "FormDefinition.h"

AEnemy_Dragon::AEnemy_Dragon()
{
	// 생성자에서 스킬 클래스 설정
	SkillClasses.Add(ESkillSlot::Active, USkill_Range::StaticClass());
}

void AEnemy_Dragon::BeginPlay()
{
	Super::BeginPlay();
	// 스킬 초기화는 부모에서 처리됨
}

void AEnemy_Dragon::ActivateSkill()
{
	// 1. 스킬 컴포넌트 가져오기
	USkillBase* Skill = Equipped.FindRef(ESkillSlot::Active);
	if (!Skill) return;

	USkill_Range* RangeSkill = Cast<USkill_Range>(Skill);
	if (!RangeSkill) return;
	
	// InitializeFromDefinition 호출 >> 현재 스킬 기본 값을 세팅
	if (DefaultFormDef && DefaultFormDef->SkillSet)
	{
		if (USkillDefinition* SkillDef = DefaultFormDef->SkillSet->Skills.FindRef(ESkillSlot::Active))
		{
			RangeSkill->InitializeFromDefinition(SkillDef);
		}
	}

	// BP_Enemy_Dragon에서 설정한 FireballClass 전달
	if (FireballClass)
	{
		RangeSkill->FireballClass = FireballClass;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[Enemy_Dragon] FireballClass is not set in blueprint!"));
		return;
	}

	// 여기서 호출되는 것은 USkill_Range::ActivateSkill()
	RangeSkill->ActivateSkill();
	RangeSkill->StopSkill(); // 사거리 스킬은 즉시 정지
	
	UE_LOG(LogTemp, Log, TEXT("[Enemy_Dragon] Range skill component activated!"));
}

