// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SkillBase.h"
#include "Skill_Ultimate.generated.h"

class UNiagaraSystem;
class UNiagaraComponent;
class ACharacter;
class AActor;

UCLASS(ClassGroup = (Skills), meta = (BlueprintSpawnableComponent))
class KHU_GEB_API USkill_Ultimate : public USkillBase
{
    GENERATED_BODY()

public:
    USkill_Ultimate();

    // ---------------- Range 설정 값 ----------------

    /** 브레스가 유지되는 시간(초) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ultimate|Range")
    float BreathDuration = 3.f;

    /** 막대의 "두께" (반지름 느낌) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ultimate|Range")
    float BreathRadius = 150.f;

    /** 막대의 "길이" (사정거리) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ultimate|Range")
    float BreathLength = 1000.f;

    /** 데미지가 들어가는 간격(초) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ultimate|Range")
    float TickInterval = 0.5f;

    /** 틱당 피해량 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ultimate|Range")
    float DamagePerTick = 10.f;

    /** 어떤 오브젝트 채널을 대상으로 Overlap 할지 (기본 Pawn) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ultimate|Range")
    TEnumAsByte<ECollisionChannel> BreathCollisionChannel = ECC_Pawn;

    /** 브레스(긴 막대) 비주얼 FX */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ultimate|Range|FX")
    TObjectPtr<UNiagaraSystem> BreathNS;

    /** 입 소켓 이름 (없으면 Actor 위치 기준) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ultimate|Range|FX")
    FName MuzzleSocketName = TEXT("MouthSocket");

    /** 디버그: 브레스 박스를 그릴지 여부 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ultimate|Range|Debug")
    bool bDrawDebugBreath = false;

    /** 디버그 박스 색상 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ultimate|Range|Debug")
    FColor DebugBreathColor = FColor::Red;

    // ---------------- Swift 설정 값 ----------------
   /** 은신 최대 지속시간 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ultimate|Swift")
    float SwiftDuration = 5.f;

    /** 은신 중 이동속도 배수 (1.5 = 50% 증가) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ultimate|Swift")
    float SwiftMoveSpeedMultiplier = 1.5f;

    /** 은신 중 첫 공격의 공격력 배수 (2.0 = 2배) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ultimate|Swift")
    float SwiftAttackMultiplier = 2.0f;

    // ---------------- Guard 설정 값 ----------------

    /** 지진 사정거리 (부채꼴의 반지름) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ultimate|Guard")
    float GuardRange = 800.f;

    /** 지진 데미지 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ultimate|Guard")
    float GuardDamage = 50.f;

    /** 부채꼴의 반각 (도) – 45면 총 90도 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ultimate|Guard")
    float GuardConeHalfAngleDeg = 45.f;

    /** 적이 땅으로 추락할 때 줄 하향 속도/힘 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ultimate|Guard")
    float GuardFallStrength = 2000.f;

    /** 기절(속박) 지속시간 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ultimate|Guard")
    float GuardStunDuration = 2.0f;

    /** Overlap에 사용할 오브젝트 채널 (기본 Pawn) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ultimate|Guard")
    TEnumAsByte<ECollisionChannel> GuardCollisionChannel = ECC_Pawn;

    /** 디버그: 부채꼴 범위를 선으로 표시할지 여부 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ultimate|Guard|Debug")
    bool bDrawDebugGuard = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ultimate|Guard|Debug")
    FColor GuardDebugColor = FColor::Yellow;

    // ---------------- Special 설정 값 ----------------

    /** 구체가 유지되는 시간(초) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ultimate|Special")
    float SpecialDuration = 5.f;

    /** 구체들이 배치될 반경(범위) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ultimate|Special")
    float SpecialRadius = 600.f;

    /** 플레이어에게 들어가는 도트 데미지 (틱당) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ultimate|Special")
    float SpecialSelfDotDamage = 30.f;

    /** 도트 데미지 간격 (초) – 총 3번 들어감 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ultimate|Special")
    float SpecialSelfDotInterval = 0.5f;

    /** 플레이어 속박 시간 (초) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ultimate|Special")
    float SpecialRootDuration = 2.0f;

    /** 소환할 오브(구체) 클래스 (BP에서 Health = 1 로 설정해두면 좋음) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ultimate|Special")
    TSubclassOf<AActor> SpecialOrbClass;

    /** 디버그: 오브 위치를 꼭짓점으로 하는 오망성을 바닥에 그릴지 여부 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ultimate|Special|Debug")
    bool bDrawSpecialPentagram = true;

    /** 오망성 라인 색상 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ultimate|Special|Debug")
    FColor SpecialPentagramColor = FColor::Purple;

protected:
    virtual void BeginPlay() override;

public:
    // USkillBase 인터페이스
    virtual void InitializeFromDefinition(const USkillDefinition* Def) override;
    virtual bool CanActivate() const override;
    virtual void ActivateSkill() override;
    virtual void StopSkill() override;

    /** 외부(HealthComponent)에서 사용할 쿼리/헬퍼 */
    bool IsSwiftStealthActive() const { return bSwiftStealthActive; }
    float GetSwiftAttackMultiplier() const { return SwiftAttackMultiplier; }

