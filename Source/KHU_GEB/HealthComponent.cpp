
#include "HealthComponent.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "Skills/SkillBase.h" 
#include "Skills/Skill_Ultimate.h"
#include "KHU_GEBCharacter.h"
#include "Enemy_AI/Enemy_Base.h"
#include "FormManagerComponent.h"
#include "StatManagerComponent.h"

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
    AActor* DamageCauser)
{
    if (RawDamage <= 0.f || MaxHealth <= 0.f) return 0.f;

    // ---------------------------
    // 1) 피격자의 방어력(%) 가져오기
    // ---------------------------
    float DefensePercent = 0.f;

    AActor* OwnerActor = GetOwner();
    if (OwnerActor)
    {
        // 플레이어인 경우: 현재 폼의 StatManager 방어력 사용
        if (const AKHU_GEBCharacter* Player = Cast<AKHU_GEBCharacter>(OwnerActor))
        {
            if (Player->StatManager && Player->FormManager)
            {
                const FFormRuntimeStats* Stats =
                    Player->StatManager->GetStats(Player->FormManager->CurrentForm);

                if (Stats)
                {
                    DefensePercent = Stats->Defense; // 단위: %
                }
            }
        }
        // 적인 경우: EnemyStats 방어력 사용
        else if (const AEnemy_Base* Enemy = Cast<AEnemy_Base>(OwnerActor))
        {
            DefensePercent = Enemy->GetDefenseStat(); // 단위: %
        }
    }

    // 0 ~ 100% 로 클램프
    const float ClampedDefense = FMath::Clamp(DefensePercent, 0.f, 100.f);

    // ---------------------------
    // 2) 최종 대미지 계산
    //    예) 방어력 60 → 40%만 받음
    // ---------------------------
    const float DamageReductionFactor = ClampedDefense / 100.f;        // 0.0 ~ 1.0
    float       FinalDamage = RawDamage * (1.f - DamageReductionFactor);

    // 방어력 100% 이상이면 데미지 0
    FinalDamage = FMath::Max(0.f, FinalDamage);

    if (FinalDamage <= 0.f)
    {
        UE_LOG(LogTemp, Log,
            TEXT("[HealthComponent] %s blocked all damage. Raw=%.1f Def=%.1f%%"),
            *GetNameSafe(OwnerActor), RawDamage, ClampedDefense);

        // 그래도 이벤트는 날려줄 수 있음 (원하면 유지/삭제 선택)
        OnDamageApplied.Broadcast(Health, RawDamage, 0.f, InstigatorActor, DamageCauser);
        return 0.f;
    }

    // ---------------------------
    // 3) 체력 감소 적용
    // ---------------------------
    const float NewHealth = FMath::Clamp(Health - FinalDamage, 0.f, MaxHealth);
    const float Delta = NewHealth - Health; // 음수(감소량)

    UE_LOG(LogTemp, Log,
        TEXT("[HealthComponent] %s took damage: Raw=%.1f Def=%.1f%% Final=%.1f NewHealth=%.1f"),
        *GetNameSafe(OwnerActor), RawDamage, ClampedDefense, FinalDamage, NewHealth);

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