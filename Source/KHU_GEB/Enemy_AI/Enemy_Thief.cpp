#include "Enemy_AI/Enemy_Thief.h"
#include "Skills.h"
#include "SkillBase.h"
#include "FormDefinition.h"
AEnemy_Thief::AEnemy_Thief()
{
	// 생성자에서 스킬 클래스 설정
	SkillClasses.Add(ESkillSlot::Active, USkill_Swift::StaticClass());
}

void AEnemy_Thief::BeginPlay()
{
	Super::BeginPlay();
	// 스킬 초기화는 부모에서 처리됨
}

void AEnemy_Thief::ActivateSkill()
	{
	// 1. 스킬 컴포넌트 가져오기
	USkillBase* Skill = Equipped.FindRef(ESkillSlot::Active);
	if (!Skill) return;

	USkill_Swift* SwiftSkill = Cast<USkill_Swift>(Skill);
	if (!SwiftSkill) return;
	
	// InitializeFromDefinition 호출 >> 각종 스킬 기본 값이 설정됨
	SwiftSkill->InitializeFromDefinition(DefaultFormDef->SkillSet->Skills.FindRef(ESkillSlot::Active));

	if (!SwiftSkill->CanActivate()) return;

	// 여기서 호출되는 것은 USkill_Swift::ActivateSkill()
	SwiftSkill->ActivateSkill(); 
	
	UE_LOG(LogTemp, Log, TEXT("[Enemy_Thief] Swift skill component activated!"));
}