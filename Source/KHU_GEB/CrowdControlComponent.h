// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CrowdControlComponent.generated.h"

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class KHU_GEB_API UCrowdControlComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UCrowdControlComponent();

    // 완전 스턴: 이동 + 스킬/공격 모두 막기
    UFUNCTION(BlueprintCallable, Category = "CC")
    void ApplyStun(float Duration);

    // 루트: 이동만 막고, 스킬/공격은 허용하고 싶다면 사용
    UFUNCTION(BlueprintCallable, Category = "CC")
    void ApplyRoot(float Duration);

    UFUNCTION(BlueprintCallable, Category = "CC")
    void ClearCC();

    // 입력 차단 여부
    bool IsMoveBlocked()   const { return bBlockMoveInput; }
    bool IsActionBlocked() const { return bBlockActions; }

    // === CC 면역 토글 ===
    void SetCCImmune(bool bImmune);
    bool IsCCImmune() const { return bCCImmune; }

private:
    UPROPERTY(Transient)
    TWeakObjectPtr<ACharacter> CachedCharacter;

    bool bBlockMoveInput = false;
    bool bBlockActions = false;

    // CC 면역 여부
    bool bCCImmune = false;

    // 현재 CC가 끝나는 시각(여러 번 Apply될 때 갱신용)
    float CCEndTime = 0.f;

    FTimerHandle CCTimerHandle;

    void InternalApplyCC(bool bBlockMove, bool bBlockAct, float Duration);
    void OnCCTimerFinished();

protected:
    virtual void BeginPlay() override;
};
