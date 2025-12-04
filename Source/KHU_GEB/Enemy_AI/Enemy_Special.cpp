// Fill out your copyright notice in the Description page of Project Settings.

#include "Enemy_AI/Enemy_Special.h"
#include "Skills/Skill_Special.h"
#include "FormDefinition.h"
#include "Skills/Skill_Ultimate.h"

AEnemy_Special::AEnemy_Special()
{
	// 생성자에서 스킬 클래스 설정
	SkillClasses.Add(ESkillSlot::Active, USkill_Special::StaticClass());
}

void AEnemy_Special::BeginPlay()
{
	Super::BeginPlay();
	// 스킬 초기화는 부모에서 처리됨
}

void AEnemy_Special::ActivateSkill()
{
	// 1. 스킬 컴포넌트 가져오기
	USkillBase* Skill = Equipped.FindRef(ESkillSlot::Active);
	if (!Skill) return;

	USkill_Special* SpecialSkill = Cast<USkill_Special>(Skill);
	if (!SpecialSkill) return;
	
	// InitializeFromDefinition 호출 >> 현재 스킬 기본 값을 세팅
	if (DefaultFormDef && DefaultFormDef->SkillSet)
	{
		if (USkillDefinition* SkillDef = DefaultFormDef->SkillSet->Skills.FindRef(ESkillSlot::Active))
		{
			SpecialSkill->InitializeFromDefinition(SkillDef);
		}
	}

	// 여기서 호출되는 것은 USkill_Special::ActivateSkill()
	SpecialSkill->ActivateSkill(); 
	
	UE_LOG(LogTemp, Log, TEXT("[Enemy_Special] Special skill component activated!"));
}

void AEnemy_Special::ActivateUltimate()
{
	// 1. 스킬 컴포넌트 가져오기
	USkillBase* Skill = Equipped.FindRef(ESkillSlot::Ultimate);
	if (!Skill) return;

	USkill_Ultimate* SpecialSkill = Cast<USkill_Ultimate>(Skill);
	if (!SpecialSkill) return;

	// InitializeFromDefinition 호출 >> 현재 스킬 기본 값을 세팅
	if (DefaultFormDef && DefaultFormDef->SkillSet)
	{
		if (USkillDefinition* SkillDef = DefaultFormDef->SkillSet->Skills.FindRef(ESkillSlot::Ultimate))
		{
			SpecialSkill->InitializeFromDefinition(SkillDef);
		}
	}

	// SpecialOrbClass 설정 (BP_Enemy_Special에서 설정한 값 사용)
	if (SpecialOrbClass)
	{
		SpecialSkill->SpecialOrbClass = SpecialOrbClass;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[Enemy_Special] SpecialOrbClass is not set in blueprint!"));
		return;
	}

	// 여기서 호출되는 것은 USkill_Ultimate::ActivateSkill()
	SpecialSkill->ActivateSkill();

	UE_LOG(LogTemp, Log, TEXT("[Enemy_Special] Special Ultimate component activated!"));
}

