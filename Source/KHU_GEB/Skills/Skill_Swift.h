// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Skills/SkillBase.h"
#include "Skill_Swift.generated.h"

class UNiagaraSystem;
class UNiagaraComponent;

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