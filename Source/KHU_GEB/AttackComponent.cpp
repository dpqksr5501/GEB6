// Fill out your copyright notice in the Description page of Project Settings.


#include "AttackComponent.h"
#include "FormDefinition.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "GameFramework/Actor.h"
#include "Components/SkeletalMeshComponent.h"
#include "InputActionValue.h"
#include "Engine/World.h"
#include "Kismet/KismetSystemLibrary.h"

UAttackComponent::UAttackComponent() {}

void UAttackComponent::BeginPlay()
{
    Super::BeginPlay();
    BindAnimDelegates(); // 초기 애님에 바인딩(없으면 내부에서 무시됨)
}

USkeletalMeshComponent* UAttackComponent::GetMesh() const
{
    return GetOwner() ? GetOwner()->FindComponentByClass<USkeletalMeshComponent>() : nullptr;
}
UAnimInstance* UAttackComponent::GetAnim() const
{
    if (auto* M = GetMesh()) return M->GetAnimInstance();
    return nullptr;
}

void UAttackComponent::SetForm(const UFormDefinition* Def)
{
    // 폼 바뀔 때: 타이머/상태 정리 후 애님 재바인딩
    ClearComboWindows();
    CurrentFormDef = Def;
    ComboIndex = 0; bCanChain = false; bAttackHeld = false;
    bAdvancedThisWindow = false; bResetOnNext = false; NextPolicy = EComboPolicy::None;
    BindAnimDelegates();
}

void UAttackComponent::BindAnimDelegates()
{
    UnbindAnimDelegates();
    if (UAnimInstance* Anim = GetAnim())
    {
        Anim->OnPlayMontageNotifyBegin.AddDynamic(this, &UAttackComponent::OnNotifyBeginReceived);
        Anim->OnPlayMontageNotifyEnd.AddDynamic(this, &UAttackComponent::OnNotifyEndReceived);
        Anim->OnMontageEnded.AddDynamic(this, &UAttackComponent::OnMontageEnded);
        BoundAnim = Anim;
    }
}
void UAttackComponent::UnbindAnimDelegates()
{
    if (UAnimInstance* Prev = BoundAnim.Get())
    {
        Prev->OnPlayMontageNotifyBegin.RemoveDynamic(this, &UAttackComponent::OnNotifyBeginReceived);
        Prev->OnPlayMontageNotifyEnd.RemoveDynamic(this, &UAttackComponent::OnNotifyEndReceived);
        Prev->OnMontageEnded.RemoveDynamic(this, &UAttackComponent::OnMontageEnded);
    }
    BoundAnim = nullptr;
}

void UAttackComponent::AttackStarted(const FInputActionValue&)
{
    bAttackHeld = true;
    if (UAnimInstance* Anim = GetAnim())
    {
        const bool bPlaying = Anim->Montage_IsPlaying(nullptr);
        if (!bPlaying)
        {
            ComboIndex = 0;
            bCanChain = false; bAdvancedThisWindow = false; bResetOnNext = false; NextPolicy = EComboPolicy::None;
            PlayCurrentComboMontage();
            return;
        }
        if (bCanChain && !bAdvancedThisWindow) { AdvanceComboImmediately(); }
    }
}
void UAttackComponent::AttackTriggered(const FInputActionValue&)
{
    bAttackHeld = true;
    if (bCanChain && !bAdvancedThisWindow) { AdvanceComboImmediately(); }
}
void UAttackComponent::AttackCompleted(const FInputActionValue&)
{
    bAttackHeld = false;
}

void UAttackComponent::PlayCurrentComboMontage(float PlayRate)
{
    if (!CurrentFormDef) return;
    UAnimInstance* Anim = GetAnim(); if (!Anim) return;

    // FormDefinition의 스텝 구조에 맞춰 접근
    const auto& Steps = CurrentFormDef->AttackMontages; // FAttackStep 배열
    if (!Steps.IsValidIndex(ComboIndex) || !Steps[ComboIndex].Montage) { ComboIndex = 0; return; }

    UAnimMontage* M = Steps[ComboIndex].Montage;
    LastAttackMontage = M;
    Anim->Montage_Play(M, PlayRate);

    // 새 타 시작: 창구 초기화 & 프레임→타이머 예약
    bCanChain = false; bAdvancedThisWindow = false; NextPolicy = EComboPolicy::None;
    ScheduleComboWindows(M, PlayRate);
}

