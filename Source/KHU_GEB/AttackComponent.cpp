//AttackComponent.cpp
// Fill out your copyright notice in the Description page of Project Settings.


#include "AttackComponent.h"
#include "FormDefinition.h"
#include "WeaponComponent.h" // 1. [추가] WeaponComponent 헤더
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "GameFramework/Actor.h"
#include "KHU_GEBCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "InputActionValue.h"
#include "Engine/World.h"
#include "Kismet/KismetSystemLibrary.h"
#include "FormManagerComponent.h"
#include "GameFramework/Character.h"



UAttackComponent::UAttackComponent() {}

void UAttackComponent::BeginPlay()
{
    Super::BeginPlay();

    // 소유자(캐릭터)에서 FormManager를 찾아 델리게이트에 바인딩합니다.
    if (AActor* Owner = GetOwner())
    {
        if (UFormManagerComponent* FormManager =
            Owner->FindComponentByClass<UFormManagerComponent>())
        {
            FormManager->OnFormChanged.AddDynamic(this,
                &UAttackComponent::OnFormChanged_Handler);

            const UFormDefinition* InitialDef =
                FormManager->FindDef(FormManager->CurrentForm);
            if (InitialDef)
            {
                OnFormChanged_Handler(FormManager->CurrentForm, InitialDef);
            }
        }
    }

    //애님 델리게이트 바인딩은 유지.
    BindAnimDelegates();
}

USkeletalMeshComponent* UAttackComponent::GetMesh() const
{
    AActor* Owner = GetOwner();
    if (!Owner) return nullptr;

    //ACharacter 대신 AKHU_GEBCharacter로 직접 캐스팅합니다.
    if (AKHU_GEBCharacter* OwnerCharacter = Cast<AKHU_GEBCharacter>(Owner))
    {
        return OwnerCharacter->GetMesh();
    }

    //혹시 몬스터(AMonsterBase)에도 이 컴포넌트가 붙을 경우를 대비
    //if (amonsterbase* ownermonster = cast<amonsterbase>(owner))
    //{
    //    return ownermonster->getmesh(); // amonsterbase도 acharacter를 상속받았으므로 getmesh()가 있음
    //}

    // 둘 다 아니라면, 일반적인 방법으로 탐색
    return Owner->FindComponentByClass<USkeletalMeshComponent>();
}

UAnimInstance* UAttackComponent::GetAnim() const
{
    if (USkeletalMeshComponent* Mesh = GetMesh())
    {
        return Mesh->GetAnimInstance();
    }
    return nullptr;
}

//SetForm 함수 수정---
//SetForm 더 이상 콜리전을 생성하지 않음.
void UAttackComponent::SetForm(const UFormDefinition* Def)
{
    BindAnimDelegates();
    CurrentFormDef = Def;

    if (!CurrentFormDef) return;

    USkeletalMeshComponent* Mesh = GetMesh();
    if (!Mesh) return;
}

