// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillBase.h"
#include "Engine/World.h"
#include "ManaComponent.h"

USkillBase::USkillBase() {}

UManaComponent* USkillBase::GetManaComponent() const
{
    if (CachedManaComp.IsValid())
        return CachedManaComp.Get();

    if (AActor* Owner = GetOwner())
    {
        if (UManaComponent* Comp = Owner->FindComponentByClass<UManaComponent>())
        {
            CachedManaComp = Comp;
            return Comp;
        }
    }
    return nullptr;
}

void USkillBase::InitializeFromDefinition(const USkillDefinition* Def)
{
    if (!Def) return;

    Params = Def->Params;
    // 필요하면 여기서 NextAvailableTime 초기화 등
}

bool USkillBase::CanActivate() const
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return false;
    }

    const float Now = World->GetTimeSeconds();

    // 1) 쿨타임 체크
    if (Now < NextAvailableTime)
    {
        UE_LOG(LogTemp, Warning, TEXT("[SkillBase] OnCooldown (%.2f sec left)"),
            NextAvailableTime - Now);
        return false;
    }

    // 2) 마나 체크
    if (UManaComponent* Mana = GetManaComponent())
    {
        const float Cost = Params.ManaCost;
        if (Cost > 0.f && !Mana->HasEnoughMana(Cost))
        {
            UE_LOG(LogTemp, Warning, TEXT("[SkillBase] Not enough mana. Need=%.1f, Current=%.1f"),
                Cost, Mana->GetCurrentMana());
            return false;
        }
    }

    // 여기까지 통과하면 "쓸 수 있다"
    return true;
}

void USkillBase::ActivateSkill()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    const float Now = World->GetTimeSeconds();

    // 1) 쿨타임 시작
    if (Params.Cooldown > 0.f)
    {
        NextAvailableTime = Now + Params.Cooldown;
    }

    // 2) 마나 소모
    if (UManaComponent* Mana = GetManaComponent())
    {
        const float Cost = Params.ManaCost;
        if (Cost > 0.f)
        {
            Mana->ConsumeMana(Cost);
        }
    }

    // 3) 디버그 로그
    UE_LOG(LogTemp, Log, TEXT("[SkillBase] ActivateSkill on %s (Slot=%d, Cost=%.1f, CD=%.2f)"),
        *GetNameSafe(this), (int32)Slot, Params.ManaCost, Params.Cooldown);
}

void USkillBase::StopSkill()
{
    UE_LOG(LogTemp, Log, TEXT("[SkillBase] StopSkill on %s"), *GetNameSafe(this));
}