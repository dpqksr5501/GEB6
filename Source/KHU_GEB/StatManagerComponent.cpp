// Fill out your copyright notice in the Description page of Project Settings.


#include "StatManagerComponent.h"

namespace
{
	// 폼 공통 최대 레벨
	constexpr int32 GMaxFormLevel = 5;
}

void FFormRuntimeStats::InitializeFromData(const UFormStatData* Data)
{
	if (!Data) return;

	AttackPerLevel = Data->AttackPerLevel;
	DefensePerLevel = Data->DefensePerLevel;
	WalkSpeed = Data->WalkSpeed;
	SprintSpeed = Data->SprintSpeed;
	Acceleration = Data->Acceleration;

	Level = Data->StartLevel;

	bFirstMinionKillGrantsLevel = Data->bFirstMinionKillGrantsLevel;
	MinionKillsPerExtraLevel = Data->MinionKillsPerExtraLevel;

	MinionKills = 0;
	bKilledElite = false;
	bKilledBoss = false;

	RecalculateDerivedStats();
}

// 레벨이 바뀌었을 때 실제 공격/방어 다시 계산
void FFormRuntimeStats::RecalculateDerivedStats()
{
	Attack = AttackPerLevel * Level;
	Defense = DefensePerLevel * Level;
}

UStatManagerComponent::UStatManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UStatManagerComponent::InitializeFromFormSet(const UFormSet* FormSet)
{
	RuntimeStats.Empty();

	if (!FormSet) return;

	for (const auto& Pair : FormSet->Forms)
	{
		const EFormType FormType = Pair.Key;
		const UFormDefinition* Def = Pair.Value;

		if (!Def || !Def->StatData) continue;

		FFormRuntimeStats NewStats;
		NewStats.InitializeFromData(Def->StatData);

		RuntimeStats.Add(FormType, NewStats);
	}

	// 초기화 이후, Base 폼 레벨을 다른 폼들 최고 레벨에 맞춰 한 번 정리
	SyncBaseLevelToHighest(false);
}

const FFormRuntimeStats* UStatManagerComponent::GetStats(EFormType FormType) const
{
	return RuntimeStats.Find(FormType);
}

FFormRuntimeStats* UStatManagerComponent::GetStatsMutable(EFormType FormType)
{
	return RuntimeStats.Find(FormType);
}

void UStatManagerComponent::AddMinionKill(EFormType FormType)
{
	if (FFormRuntimeStats* Stats = RuntimeStats.Find(FormType))
	{
		Stats->MinionKills++;

		bool bShouldLevelUp = false;

		// 1) 첫 Minion을 잡으면 레벨 +1
		if (Stats->bFirstMinionKillGrantsLevel && Stats->MinionKills == 1)
		{
			bShouldLevelUp = true;
		}

		// 2) MinionKillsPerExtraLevel 마리마다 레벨 +1
		if (Stats->MinionKillsPerExtraLevel > 0 &&
			Stats->MinionKills > 0 &&
			Stats->MinionKills % Stats->MinionKillsPerExtraLevel == 0)
		{
			bShouldLevelUp = true;
		}

		if (bShouldLevelUp)
		{
			// 여기서 최대 5레벨로 클램프
			const int32 NewLevel = FMath::Clamp(Stats->Level + 1, 1, GMaxFormLevel);

			// 이미 최대 레벨이면 더 올리지 않음
			if (NewLevel != Stats->Level)
			{
				Stats->Level = NewLevel;
				Stats->RecalculateDerivedStats();

				OnFormLevelUp.Broadcast(FormType, Stats->Level);
			}
		}
	}

	SyncBaseLevelToHighest(true);
}

void UStatManagerComponent::MarkEliteKilled(EFormType FormType)
{
	if (FFormRuntimeStats* Stats = RuntimeStats.Find(FormType))
	{
		// 한 번 true되면 그대로 유지
		Stats->bKilledElite = true;
	}
}

void UStatManagerComponent::MarkBossKilled(EFormType FormType)
{
	if (FFormRuntimeStats* Stats = RuntimeStats.Find(FormType))
	{
		Stats->bKilledBoss = true;
	}
}

void UStatManagerComponent::AddLevel(EFormType FormType, int32 Amount)
{
	if (FFormRuntimeStats* Stats = RuntimeStats.Find(FormType))
	{
		const int32 OldLevel = Stats->Level;

		// 1 ~ GMaxFormLevel 사이로 클램프
		Stats->Level = FMath::Clamp(Stats->Level + Amount, 1, GMaxFormLevel);

		// 레벨이 실제로 변했을 때만 처리
		if (Stats->Level != OldLevel)
		{
			Stats->RecalculateDerivedStats();
			OnFormLevelUp.Broadcast(FormType, Stats->Level);
		}
	}

	// 퀘스트 보상 등으로 레벨을 직접 올려도 Base가 최고 레벨을 따라가도록 유지
	SyncBaseLevelToHighest(true);
}


////
int32 UStatManagerComponent::GetLevelStat(EFormType FormType) const
{
	if (const FFormRuntimeStats* Stats = RuntimeStats.Find(FormType))
	{
		return Stats->Level;
	}

	// 못 찾으면 0 리턴 (원하면 1로 바꿔도 됨)
	return 0;
}

int32 UStatManagerComponent::GetMinionKills(EFormType FormType) const
{
	if (const FFormRuntimeStats* Stats = RuntimeStats.Find(FormType))
	{
		return Stats->MinionKills;
	}
	return 0;
}

