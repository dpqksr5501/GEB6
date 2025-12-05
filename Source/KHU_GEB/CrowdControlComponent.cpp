// Fill out your copyright notice in the Description page of Project Settings.


#include "CrowdControlComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

UCrowdControlComponent::UCrowdControlComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UCrowdControlComponent::BeginPlay()
{
    Super::BeginPlay();
    CachedCharacter = Cast<ACharacter>(GetOwner());
}

void UCrowdControlComponent::InternalApplyCC(bool bBlockMove, bool bBlockAct, float Duration)
{
    if (Duration <= 0.f) return;

    UWorld* World = GetWorld();
    if (!World) return;

    bBlockMoveInput = bBlockMoveInput || bBlockMove;
    bBlockActions = bBlockActions || bBlockAct;

    // 한 번이라도 CC 걸릴 때 즉시 멈춰주기
    if (bBlockMoveInput)
    {
        if (ACharacter* Char = CachedCharacter.Get())
        {
            if (UCharacterMovementComponent* MoveComp = Char->GetCharacterMovement())
            {
                MoveComp->StopMovementImmediately();
            }
        }
    }

    const float Now = World->GetTimeSeconds();
    const float NewEndTime = Now + Duration;

    // 더 긴 CC가 들어오면 끝나는 시간 연장
    if (NewEndTime > CCEndTime)
    {
        CCEndTime = NewEndTime;

        World->GetTimerManager().ClearTimer(CCTimerHandle);
        World->GetTimerManager().SetTimer(
            CCTimerHandle,
            this,
            &UCrowdControlComponent::OnCCTimerFinished,
            Duration,
            false
        );
    }
}

void UCrowdControlComponent::ApplyStun(float Duration)
{
    InternalApplyCC(true, true, Duration);
}

void UCrowdControlComponent::ApplyRoot(float Duration)
{
    InternalApplyCC(true, false, Duration);
}

void UCrowdControlComponent::OnCCTimerFinished()
{
    UWorld* World = GetWorld();
    if (!World) return;

    const float Now = World->GetTimeSeconds();
    if (Now < CCEndTime)
    {
        // 더 긴 CC가 다시 걸린 상태 → 다시 세팅
        const float Remaining = CCEndTime - Now;
        World->GetTimerManager().SetTimer(
            CCTimerHandle,
            this,
            &UCrowdControlComponent::OnCCTimerFinished,
            Remaining,
            false
        );
        return;
    }

    ClearCC();
}

void UCrowdControlComponent::ClearCC()
{
    bBlockMoveInput = false;
    bBlockActions = false;

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(CCTimerHandle);
    }
}
