// Fill out your copyright notice in the Description page of Project Settings.

#include "Enemy_AI/Enemy_Base.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "HealthComponent.h"
#include "WeaponComponent.h"
#include "FormDefinition.h"
#include "SkillBase.h"
#include "ManaComponent.h"

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
	// WeaponComponent 생성 및 초기화
	WeaponComp = CreateDefaultSubobject<UWeaponComponent>(TEXT("WeaponComp"));

	if (!WeaponComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("AEnemy_Base::BeginPlay - WeaponComp creation failed"));
	}
	ManaComp = CreateDefaultSubobject<UManaComponent>(TEXT("ManaComp"));
	if (!ManaComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("AEnemy_Base::BeginPlay - ManaComp creation failed"));
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
	if(WeaponComp && DefaultWeaponData)
	{
		WeaponComp->SetWeaponDefinition(DefaultWeaponData);
		// UE_LOG로 SetWeaponDefinition 호출 여부 확인 및 DefaultWeaponData 정보 출력
		UE_LOG(LogTemp, Warning, TEXT("AEnemy_Base::BeginPlay - SetWeaponDefinition called with DefaultWeaponData: %s"), *DefaultWeaponData->GetName());
	}
	else {
		// WeaponComp가 없는지, DefaultWeaponData가 없는지 확인 및 로그 출력
		UE_LOG(LogTemp, Warning, TEXT("AEnemy_Base::BeginPlay - WeaponComp or DefaultWeaponData is null. WeaponComp: %s, DefaultWeaponData: %s"),
			WeaponComp ? *WeaponComp->GetName() : TEXT("null"),
			DefaultWeaponData ? *DefaultWeaponData->GetName() : TEXT("null"));
	}
	if (ManaComp) {
		// ManaComp는 초기화 함수가 없는거 같은데?
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
				NewSkill->RegisterComponent();
				Equipped.Add(Pair.Key, NewSkill);
				
				UE_LOG(LogTemp, Log, TEXT("[Enemy_Base] Initialized skill: %s"), 
					*NewSkill->GetClass()->GetName());
			}
		}
	}
}

// 기본 구현 (하위 클래스가 오버라이드하지 않으면 이게 호출됨)
void AEnemy_Base::ActivateSkill()
{
	UE_LOG(LogTemp, Warning, 
		TEXT("[Enemy_Base] ActivateSkill called but not overridden! Class: %s"), 
		*GetClass()->GetName());
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
		else {
			UE_LOG(LogTemp, Warning, TEXT("AEnemy_Base::TakeDamage - Currently attacking, state not changed."));
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
