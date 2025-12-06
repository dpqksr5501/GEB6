#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Templates/SubclassOf.h"
#include "EnemySpawnDirector.generated.h"

class AEnemy_Base;

// 모든 적이 처치되었을 때 호출되는 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAllEnemiesDefeated);

/**
 * 게임 레벨 전체의 적 스폰 수를 관리하는 서브시스템
 * 개별 스포너가 스폰을 요청할 시 스폰 수를 확인 후 허가/거부
 * 여러 Enemy 타입별로 독립적인 스폰 관리
 */
UCLASS()
class KHU_GEB_API UEnemySpawnDirector : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    /**
     * 레벨의 최대 동시 스폰 수를 설정합니다. (예: 레벨 블루프린트의 BeginPlay에서 호출)
     * @param NewMax 동시에 맵에 존재할 수 있는 최대 Enemy 수
     */
    UFUNCTION(BlueprintCallable, Category = "Enemy Spawn Director")
    void SetMaxConcurrentSpawns(int32 NewMax);

    /**
     * 총 누적 소환 한도를 설정합니다.
     * @param NewMax 게임 전체에서 소환할 수 있는 총 Enemy 수
     */
    UFUNCTION(BlueprintCallable, Category = "Enemy Spawn Director")
    void SetMaxTotalSpawns(int32 NewMax);

    /**
     * 특정 Enemy 타입의 최대 스폰 수를 설정합니다.
     * @param EnemyClass 설정할 Enemy 클래스
     * @param MaxCount 해당 클래스의 최대 스폰 수
     */
    UFUNCTION(BlueprintCallable, Category = "Enemy Spawn Director")
    void SetMaxSpawnsForEnemyType(TSubclassOf<AEnemy_Base> EnemyClass, int32 MaxCount);

    /**
     * 스폰을 요청합니다. 예산이 남아있으면 1 소모하고 true를 반환합니다.
     * @param EnemyClass 스폰할 Enemy 클래스
     * @return 스폰이 허가되면 true, 예산이 없으면 false
     */
    UFUNCTION(BlueprintCallable, Category = "Enemy Spawn Director")
    bool RequestSpawn(TSubclassOf<AEnemy_Base> EnemyClass);

    /**
     * Enemy가 사망했을 때 호출됩니다. 현재 살아있는 수를 감소시킵니다.
     * @param EnemyClass 사망한 Enemy 클래스
     */
    UFUNCTION(BlueprintCallable, Category = "Enemy Spawn Director")
    void OnEnemyDied(TSubclassOf<AEnemy_Base> EnemyClass);

    /** 현재 살아있는 Enemy 수를 반환합니다. */
    UFUNCTION(BlueprintPure, Category = "Enemy Spawn Director")
    int32 GetCurrentAliveCount() const { return CurrentAliveCount; }

    /** 현재까지 누적 스폰된 횟수를 반환합니다. */
    UFUNCTION(BlueprintPure, Category = "Enemy Spawn Director")
    int32 GetCurrentSpawnCount() const { return TotalSpawnedCount; }

    /**
     * 특정 Enemy 타입의 현재 스폰 수를 반환합니다.
     * @param EnemyClass 조회할 Enemy 클래스
     * @return 해당 클래스의 현재 스폰 수
     */
    UFUNCTION(BlueprintPure, Category = "Enemy Spawn Director")
    int32 GetCurrentSpawnCountForEnemyType(TSubclassOf<AEnemy_Base> EnemyClass) const;

    /** 설정된 최대 동시 스폰 수를 반환합니다. */
    UFUNCTION(BlueprintPure, Category = "Enemy Spawn Director")
    int32 GetMaxConcurrentSpawns() const { return MaxConcurrentSpawns; }

    /** 설정된 최대 스폰 예산을 반환합니다. */
    UFUNCTION(BlueprintPure, Category = "Enemy Spawn Director")
    int32 GetMaxTotalSpawns() const { return MaxTotalSpawns; }

    /**
     * 특정 Enemy 타입의 최대 스폰 수를 반환합니다.
     * @param EnemyClass 조회할 Enemy 클래스
     * @return 해당 클래스의 최대 스폰 수
     */
    UFUNCTION(BlueprintPure, Category = "Enemy Spawn Director")
    int32 GetMaxSpawnsForEnemyType(TSubclassOf<AEnemy_Base> EnemyClass) const;

    /** 모든 카운터를 0으로 리셋합니다. */
    UFUNCTION(BlueprintCallable, Category = "Enemy Spawn Director")
    void ResetCounter() { TotalSpawnedCount = 0; CurrentAliveCount = 0; SpawnedCounts.Reset(); AliveCounts.Reset(); bAllEnemiesDefeated = false; }

    /**
     * 특정 Enemy 타입의 카운터를 0으로 리셋합니다.
     * @param EnemyClass 리셋할 Enemy 클래스
     */
    UFUNCTION(BlueprintCallable, Category = "Enemy Spawn Director")
    void ResetCounterForEnemyType(TSubclassOf<AEnemy_Base> EnemyClass);

    /**
     * 모든 카운터를 초기화합니다. 레벨 전환 시 호출하여 이전 레벨의 카운트를 정리합니다.
     */
    UFUNCTION(BlueprintCallable, Category = "Enemy Spawn Director")
    void ResetAllCounters();

    /** 모든 적이 처치되었을 때 발생하는 델리게이트 */
    UPROPERTY(BlueprintAssignable, Category = "Enemy Spawn Director")
    FOnAllEnemiesDefeated OnAllEnemiesDefeated;

private:
    /** 현재까지 총 누적 스폰된 횟수 (감소하지 않음) */
    int32 TotalSpawnedCount = 0;

    /** 현재 살아있는 Enemy 수 (사망 시 감소) */
    int32 CurrentAliveCount = 0;

    /** 동시에 맵에 존재할 수 있는 최대 Enemy 수 */
    int32 MaxConcurrentSpawns = 0;

    /** 총 누적 소환 한도 */
    int32 MaxTotalSpawns = 0;

    /** 모든 적이 처치되었는지 여부 (이벤트 중복 방지) */
    bool bAllEnemiesDefeated = false;

    /** Enemy 타입별 현재 살아있는 수 */
    UPROPERTY()
    TMap<TSubclassOf<AEnemy_Base>, int32> AliveCounts;

    /** Enemy 타입별 현재까지 스폰된 횟수 (예산 소모량) */
    UPROPERTY()
    TMap<TSubclassOf<AEnemy_Base>, int32> SpawnedCounts;

    /** Enemy 타입별 최대 스폰 한도 */
    UPROPERTY()
    TMap<TSubclassOf<AEnemy_Base>, int32> MaxSpawnsPerType;
};