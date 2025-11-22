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
    /** 발사할 화염구 액터 (보통 Projectile 블루프린트) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Range")
    TSubclassOf<AFireballProjectile> FireballClass;

    /** 화염구를 발사할 소켓 이름 (캐릭터 Mesh에 이 이름의 소켓 필요) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Range")
    FName MuzzleSocketName = TEXT("MouthSocket");

    /** 초기 발사 각도 오프셋 (Pitch, 도 단위). 음수면 위로, 양수면 아래로. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Range")
    float LaunchPitchOffsetDegrees = -10.f;   // 기본값: 살짝 위로

    virtual void ActivateSkill() override;
};

/*=============================Swift=============================*/
UCLASS(ClassGroup = (Skills), meta = (BlueprintSpawnableComponent))
class KHU_GEB_API USkill_Swift : public USkillBase
{
    GENERATED_BODY()

public:
    /** 한 번 점멸할 때 이동할 거리 (SkillDefinition의 Range로 덮어씌워질 수 있음) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Swift")
    float DashDistance = 1000.f;

    /** 시작~끝을 잇는 직육면체의 “가로(옆)” 반폭 (Y축) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Swift")
    float BoxHalfWidth = 150.f;

    /** 시작~끝을 잇는 직육면체의 “세로(위)” 반높이 (Z축) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Swift")
    float BoxHalfHeight = 100.f;

    /** 박스 데미지를 몇 번 샘플링할지 (클수록 같은 적에게 여러 번 데미지) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Swift")
    int32 DamageSamples = 3;

    virtual void InitializeFromDefinition(const USkillDefinition* Def) override;
    virtual bool CanActivate() const override;
    virtual void ActivateSkill() override;
    virtual void StopSkill() override;

};

/*=============================Guard=============================*/
UCLASS(ClassGroup = (Skills), meta = (BlueprintSpawnableComponent))
class KHU_GEB_API USkill_Guard : public USkillBase
{
    GENERATED_BODY()

public:
    /** 한 번 스킬을 사용할 때 가지고 있는 총 보호막 개수 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Guard")
    int32 MaxShields = 3;

    /** 우클릭을 떼었을 때, 소모된 보호막 수 × Params.Damage 를 줄 광역 공격 반경 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Guard")
    float ExplosionRadius = 400.f;

    /** 배리어 한 장이 깎일 때마다 소모할 마나량 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Guard")
    float ManaPerShield = 10.f;

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

private:
    int32 RemainingShields = 0;
    int32 ConsumedShields = 0;
    bool  bIsActive = false;
    bool  bEndedByDepletion = false;
};

/*=============================Special=============================*/
UCLASS(ClassGroup = (Skills), meta = (BlueprintSpawnableComponent))
class KHU_GEB_API USkill_Special : public USkillBase
{
    GENERATED_BODY()

public:

};