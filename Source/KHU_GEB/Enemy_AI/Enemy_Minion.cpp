// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemy_AI/Enemy_Minion.h"
#include "Pooling/EnemyPoolSubsystem.h"
#include "Pooling/EnemySpawnDirector.h"
#include "TimerManager.h"

void AEnemy_Minion::BeginPlay() {
	Super::BeginPlay();
}

void AEnemy_Minion::OnDeath()
{
	UE_LOG(LogTemp, Warning, TEXT("AEnemy_Minion::OnDeath - Enemy has died! Waiting for death animation..."));

	Super::OnDeath();

	// 사망 시 모든 타이머 정리 (Swift 스킬 등의 지속 데미지 방지)
	UWorld* World = GetWorld();
	if (World)
	{
		World->GetTimerManager().ClearAllTimersForObject(this);
		UE_LOG(LogTemp, Log, TEXT("AEnemy_Minion::OnDeath - Cleared all timers for this actor."));
	}

	// 사망 몽타주 재생 후 지연 처리 (타이머 사용)
	if (World)
	{
		World->GetTimerManager().SetTimer(
			DeathTimerHandle,
			this,
			&AEnemy_Minion::ExecuteDelayedDeath,
			DeathDelay,
			false  // 한 번만 실행
		);
	}
}

void AEnemy_Minion::ExecuteDelayedDeath()
{
	UE_LOG(LogTemp, Warning, TEXT("AEnemy_Minion::ExecuteDelayedDeath - Returning to pool after %f seconds"), DeathDelay);

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