void UAttackComponent::BindAnimDelegates()
{
    //이전에 바인딩된 애님 인스턴스가 있다면, 먼저 연결을 해제합니다.
    UnbindAnimDelegates();

    //현재 유효한 새 애님 인스턴스를 가져옵니다.
    UAnimInstance* Anim = GetAnim();
    if (Anim)
    {
        //새 애님 인스턴스의 델리게이트에 우리 함수들을 연결합니다.
        Anim->OnPlayMontageNotifyBegin.AddDynamic(this, &UAttackComponent::OnNotifyBeginReceived);
        Anim->OnPlayMontageNotifyEnd.AddDynamic(this, &UAttackComponent::OnNotifyEndReceived);
        Anim->OnMontageEnded.AddDynamic(this, &UAttackComponent::OnMontageEnded);

        //[핵심] "현재 바인딩된 애님 인스턴스"가 무엇인지 변수에 저장합니다.
        BoundAnim = Anim;
    }
}
void UAttackComponent::UnbindAnimDelegates()
{
    //이전에 저장해 둔(BoundAnim) 애님 인스턴스 포인터를 가져옵니다.
    UAnimInstance* Prev = BoundAnim.Get(); // TWeakObjectPtr에서 가져옴
    if (Prev)
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
    // 로직 순서 변경 
    //콤보 연계가 가능한지 확인합니다. 
    // (Save 프레임이 지났고, 이번 창에서 아직 연계하지 않았는지) 
    if (bCanChain && !bAdvancedThisWindow)
    {
        // 2. 가능하다면 즉시 다음 콤보로 넘어갑니다. 
        AdvanceComboImmediately();
        // 3. 연계를 했으므로 함수를 종료합니다. (밑의 1타 재생 로직 실행 방지) 
        return;
    }

    // 콤보 연계 상태가 아닐 때,
    if (bIsAttacking)
    {
        return;
    }

    // bIsAttacking이 false일 때 (즉, 공격 중이 아닐 때) 1타 콤보를 새로 시작합니다.
    // *오직 콤보 인덱스가 0일 때만* 1타를 시작해야 합니다.
    if (ComboIndex == 0)
    {
        if (UAnimInstance* Anim = GetAnim())
        {
            // (안전장치) 몽타주가 정말 안 돌고 있는지 한 번 더 확인
            if (!Anim->Montage_IsPlaying(nullptr))
            {
                // 콤보 상태 초기화 (ComboIndex는 이미 0)
                bCanChain = false;
                bAdvancedThisWindow = false;
                bResetOnNext = false;
                NextPolicy = EComboPolicy::None;

                PlayCurrentComboMontage(); // 1타 콤보 시작
            }
        }
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

    if (!CurrentFormDef) {
        return;
    }

    UAnimInstance* Anim = GetAnim();
    if (!Anim) return;

    // 몽타주를 재생하기 직전에, '공격 중' 상태 플래그를 true로 설정
    bIsAttacking = true;

    // FormDefinition의 스텝 구조에 맞춰 접근
    const auto& Steps = CurrentFormDef->AttackMontages;

    // [수정된 부분 1] .Montage 검사를 제거합니다.
    if (!Steps.IsValidIndex(ComboIndex)) {
        ComboIndex = 0; return;
    }

    // --- 여기부터가 FullBody/UpperBody 선택 로직 ---

    const FAttackStep& CurrentStep = Steps[ComboIndex];

    // 1. 현재 캐릭터의 속도를 가져옵니다.
    float CurrentSpeed = 0.0f;
    if (AActor* Owner = GetOwner())
    {
        if (APawn* OwnerPawn = Cast<APawn>(Owner))
        {
            CurrentSpeed = OwnerPawn->GetVelocity().Size();
        }
    }

    // 2. 속도에 따라 재생할 몽타주를 선택합니다.
    UAnimMontage* MontageToPlay = nullptr;
    const float FullBodySpeedThreshold = 20.0f;

    if (CurrentSpeed < FullBodySpeedThreshold)
    {
        // 속도가 20 미만이면 FullBody 몽타주를 선택
        MontageToPlay = CurrentStep.Montage_FullBody;
    }
    else
    {
        // 그렇지 않으면 UpperBody 몽타주를 선택
        MontageToPlay = CurrentStep.Montage_UpperBody;
    }

    // 3. (필수) 두 슬롯 중 하나라도 비어있으면 공격이 멈추므로, 유효성 검사를 합니다.
    if (!MontageToPlay)
    {
        if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("[2-ERROR] Montage is MISSING in DA for ComboIndex %d!"), ComboIndex));
        ComboIndex = 0; return;
    }

    //M 변수에 MontageToPlay를 할당합니다.
    UAnimMontage* M = MontageToPlay;
    LastAttackMontage = M;
    Anim->Montage_Play(M, PlayRate);

    // 새 콤보 시작: 창구 초기화 & 프레임 타이머 예약
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

    /*if (UAnimInstance* Anim = GetAnim())
    {
        if (LastAttackMontage) { Anim->Montage_Stop(0.05f, LastAttackMontage); }
    }*/
    //애니메이션 보간을 위해 삭제

    bAdvancedThisWindow = true;
    bResetOnNext = false; NextPolicy = EComboPolicy::None;
    bIsChaining = true;

    PlayCurrentComboMontage();
}

void UAttackComponent::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
    // 이전 몽타주의 End가 늦게 와도, '그 몽타주' 소유 타이머만 지움
    if (Montage == WindowOwnerMontage) { ClearComboWindows(); }


    // 만약 "체인 중" 플래그가 true라면 (AdvanceComboImmediately가 방금 호출됨),
    // 이 OnMontageEnded는 이전 몽타주가 '중단'된 콜백입니다.
    // 새 몽타주가 이미 bIsAttacking=true로 재생 중이므로,
    // bIsAttacking을 false로 바꾸면 안 됩니다.
    if (bIsChaining)
    {
        bIsChaining = false; // 플래그만 리셋하고 종료
        return;
    }


    // 체인 중이 아닐 때 (즉, 몽타주가 자연스럽게 끝났거나, 피격 등으로 중단됨)
    // '공격 중' 상태 플래그를 false로 해제합니다.
    bIsAttacking = false;

    // 몽타주가 '자연스럽게' 끝났다면 (binterrupted == false),
    // 연타 중(bAttackHeld == true)이더라도 콤보가 리셋되어야
    // AttackStarted가 ComboIndex 0으로 1타를 다시 시작하는 버그를 막을 수 있습니다.
    // (단, ResetTimer가 이미 발동한 경우는 제외)
    if (!bInterrupted && !bResetOnNext) // <-- 대문자 'I'를 소문자 'i'로 수정
    {
        // 몽타주가 자연스럽게 끝났는데 콤보 연계(bIsChaining)도 없었음
        // = 콤보 실패
        ComboIndex = 0;
        bCanChain = false;
        bAdvancedThisWindow = false;
        bResetOnNext = false;
        NextPolicy = EComboPolicy::None;
    }
}

