// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SkillBase.h"
#include "Skills.generated.h"

class UNiagaraSystem;
class UNiagaraComponent;

/*=============================Base=============================*/
UCLASS(ClassGroup = (Skills), meta = (BlueprintSpawnableComponent))
class KHU_GEB_API USkill_Base : public USkillBase
{
    GENERATED_BODY()

public:
    virtual void ActivateSkill() override;
    
private:

};

/*=============================Range=============================*/
UCLASS(ClassGroup = (Skills), meta = (BlueprintSpawnableComponent))
class KHU_GEB_API USkill_Range : public USkillBase
{
    GENERATED_BODY()

public:
    // 튜닝 파라미터(Definition 기본값 + 개별 스킬 확장)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Range")
    FName MouthSocket = "MouthSocket";
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Range")
    float TickInterval = 0.1f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Range")
    float MaxDuration = 3.0f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Range")
    float ConeAngleDeg = 30.f;

    // 비주얼
    UPROPERTY(EditAnywhere, Category = "Range|FX")
    TObjectPtr<UNiagaraSystem> SkillNS;

    // 내부
    UPROPERTY() TObjectPtr<UNiagaraComponent> SpawnedNS = nullptr;
    FTimerHandle TickHandle, DurationHandle;

    virtual void InitializeFromDefinition(const USkillDefinition* Def) override
    {
        // 기존 Params 사용 (Damage/Range 등)  :contentReference[oaicite:5]{index=5}
        Params = Def ? Def->Params : FSkillParams{};
    }

    virtual bool CanActivate() const override { return true; }
    virtual void ActivateSkill() override;
    virtual void StopSkill() override;

private:
    FSkillParams Params; // Damage/Range 사용
    void TickBreath();
};

/*=============================Swift=============================*/
UCLASS(ClassGroup = (Skills), meta = (BlueprintSpawnableComponent))
class KHU_GEB_API USkill_Swift : public USkillBase
{
    GENERATED_BODY()

public:

};

/*=============================Guard=============================*/
UCLASS(ClassGroup = (Skills), meta = (BlueprintSpawnableComponent))
class KHU_GEB_API USkill_Guard : public USkillBase
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, Category = "Guard")
    float Duration = 3.0f;

    UPROPERTY(EditAnywhere, Category = "Guard|FX")
    TObjectPtr<UNiagaraSystem> SkillNS;

    UPROPERTY() TObjectPtr<UNiagaraComponent> SpawnedNS = nullptr;

    virtual void InitializeFromDefinition(const USkillDefinition* Def) override
    {
        Params = Def ? Def->Params : FSkillParams{}; // 쿨다운 등 나중에 활용  :contentReference[oaicite:9]{index=9}
    }

    virtual bool CanActivate() const override { return true; }
    virtual void ActivateSkill() override;
    virtual void StopSkill() override;

private:
    FTimerHandle DurationHandle;
    FSkillParams Params;
};

/*=============================Special=============================*/
UCLASS(ClassGroup = (Skills), meta = (BlueprintSpawnableComponent))
class KHU_GEB_API USkill_Special : public USkillBase
{
    GENERATED_BODY()

public:

};