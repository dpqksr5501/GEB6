// Fill out your copyright notice in the Description page of Project Settings.

#include "Enemy_AI/Enemy_Base.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "HealthComponent.h"
#include "WeaponComponent.h"
#include "FormDefinition.h"
#include "Skills/SkillBase.h"
#include "JumpComponent.h"
#include "CrowdControlComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/GameInstance.h"
#include "Pooling/EnemyPoolSubsystem.h"
#include "Pooling/EnemySpawnDirector.h"
#include "KHU_GEBCharacter.h"
#include "StatManagerComponent.h"
#include "Kismet/GameplayStatics.h"
#include "FormManagerComponent.h"
#include "AIController.h"

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

	CrowdControlComp = CreateDefaultSubobject<UCrowdControlComponent>(TEXT("CrowdControlComp"));
	if (!CrowdControlComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("AEnemy_Base::BeginPlay - CrowdControlComp creation failed"));
	}

}

// Called when the game starts or when spawned
void AEnemy_Base::BeginPlay()
{
	Super::BeginPlay();

	if (AAIController* AIController = Cast<AAIController>(GetController()))
	{
		BlackboardComp = AIController->GetBlackboardComponent();

		if (BlackboardComp)
		{
			UE_LOG(LogTemp, Log, TEXT("[Enemy_Base] BlackboardComp initialized in BeginPlay for %s"), *GetName());
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[Enemy_Base] BlackboardComp is null in BeginPlay for %s - AI might not be possessed yet"), *GetName());
		}
	}
	else
	{
		// AI Controller가 아직 빙의되지 않았을 경우, 다음 프레임에 재시도
		UE_LOG(LogTemp, Warning, TEXT("[Enemy_Base] AI Controller not possessed yet in BeginPlay for %s - will retry next frame"), *GetName());

		GetWorld()->GetTimerManager().SetTimerForNextTick([this]()
			{
				if (AAIController* AIController = Cast<AAIController>(GetController()))
				{
					BlackboardComp = AIController->GetBlackboardComponent();

					if (BlackboardComp)
					{
						UE_LOG(LogTemp, Log, TEXT("[Enemy_Base] BlackboardComp initialized on next frame for %s"), *GetName());
					}
					else
					{
						UE_LOG(LogTemp, Error, TEXT("[Enemy_Base] BlackboardComp still null after retry for %s"), *GetName());
					}
				}
			});
	}
	if (DefaultFormDef)
	{
		EnemyFormType = DefaultFormDef->FormType;
	}
	
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

	// 스탯 초기화
	if (DefaultFormDef && DefaultFormDef->StatData)
	{
		// FormStatData 기반으로 기본값 세팅
		EnemyStats.InitializeFromData(DefaultFormDef->StatData);

		// BP에서 지정한 EnemyLevel로 덮어쓰기
		if (EnemyLevel > 0)
		{
			EnemyStats.Level = EnemyLevel;
			EnemyStats.RecalculateDerivedStats();
		}

		// 이동 관련 스탯 적용
		if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
		{
			MoveComp->MaxWalkSpeed = EnemyStats.WalkSpeed;
			MoveComp->MaxAcceleration = EnemyStats.Acceleration;
			// 필요하면 스프린트/추격 속도 등도 여기서 활용
		}

		UE_LOG(LogTemp, Log,
			TEXT("[Enemy_Base] Stats initialized. Level=%d, Atk=%.1f, Def=%.1f, WalkSpeed=%.1f"),
			EnemyStats.Level, EnemyStats.Attack, EnemyStats.Defense, EnemyStats.WalkSpeed);
	}
	else
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[Enemy_Base] DefaultFormDef or StatData is null. Stats not initialized."));
	}

	// 2. [추가됨] 플레이어 레벨업 이벤트 구독 (앞으로의 변화 감지)
	AKHU_GEBCharacter* PlayerChar = Cast<AKHU_GEBCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
	if (PlayerChar && PlayerChar->StatManager)
	{
		// "레벨업 하면 내 함수(OnPlayerLevelUp)를 실행해줘!"
		PlayerChar->StatManager->OnFormLevelUp.AddDynamic(this, &AEnemy_Base::OnPlayerLevelUp);
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

void AEnemy_Base::ActivateSkill()
{
	if (CrowdControlComp && CrowdControlComp->IsMoveBlocked()) return;
}

void AEnemy_Base::ActivateUltimate()
{
	if (CrowdControlComp && CrowdControlComp->IsMoveBlocked()) return;
}

// 다른 Actor의 ApplyDamage에 의해 호출됨
float AEnemy_Base::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	// AActor 기본 처리 (델리게이트 브로드캐스트 등)
	const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	UE_LOG(LogTemp, Warning,
		TEXT("AEnemy_Base::TakeDamage called. DamageAmount: %f, ActualDamage: %f"),
		DamageAmount, ActualDamage);

	if (!HealthComp || ActualDamage <= 0.f) return 0.f;

	// 1) 실제 공격자 Actor 추출 (컨트롤러의 Pawn → 없으면 DamageCauser)
	AActor* InstigatorActor = nullptr;
	if (EventInstigator) { InstigatorActor = EventInstigator->GetPawn(); }
	if (!InstigatorActor) { InstigatorActor = DamageCauser; }

	// 2) 팀/적 판정: 적이 아니면 데미지 무시
	if (!IsEnemyFor(InstigatorActor))
	{
		UE_LOG(LogTemp, Verbose,
			TEXT("[Enemy_Base] Ignore damage from non-enemy: %s"),
			*GetNameSafe(InstigatorActor));
		return 0.f;
	}

	// 3) 여기까지 왔으면 '적'이 맞으므로 체력 감소
	const float FinalDamage = HealthComp->ApplyDamage(ActualDamage, InstigatorActor, DamageCauser);

	// 4) 사망 여부 플래그
	const bool bJustDied = HealthComp->IsDead();

	// 5) Blackboard 상태 갱신
	if (BlackboardComp)
	{
		const EEnemyState CurrentState = static_cast<EEnemyState>(
			BlackboardComp->GetValueAsEnum("EnemyState"));

		if (!bJustDied)
		{
			// 아직 살아있으면 Damaged 상태로 전환(공격 중이 아닐 때만)
			if (CurrentState != EEnemyState::EES_Attacking && CurrentState != EEnemyState::EES_Groggy)
			{
				BlackboardComp->SetValueAsEnum("EnemyState",
					static_cast<uint8>(EEnemyState::EES_Damaged));

				UE_LOG(LogTemp, Warning,
					TEXT("AEnemy_Base::TakeDamage - Health after damage: %f"),
					HealthComp->Health);
			}
			else
			{
				UE_LOG(LogTemp, Warning,
					TEXT("AEnemy_Base::TakeDamage - Currently attacking, state not changed. Health after damage: %f"),
					HealthComp->Health);
			}
		}
		else
		{
			BlackboardComp->SetValueAsEnum("EnemyState",
				static_cast<uint8>(EEnemyState::EES_Dead));
			UE_LOG(LogTemp, Warning,
				TEXT("AEnemy_Base::TakeDamage - Enemy health is zero or below, state set to Dead."));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning,
			TEXT("AEnemy_Base::TakeDamage - BlackboardComp is null"));
	}

	// 6) 실제 사망 처리 + 폼 레벨업
	if (bJustDied)
	{
		if (InstigatorActor) { HandleKilledBy(InstigatorActor); }
		OnDeath();
	}

	return FinalDamage;
}