int32 UStatManagerComponent::GetRequiredMinionKillsForThisLevel(EFormType FormType) const
{
	if (const FFormRuntimeStats* Stats = RuntimeStats.Find(FormType))
	{
		// 아직 한 마리도 안 잡았고, 첫 킬에 레벨업이면 → 1마리 필요
		if (Stats->bFirstMinionKillGrantsLevel && Stats->MinionKills == 0)
		{
			return 1;
		}

		// 이후에는 MinionKillsPerExtraLevel 기준
		if (Stats->MinionKillsPerExtraLevel > 0)
		{
			return Stats->MinionKillsPerExtraLevel;
		}

		// 첫 킬만 레벨업이고 더 이상 안 오르면 0
		return 0;
	}
	return 0;
}

int32 UStatManagerComponent::GetRemainingMinionKillsToNextLevel(EFormType FormType) const
{
	if (const FFormRuntimeStats* Stats = RuntimeStats.Find(FormType))
	{
		const int32 Required = GetRequiredMinionKillsForThisLevel(FormType);
		if (Required <= 0)
		{
			return 0;
		}

		// 첫 킬만 레벨업이고, 이미 한 마리 잡았고, 추가 레벨업이 없다면
		if (Stats->bFirstMinionKillGrantsLevel &&
			Stats->MinionKills >= 1 &&
			Stats->MinionKillsPerExtraLevel <= 0)
		{
			return 0;
		}

		// 첫 킬 전 구간
		if (Stats->bFirstMinionKillGrantsLevel && Stats->MinionKills == 0)
		{
			return 1;
		}

		// 그 이후 구간: 첫 킬 후부터를 기준으로 세그먼트 계산
		int32 BaseKills = Stats->bFirstMinionKillGrantsLevel
			? Stats->MinionKills - 1       // 첫 킬 이후부터 카운트
			: Stats->MinionKills;

		if (BaseKills < 0)
		{
			BaseKills = 0;
		}

		const int32 KillsInCycle = BaseKills % Required;

		if (KillsInCycle == 0)
		{
			// 방금 레벨업 직후 → 다음 레벨까지 Required 마리 필요
			return Required;
		}

		return Required - KillsInCycle;
	}
	return 0;
}

float UStatManagerComponent::GetMinionKillProgress01(EFormType FormType) const
{
	if (const FFormRuntimeStats* Stats = RuntimeStats.Find(FormType))
	{
		const int32 Required = GetRequiredMinionKillsForThisLevel(FormType);
		if (Required <= 0)
		{
			return 1.0f;
		}

		// 첫 킬 전
		if (Stats->bFirstMinionKillGrantsLevel && Stats->MinionKills == 0)
		{
			return 0.0f;
		}

		// 첫 킬만 레벨업이고 더 이상 안 오르면 항상 1
		if (Stats->bFirstMinionKillGrantsLevel &&
			Stats->MinionKills >= 1 &&
			Stats->MinionKillsPerExtraLevel <= 0)
		{
			return 1.0f;
		}

		int32 BaseKills = Stats->bFirstMinionKillGrantsLevel
			? Stats->MinionKills - 1
			: Stats->MinionKills;

		if (BaseKills < 0)
		{
			BaseKills = 0;
		}

		const int32 KillsInCycle = BaseKills % Required;

		if (KillsInCycle == 0)
		{
			// 레벨업 직후 → 게이지 0으로 리셋된 느낌
			return 0.0f;
		}

		const float Progress = static_cast<float>(KillsInCycle) / static_cast<float>(Required);
		return FMath::Clamp(Progress, 0.0f, 1.0f);
	}
	return 0.0f;
}

void UStatManagerComponent::RegisterKill(EFormType FormType, EEnemyKind Kind)
{
	switch (Kind)
	{
	case EEnemyKind::Minion:
		// 기존 로직 그대로: 미니언은 경험치/레벨업 처리
		AddMinionKill(FormType);
		break;

	case EEnemyKind::Elite:
		// Elite 처치: 해당 폼의 bKilledElite = true
		MarkEliteKilled(FormType);
		break;

	case EEnemyKind::Boss:
		// Boss 처치: 해당 폼의 bKilledBoss = true
		MarkBossKilled(FormType);
		break;

	default:
		// 혹시 모르는 값은 Minion처럼 처리
		AddMinionKill(FormType);
		break;
	}
}

void UStatManagerComponent::SyncBaseLevelToHighest(bool bBroadcastEvent)
{
	// Base 폼이 없으면 아무 것도 하지 않음
	FFormRuntimeStats* BaseStats = RuntimeStats.Find(EFormType::Base);
	if (!BaseStats) return;

	int32 MaxOtherLevel = 0;

	// 다른 폼들 중 최대 레벨을 찾는다.
	for (const auto& Pair : RuntimeStats)
	{
		const EFormType FormType = Pair.Key;
		const FFormRuntimeStats& Stats = Pair.Value;

		if (FormType == EFormType::Base) continue;

		if (Stats.Level > MaxOtherLevel)
		{
			MaxOtherLevel = Stats.Level;
		}
	}

	// Base는 항상 최소 1레벨 유지 + 최대 GMaxFormLevel 제한
	const int32 DesiredLevel =
		FMath::Clamp(FMath::Max(1, MaxOtherLevel), 1, GMaxFormLevel);


	// 이미 원하는 레벨이면 건드리지 않는다.
	if (BaseStats->Level == DesiredLevel) return;

	BaseStats->Level = DesiredLevel;
	BaseStats->RecalculateDerivedStats();

	if (bBroadcastEvent)
	{
		OnFormLevelUp.Broadcast(EFormType::Base, BaseStats->Level);
	}
}
