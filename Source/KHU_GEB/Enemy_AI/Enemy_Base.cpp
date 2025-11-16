// Fill out your copyright notice in the Description page of Project Settings.

#include "Enemy_AI/Enemy_Base.h"
#include "Engine/Engine.h"
#include "FormManagerComponent.h"
#include "SkillManagerComponent.h"
#include "BehaviorTree/BlackboardComponent.h"

// Sets default values
AEnemy_Base::AEnemy_Base()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void AEnemy_Base::BeginPlay()
{
	Super::BeginPlay();

	// SkillClasses가 설정되어 있으면 자동으로 초기화
	if (SkillClasses.Num() > 0 && Equipped.Num() == 0)
	{
		InitializeSkills();
	}
}

void AEnemy_Base::InitializeSkills()
{
	Equipped.Empty();

	for (const TPair<ESkillSlot, TSubclassOf<USkillBase>>& Pair : SkillClasses)
	{
		if (Pair.Value)
		{
			USkillBase* NewSkill = NewObject<USkillBase>(this, Pair.Value);
			if (NewSkill)
			{
				Equipped.Add(Pair.Key, NewSkill);
			}
		}
	}
}


float AEnemy_Base::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	
	UE_LOG(LogTemp, Warning, TEXT("AEnemy_Base::TakeDamage called. DamageAmount: %f, ActualDamage: %f"), DamageAmount, ActualDamage);

	//현재 상태를 저장하는 CurrentState 변수를 선언하고, Blackboard에서 EnemyState 값을 가져와 저장
	EEnemyState CurrentState = (EEnemyState)BlackboardComp->GetValueAsEnum("EnemyState");

	if (BlackboardComp) {
		// 현재 공격 중이 아니라면 피격 중으로 변경
		if (CurrentState != EEnemyState::EES_Attacking)
		{
			BlackboardComp->SetValueAsEnum("EnemyState", (uint8)EEnemyState::EES_Damaged);
		}
	}
	else {
		UE_LOG(LogTemp, Warning, TEXT("AEnemy_Base::TakeDamage - BlackboardComp is null"));
	}

	return ActualDamage;
}
