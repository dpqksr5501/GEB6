#include "EnemySpawnDirector.h"
#include "Enemy_AI/Enemy_Base.h"

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

    // 전체 스폰 한도 확인
    if (TotalSpawnedCount >= MaxTotalSpawns)
    {
        return false;
    }

    // 해당 타입의 스폰 한도 확인 (설정된 경우)
    // Find는 해시맵 방식이라 평균 O(1)이기 때문에 문제 없을 듯
    if (const int32* MaxForType = MaxSpawnsPerType.Find(EnemyClass))
    {
        const int32* CurrentForType = SpawnedCounts.Find(EnemyClass);

        int32 CurrentCount = CurrentForType ? *CurrentForType : 0;
        
        if (CurrentCount >= *MaxForType)
        {
            return false;
        }
    }

    // 스폰 허가 - 카운터 증가
    TotalSpawnedCount++;
    SpawnedCounts.FindOrAdd(EnemyClass, 0)++;

    return true; // 스폰 허가
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
        TotalSpawnedCount -= *Count; // 전체에서 해당 타입 카운트만큼 빼기
        *Count = 0;
    }
}