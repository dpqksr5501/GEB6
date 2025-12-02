
#include "HealthComponent.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "SkillBase.h" 
#include "Skills/Skill_Ultimate.h" 

UHealthComponent::UHealthComponent()
{
    PrimaryComponentTick.bCanEverTick = false; // 틱 안 쓰면 false 권장

    MaxHealth = 100.f; //테스트를 위해서 추가 11/24
    Health = 100.f; //테스트를 위해서 추가 11/24
}

void UHealthComponent::TickComponent(float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}


void UHealthComponent::BeginPlay()
{
    Super::BeginPlay();

    if (Health <= 0.f)
    {
        Health = MaxHealth;
    }

}

void UHealthComponent::InitializeHealth(float InMaxHealth, float InStartHealth)
{
    MaxHealth = FMath::Max(0.f, InMaxHealth);
    const float Start = (InStartHealth < 0.f) ? MaxHealth : InStartHealth;
    const float Clamped = FMath::Clamp(Start, 0.f, MaxHealth);
    const float Delta = Clamped - Health;
    Health = Clamped;
    OnHealthChanged.Broadcast(Health, Delta);
}

void UHealthComponent::ReduceHealth(float Amount)
{
    if (Amount <= 0.f || MaxHealth <= 0.f) return;

    const float NewHealth = FMath::Clamp(Health - Amount, 0.f, MaxHealth);
    const float Delta = NewHealth - Health; // 음수(감소량)
    ApplyHealth(NewHealth, Delta);
    HandleDeathIfNeeded();
}

void UHealthComponent::AddHealth(float Amount)
{
    if (Amount <= 0.f || MaxHealth <= 0.f)
    {
        return;
    }

    const float NewHealth = FMath::Clamp(Health + Amount, 0.f, MaxHealth);
    const float Delta = NewHealth - Health; // 양수(회복량)
    ApplyHealth(NewHealth, Delta);
}

void UHealthComponent::SetMaxHealth(float NewMaxHealth, bool bClampCurrentToNewMax)
{
    NewMaxHealth = FMath::Max(0.f, NewMaxHealth);

    // 현재 체력 비율 유지하고 싶다면 아래 두 줄을 사용:
    // const float Ratio = (MaxHealth > 0.f) ? (Health / MaxHealth) : 0.f;
    // Health = Ratio * NewMaxHealth;

    MaxHealth = NewMaxHealth;

    if (bClampCurrentToNewMax)
    {
        const float NewHealth = FMath::Clamp(Health, 0.f, MaxHealth);
        const float Delta = NewHealth - Health;
        ApplyHealth(NewHealth, Delta);
        HandleDeathIfNeeded();
    }
}

void UHealthComponent::ApplyHealth(float NewHealth, float Delta)
{
    if (FMath::IsNearlyEqual(NewHealth, Health)) return;
    Health = NewHealth;
    OnHealthChanged.Broadcast(Health, Delta);
}

void UHealthComponent::HandleDeathIfNeeded()
{
    if (Health <= 0.f)
    {
        OnDeath.Broadcast();
        // 필요 시: 소유 액터 처리 (Destroy 등)
        // if (AActor* Owner = GetOwner()) { Owner->Destroy(); }
    }
}

float UHealthComponent::ApplyDamageSpec(const FDamageSpec& Spec)
{   
    // 수정 가능한 로컬 사본
    FDamageSpec LocalSpec = Spec;

    UE_LOG(LogTemp, Log,
        TEXT("[HealthComponent] ApplyDamageSpec: RawDamage=%.1f, bIgnoreDefense=%d, bPeriodic=%d, HitCount=%d"),
        LocalSpec.RawDamage,
        LocalSpec.bIgnoreDefense ? 1 : 0,
        LocalSpec.bPeriodic ? 1 : 0,
        LocalSpec.HitCount);

    float FinalDamage = 0.f;

    // 0) Swift 궁극기 은신 중 첫 공격이면 공격력 배수 적용 + 은신 해제
    if (LocalSpec.Instigator.IsValid())
    {
        if (AActor* InstActor = LocalSpec.Instigator.Get())
        {
            if (USkill_Ultimate* Ultimate = InstActor->FindComponentByClass<USkill_Ultimate>())
            {
                if (Ultimate->IsSwiftStealthActive())
                {
                    // 도트/주기 데미지(Periodic)는 제외하고 “한 번 때리는 공격”에만 배수
                    if (!LocalSpec.bPeriodic && LocalSpec.RawDamage > 0.f)
                    {
                        const float Mult = Ultimate->GetSwiftAttackMultiplier();
                        if (Mult > 0.f && !FMath::IsNearlyEqual(Mult, 1.f))
                        {
                            LocalSpec.RawDamage *= Mult;
                        }
                    }

                    // 첫 공격과 동시에 은신 해제
                    Ultimate->OnAttackFromStealth();
                }
            }
        }
    }

    // 1) 고정 도트 피해 모드: RawDamage * HitCount 를 그대로 HP에서 차감
    if (LocalSpec.bFixedDot)
    {
        const int32 Count = FMath::Max(LocalSpec.HitCount, 1);
        FinalDamage = LocalSpec.RawDamage * Count;

        if (FinalDamage <= 0.f)
        {
            return 0.f;
        }

        const float NewHealth = FMath::Clamp(Health - FinalDamage, 0.f, MaxHealth);
        const float Delta = NewHealth - Health; // 음수
        ApplyHealth(NewHealth, Delta);          // 내부에서 OnHealthChanged, OnDeath 호출

        return FinalDamage;
    }

    // 2) 일반 데미지
    if (LocalSpec.RawDamage <= 0.f || MaxHealth <= 0.f) return 0.f;

    const float Raw = LocalSpec.RawDamage;
    FinalDamage = Raw;

    // TODO: 방어력/저항 등은 나중에 여기서 반영 (bIgnoreDefense 활용)

    const float NewHealth = FMath::Clamp(Health - FinalDamage, 0.f, MaxHealth);
    const float Delta = NewHealth - Health;

    ApplyHealth(NewHealth, Delta);
    HandleDeathIfNeeded();

    AActor* InstigatorActor = LocalSpec.Instigator.Get();
    USkillBase* SourceSkill = LocalSpec.SourceSkill.Get();

    OnDamageApplied.Broadcast(Health, FinalDamage, Raw, InstigatorActor, SourceSkill);

    return FinalDamage;
}

bool UHealthComponent::IsDead() const { return Health <= 0.f; }