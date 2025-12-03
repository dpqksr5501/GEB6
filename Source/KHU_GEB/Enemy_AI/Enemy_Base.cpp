// Fill out your copyright notice in the Description page of Project Settings.

#include "Enemy_AI/Enemy_Base.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "HealthComponent.h"
#include "WeaponComponent.h"
#include "FormDefinition.h"
#include "Skills/SkillBase.h"
#include "JumpComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/GameInstance.h"
#include "Pooling/EnemyPoolSubsystem.h"
#include "Pooling/EnemySpawnDirector.h"
#include "Enemy_Minion.h"

// Sets default values
AEnemy_Base::AEnemy_Base()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// CharacterMovement 설정 추가
	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	if (MoveComp)
	{
		MoveComp->JumpZVelocity = 600.f;  // 점프 속도 설정
		MoveComp->AirControl = 0.5f;       // 공중 제어력
		MoveComp->GravityScale = 1.0f;     // 중력 스케일
	}

	MeshRoot = CreateDefaultSubobject<USceneComponent>(TEXT("MeshRoot"));
	MeshRoot->SetupAttachment(RootComponent);
	GetMesh()->SetupAttachment(MeshRoot);

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
	// JumpComponent 생성 및 초기화
	JumpComp = CreateDefaultSubobject<UJumpComponent>(TEXT("JumpComp"));
	if (!JumpComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("AEnemy_Base::BeginPlay - JumpComp creation failed"));
	}
}

// Called when the game starts or when spawned
void AEnemy_Base::BeginPlay()
{
	Super::BeginPlay();
	
	if(WeaponComp && DefaultWeaponData)
	{
		WeaponComp->SetWeaponDefinition(DefaultWeaponData);
	}
	else {
		// WeaponComp가 없는지, DefaultWeaponData가 없는지 확인 및 로그 출력
		UE_LOG(LogTemp, Warning, TEXT("AEnemy_Base::BeginPlay - WeaponComp or DefaultWeaponData is null. WeaponComp: %s, DefaultWeaponData: %s"),
			WeaponComp ? *WeaponComp->GetName() : TEXT("null"),
			DefaultWeaponData ? *DefaultWeaponData->GetName() : TEXT("null"));
	}
	
	// SkillClasses가 설정되어 있으면 자동으로 초기화
	if (SkillClasses.Num() > 0 && Equipped.Num() == 0)
	{
		InitializeSkills();
	}
	
	// JumpComponent 폼 초기화
	if (JumpComp && DefaultFormDef)
	{
		// DefaultFormDef의 FormType을 읽어서 JumpComponent에 설정
		JumpComp->SetForm(DefaultFormDef->FormType, DefaultFormDef);
		UE_LOG(LogTemp, Log, TEXT("[Enemy_Base] JumpComp initialized with FormType: %d"), 
			static_cast<int32>(DefaultFormDef->FormType));
	}
	else
	{
		if (!JumpComp)
		{
			UE_LOG(LogTemp, Error, TEXT("[Enemy_Base] JumpComp is null! Class: %s"), 
				*GetClass()->GetName());
		}
		if (!DefaultFormDef)
		{
			UE_LOG(LogTemp, Error, TEXT("[Enemy_Base] DefaultFormDef is null! Class: %s"), 
				*GetClass()->GetName());
		}
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

// 기본 구현 (하위 클래스가 오버라이드하지 않으면 이게 호출됨)
void AEnemy_Base::ActivateUltimate()
{
	UE_LOG(LogTemp, Warning,
		TEXT("[Enemy_Base] ActivateUltimate called but not overridden! Class: %s"),
		*GetClass()->GetName());
}

// 다른 Actor의 ApplyDamage에 의해 호출됨
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
			// 중첩 감소 인해 주석 처리
			// HealthComp->ReduceHealth(ActualDamage);
			if (HealthComp->Health <= 0.f)
			{
				BlackboardComp->SetValueAsEnum("EnemyState", (uint8)EEnemyState::EES_Dead);
				UE_LOG(LogTemp, Warning, TEXT("AEnemy_Base::TakeDamage - Enemy health is zero or below, state set to Dead."));
			}
			else {
				UE_LOG(LogTemp, Warning, TEXT("AEnemy_Base::TakeDamage - Health after damage: %f"), HealthComp->Health);
			}
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

	// Director에게 사망 알림
	UGameInstance* GI = GetGameInstance();
	if (GI)
	{
		UEnemySpawnDirector* SpawnDirector = GI->GetSubsystem<UEnemySpawnDirector>();
		if (SpawnDirector)
		{
			SpawnDirector->OnEnemyDied(GetClass());
		}

		// Pool에 반환
		UEnemyPoolSubsystem* EnemyPool = GI->GetSubsystem<UEnemyPoolSubsystem>();
		if (EnemyPool)
		{
			EnemyPool->ReturnEnemyToPool(this);
		}
	}
}
