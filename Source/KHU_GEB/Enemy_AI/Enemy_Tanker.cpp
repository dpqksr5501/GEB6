// Fill out your copyright notice in the Description page of Project Settings.

#include "Enemy_AI/Enemy_Tanker.h"
#include "Skills.h"
#include "SkillBase.h"
#include "FormDefinition.h"

AEnemy_Tanker::AEnemy_Tanker()
{
	// 생성자에서 스킬 클래스 설정
	SkillClasses.Add(ESkillSlot::Active, USkill_Guard::StaticClass());
}

void AEnemy_Tanker::BeginPlay()
{
	Super::BeginPlay();
	// 스킬 초기화는 부모에서 처리됨
}

void AEnemy_Tanker::ActivateSkill()
{
	// 1. 스킬 컴포넌트 가져오기
	USkillBase* Skill = Equipped.FindRef(ESkillSlot::Active);
	if (!Skill) return;

	USkill_Guard* GuardSkill = Cast<USkill_Guard>(Skill);
	if (!GuardSkill) return;
	
	// InitializeFromDefinition 호출 >> 현재 스킬 기본 값을 세팅
	GuardSkill->InitializeFromDefinition(DefaultFormDef->SkillSet->Skills.FindRef(ESkillSlot::Active));
	
	// 마나 검사를 해서 꺼놨음. 문제가 되면 키죠
	//if (!GuardSkill->CanActivate()) return; 

	// 여기서 호출되는 것은 USkill_Guard::ActivateSkill()
	GuardSkill->ActivateSkill(); 
	
	UE_LOG(LogTemp, Log, TEXT("[Enemy_Tanker] Guard skill component activated!"));
}