void AEnemy_Base::OnDeath()
{
	// 델리게이트 방송 (나 죽음..)
	if (OnEnemyDied.IsBound())
	{
		OnEnemyDied.Broadcast();
	}
}

bool AEnemy_Base::IsEnemyFor(const AActor* Other) const
{
	const AEnemy_Base* OtherChar = Cast<AEnemy_Base>(Other);
	if (OtherChar) return false;
	else return true;
}

AActor* AEnemy_Base::GetCurrentTarget() const
{
	if (!BlackboardComp) return nullptr;

	// 여기 키 이름은 실제 BT/BB에서 쓰는 이름에 맞춰 주세요.
	static const FName TargetKeyName(TEXT("Target"));

	UObject* Value = BlackboardComp->GetValueAsObject(TargetKeyName);
	AActor* Target = Cast<AActor>(Value);

	// 유효성 체크 추가
	if (!IsValid(Target)) { return nullptr; }

	return Target;
}

void AEnemy_Base::HandleKilledBy(AActor* Killer)
{
	if (!Killer) return;

	AKHU_GEBCharacter* Player = Cast<AKHU_GEBCharacter>(Killer);
	if (!Player) return;

	if (!Player->StatManager) return;

	// 이 적이 어떤 FormType에 속하는지 결정
	EFormType FormTypeForExp = EnemyFormType;

	// 혹시 EnemyFormType이 기본값인 경우, DefaultFormDef에서 한 번 더 가져오기
	if (FormTypeForExp == EFormType::Base && DefaultFormDef)
	{
		FormTypeForExp = DefaultFormDef->FormType;
	}

	// --- Minion 레벨 < Player 폼 레벨이면 경험치 X ---
	if (EnemyKind == EEnemyKind::Minion)
	{
		// 플레이어 StatManager에서 해당 폼 레벨 가져오기
		const int32 PlayerFormLevel = Player->StatManager->GetLevelStat(FormTypeForExp); // 

		// Minion 레벨이 더 낮으면 경험치/레벨업 처리 스킵
		//수정...
		if (PlayerFormLevel >= 5)
		{
			UE_LOG(LogTemp, Log,
				TEXT("[Enemy_Base] Minion level %d < player form level %d (FormType=%d). No EXP granted."),
				EnemyLevel,
				PlayerFormLevel,
				static_cast<int32>(FormTypeForExp));

			return;
		}
	}

	Player->StatManager->RegisterKill(FormTypeForExp, EnemyKind);

	UE_LOG(LogTemp, Log,
		TEXT("[Enemy_Base] Killed by %s, registered kill for form %d"),
		*GetNameSafe(Player),
		static_cast<int32>(FormTypeForExp));
}

