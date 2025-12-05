// Fill out your copyright notice in the Description page of Project Settings.

#include "Enemy_AI/Enemy_Dragon.h"
#include "Skills/Skill_Range.h"
#include "Skills/FireballProjectile.h"
#include "FormDefinition.h"
#include "Skills/Skill_Ultimate.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "EnemyAnimIntance.h"
#include "KHU_GEBCharacter.h"
#include "Skills/SkillManagerComponent.h"
#include "Skills/Skill_Guard.h"
#include "FormManagerComponent.h"

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

float AEnemy_Dragon::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, 
	AController* EventInstigator, AActor* DamageCauser)
{
	// 1. 공격자 Actor 추출 (EventInstigator의 Pawn 또는 DamageCauser)
	AActor* InstigatorActor = nullptr;
	if (EventInstigator) 
	{ 
		InstigatorActor = EventInstigator->GetPawn(); 
	}
	if (!InstigatorActor) 
	{ 
		InstigatorActor = DamageCauser; 
	}

	// 2. 공격자가 플레이어 캐릭터인지 확인
	AKHU_GEBCharacter* PlayerChar = Cast<AKHU_GEBCharacter>(InstigatorActor);
	
	// 3. Guard 궁극기 사용 중인지 확인
	bool bIsGuardUltimate = false;
	if (PlayerChar && PlayerChar->SkillManager)
	{
		// Ultimate 슬롯의 스킬 가져오기
		if (USkillBase* UltimateSkill = PlayerChar->SkillManager->Equipped.FindRef(ESkillSlot::Ultimate))
		{
			if (USkill_Ultimate* Ultimate = Cast<USkill_Ultimate>(UltimateSkill))
			{
				// 현재 폼 타입 확인
				EFormType CurrentForm = PlayerChar->FormManager ? 
					PlayerChar->FormManager->CurrentForm : EFormType::Base;
				
				// Guard 폼이고 궁극기가 방금 발동된 경우
				if (CurrentForm == EFormType::Guard)
				{
					UE_LOG(LogTemp, Warning, 
						TEXT("[Enemy_Dragon] Hit by Guard Ultimate from player: %s"), 
						*PlayerChar->GetName());
					bIsGuardUltimate = true;
				}
			}
		}
	}

	// 4. 자신이 점프 중인지 확인 (AnimInstance의 bIsJumping)
	bool bIsDragonJumping = false;
	if (UAnimInstance* AnimInst = GetMesh()->GetAnimInstance())
	{
		if (UEnemyAnimIntance* EnemyAnim = Cast<UEnemyAnimIntance>(AnimInst))
		{
			bIsDragonJumping = EnemyAnim->bIsJumping;
		}
	}

	// 5. Guard 궁극기 + 점프 중이면 Groggy 상태로 전환
	if (bIsGuardUltimate && bIsDragonJumping && BlackboardComp)
	{
		BlackboardComp->SetValueAsEnum("EnemyState", static_cast<uint8>(EEnemyState::EES_Groggy));
		
		UE_LOG(LogTemp, Warning, 
			TEXT("[Enemy_Dragon] Hit by Guard Ultimate while jumping! State changed to Groggy"));
	}

	// 6. 부모 클래스의 TakeDamage 호출 (일반 데미지 처리)
	return Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
}

void AEnemy_Dragon::ActivateSkill()
{
	Super::ActivateSkill();

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

void AEnemy_Dragon::ActivateUltimate()
{	
	Super::ActivateUltimate();

	// 1. 스킬 컴포넌트 가져오기
	USkillBase* Skill = Equipped.FindRef(ESkillSlot::Ultimate);
	if (!Skill) return;
	UE_LOG(LogTemp, Log, TEXT("[Enemy_Dragon] Activating Ultimate Skill..."));
	USkill_Ultimate* RangeSkill = Cast<USkill_Ultimate>(Skill);
	if (!RangeSkill) return;

	// InitializeFromDefinition 호출 >> 현재 스킬 기본 값을 세팅
	if (DefaultFormDef && DefaultFormDef->SkillSet)
	{
		if (USkillDefinition* SkillDef = DefaultFormDef->SkillSet->Skills.FindRef(ESkillSlot::Ultimate))
		{
			RangeSkill->InitializeFromDefinition(SkillDef);
		}
	}

	// 여기서 호출되는 것은 USkill_Range::ActivateSkill()
	RangeSkill->ActivateSkill();

	UE_LOG(LogTemp, Log, TEXT("[Enemy_Dragon] Range Ultimate component activated!"));
}

