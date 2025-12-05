// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FormDefinition.h"
#include "Skills/SkillDefinition.h"
#include "SkillBase.generated.h"

class UManaComponent;
class USkillManagerComponent;
class AKHU_GEBCharacter;

UCLASS( Abstract, Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class KHU_GEB_API USkillBase : public UActorComponent
{
	GENERATED_BODY()

public:	
	USkillBase();

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	ESkillSlot Slot = ESkillSlot::Active;

protected:

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Skill")
    FSkillParams Params;

    // 다음에 스킬을 다시 쓸 수 있는 게임 시간
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Skill")
    float NextAvailableTime = 0.f;

    mutable TWeakObjectPtr<USkillManagerComponent> CachedManager;
    USkillManagerComponent* GetSkillManager() const;

    // 캐릭터의 마나 컴포넌트 캐시
    mutable TWeakObjectPtr<UManaComponent> CachedManaComp;
    UManaComponent* GetManaComponent() const;

    // 현재 폼의 스킬 몽타주를 재생하는 헬퍼 함수
    void PlayFormSkillMontage();

    // 현재 폼의 궁극기 몽타주를 재생하는 헬퍼 함수
    void PlayFormUltimateMontage();

    EFormType GetCurrentFormType() const;

    /** 현재 시전자(플레이어/적)의 "해당 폼 레벨"을 구한다. 최소 1 */
    float GetCurrentOwnerLevel() const;

    /** 현재 레벨 기준 스킬 대미지 */
    float GetDamageForCurrentLevel() const;

    /** 현재 레벨 기준 스킬 마나 소모량 */
    float GetManaCostForCurrentLevel() const;

public:
    // SkillDefinition에서 주입
    virtual void InitializeFromDefinition(const USkillDefinition* Def);

    // 1) 여기서 쿨타임 + 마나 둘 다 체크
    virtual bool CanActivate() const;

    // 2) 여기서 실제로 쿨타임 시작 + 마나 차감
    virtual void ActivateSkill();

    virtual void StopSkill();

};