// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SkillDefinition.h"
#include "SkillBase.generated.h"

class UManaComponent;

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

    // 캐릭터의 마나 컴포넌트 캐시
    mutable TWeakObjectPtr<UManaComponent> CachedManaComp;

    UManaComponent* GetManaComponent() const;

public:
    // SkillDefinition에서 주입
    virtual void InitializeFromDefinition(const USkillDefinition* Def);

    // 1) 여기서 쿨타임 + 마나 둘 다 체크
    virtual bool CanActivate() const;

    // 2) 여기서 실제로 쿨타임 시작 + 마나 차감
    virtual void ActivateSkill();

    virtual void StopSkill();

};