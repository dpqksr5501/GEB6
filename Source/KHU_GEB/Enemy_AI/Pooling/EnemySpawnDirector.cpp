#include "EnemySpawnDirector.h"
#include "Enemy_AI/Enemy_Base.h"

void UEnemySpawnDirector::SetMaxConcurrentSpawns(int32 NewMax)
{
    MaxConcurrentSpawns = NewMax > 0 ? NewMax : 0;
}

void UEnemySpawnDirector::SetMaxTotalSpawns(int32 NewMax)
{
    MaxTotalSpawns = NewMax > 0 ? NewMax : 0;
}

void UEnemySpawnDirector::SetMaxSpawnsForEnemyType(TSubclassOf<AEnemy_Base> EnemyClass, int32 MaxCount)
{
    if (EnemyClass)
    {
        MaxSpawnsPerType.Add(EnemyClass, MaxCount > 0 ? MaxCount : 0);
    }
}

bool UEnemySpawnDirector::RequestSpawn(TSubclassOf<AEnemy_Base> EnemyClass)
{
    if (!EnemyClass)
    {
        return false;
    }

    // 1. 총 누적 스폰 한도 확인
    if (TotalSpawnedCount >= MaxTotalSpawns)
    {
        UE_LOG(LogTemp, Warning, TEXT("EnemySpawnDirector: Total spawn limit reached (%d/%d)"), 
               TotalSpawnedCount, MaxTotalSpawns);
        return false;
    }

    // 2. 현재 살아있는 Enemy 수 확인
    if (CurrentAliveCount >= MaxConcurrentSpawns)
    {
        UE_LOG(LogTemp, Warning, TEXT("EnemySpawnDirector: Concurrent spawn limit reached (%d/%d)"), 
               CurrentAliveCount, MaxConcurrentSpawns);
        return false;
    }

    // 3. 해당 타입의 스폰 한도 확인 (설정된 경우)
    if (const int32* MaxForType = MaxSpawnsPerType.Find(EnemyClass))
    {
        const int32* CurrentForType = SpawnedCounts.Find(EnemyClass);
        int32 CurrentCount = CurrentForType ? *CurrentForType : 0;
        
        if (CurrentCount >= *MaxForType)
        {
            UE_LOG(LogTemp, Warning, TEXT("EnemySpawnDirector: Type spawn limit reached for %s (%d/%d)"), 
                   *EnemyClass->GetName(), CurrentCount, *MaxForType);
            return false;
        }
    }

    // 스폰 허가 - 카운터 증가
    TotalSpawnedCount++;
    CurrentAliveCount++;
    SpawnedCounts.FindOrAdd(EnemyClass, 0)++;
    AliveCounts.FindOrAdd(EnemyClass, 0)++;

    UE_LOG(LogTemp, Log, TEXT("EnemySpawnDirector: Spawn approved. Total: %d/%d, Alive: %d/%d"), 
           TotalSpawnedCount, MaxTotalSpawns, CurrentAliveCount, MaxConcurrentSpawns);

    return true;
}

void UEnemySpawnDirector::OnEnemyDied(TSubclassOf<AEnemy_Base> EnemyClass)
{
    if (!EnemyClass)
    {
        return;
    }

    // 현재 살아있는 수 감소
    CurrentAliveCount = FMath::Max(0, CurrentAliveCount - 1);

    // 타입별 살아있는 수 감소
    if (int32* Count = AliveCounts.Find(EnemyClass))
    {
        *Count = FMath::Max(0, *Count - 1);
    }

    UE_LOG(LogTemp, Log, TEXT("EnemySpawnDirector: Enemy died. Total: %d/%d, Alive: %d/%d"), 
           TotalSpawnedCount, MaxTotalSpawns, CurrentAliveCount, MaxConcurrentSpawns);

    // 모든 적이 처치되었는지 확인
    // 조건: MaxTotalSpawns만큼 스폰되었고, 살아있는 적이 0이며, 아직 이벤트가 발생하지 않았을 때
    if (!bAllEnemiesDefeated && 
        TotalSpawnedCount >= MaxTotalSpawns && 
        CurrentAliveCount == 0)
    {
        bAllEnemiesDefeated = true;
        
        UE_LOG(LogTemp, Warning, TEXT("EnemySpawnDirector: All enemies defeated! Broadcasting event."));
        
        // 델리게이트 브로드캐스트 - BP_PotionSpawner가 구독하고 있으면 SpawnPotion 호출됨
        OnAllEnemiesDefeated.Broadcast();
    }
}

int32 UEnemySpawnDirector::GetCurrentSpawnCountForEnemyType(TSubclassOf<AEnemy_Base> EnemyClass) const
{
    if (const int32* Count = SpawnedCounts.Find(EnemyClass))
    {
        return *Count;
    }
    return 0;
}

int32 UEnemySpawnDirector::GetMaxSpawnsForEnemyType(TSubclassOf<AEnemy_Base> EnemyClass) const
{
    if (const int32* MaxCount = MaxSpawnsPerType.Find(EnemyClass))
    {
        return *MaxCount;
    }
    return -1; // 설정되지 않음을 의미
}

void UEnemySpawnDirector::ResetCounterForEnemyType(TSubclassOf<AEnemy_Base> EnemyClass)
{
    if (int32* Count = SpawnedCounts.Find(EnemyClass))
    {
        TotalSpawnedCount -= *Count;
        *Count = 0;
    }

    if (int32* AliveCount = AliveCounts.Find(EnemyClass))
    {
        CurrentAliveCount -= *AliveCount;
        *AliveCount = 0;
    }
}