// Fill out your copyright notice in the Description page of Project Settings.

#include "Enemy_AI/Enemy_Tanker.h"
#include "Skills.h"
#include "SkillBase.h"
#include "FormDefinition.h"


AEnemy_Tanker::AEnemy_Tanker()
{
	// Tick 활성화
	PrimaryActorTick.bCanEverTick = true;
	
	// 생성자에서 스킬 클래스 설정
	SkillClasses.Add(ESkillSlot::Active, USkill_Guard::StaticClass());
}

void AEnemy_Tanker::BeginPlay()
{
	Super::BeginPlay();
	// 스킬 초기화는 부모에서 처리됨
	// CachedGuardSkill에 자신의 스킬을 CachedGuardSkill에 맞게 캐스팅하여 할당
	CachedGuardSkill = Cast<USkill_Guard>(Equipped.FindRef(ESkillSlot::Active));

}

void AEnemy_Tanker::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	// Guard 스킬이 활성화되어 있고, 캐시된 참조가 유효한 경우에만 검사
	if (bIsGuardSkillActive && CachedGuardSkill)
	{
		// ConsumedShields가 RemainingShields보다 커지면 스킬 중단
		if (CachedGuardSkill->ConsumedShields > CachedGuardSkill->RemainingShields)
		{
			UE_LOG(LogTemp, Log, TEXT("[Enemy_Tanker] ConsumedShields(%d) > RemainingShields(%d). Stopping Guard skill."),
				CachedGuardSkill->ConsumedShields, CachedGuardSkill->RemainingShields);
			
			CachedGuardSkill->StopSkill();
			bIsGuardSkillActive = false;
			CachedGuardSkill = nullptr;
		}
	}
}

float AEnemy_Tanker::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, 
	AController* EventInstigator, AActor* DamageCauser)
{
	if (CachedGuardSkill) {
		if (bIsGuardSkillActive) {
			bool bDamageHandled = CachedGuardSkill->HandleIncomingDamage(DamageAmount, nullptr, EventInstigator, DamageCauser);
			if (bDamageHandled)
			{
				UE_LOG(LogTemp, Log, TEXT("[Enemy_Tanker] Damage of %f handled by Guard skill."), DamageAmount);
				return 0.0f; // 피해 무시
			}
			else {
				UE_LOG(LogTemp, Log, TEXT("[Enemy_Tanker] bDamageHandled false. Damage 0"));
				return 0;
			}
		}
		else {
			// 부모 (Enemy_Base)의 TakeDamage 호출
			return Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
		}
	}
	else {
		UE_LOG(LogTemp, Log, TEXT("[Enemy_Tanker] CachedGuardSkill is null."));
		return 0.0f;
	}
}

void AEnemy_Tanker::ActivateSkill()
{
	// 1. 스킬 컴포넌트 가져오기
	USkillBase* Skill = Equipped.FindRef(ESkillSlot::Active);
	if (!Skill) 
	{
		UE_LOG(LogTemp, Warning, TEXT("[Enemy_Tanker] No skill found in Active slot."));
		return;
	}

	USkill_Guard* GuardSkill = Cast<USkill_Guard>(Skill);
	if (!GuardSkill) 
	{
		UE_LOG(LogTemp, Warning, TEXT("[Enemy_Tanker] Skill is not USkill_Guard."));
		return;
	}
	
	// InitializeFromDefinition 호출 >> 현재 스킬 기본 값을 세팅
	if (DefaultFormDef && DefaultFormDef->SkillSet)
	{
		const USkillDefinition* SkillDef = DefaultFormDef->SkillSet->Skills.FindRef(ESkillSlot::Active);
		if (SkillDef)
		{
			GuardSkill->InitializeFromDefinition(SkillDef);
		}
	}
	
	// 마나 검사를 해서 꺼놨음. 문제가 되면 키죠
	//if (!GuardSkill->CanActivate()) return; 

	// 여기서 호출되는 것은 USkill_Guard::ActivateSkill()
	GuardSkill->ActivateSkill(); 
	
	// Guard 스킬 활성화 상태 추적
	bIsGuardSkillActive = true;
	CachedGuardSkill = GuardSkill;
	
	UE_LOG(LogTemp, Log, TEXT("[Enemy_Tanker] Guard skill component activated!"));
}