void AEnemy_Base::SetLevel(int32 NewLevel)
{
	EnemyLevel = NewLevel;
	EnemyStats.Level = NewLevel;
	EnemyStats.RecalculateDerivedStats();
	return;
}


void AEnemy_Base::ApplyHealthScaling()
{
	if (!HealthComp) return;

	// 1. 플레이어 정보 가져오기
	AKHU_GEBCharacter* PlayerChar = Cast<AKHU_GEBCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));

	// 플레이어가 없거나 스탯 매니저가 없으면 -> 기본값(100) 유지하고 종료
	if (!PlayerChar || !PlayerChar->StatManager) return;

	// 2. 플레이어 레벨 가져오기 (Base 폼 기준)
	int32 PlayerLevel = PlayerChar->StatManager->GetLevelStat(EFormType::Base);
	if (PlayerLevel < 1) PlayerLevel = 1; // 안전장치

	// 3. [핵심] 보스와 엘리트의 성장 수치 설정
	float BaseStatsHealth = 0.f;  // 1레벨일 때의 체력 (500)
	float HealthPerLevel = 0.f;   // 레벨당 증가량 (400)

	if (EnemyKind == EEnemyKind::Boss)
	{
		BaseStatsHealth = 500.0f; // [사용자 요청] 1레벨 기본 체력
		HealthPerLevel = 350.0f;  // [사용자 요청] 레벨업당 증가량
	}
	else if (EnemyKind == EEnemyKind::Elite)
	{
		// 예: 엘리트는 100에서 시작하고 100씩 증가
		BaseStatsHealth = 100.0f;
		HealthPerLevel = 150.0f;
	}
	else
	{
		return; // 미니언은 스케일링 안 함
	}

	// 4. 공식 적용
	// 공식: 기본체력 + (레벨-1 * 증가량)
	// Lv 1: 500 + (0 * 300) = 500
	// Lv 2: 500 + (1 * 200) = 700
	float NewMaxHealth = BaseStatsHealth + ((PlayerLevel - 1) * HealthPerLevel);

	// 5. 체력 적용 (최대 체력과 현재 체력을 꽉 채움)
	HealthComp->InitializeHealth(NewMaxHealth, NewMaxHealth);

	UE_LOG(LogTemp, Log, TEXT("[Boss Scaling] Level: %d, FinalHP: %.1f"), PlayerLevel, NewMaxHealth);
}


void AEnemy_Base::OnPlayerLevelUp(EFormType FormType, int32 NewLevel)
{
	// 우리는 'Base' 폼 레벨을 기준으로 스케일링하기로 했으므로,
	// Base 폼이 레벨업했을 때만 체력을 갱신합니다.
	if (FormType == EFormType::Base)
	{
		ApplyHealthScaling();
	}
}