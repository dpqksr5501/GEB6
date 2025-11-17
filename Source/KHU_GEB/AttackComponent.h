//AttackComponent.h
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Animation/AnimSequenceBase.h"
#include "AttackComponent.generated.h"

class UFormDefinition;
class USkeletalMeshComponent;
class UAnimInstance;
class UAnimMontage;
class UShapeComponent; //공격 판정 콜리전을 사용하기 위해 전방선언
class USphereComponent;
class UBoxComponent;
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
    /** 컴포넌트가 현재 공격 몽타주를 재생 중인지 확인합니다. */
    

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


    //콜리전 오버랩 함수 선언(이름 변경 없음)
    UFUNCTION()
    void OnAttackOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

    UPROPERTY(Transient)
    TArray<TObjectPtr<AActor>> HitActorsThisSwing;

    /** FormManager로부터 폼 변경 이벤트를 수신할 핸들러입니다. */
    UFUNCTION()
    void OnFormChanged_Handler(EFormType NewForm, const UFormDefinition* Def);

    /** BeginPlay에서 호출되어 콜리전 풀을 생성합니다. */
    void InitializeColliderPool(int32 PoolSize = 5); // 폼 당 최대 5개로 가정

    /** 모든 활성 콜리전의 사용을 중지하고 풀로 되돌립니다. */
    void DeactivateAllColliders();

    /** 풀에서 사용 가능한 박스 콜리전을 가져옵니다. */
    UBoxComponent* GetPooledBoxCollider();

    /** 풀에서 사용 가능한 구체 콜리전을 가져옵니다. */
    USphereComponent* GetPooledSphereCollider();

    // 공용 생성 함수 풀 생성  부족 시 생성 모두 여기서 처리
    UBoxComponent* CreateNewBoxCollider();
    USphereComponent* CreateNewSphereCollider();

    /** (bIsAttacking과 별개로) 콤보가 다음 몽타주로 '연계되는 순간'인지 확인합니다. */
    UPROPERTY(Transient)
    bool bIsChaining = false;

private:

    /** 미리 생성해 둔 구체 콜리전 풀 */
    UPROPERTY(Transient)
    TArray<TObjectPtr<USphereComponent>> SphereColliderPool;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UBoxComponent>> BoxColliderPool;
   

    /** 현재 폼에서 '활성화'되어 사용 중인 콜리전 목록 (풀의 일부) */
    UPROPERTY(Transient)
    TArray<TObjectPtr<UShapeComponent>> ActiveColliders;

    //캐싱 : 몽타주별 계산된 FPS 저장하는 곳
    mutable TMap<const UAnimMontage*, float> CachedMontageFPS;

public:

    /*컴포넌트가 현재 공격 몽타주를 재생 중인지 확인합니다.*/
    UPROPERTY(Transient, BlueprintReadOnly, Category = "State")
    bool bIsAttacking = false;

    // 외부(캐릭터) API
    void SetForm(const UFormDefinition* Def);
    void AttackStarted(const FInputActionValue&);    // 입력: 누름
    void AttackTriggered(const FInputActionValue&);  // 입력: 홀드 중
    void AttackCompleted(const FInputActionValue&);  // 입력: 뗌

    // (선택) 전투 중단/리셋
    void ResetComboHard();



protected:
    // 델리게이트 핸들러
    UFUNCTION() void OnMontageEnded(UAnimMontage* Montage, bool bInterrupted);
    UFUNCTION() void OnNotifyBeginReceived(FName NotifyName, const FBranchingPointNotifyPayload& Payload);
    UFUNCTION() void OnNotifyEndReceived(FName NotifyName, const FBranchingPointNotifyPayload& Payload);

};


