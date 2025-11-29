// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SkillBase.h"
#include "Skills.generated.h"

class UNiagaraSystem;
class UNiagaraComponent;
class AFireballProjectile;

/*=============================Base=============================*/
UCLASS(ClassGroup = (Skills), meta = (BlueprintSpawnableComponent))
class KHU_GEB_API USkill_Base : public USkillBase
{
    GENERATED_BODY()

public:
    /*virtual void ActivateSkill() override;*/
    
private:

};

/*=============================Range=============================*/
UCLASS(ClassGroup = (Skills), meta = (BlueprintSpawnableComponent))
class KHU_GEB_API USkill_Range : public USkillBase
{
    GENERATED_BODY()

public:
    USkill_Range();   // ← 생성자 추가

    /** 발사할 화염구 액터 (보통 Projectile 블루프린트) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Range")
    TSubclassOf<AFireballProjectile> FireballClass;

    /** 화염구를 발사할 소켓 이름 (캐릭터 Mesh에 이 이름의 소켓 필요) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Range")
    FName MuzzleSocketName = TEXT("MouthSocket");

    /** (예전) 초기 발사 각도 오프셋 – 이제는 “기본 발사” fallback에만 사용 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Range")
    float LaunchPitchOffsetDegrees = -10.f;

    /** 스킬 시전 시, 입 소켓에서 나갈 발사 이펙트 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Range|FX")
    TObjectPtr<UNiagaraSystem> CastNS;

    /** 화염구 비행 중에 붙을 나이아가라(꼬리/코어 등) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Range|FX")
    TObjectPtr<UNiagaraSystem> ProjectileNS;

    /** 조준 중에 바닥에 보여줄 범위 표시 나이아가라(원형 영역 같은 이펙트) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Range|Target")
    TObjectPtr<UNiagaraSystem> TargetAreaNS;

    /** 조준 원(빨간 서클)의 실제 반경 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Range|Target")
    float TargetRadius = 0.f;

    /** 캐릭터를 중심으로 조준 원의 "중심"이 이동할 수 있는 최대 거리 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Range|Target")
    float MaxAimDistance = 1200.f;

    /** 조준 중 이동키로 원을 움직일 때 사용할 속도 (유닛/초) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Range|Target")
    float AimMoveSpeed = 600.f;

    /** 조준 원의 중앙을 땅에 붙일 때 쓸 라인트레이스 높이 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Range|Target")
    float GroundTraceHalfHeight = 5000.f;

    /** 지면에서 살짝 띄우기 위한 오프셋 (이펙트가 묻히지 않게) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Range|Target")
    float GroundOffsetZ = 5.f;

    virtual void InitializeFromDefinition(const USkillDefinition* Def) override;
    virtual void ActivateSkill() override;
    virtual void StopSkill() override;
    virtual void TickComponent(
        float DeltaTime,
        ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

    /** 조준 중 이동 입력을 받아 저장 (캐릭터 Move에서 호출) */
    void HandleAimMoveInput(const FVector2D& Input);

private:
    bool bIsAiming = false;
    bool bHasValidTarget = false;
    FVector CurrentTargetLocation = FVector::ZeroVector;

    UPROPERTY() TObjectPtr<UNiagaraComponent> TargetAreaComp = nullptr;

    FVector2D AimMoveInput = FVector2D::ZeroVector;

    float GetCurrentTargetRadius() const;
    float GetMaxAimDistance() const;

    void SpawnOrUpdateIndicator();
    void CleanupIndicator();
    void SpawnProjectileTowards(const FVector& TargetLocation);
    void SpawnDefaultProjectile();
};

/*=============================Swift=============================*/
UCLASS(ClassGroup = (Skills), meta = (BlueprintSpawnableComponent))
class KHU_GEB_API USkill_Swift : public USkillBase
{
    GENERATED_BODY()

public:
    /** 한 번 점멸할 때 이동할 거리 (SkillDefinition의 Range로 덮어씌워질 수 있음) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Swift")
    float DashDistance = 0.f;

    /** 시작~끝을 잇는 직육면체의 “가로(옆)” 반폭 (Y축) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Swift")
    float BoxHalfWidth = 150.f;

    /** 시작~끝을 잇는 직육면체의 “세로(위)” 반높이 (Z축) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Swift")
    float BoxHalfHeight = 100.f;

    /** 박스 데미지를 몇 번 샘플링할지 (클수록 같은 적에게 여러 번 데미지) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Swift")
    int32 DamageSamples = 10;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Swift")
    float DamagePerSample = 0.f;

    /** 범위 내의 적 위치에 생성될 나이아가라 (타격 이펙트) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Swift|FX")
    TObjectPtr<UNiagaraSystem> HitNS;

    virtual void InitializeFromDefinition(const USkillDefinition* Def) override;
    virtual bool CanActivate() const override;
    virtual void ActivateSkill() override;
    virtual void StopSkill() override;

private:
    /** Swift에 맞은 타겟들 (10타 동안 계속 두들길 대상) */
    UPROPERTY()
    TArray<TWeakObjectPtr<ACharacter>> SwiftTargets;

    /** 현재 몇 번째 타격인지 (1 ~ DamageSamples) */
    int32 CurrentHitIndex = 0;

    /** 타격 간격(초). 10타라면 0.08 ~ 0.1 정도가 체감 좋음 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Swift", meta = (AllowPrivateAccess = "true"))
    float HitInterval = 0.08f;

    /** 타격용 타이머 핸들 */
    FTimerHandle SwiftDamageTimerHandle;

