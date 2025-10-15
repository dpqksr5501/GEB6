
#include "HealthComponent.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"


UHealthComponent::UHealthComponent()
{
    PrimaryComponentTick.bCanEverTick = false; // ƽ �� ���� false ����
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
    const float Delta = NewHealth - Health; // ����(���ҷ�)
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
    const float Delta = NewHealth - Health; // ���(ȸ����)
    ApplyHealth(NewHealth, Delta);
}

void UHealthComponent::SetMaxHealth(float NewMaxHealth, bool bClampCurrentToNewMax)
{
    NewMaxHealth = FMath::Max(0.f, NewMaxHealth);

    // ���� ü�� ���� �����ϰ� �ʹٸ� �Ʒ� �� ���� ���:
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
        // �ʿ� ��: ���� ���� ó�� (Destroy ��)
        // if (AActor* Owner = GetOwner()) { Owner->Destroy(); }
    }
}