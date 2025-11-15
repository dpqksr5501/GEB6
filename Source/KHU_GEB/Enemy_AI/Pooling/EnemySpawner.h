#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EnemySpawner.generated.h"

class UEnemyPoolSubsystem;
class UEnemySpawnDirector;
class AEnemy_Base;

/**
 * 레벨에 배치되어 실제 적 스폰을 담당하는 액터입니다.
 * 지정된 시간 간격으로, 위치가 비어있는지 확인한 뒤,
 * Director에게 예산을 요청하고, Pool에서 적을 가져와 배치합니다.
 */
UCLASS()
class KHU_GEB_API AEnemySpawner : public AActor
{
    GENERATED_BODY()

public:
    AEnemySpawner();
    virtual void Tick(float DeltaTime) override;

protected:
    virtual void BeginPlay() override;

    /** 이 스포너가 스폰할 적(Enemy) 블루프린트 클래스 */
    UPROPERTY(EditAnywhere, Category = "Enemy Spawner")
    TSubclassOf<AEnemy_Base> EnemyClassToSpawn;

    /** 각 스폰 시도 사이의 시간 간격 (초) */
    UPROPERTY(EditAnywhere, Category = "Enemy Spawner")
    float SpawnInterval = 2.0f;

    /** 스폰 포인트가 비어있는지 확인할 때 사용할 반경 */
    UPROPERTY(EditAnywhere, Category = "Enemy Spawner")
    float SpawnPointCheckRadius = 100.0f;

private:
    /** 다음 스폰까지 남은 시간 */
    float SpawnTimer = 0.0f;

    /** 스폰을 시도하는 메인 로직 */
    void TrySpawnEnemy();

    /** 스폰 포인트가 다른 적에 의해 점유되었는지 확인합니다. */
    bool IsOccupied() const;

    /** 사용할 EnemyPool과 Spawndirector */
    UPROPERTY()
    TObjectPtr<UEnemyPoolSubsystem> EnemyPool;

    UPROPERTY()
    TObjectPtr<UEnemySpawnDirector> SpawnDirector;
};