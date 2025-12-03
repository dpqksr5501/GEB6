// Fill out your copyright notice in the Description page of Project Settings.


#include "Skills/Skill_Guard.h"
#include "GameFramework/Character.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "KHU_GEBCharacter.h"
#include "HealthComponent.h" 
#include "ManaComponent.h"
#include "SkillManagerComponent.h"
#include "Enemy_Base.h"

void USkill_Guard::InitializeFromDefinition(const USkillDefinition* Def)
{
    Super::InitializeFromDefinition(Def);

    if (Params.ManaCost > 0.f) { ManaPerShield = Params.ManaCost; }
    if (Params.Damage > 0.f) { DamagePerSheild = Params.Damage; }
    if (Params.Range > 0.f) { ExplosionRadius = Params.Range; }
}

bool USkill_Guard::CanActivate() const
{
    if (bIsActive)
    {
        UE_LOG(LogTemp, Verbose, TEXT("[Skill_Guard] CanActivate? false (already active)"));
        return false;
    }

    UManaComponent* Mana = GetManaComponent();
    if (!Mana)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Skill_Guard] CanActivate? false (no ManaComponent)"));
        return false;
    }

    const float Current = Mana->GetCurrentMana();
    const float Target = 100.f;
    const float Tolerance = 0.1f; // float 오차 방지용

    if (FMath::Abs(Current - Target) > Tolerance)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[Skill_Guard] CanActivate? false (Mana must be 100. Current=%.1f)"),
            Current);
        return false;
    }

    // 쿨타임은 지금은 사용하지 않음 (필요하면 여기서 Now < NextAvailableTime 체크 추가)
    return true;
}

void USkill_Guard::ActivateSkill()
{
    // 이미 켜져 있으면 다시 초기화하지 않고 무시
    if (bIsActive)
    {
        UE_LOG(LogTemp, Verbose,
            TEXT("[Skill_Guard] ActivateSkill called while already active. Ignoring."));
        return;
    }

    UWorld* World = GetWorld();
    AActor* Owner = GetOwner();
    if (!World || !Owner) return;

    //애니메이션 몽타주 재생
    PlayFormSkillMontage();

    bIsActive = true;
    bEndedByDepletion = false;

    RemainingShields = FMath::Max(MaxShields, 0);
    ConsumedShields = 0;

    if (UManaComponent* Mana = GetManaComponent()) { Mana->AddRegenBlock(); }

    if (USkillManagerComponent* Manager = GetSkillManager())
    {
        Manager->OnGuardSkillStarted(this);
    }

    // 보호막 이펙트 켜기
    if (SkillNS && !SpawnedNS)
    {
        UE_LOG(LogTemp, Log, TEXT("[Skill_Guard] Spawning shield effect."));
        SpawnedNS = UNiagaraFunctionLibrary::SpawnSystemAttached(
            SkillNS,
            Owner->GetRootComponent(),
            NAME_None,
            FVector::ZeroVector,
            FRotator::ZeroRotator,
            EAttachLocation::SnapToTarget,
            true);
    }

    UE_LOG(LogTemp, Log, TEXT("[Skill_Guard] Activated: MaxShields=%d"), RemainingShields);
}

bool USkill_Guard::HandleIncomingDamage(
    float Damage,
    const UDamageType* DamageType,
    AController* InstigatedBy,
    AActor* DamageCauser)
{
    // 스킬이 꺼져 있거나, 남은 배리어가 없으면 그냥 맞게 둔다.
    if (!bIsActive || RemainingShields <= 0)
    {
        UE_LOG(LogTemp, Verbose,
            TEXT("[Skill_Guard] HandleIncomingDamage: no shield (Active=%d, Remaining=%d)"),
            bIsActive ? 1 : 0, RemainingShields);
        return false;
    }

    // 배리어 1개 소모
    RemainingShields = FMath::Max(RemainingShields - 1, 0);
    ConsumedShields++;

    UE_LOG(LogTemp, Log,
        TEXT("[Skill_Guard] Damage absorbed. RemainingShields=%d, Consumed=%d"),
        RemainingShields, ConsumedShields);

    if (RemainingShields <= 0)
    {
        bEndedByDepletion = true;
        UE_LOG(LogTemp, Log,
            TEXT("[Skill_Guard] Shields depleted. No explosion will occur on stop."));
    }

    if (UManaComponent* Mana = GetManaComponent())
    {
        if (ManaPerShield > 0.f)
        {
            const float Before = Mana->GetCurrentMana();
            Mana->ConsumeMana(ManaPerShield);
            const float After = Mana->GetCurrentMana();

            UE_LOG(LogTemp, Log,
                TEXT("[Skill_Guard] Consumed mana per shield: %.1f -> %.1f (Delta=%.1f)"),
                Before, After, Before - After);
        }
    }

    return true;
}

