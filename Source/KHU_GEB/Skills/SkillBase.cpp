// Fill out your copyright notice in the Description page of Project Settings.


#include "Skills/SkillBase.h"
#include "GameFramework/Actor.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "KHU_GEBCharacter.h"
#include "Enemy_Base.h"
#include "HealthComponent.h"
#include "ManaComponent.h"
#include "FormManagerComponent.h"
#include "SkillManagerComponent.h"

USkillBase::USkillBase() {}

USkillManagerComponent* USkillBase::GetSkillManager() const
{
    if (CachedManager.IsValid())
        return CachedManager.Get();


    if (AActor* Owner = GetOwner())
    {
        if (USkillManagerComponent* Comp = Owner->FindComponentByClass<USkillManagerComponent>())
        {
            CachedManager = Comp;
            return Comp;
        }
    }
    return nullptr;
}

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
    Params = Def ? Def->Params : FSkillParams{};
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

float USkillBase::DealSkillDamage(
    AActor* Target,
    float Amount,
    bool bIgnoreDefense,
    bool bPeriodic,
    int32 HitCount)
{
    UE_LOG(LogTemp, Log,
        TEXT("[SkillBase] DealSkillDamage: Target=%s, Amount=%.1f, IgnoreDefense=%d, Periodic=%d, HitCount=%d"),
		*GetNameSafe(Target), Amount, bIgnoreDefense ? 1 : 0, bPeriodic ? 1 : 0, HitCount);
    if (!Target || Amount <= 0.f)
    {
        return 0.f;
    }

    // 1) 우선 HealthComponent 경로 시도
    if (UHealthComponent* Health = Target->FindComponentByClass<UHealthComponent>())
    {
        FDamageSpec Spec;
        Spec.RawDamage = Amount;
        Spec.bIgnoreDefense = bIgnoreDefense;
        Spec.bPeriodic = bPeriodic;
        Spec.HitCount = HitCount;
        Spec.Instigator = GetOwner();
        Spec.SourceSkill = this;

        return Health->ApplyDamageSpec(Spec);
    }

    // 2) HealthComponent가 없다면, 기존 ApplyDamage로 대체
    AActor* OwnerActor = GetOwner();
    AController* InstigatorController =
        OwnerActor ? OwnerActor->GetInstigatorController() : nullptr;

    UGameplayStatics::ApplyDamage(
        Target,
        Amount,
        InstigatorController,
        OwnerActor,
        nullptr);

    return Amount;
}

//스킬 사용 시 몽타주 재생 헬퍼 함수
void USkillBase::PlayFormSkillMontage()
{
    AActor* Owner = GetOwner();
    if (!Owner) return;

    // 1. 먼저 플레이어 캐릭터로 시도
    if (AKHU_GEBCharacter* OwnerChar = Cast<AKHU_GEBCharacter>(Owner))
    {
        if (!OwnerChar->FormManager) return;

        // 현재 폼 정의 가져오기
        const UFormDefinition* Def = OwnerChar->FormManager->FindDef(OwnerChar->FormManager->CurrentForm);

        // 몽타주 재생
        if (Def && Def->SkillMontage)
        {
            if (UAnimInstance* Anim = OwnerChar->GetMesh()->GetAnimInstance())
            {
                Anim->Montage_Play(Def->SkillMontage);
                UE_LOG(LogTemp, Log, TEXT("[SkillBase] Playing Player SkillMontage"));
            }
        }
        return;
    }

    // 2. 플레이어가 아니면 Enemy로 시도
    if (AEnemy_Base* EnemyChar = Cast<AEnemy_Base>(Owner))
    {
        // Enemy도 DefaultFormDef를 가지고 있음
        if (!EnemyChar->DefaultFormDef) return;

        const UFormDefinition* Def = EnemyChar->DefaultFormDef;

        // 몽타주 재생
        if (Def && Def->SkillMontage)
        {
            if (UAnimInstance* Anim = EnemyChar->GetMesh()->GetAnimInstance())
            {
                Anim->Montage_Play(Def->SkillMontage);
                UE_LOG(LogTemp, Log, TEXT("[SkillBase] Playing Enemy SkillMontage"));
            }
        }
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("[SkillBase] Owner is neither Player nor Enemy!"));
}

EFormType USkillBase::GetCurrentFormType() const
{
    // 소유자가 KHU_GEBCharacter 이고 FormManager가 있다면, 그 값을 그대로 사용
    if (const AKHU_GEBCharacter* OwnerChar = Cast<AKHU_GEBCharacter>(GetOwner()))
    {
        if (OwnerChar->FormManager)
        {
            return OwnerChar->FormManager->CurrentForm;
        }
		else return EFormType::Base;
    }

    // Enemy 등 다른 타입에서 FormManager를 안 쓰는 경우를 대비한 기본값
	else if (const AEnemy_Base* EnemyChar = Cast<AEnemy_Base>(GetOwner()))
    {
        return EnemyChar->DefaultFormDef ? EnemyChar->DefaultFormDef->FormType : EFormType::Base;
    }
	else return EFormType::Base;
}

void ApplyFixedDotDamage(USkillBase* SourceSkill, ACharacter* Target, float DamagePerTick, int32 HitCount)
{
    if (!Target || DamagePerTick <= 0.f || HitCount <= 0) return;

    UHealthComponent* Health = Target->FindComponentByClass<UHealthComponent>();
    if (!Health) return;

    FDamageSpec Spec;
    Spec.RawDamage = DamagePerTick;
    Spec.bIgnoreDefense = true;     // 방어력 무시
    Spec.bPeriodic = true;          // 주기적(DoT) 플래그
    Spec.bFixedDot = true;          // 고정 도트 모드 ON
    Spec.HitCount = HitCount;
    Spec.Instigator = SourceSkill ? SourceSkill->GetOwner() : nullptr;
    Spec.SourceSkill = SourceSkill;

    Health->ApplyDamageSpec(Spec);
}