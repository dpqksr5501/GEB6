// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AttackComponent.generated.h"

class UFormDefinition;
class USkeletalMeshComponent;
class UAnimInstance;
class UAnimMontage;
struct FInputActionValue;

UENUM()
enum class EComboPolicy : uint8 { None, Advance, Reset };

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class KHU_GEB_API UAttackComponent : public UActorComponent
{
	GENERATED_BODY()

    // 데이터/상태
    UPROPERTY(Transient) const UFormDefinition* CurrentFormDef = nullptr;
    UPROPERTY(Transient) int32 ComboIndex = 0;

    UPROPERTY(Transient) bool bCanChain = false;
    UPROPERTY(Transient) bool bAttackHeld = false;
    UPROPERTY(Transient) bool bAdvancedThisWindow = false;
    UPROPERTY(Transient) bool bResetOnNext = false;
    UPROPERTY(Transient) EComboPolicy NextPolicy = EComboPolicy::None;

    // 애님 추적/레이스 가드
    UPROPERTY(Transient) TWeakObjectPtr<UAnimInstance> BoundAnim;
    UPROPERTY(Transient) TObjectPtr<UAnimMontage> LastAttackMontage = nullptr;
    UPROPERTY(Transient) TObjectPtr<UAnimMontage> WindowOwnerMontage = nullptr;

public:	
	// Sets default values for this component's properties
	UAttackComponent();

private:
    // 타이머
    FTimerHandle Timer_Save, Timer_Reset;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

    // 내부 로직
    void PlayCurrentComboMontage(float PlayRate = 1.f);
    void ComputeNextIndex();
    void AdvanceComboImmediately();

    // Save / Reset 창구
    void Notify_SaveAttack();
    void Notify_ResetCombo();

    // 프레임→타이머
    void ScheduleComboWindows(UAnimMontage* Montage, float PlayRate);
    void ClearComboWindows();
    float GetFPSFor(const UAnimMontage* Montage, float OverrideFPS) const;

    // 애님 델리게이트
    void BindAnimDelegates();
    void UnbindAnimDelegates();

    // 유틸
    USkeletalMeshComponent* GetMesh() const;
    UAnimInstance* GetAnim() const;


public:	
    // 외부(캐릭터) API
    void SetForm(const UFormDefinition* Def);
    void AttackStarted(const FInputActionValue&);    // 입력: 누름
    void AttackTriggered(const FInputActionValue&);  // 입력: 홀드 중
    void AttackCompleted(const FInputActionValue&);  // 입력: 뗌

    // (선택) 전투 중단/리셋
    void ResetComboHard();


    /** 컴포넌트가 현재 공격 몽타주를 재생 중인지 확인합니다. */
    UPROPERTY(Transient, BlueprintReadOnly, Category = "State")
    bool bIsAttacking = false;

protected:
    // 델리게이트 핸들러
    UFUNCTION() void OnMontageEnded(UAnimMontage* Montage, bool bInterrupted);
    UFUNCTION() void OnNotifyBeginReceived(FName NotifyName, const FBranchingPointNotifyPayload& Payload);
    UFUNCTION() void OnNotifyEndReceived(FName NotifyName, const FBranchingPointNotifyPayload& Payload);

};
