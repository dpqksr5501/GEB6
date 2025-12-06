#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EnemySpawner.generated.h"

class AEnemy_Base;
class AEnemy_Minion;
class USphereComponent;

/**
 * 플레이어 감지 시 Enemy를 직접 소환하는 스포너
 * 소환한 적들을 추적하며, 모두 사망 시 연결된 포션 스포너들에게 알림
 */
UCLASS()
class KHU_GEB_API AEnemySpawner : public AActor
{
	GENERATED_BODY()
	
public:	
	AEnemySpawner();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

	/** Enemy_Minion이 사망했을 때 호출되는 함수 */
	UFUNCTION()
	void OnSpawnedEnemyDied(AEnemy_Minion* DeadMinion);

private:
	/** 플레이어 감지 및 소환 처리 */
	void CheckPlayerDetection();

	/** Enemy를 실제로 소환하는 함수 */
	void SpawnEnemy();

	/** 타이머로 호출되는 소환 함수 */
	void SpawnEnemyWithTimer();

	/** 소환 타이머를 시작 */
	void StartSpawnTimer();

	/** 소환 타이머를 중지 */
	void StopSpawnTimer();

	/** 랜덤한 소환 위치를 계산 (감지 범위 내) */
	FVector GetRandomSpawnLocation() const;

	/** 모든 소환된 적이 사망했는지 확인 */
	void CheckAllEnemiesDefeated();

	/** 연결된 포션 스포너들에게 알림 */
	void NotifyPotionSpawners();

	// ===== 블루프린트 노출 변수 =====

	/** 플레이어 감지 범위 (반지름) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner Settings", meta = (AllowPrivateAccess = "true"))
	float DetectionRadius = 1500.0f;

	/** 소환할 Enemy 블루프린트 클래스 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner Settings", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<AEnemy_Base> EnemyClassToSpawn;

	/** 소환할 Enemy의 레벨 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner Settings", meta = (AllowPrivateAccess = "true"))
	float EnemyLevel = 1.0f;

	/** 소환할 Enemy의 총 개수 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner Settings", meta = (AllowPrivateAccess = "true", ClampMin = "1"))
	int32 EnemiesToSpawn = 5;

	/** Enemy 소환 간격 (초) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner Settings", meta = (AllowPrivateAccess = "true", ClampMin = "0.1"))
	float SpawnInterval = 1.0f;

	/** 이 스포너와 연결된 포션 스포너 배열 (BP_PotionSpawner 등) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner Settings", meta = (AllowPrivateAccess = "true"))
	TArray<AActor*> ConnectedPotionSpawners;

	// ===== 내부 상태 변수 =====

	/** 감지 범위를 시각화하는 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	USphereComponent* DetectionSphere;

	/** 현재 소환된 Enemy들의 참조 배열 */
	UPROPERTY()
	TArray<AEnemy_Base*> SpawnedEnemies;

	/** 플레이어가 감지되었는지 여부 */
	bool bPlayerDetected = false;

	/** 소환 타이머 핸들 */
	FTimerHandle SpawnTimerHandle;

	/** 현재까지 소환된 Enemy 수 */
	int32 CurrentSpawnedCount = 0;

	/** 모든 적 처치 알림을 이미 보냈는지 여부 */
	bool bHasNotifiedPotionSpawners = false;
};