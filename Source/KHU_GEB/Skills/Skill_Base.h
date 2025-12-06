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

protected:
    /** 검 트레일용 나이아가라 시스템 (리본/트레일 타입 추천) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX")
    UNiagaraSystem* SwordTrailSystem = nullptr;

    /** 검 트레일을 붙일 소켓 이름 (캐릭터/무기 메시에 존재해야 함) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX")
    FName SwordTrailSocketName = TEXT("WeaponTrail");

public:
    USkill_Base();

    /** Base 스킬 사용 시: 몽타주 재생 + 시전자 중심 구형 범위 데미지 */
    virtual void ActivateSkill() override;
};