    /** 타격 한 번 수행 (타이머 콜백) */
    void HandleSwiftDamageTick();

};

/*=============================Guard=============================*/
UCLASS(ClassGroup = (Skills), meta = (BlueprintSpawnableComponent))
class KHU_GEB_API USkill_Guard : public USkillBase
{
    GENERATED_BODY()

public:
    /** 한 번 스킬을 사용할 때 가지고 있는 총 보호막 개수 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Guard")
    int32 MaxShields = 10;

    /** 배리어 한 장이 깎일 때마다 소모할 마나량 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Guard")
    float ManaPerShield = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Guard")
    float DamagePerSheild = 0.f;

    /** 우클릭을 떼었을 때, 소모된 보호막 수 × DamagePerSheild 를 줄 광역 공격 반경 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Guard")
    float ExplosionRadius = 0.f;

    /** 가동 중 나이아가라 보호막 이펙트 */
    UPROPERTY(EditAnywhere, Category = "Guard|FX")
    TObjectPtr<UNiagaraSystem> SkillNS;

    UPROPERTY()
    TObjectPtr<UNiagaraComponent> SpawnedNS = nullptr;

    virtual void InitializeFromDefinition(const USkillDefinition* Def) override;

    /** 이미 켜져 있을 땐 다시 못 켜도록 + 쿨타임 체크 */
    virtual bool CanActivate() const override;
    virtual void ActivateSkill() override;
    virtual void StopSkill() override;

    /** 캐릭터의 HandleAnyDamage에서 호출해서, 데미지를 보호막으로 막을지 여부를 판단 */
    bool HandleIncomingDamage(float Damage,
        const UDamageType* DamageType,
        AController* InstigatedBy,
        AActor* DamageCauser);

    bool IsActive() const { return bIsActive; }

    // Enemy가 쓰기 위해 Public으로 옮겼습니다.
    int32 RemainingShields = 0;
    int32 ConsumedShields = 0;

private:
    bool  bIsActive = false;
    bool  bEndedByDepletion = false;
};

/*=============================Special=============================*/
UCLASS(ClassGroup = (Skills), meta = (BlueprintSpawnableComponent))
class KHU_GEB_API USkill_Special : public USkillBase
{
    GENERATED_BODY()

public:
    /** 플레이어를 중심으로 따라다닐 흑안개 나이아가라 시스템 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Special|FX")
    TObjectPtr<UNiagaraSystem> DarkFogNS;

    /** 흑안개를 붙일 소켓 이름 (없으면 루트/메시에 부착) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Special|FX")
    FName AttachSocketName = NAME_None;

    /** 플레이어 기준 상대 위치 (기본: 발 밑) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Special|FX")
    FVector RelativeOffset = FVector(0.f, 0.f, -30.f);

    /** 스킬이 유지되는 시간(초) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Special|Buff")
    float Duration = 5.0f;

    /** 스킬이 켜져 있는 동안 플레이어 이동속도 배율 (1.5 = 50% 증가) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Special|Buff")
    float SelfMoveSpeedMultiplier = 1.5f;

    /** 흑안개 안의 적 이동속도 배율 (0.5 = 50% 감소) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Special|Buff")
    float EnemyMoveSpeedMultiplier = 0.5f;

    /** 흑안개 효과가 적용되는 반경 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Special|Buff")
    float FogRadius = 0.f;

    /** 적 슬로우를 갱신할 주기(초) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Special|Buff")
    float SlowTickInterval = 0.2f;

    /** 2초마다 플레이어가 회복할 양 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Special|Effect")
    float SelfHealPerTick = 0.f;

    /** 2초마다 흑안개 안의 적에게 들어갈 고정 피해 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Special|Effect")
    float DotDamagePerTick = 0.f;

    /** 힐/도트 틱 간격(초). 기본 2초 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Special|Effect")
    float EffectTickInterval = 2.f;

    virtual void InitializeFromDefinition(const USkillDefinition* Def);
    virtual bool CanActivate() const override;
    virtual void ActivateSkill() override;
    virtual void StopSkill() override; // 입력 해제용 (지속 스킬이라 무시할 예정)

private:
    /** 현재 Special 이 켜져 있는지 여부 */
    bool bIsActive = false;

    /** 실제로 붙어서 따라다니는 나이아가라 컴포넌트 */
    UPROPERTY()
    TObjectPtr<UNiagaraComponent> SpawnedNS = nullptr;

    /** 지속시간 관리용 타이머 */
    FTimerHandle DurationTimerHandle;

    /** 슬로우 효과 갱신용 타이머 */
    FTimerHandle SlowTickTimerHandle;

    /** 힐/도트 효과용 타이머 */
    FTimerHandle EffectTickTimerHandle;

    /** Special을 쓴 플레이어 캐릭터 캐시 */
    TWeakObjectPtr<class AKHU_GEBCharacter> CachedOwnerChar;

    /** 슬로우가 적용된 적들의 원래 속도 저장 */
    TMap<TWeakObjectPtr<class ACharacter>, float> OriginalEnemySpeeds;

    /** 지속시간이 끝났을 때 호출 */
    void OnDurationEnded();

    /** 현재 흑안개 영역 안의 적들을 찾아 슬로우/복구 처리 */
    void UpdateFogEffects();

    /** 힐/도트 틱 */
    void OnEffectTick();

    /** Special을 실제로 종료(버프/슬로우/이펙트 정리) */
    void EndSpecial();
};