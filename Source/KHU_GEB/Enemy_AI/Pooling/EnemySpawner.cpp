#include "EnemySpawner.h"
#include "Enemy_AI/Enemy_Base.h"
#include "Enemy_AI/Enemy_Minion.h"
#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "TimerManager.h"

AEnemySpawner::AEnemySpawner()
{
	PrimaryActorTick.bCanEverTick = true;

	// 루트 컴포넌트로 Scene Component 생성
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	// 감지 범위 시각화용 Sphere Component
	DetectionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("DetectionSphere"));
	DetectionSphere->SetupAttachment(RootComponent);
	DetectionSphere->SetSphereRadius(DetectionRadius);
	DetectionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	DetectionSphere->SetHiddenInGame(true); // 게임 플레이 중 숨김
}

void AEnemySpawner::BeginPlay()
{
	Super::BeginPlay();
	
	// DetectionSphere 반지름을 설정값으로 업데이트
	if (DetectionSphere)
	{
		DetectionSphere->SetSphereRadius(DetectionRadius);
	}

	// 소환할 클래스 유효성 검사
	if (!EnemyClassToSpawn)
	{
		UE_LOG(LogTemp, Error, TEXT("EnemySpawner [%s]: EnemyClassToSpawn is not set!"), *GetName());
	}
}

void AEnemySpawner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	CheckPlayerDetection();
}

void AEnemySpawner::CheckPlayerDetection()
{
	// 플레이어 캐릭터 가져오기
	ACharacter* PlayerCharacter = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
	if (!PlayerCharacter)
	{
		return;
	}

	// 플레이어와의 거리 계산
	float DistanceToPlayer = FVector::Dist(GetActorLocation(), PlayerCharacter->GetActorLocation());
	bool bPlayerInRange = DistanceToPlayer <= DetectionRadius;

	// 플레이어가 범위 내에 들어왔을 때
	if (bPlayerInRange && !bPlayerDetected)
	{
		bPlayerDetected = true;

		// 아직 소환할 Enemy가 남아있으면 타이머 시작
		if (CurrentSpawnedCount < EnemiesToSpawn)
		{
			UE_LOG(LogTemp, Log, TEXT("EnemySpawner [%s]: Player detected! Starting spawn sequence..."), *GetName());
			StartSpawnTimer();
		}
	}
	// 플레이어가 범위를 벗어났을 때
	else if (!bPlayerInRange && bPlayerDetected)
	{
		bPlayerDetected = false;

		// 소환 타이머 중지
		StopSpawnTimer();
		UE_LOG(LogTemp, Warning, TEXT("EnemySpawner [%s]: Player left detection range. Spawn paused at %d/%d"), 
			*GetName(), CurrentSpawnedCount, EnemiesToSpawn);
	}
}

void AEnemySpawner::StartSpawnTimer()
{
	if (!GetWorld())
	{
		return;
	}

	// 이미 타이머가 활성화되어 있으면 중복 시작 방지
	if (GetWorld()->GetTimerManager().IsTimerActive(SpawnTimerHandle))
	{
		return;
	}

	// SpawnInterval 간격으로 반복 타이머 설정
	GetWorld()->GetTimerManager().SetTimer(
		SpawnTimerHandle,
		this,
		&AEnemySpawner::SpawnEnemyWithTimer,
		SpawnInterval,
		true, // 반복
		SpawnInterval // 첫 실행까지 대기 시간 (SpawnInterval 후 첫 소환)
	);

	UE_LOG(LogTemp, Log, TEXT("EnemySpawner [%s]: Spawn timer started. First spawn in %.1f seconds"), 
		*GetName(), SpawnInterval);
}

void AEnemySpawner::StopSpawnTimer()
{
	if (!GetWorld())
	{
		return;
	}

	// 타이머 중지
	GetWorld()->GetTimerManager().ClearTimer(SpawnTimerHandle);
}

void AEnemySpawner::SpawnEnemyWithTimer()
{
	// 소환 완료 확인
	if (CurrentSpawnedCount >= EnemiesToSpawn)
	{
		StopSpawnTimer();
		UE_LOG(LogTemp, Log, TEXT("EnemySpawner [%s]: All enemies spawned (%d/%d)"), 
			*GetName(), CurrentSpawnedCount, EnemiesToSpawn);
		return;
	}

	// 플레이어가 여전히 범위 내에 있는지 확인 (이중 체크)
	if (!bPlayerDetected)
	{
		StopSpawnTimer();
		return;
	}

	// Enemy 소환
	SpawnEnemy();
	CurrentSpawnedCount++;

	UE_LOG(LogTemp, Log, TEXT("EnemySpawner [%s]: Spawned enemy %d/%d"), 
		*GetName(), CurrentSpawnedCount, EnemiesToSpawn);

	// 마지막 Enemy 소환 후 타이머 정리
	if (CurrentSpawnedCount >= EnemiesToSpawn)
	{
		StopSpawnTimer();
	}
}

