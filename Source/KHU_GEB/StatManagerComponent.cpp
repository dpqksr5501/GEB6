// Fill out your copyright notice in the Description page of Project Settings.


#include "StatManagerComponent.h"

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
			Stats->Level = FMath::Max(1, Stats->Level + 1);

			Stats->RecalculateDerivedStats();


			////
			OnFormLevelUp.Broadcast(FormType, Stats->Level);
			////
		}
	}
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
		Stats->Level = FMath::Max(1, Stats->Level + Amount);
		Stats->RecalculateDerivedStats();

		////
		OnFormLevelUp.Broadcast(FormType, Stats->Level);
		////
	}
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
