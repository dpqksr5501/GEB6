// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Skills/SkillBase.h"
#include "Skill_Guard.generated.h"

class UNiagaraSystem;
class UNiagaraComponent;

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

    /** 우클릭을 떼었을 때, 소모된 보호막 수 × DamagePerSheild 를 줄 광역 공격 반경 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Guard")
    float ExplosionRadius = 0.f;

    /** 쉴드가 모두 소진되었을 때 시전자에게 적용할 스턴 시간 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Guard")
    float DepletionStunDuration = 5.f;

    /** 가동 중 나이아가라 보호막 이펙트 */
    UPROPERTY(EditAnywhere, Category = "Guard|FX")
    TObjectPtr<UNiagaraSystem> SkillNS;

    UPROPERTY(EditAnywhere, Category = "Guard|FX")
    float SkillReferenceRadius = 100.f;

    /** 폭발 범위를 표시할 나이아가라 (원형/구형) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Guard|FX")
    TObjectPtr<UNiagaraSystem> ExplosionNS;

    UPROPERTY(EditAnywhere, Category = "Guard|FX")
    float ExplosionReferenceRadius = 100.f;

    UPROPERTY(EditAnywhere, Category = "Guard|FX")
    FVector GuardBlockOffset = FVector(0.f, 0.f, -80.f);

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
