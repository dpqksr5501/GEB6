#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Templates/SubclassOf.h"
#include "EnemySpawnDirector.generated.h"

class AEnemy_Base;

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
     * 레벨의 최대 스폰 예산을 설정합니다. (예: 레벨 블루프린트의 BeginPlay에서 호출)
     * @param NewMax 전체 최대 스폰 수 (모든 Enemy 타입 합계)
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


    /** 현재까지 스폰된 횟수를 반환합니다. */
    UFUNCTION(BlueprintPure, Category = "Enemy Spawn Director")
    int32 GetCurrentSpawnCount() const { return TotalSpawnedCount; }

    /**
     * 특정 Enemy 타입의 현재 스폰 수를 반환합니다.
     * @param EnemyClass 조회할 Enemy 클래스
     * @return 해당 클래스의 현재 스폰 수
     */
    UFUNCTION(BlueprintPure, Category = "Enemy Spawn Director")
    int32 GetCurrentSpawnCountForEnemyType(TSubclassOf<AEnemy_Base> EnemyClass) const;

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

    /** 카운터를 0으로 리셋합니다. */
    UFUNCTION(BlueprintCallable, Category = "Enemy Spawn Director")
    void ResetCounter() { TotalSpawnedCount = 0; SpawnedCounts.Reset(); }

    /**
     * 특정 Enemy 타입의 카운터를 0으로 리셋합니다.
     * @param EnemyClass 리셋할 Enemy 클래스
     */
    UFUNCTION(BlueprintCallable, Category = "Enemy Spawn Director")
    void ResetCounterForEnemyType(TSubclassOf<AEnemy_Base> EnemyClass);

private:
    /** 현재까지 총 스폰된 횟수 (예산 소모량) */
    int32 TotalSpawnedCount = 0;

    /** 이 레벨의 총 스폰 한도 (예산) */
    int32 MaxTotalSpawns = 30; // 기본값

    /** Enemy 타입별 현재까지 스폰된 횟수 */
    UPROPERTY()
    TMap<TSubclassOf<AEnemy_Base>, int32> SpawnedCounts;

    /** Enemy 타입별 최대 스폰 한도 */
    UPROPERTY()
    TMap<TSubclassOf<AEnemy_Base>, int32> MaxSpawnsPerType;
};