void UAttackComponent::ComputeNextIndex()
{
    const auto& Steps = CurrentFormDef->AttackMontages;
    const int32 Num = Steps.Num();
    if (Num <= 0) { ComboIndex = 0; return; }
    ComboIndex = bResetOnNext ? 0 : (ComboIndex + 1) % Num;
}

void UAttackComponent::AdvanceComboImmediately()
{
    if (!bCanChain || bAdvancedThisWindow) return;

    ComputeNextIndex();

    // 현재 몽타주용 타이머 먼저 정리(레이스 가드)
    ClearComboWindows();

    if (UAnimInstance* Anim = GetAnim())
    {
        if (LastAttackMontage) { Anim->Montage_Stop(0.05f, LastAttackMontage); }
    }

    bAdvancedThisWindow = true;
    bResetOnNext = false; NextPolicy = EComboPolicy::None;

    PlayCurrentComboMontage();
}

void UAttackComponent::OnMontageEnded(UAnimMontage* Montage, bool /*bInterrupted*/)
{
    // 이전 몽타주의 End가 늦게 와도, '그 몽타주' 소유 타이머만 지움
    if (Montage == WindowOwnerMontage) { ClearComboWindows(); }

    if (!bAttackHeld)
    {
        ComboIndex = 0;
        bCanChain = false; bAdvancedThisWindow = false; bResetOnNext = false; NextPolicy = EComboPolicy::None;
    }
}

void UAttackComponent::OnNotifyBeginReceived(FName Name, const FBranchingPointNotifyPayload&)
{
    if (Name == TEXT("SaveAttack")) { Notify_SaveAttack(); }
    else if (Name == TEXT("ResetCombo")) { Notify_ResetCombo(); }
}
void UAttackComponent::OnNotifyEndReceived(FName, const FBranchingPointNotifyPayload&) {}

void UAttackComponent::Notify_SaveAttack()
{
    bCanChain = true;
    bAdvancedThisWindow = false;
    if (bAttackHeld && !bAdvancedThisWindow) { AdvanceComboImmediately(); }
}
void UAttackComponent::Notify_ResetCombo()
{
    if (NextPolicy != EComboPolicy::Advance) NextPolicy = EComboPolicy::Reset;
    bResetOnNext = true;
}

float UAttackComponent::GetFPSFor(const UAnimMontage* /*Montage*/, float OverrideFPS) const
{
    return (OverrideFPS > 0.f) ? OverrideFPS : 30.f;
}
void UAttackComponent::ScheduleComboWindows(UAnimMontage* Montage, float PlayRate)
{
    ClearComboWindows();
    const auto& Steps = CurrentFormDef->AttackMontages;
    if (!Steps.IsValidIndex(ComboIndex)) return;
    const auto& S = Steps[ComboIndex];

    const float fps = GetFPSFor(Montage, S.OverrideFPS);
    const int32 SaveF = FMath::Max(0, S.SaveFrame);
    const int32 ResetF = FMath::Max(SaveF, S.ResetFrame);
    const float saveSec = (float)SaveF / fps / FMath::Max(PlayRate, KINDA_SMALL_NUMBER);
    const float resetSec = (float)ResetF / fps / FMath::Max(PlayRate, KINDA_SMALL_NUMBER);

    GetWorld()->GetTimerManager().SetTimer(Timer_Save, this, &UAttackComponent::Notify_SaveAttack, saveSec, false);
    GetWorld()->GetTimerManager().SetTimer(Timer_Reset, this, &UAttackComponent::Notify_ResetCombo, resetSec, false);
    WindowOwnerMontage = Montage; // 타이머의 소유 몽타주
}
void UAttackComponent::ClearComboWindows()
{
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(Timer_Save);
        GetWorld()->GetTimerManager().ClearTimer(Timer_Reset);
    }
    WindowOwnerMontage = nullptr;
}

void UAttackComponent::ResetComboHard()
{
    ClearComboWindows();
    ComboIndex = 0;
    bCanChain = false; bAttackHeld = false; bAdvancedThisWindow = false; bResetOnNext = false; NextPolicy = EComboPolicy::None;
    // 필요하면 현재 몽타주 즉시 정지
    if (UAnimInstance* Anim = GetAnim()) { if (LastAttackMontage) Anim->Montage_Stop(0.05f, LastAttackMontage); }
}
