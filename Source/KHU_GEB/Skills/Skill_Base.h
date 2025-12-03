// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Skills/SkillBase.h"
#include "Skill_Base.generated.h"

class UNiagaraSystem;
class UNiagaraComponent;

UCLASS(ClassGroup = (Skills), meta = (BlueprintSpawnableComponent))
class KHU_GEB_API USkill_Base : public USkillBase
{
    GENERATED_BODY()

public:
    USkill_Base();

    /** Base 스킬 사용 시: 몽타주 재생 + 시전자 중심 구형 범위 데미지 */
    virtual void ActivateSkill() override;
};