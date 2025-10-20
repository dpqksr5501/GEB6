// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerStatsComponent.h"

// Sets default values for this component's properties
UPlayerStatsComponent::UPlayerStatsComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}


// Called when the game starts
void UPlayerStatsComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}

/*
// Called every frame
void UPlayerStatsComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}
*/

void UPlayerStatsComponent::RecalcForForm(EPlayerForm Form)
{
    if (FFormStatState* S = FormStats.Find(Form))
    {
        UFormStatsData* D = S->StatsData.LoadSynchronous();
        if (!D) return;
        auto clamp = [](int32 L, int32 Max) { return FMath::Clamp(L, 0, Max); };

        const int32 aL = clamp(S->Progress.AttackLevel, D->Attack.MaxLevel);
        const int32 dL = clamp(S->Progress.DefenseLevel, D->Defense.MaxLevel);
        const int32 mL = clamp(S->Progress.MoveSpeedLevel, D->MoveSpeed.MaxLevel);
        const int32 cL = clamp(S->Progress.SkillCooldownLevel, D->SkillCooldown.MaxLevel);

        S->Attack = D->Attack.Base + D->Attack.PerLevel * aL;
        S->Defense = D->Defense.Base + D->Defense.PerLevel * dL;
        S->MoveSpeed = D->MoveSpeed.Base + D->MoveSpeed.PerLevel * mL;
        // 쿨다운 최소 하한(예: 0.05초) 보정
        S->SkillCooldown = FMath::Max(0.05f, D->SkillCooldown.Base + D->SkillCooldown.PerLevel * cL);
    }
}

void UPlayerStatsComponent::ApplyUpgrade(EPlayerForm Form, EUpgradableStatType Stat, int32 Levels)
{
    if (FFormStatState* S = FormStats.Find(Form))
    {
        UFormStatsData* D = S->StatsData.LoadSynchronous();
        if (!D) return;
        auto add = [&](int32& Cur, int32 Max) { Cur = FMath::Clamp(Cur + Levels, 0, Max); };
        
        switch (Stat)
        {
        case EUpgradableStatType::Attack:        add(S->Progress.AttackLevel, D->Attack.MaxLevel); break;
        case EUpgradableStatType::Defense:       add(S->Progress.DefenseLevel, D->Defense.MaxLevel); break;
        case EUpgradableStatType::MoveSpeed:     add(S->Progress.MoveSpeedLevel, D->MoveSpeed.MaxLevel); break;
        case EUpgradableStatType::SkillCooldown: add(S->Progress.SkillCooldownLevel, D->SkillCooldown.MaxLevel); break;
        }
        RecalcForForm(Form);
    }
}

float UPlayerStatsComponent::GetAttack(EPlayerForm Form) const
{
    if (const auto* S = FormStats.Find(Form)) return S->Attack;
    return 0.f;
}

float UPlayerStatsComponent::GetDefense(EPlayerForm Form) const
{ 
    if (const auto* S = FormStats.Find(Form)) return S->Defense;
    return 0.f;
}

float UPlayerStatsComponent::GetMoveSpeed(EPlayerForm Form) const
{
    if (const auto* S = FormStats.Find(Form)) return S->MoveSpeed;
    return 600.f;
}

float UPlayerStatsComponent::GetSkillCooldown(EPlayerForm Form) const
{
    if (const auto* S = FormStats.Find(Form)) return S->SkillCooldown;
    return 5.f;
}