void AEnemySpawner::SpawnEnemy()
{
	if (!EnemyClassToSpawn)
	{
		UE_LOG(LogTemp, Error, TEXT("EnemySpawner [%s]: Cannot spawn - EnemyClassToSpawn is null!"), *GetName());
		return;
	}

	// 랜덤한 소환 위치 계산
	FVector SpawnLocation = GetRandomSpawnLocation();
	FRotator SpawnRotation = FRotator::ZeroRotator;

	// SpawnParameters 설정
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	// Enemy 소환
	AEnemy_Base* SpawnedEnemy = GetWorld()->SpawnActor<AEnemy_Base>(
		EnemyClassToSpawn,
		SpawnLocation,
		SpawnRotation,
		SpawnParams
	);

	if (SpawnedEnemy)
	{
		// Enemy 레벨 설정
		SpawnedEnemy->SetLevel(EnemyLevel);

		// 소환된 Enemy 배열에 추가
		SpawnedEnemies.Add(SpawnedEnemy);

		// Enemy_Minion인 경우 사망 델리게이트 구독
		AEnemy_Minion* Minion = Cast<AEnemy_Minion>(SpawnedEnemy);
		if (Minion)
		{
			Minion->OnMinionDeath.AddDynamic(this, &AEnemySpawner::OnSpawnedEnemyDied);
		}

		UE_LOG(LogTemp, Log, TEXT("EnemySpawner [%s]: Spawned Enemy [%s] at location %s with Level %.1f"), 
			*GetName(), *SpawnedEnemy->GetName(), *SpawnLocation.ToString(), EnemyLevel);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("EnemySpawner [%s]: Failed to spawn Enemy!"), *GetName());
	}
}

FVector AEnemySpawner::GetRandomSpawnLocation() const
{
	// 감지 범위 내에서 랜덤한 2D 위치 생성
	float RandomAngle = FMath::FRandRange(0.0f, 2.0f * PI);
	float RandomRadius = FMath::FRandRange(0.0f, DetectionRadius);

	FVector RandomOffset = FVector(
		FMath::Cos(RandomAngle) * RandomRadius,
		FMath::Sin(RandomAngle) * RandomRadius,
		0.0f // Z축은 스포너와 같은 높이
	);

	return GetActorLocation() + RandomOffset;
}

void AEnemySpawner::OnSpawnedEnemyDied(AEnemy_Minion* DeadMinion)
{
	if (!DeadMinion)
	{
		return;
	}

	// 사망한 Enemy를 배열에서 제거
	SpawnedEnemies.Remove(DeadMinion);

	UE_LOG(LogTemp, Log, TEXT("EnemySpawner [%s]: Enemy [%s] died. Remaining enemies: %d"), 
		*GetName(), *DeadMinion->GetName(), SpawnedEnemies.Num());

	// 모든 적이 사망했는지 확인
	CheckAllEnemiesDefeated();
}

void AEnemySpawner::CheckAllEnemiesDefeated()
{
	// 유효한 Enemy가 하나도 남아있지 않고, 모든 소환이 완료되었으면
	if (SpawnedEnemies.Num() == 0 && 
		CurrentSpawnedCount >= EnemiesToSpawn && 
		!bHasNotifiedPotionSpawners)
	{
		UE_LOG(LogTemp, Warning, TEXT("EnemySpawner [%s]: All enemies defeated!"), *GetName());

		// 포션 스포너들에게 알림
		NotifyPotionSpawners();
	}
}

void AEnemySpawner::NotifyPotionSpawners()
{
	if (bHasNotifiedPotionSpawners)
	{
		return; // 중복 알림 방지
	}

	bHasNotifiedPotionSpawners = true;

	// 연결된 모든 포션 스포너에게 알림
	for (AActor* PotionSpawner : ConnectedPotionSpawners)
	{
		if (PotionSpawner)
		{
			UE_LOG(LogTemp, Log, TEXT("EnemySpawner [%s]: Notifying PotionSpawner [%s]"), 
				*GetName(), *PotionSpawner->GetName());

			// BP_PotionSpawner의 SpawnPotion 함수 호출
			UFunction* SpawnFunction = PotionSpawner->FindFunction(FName(TEXT("SpawnPotion")));
			if (SpawnFunction)
			{
				PotionSpawner->ProcessEvent(SpawnFunction, nullptr);
			}
		}
	}

	if (ConnectedPotionSpawners.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("EnemySpawner [%s]: No connected PotionSpawners to notify!"), *GetName());
	}
}