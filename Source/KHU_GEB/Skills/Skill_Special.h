// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SkillBase.h"
#include "Skill_Special.generated.h"

class UNiagaraSystem;
class UNiagaraComponent;

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