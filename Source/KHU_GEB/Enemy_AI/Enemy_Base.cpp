// Fill out your copyright notice in the Description page of Project Settings.

#include "Enemy_AI/Enemy_Base.h"
#include "Engine/Engine.h"
#include "FormManagerComponent.h"
#include "SkillManagerComponent.h"
#include "HealthComponent.h"
#include "BehaviorTree/BlackboardComponent.h"

// Sets default values
AEnemy_Base::AEnemy_Base()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// HealthComponent 생성 및 초기화
	HealthComp = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComp"));
	if (!HealthComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("AEnemy_Base::BeginPlay - HealthComp creation failed"));
	}
}

// Called when the game starts or when spawned
void AEnemy_Base::BeginPlay()
{
	Super::BeginPlay();
	
	// 죽음 델리게이트에 OnDeath 함수 바인딩
	if (HealthComp)
	{
		HealthComp->OnDeath.AddDynamic(this, &AEnemy_Base::OnDeath);
	}
	
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
			HealthComp->ReduceHealth(ActualDamage);
			UE_LOG(LogTemp, Warning, TEXT("AEnemy_Base::TakeDamage - Health after damage: %f"), HealthComp->Health);
		}
	}
	else {
		UE_LOG(LogTemp, Warning, TEXT("AEnemy_Base::TakeDamage - BlackboardComp is null"));
	}

	return ActualDamage;
}

void AEnemy_Base::OnDeath()
{
	UE_LOG(LogTemp, Warning, TEXT("AEnemy_Base::OnDeath - Enemy has died!"));
}