void USkill_Guard::StopSkill()
{
    // 이미 꺼져 있으면 무시
    if (!bIsActive)
    {
        Super::StopSkill();
        return;
    }

    UWorld* World = GetWorld();
    AActor* Owner = GetOwner();

    if (USkillManagerComponent* Manager = GetSkillManager())
    {
        Manager->OnGuardSkillEnded(this);
    }

    //우클릭 떼면 몽타주 재생 해제
    if (ACharacter* OwnerChar = Cast<ACharacter>(Owner))
    {
        if (UAnimInstance* Anim = OwnerChar->GetMesh()->GetAnimInstance())
        {
            // 현재 재생 중인 몽타주가 있다면 0.2초 동안 보간 멈춤
            Anim->Montage_Stop(0.2f, nullptr);
        }
    }
    if (AEnemy_Base* OwnerChar = Cast<AEnemy_Base>(Owner))
    {
        if (UAnimInstance* Anim = OwnerChar->GetMesh()->GetAnimInstance())
        {
            // 현재 재생 중인 몽타주가 있다면 0.2초 동안 보간 멈춤
            Anim->Montage_Stop(0.2f, nullptr);
        }
    }


    // 이펙트 끄기
    if (SpawnedNS)
    {
        SpawnedNS->Deactivate();
        SpawnedNS = nullptr;
    }

    // --- 3-1. 마나를 0으로 설정 ---
    if (UManaComponent* Mana = GetManaComponent())
    {
        const float Current = Mana->GetCurrentMana();
        if (Current > 0.f)
        {
            // 현재 마나만큼 한 번에 소모 → 0이 됨
            Mana->ConsumeMana(Current);
        }
        UE_LOG(LogTemp, Log,
            TEXT("[Skill_Guard] Mana set to 0 on skill end (was %.1f)"), Current);

        Mana->RemoveRegenBlock();
    }

    // --- 3-2. 배리어가 소모된 만큼 광역 대미지 ---
    if (World && Owner && ConsumedShields > 0 && DamagePerSheild > 0.f && !bEndedByDepletion)
    {
        const float TotalDamage = DamagePerSheild * ConsumedShields;

        TArray<AActor*> IgnoreActors;
        IgnoreActors.Add(Owner);

        UE_LOG(LogTemp, Log,
            TEXT("[Skill_Guard] Stop: ConsumedShields=%d, TotalDamage=%.1f"),
            ConsumedShields, TotalDamage);

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
        // 폭발 범위 디버그 표시
        DrawDebugSphere(
            World,
            Owner->GetActorLocation(), // 폭발 중심
            ExplosionRadius,           // 반경
            32,                        // 세그먼트 수
            FColor::Yellow,            // 색
            false,                     // 영구 여부 (false: Duration 동안만 보임)
            1.5f,                      // Duration (초)
            0,                         // Depth Priority
            3.f                        // 선 두께
        );
#endif

        UGameplayStatics::ApplyRadialDamage(
            World,
            TotalDamage,
            Owner->GetActorLocation(),
            ExplosionRadius,
            nullptr,            // Default DamageType
            IgnoreActors,
            Owner,
            Owner->GetInstigatorController(),
            true);              // Do full damage
    }
    else
    {
        UE_LOG(LogTemp, Log,
            TEXT("[Skill_Guard] Stop: No shields consumed or no damage value. No explosion."));
    }

    // 상태 리셋
    bIsActive = false;
    RemainingShields = 0;
    ConsumedShields = 0;
    bEndedByDepletion = false;

    Super::StopSkill();
}
