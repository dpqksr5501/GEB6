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
	}
}