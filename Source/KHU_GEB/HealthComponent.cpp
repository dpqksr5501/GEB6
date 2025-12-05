
#include "HealthComponent.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "Skills/SkillBase.h" 
#include "Skills/Skill_Ultimate.h" 

#include "Animation/AnimInstance.h"


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

float UHealthComponent::ApplyDamage(
    float   RawDamage,
    AActor* InstigatorActor,
    AActor* DamageCauser
)
{
    if (RawDamage <= 0.f || MaxHealth <= 0.f) return 0.f;

    // 나중에 방어력/저항 등을 여기에서 계산해도 됨
    const float FinalDamage = RawDamage;

    const float NewHealth = FMath::Clamp(Health - FinalDamage, 0.f, MaxHealth);
    const float Delta = NewHealth - Health;

    UE_LOG(LogTemp, Log,
        TEXT("[HealthComponent] %s took damage: Raw=%.1f Final=%.1f NewHealth=%.1f"),
        *GetNameSafe(GetOwner()), RawDamage, FinalDamage, NewHealth);

    ApplyHealth(NewHealth, Delta);
    HandleDeathIfNeeded();

    OnDamageApplied.Broadcast(Health, RawDamage, FinalDamage, InstigatorActor, DamageCauser);

    return FinalDamage;
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

bool UHealthComponent::IsDead() const { return Health <= 0.f; }