// 애님 노티파이(Notify) 수신 함수
void UAttackComponent::OnNotifyBeginReceived(FName NotifyName, const FBranchingPointNotifyPayload& Payload)
{
    AActor* Owner = GetOwner();
    if (!Owner) return;

    //[추가] WeaponComponent를 찾습니다.
    UWeaponComponent* WeaponComp = Owner->FindComponentByClass<UWeaponComponent>();
    if (!WeaponComp) return;

    //디버깅 메시지 WeaponComponent가 없는 경우 에러 로그
    if (!WeaponComp)
    {
        UE_LOG(LogTemp, Error, TEXT("[AttackComponent] OnNotifyBeginReceived: FAILED to find WeaponComponent!"));
        return;
    }


    if (NotifyName == TEXT("StartAttack"))
    {

        //디버깅 메시지 "StartAttack" 노티파이 수신 및 위임 로그
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan,
                TEXT("[AttackComponent] Notify: StartAttack -> Delegating to WeaponComponent"));
        }
        //[수정] 콜리전 활성화를 WeaponComponent에 위임
        WeaponComp->EnableCollision();

    }
    else if (NotifyName == TEXT("EndAttack"))
    {
        //디버깅 메시지 "EndAttack" 노티파이 수신 및 위임 로그
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan,
                TEXT("[AttackComponent] Notify: EndAttack -> Delegating to WeaponComponent"));
        }

        //[수정] 콜리전 비활성화를 WeaponComponent에 위임
        WeaponComp->DisableCollision();
 
    }
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

float UAttackComponent::GetFPSFor(const UAnimMontage* Montage, float OverrideFPS) const
{
    // 1. DA에서 사용자가 OverrideFPS를 명시한 경우 (우선)
    if (OverrideFPS > 0.f)
    {
        return OverrideFPS;
    }

    // 2. 캐시 확인 (mutable TMap 사용)
    if (Montage)
    {
        if (const float* Cached = CachedMontageFPS.Find(Montage))
        {
            return *Cached;
        }

        const int32 TotalKeys = Montage->GetNumberOfSampledKeys();
        const float Length = Montage->GetPlayLength();

        if (TotalKeys > 0 && Length > KINDA_SMALL_NUMBER)
        {
            const float CalculatedFPS = float(TotalKeys) / Length;
            CachedMontageFPS.Add(Montage, CalculatedFPS);
            return CalculatedFPS;
        }
    }

    // 3. 디폴트
    return 30.f;
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
    // 콤보를 강제 리셋하므로, 모든 상태 플래그를 해제합니다.
    bIsAttacking = false;
    bIsChaining = false;

    if (UAnimInstance* Anim = GetAnim()) { if (LastAttackMontage) Anim->Montage_Stop(0.05f, LastAttackMontage); }
}

// 오버랩(히트) 함수
void UAttackComponent::OnAttackOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{

    if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan, TEXT("=== OnAttackOverlap Triggered! ==="));

    
    AActor* Owner = GetOwner();
    if (!Owner || OtherActor == Owner) return;
    if (HitActorsThisSwing.Contains(OtherActor)) return;

    // ... (데미지 적용 로직) ...
    // ... (대미지 계산 로직, 4/5번 개선안 미적용 상태) ...
    float DamageToApply = 10.f; // [개선안 5번 미적용 시 하드코딩된 대미지]

    APawn* OwnerPawn = Cast<APawn>(Owner);

    UGameplayStatics::ApplyDamage(OtherActor,
        DamageToApply,
        (OwnerPawn ? OwnerPawn->GetController() : nullptr),
        Owner,
        nullptr);

    HitActorsThisSwing.Add(OtherActor);

    // [!!! 기존 UE_LOG 대신 또는 함께 이 코드를 추가 !!!]
    if (GEngine)
    {
        FString Msg = FString::Printf(TEXT("ATTACK HIT! %s -> %s (%f DMG)"),
            *Owner->GetName(), *OtherActor->GetName(), DamageToApply);
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, Msg);
    }

    // 기존 로그도 유용하므로 남겨둡니다.
    UE_LOG(LogTemp, Warning, TEXT("Attack Hit via %s: %s"),
        *OverlappedComponent->GetName(), *OtherActor->GetName());
}

/** 폼 변경 핸들러 구현 */
void UAttackComponent::OnFormChanged_Handler(EFormType NewForm, const UFormDefinition* Def)
{
    ResetComboHard();
    // 캐릭터를 거치지 않고 직접 SetForm을 호출합니다.
    SetForm(Def);
}