    /** “공격해서 데미지를 준 순간” 호출 → 은신 해제 */
    void OnAttackFromStealth();

private:
    // Range용 헬퍼들

    /** Range 궁극기 (브레스) */
    void ActivateRangeUltimate();

    /** TickInterval 마다 호출되어 Overlap 중인 적에게 데미지 부여 */
    void OnBreathTick();

    /** BreathDuration 이 끝났을 때 호출 */
    void OnBreathDurationEnded();

    bool GetBreathBox(
        FVector& OutCenter,
        FQuat& OutRotation,
        FVector& OutHalfExtent) const;

    // Swift용 헬퍼들

    /** Swift 궁극기 (은신) */
    void ActivateSwiftUltimate();

    /** Swift 은신이 지속시간으로 끝났을 때 */
    void OnSwiftDurationEnded();

    /** Swift 은신을 실제로 종료(이속/투명 복구) */
    void EndSwiftUltimate();

    /** 소유자가 데미지를 받았을 때 콜백 → 피격 시 은신 해제 */
    UFUNCTION()
    void HandleOwnerDamaged(float NewHealth, float RawDamage, float FinalDamage,
        AActor* InstigatorActor, AActor* DamageCauser);

    // Guard용 헬퍼들

    /** Guard 궁극기 (부채꼴 지진) */
    void ActivateGuardUltimate();

    /** 개별 타겟 기절 해제 */
    void EndGuardStun(ACharacter* Target);

    // Special용 헬퍼들

    /** Special 궁극기 (플레이어 속박) */
    void ActivateSpecialUltimate();

    /** Special 지속시간 끝났을 때 호출 */
    void OnSpecialDurationEnded();

    /** 플레이어에게 도트 데미지 1틱 적용 */
    void OnSpecialSelfDotTick();

    /** 플레이어에게 속박 부여 */
    void ApplyRootToPlayer(float Duration);

    /** 속박 해제 */
    void EndSpecialRoot();

    /** Orb 월드 위치들을 이용해, 바닥 위에 오망성(별) 라인을 그린다. */
    void DrawPentagramOnGround(const TArray<FVector>& OrbWorldLocations);

    /** 구체가 파괴될 때 호출 (남은 구체 수 카운트용) */
    UFUNCTION()
    void HandleSpecialOrbDestroyed(AActor* DestroyedActor);

private:
    // ---------------- Range 상태 ----------------
    /** 현재 Range 궁극기가 켜져 있는지 */
    bool bIsActive = false;

    /** 실제로 붙어서 나가는 브레스 FX */
    UPROPERTY(Transient)
    TObjectPtr<UNiagaraComponent> SpawnedBreathNS = nullptr;

    /** 지속시간 타이머 */
    FTimerHandle DurationTimerHandle;

    /** 데미지 틱 타이머 */
    FTimerHandle TickTimerHandle;

    // ---------------- Swift 상태 ----------------
    /** 현재 Swift 은신이 켜져 있는지 */
    bool bSwiftStealthActive = false;

    /** 은신 시 버프를 걸었던 소유 캐릭터 캐시 */
    UPROPERTY(Transient)
    TWeakObjectPtr<ACharacter> SwiftOwnerChar;

    /** 은신 시작 전 이동속도 저장 */
    float SwiftOriginalMaxWalkSpeed = 0.f;

    /** Swift 은신 지속시간 타이머 */
    FTimerHandle SwiftDurationTimerHandle;

    // ---------------- Special 상태 ----------------

    /** 소환된 구체들 */
    UPROPERTY(Transient)
    TArray<TWeakObjectPtr<AActor>> SpecialOrbs;

    /** Special 구체 지속시간 타이머 */
    FTimerHandle SpecialDurationTimerHandle;

    /** 플레이어에게 들어갈 도트 데미지 타이머 */
    FTimerHandle SpecialSelfDotTimerHandle;

    /** 남은 도트 횟수 (항상 3에서 시작) */
    int32 SpecialSelfDotTicksRemaining = 0;

    /** 속박된 플레이어 캐릭터 */
    UPROPERTY(Transient)
    TWeakObjectPtr<ACharacter> SpecialRootedPlayer;

    /** 속박 해제 타이머 */
    FTimerHandle SpecialRootTimerHandle;

};