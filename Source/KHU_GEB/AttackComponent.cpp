//AttackComponent.cpp
// Fill out your copyright notice in the Description page of Project Settings.


#include "AttackComponent.h"
#include "FormDefinition.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "GameFramework/Actor.h"
#include "KHU_GEBCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "InputActionValue.h"
#include "Engine/World.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Components/SphereComponent.h" // Sphere 생성용
#include "Components/BoxComponent.h"     // Box 생성용
#include "Kismet/GameplayStatics.h"
#include "FormManagerComponent.h"


UAttackComponent::UAttackComponent() {}

void UAttackComponent::BeginPlay()
{
    Super::BeginPlay();

    // 소유자(캐릭터)에서 FormManager를 찾아 델리게이트에 바인딩합니다.
    if (AActor* Owner = GetOwner())
    {
        if (UFormManagerComponent* FormManager = Owner->FindComponentByClass<UFormManagerComponent>())
        {
            //델리게이트 바인딩
            FormManager->OnFormChanged.AddDynamic(this, &UAttackComponent::OnFormChanged_Handler);

            // 바인딩 직후, FormManager의 현재 폼을 가져와서 수동으로 초기화해줍니다.
            // (InitializeForms 방송을 놓쳤을 경우를 대비)
            const UFormDefinition* InitialDef = FormManager->FindDef(FormManager->CurrentForm);
            if (InitialDef)
            {
                OnFormChanged_Handler(FormManager->CurrentForm, InitialDef);
            }
        }
    }
    InitializeColliderPool(5);

    //if (!BoundAnim.IsValid())
    //{
    //    BindAnimDelegates();
    //}
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
// [수정 후] SetForm (데이터 기반으로 변경)
void UAttackComponent::SetForm(const UFormDefinition* Def)
{
    CurrentFormDef = Def;
    //기존 폼의 콜리전을 파괴하지 않고 "비활성화" (풀로 반환)
    DeactivateAllColliders();

    BindAnimDelegates(); // 애님 델리게이트 바인딩

    if (!CurrentFormDef) return;

    USkeletalMeshComponent* Mesh = GetMesh();
    if (!Mesh) return;

    // 1-2. 폼 정의(Form Definition)의 Hitboxes 배열을 순회
    for (const FHitboxConfig& Config : CurrentFormDef->Hitboxes)
    {
        UShapeComponent* NewCollider = nullptr;

        // 1-3. 설정에 따라 풀에서 콜리전 가져오기
        if (Config.Shape == EHitboxShape::Sphere)
        {
            USphereComponent* Sphere = GetPooledSphereCollider();
            if (Sphere)
            {
                Sphere->SetSphereRadius(Config.Size.X); // Size.X를 Radius로 사용
                NewCollider = Sphere;
            }
        }
        else // EHitboxShape::Box
        {
            UBoxComponent* Box = GetPooledBoxCollider();
            if (Box)
            {
                Box->SetBoxExtent(Config.Size); // Size를 BoxExtent로 사용
                NewCollider = Box;
            }
        }

        // 1-4. 콜리전을 소켓에 부착하고 활성 목록에 추가
        if (NewCollider)
        {
            // 소켓에 부착하기 *전에* 컴포넌트 자체의 상대 오프셋을 설정합니다.
            // 최종 위치 = (소켓의 월드 위치) + (이 오프셋이 적용된 상대 위치)
            NewCollider->SetRelativeLocationAndRotation(Config.RelativeLocation, Config.RelativeRotation);

            // 소켓에 부착합니다. (규칙은 KeepRelativeTransform 유지)
            NewCollider->AttachToComponent(Mesh, FAttachmentTransformRules::KeepRelativeTransform, Config.SocketName);

            ActiveColliders.Add(NewCollider);
        }
    }
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
    if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::White, TEXT("[1] AttackStarted: Input Received"));
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
    // [디버그 2]
    if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::White, TEXT("[2] PlayCurrentComboMontage: Called"));
    if (!CurrentFormDef) { 
        // [디버그 2-E]
        if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("[2-ERROR] CurrentFormDef is NULL!"));
        return;
    }
    UAnimInstance* Anim = GetAnim(); if (!Anim) return;

    // FormDefinition의 스텝 구조에 맞춰 접근
    const auto& Steps = CurrentFormDef->AttackMontages; // FAttackStep 배열
    if (!Steps.IsValidIndex(ComboIndex) || !Steps[ComboIndex].Montage) { 
        // [디버그 2-E]
        if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("[2-ERROR] Montage is MISSING in DA!"));
        ComboIndex = 0; return;
    }
    // [디버그 3]
    if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::White, TEXT("[3] Montage_Play: EXECUTED"));

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
    // 몽타주가 끝나면 이 스윙에서 적중한 목록을 초기화
    HitActorsThisSwing.Empty();
    // 이전 몽타주의 End가 늦게 와도, '그 몽타주' 소유 타이머만 지움
    if (Montage == WindowOwnerMontage) { ClearComboWindows(); }

    if (!bAttackHeld)
    {
        ComboIndex = 0;
        bCanChain = false; bAdvancedThisWindow = false; bResetOnNext = false; NextPolicy = EComboPolicy::None;
    }
}

// 애님 노티파이(Notify) 수신 함수
void UAttackComponent::OnNotifyBeginReceived(FName NotifyName, const FBranchingPointNotifyPayload& Payload)
{
    // ... (기존 콤보 로직: Notify_SaveAttack, Notify_ResetCombo 등) ...
    // [디버그 4] (어떤 노티파이든 받으면 출력)
    if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Purple,
        FString::Printf(TEXT("[4] OnNotifyBeginReceived: %s"), *NotifyName.ToString()));

    if (NotifyName == TEXT("StartAttack"))
    {
        // [디버그 5](StartAttack 노티파이를 받으면 출력)
        if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Orange, TEXT("[5] COLLISION ENABLED (StartAttack)"));
        HitActorsThisSwing.Empty(); // 맞은 액터 목록 초기화

        // 현재 폼이 가진 모든 콜리전 볼륨(히트박스)을 활성화합니다.
        for (UShapeComponent* Collider : ActiveColliders)
        {
            if (Collider)
            {
                Collider->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
            }
        }
    }
    else if (NotifyName == TEXT("EndAttack"))
    {
        // [디버그 5-End]
        if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Orange, TEXT("[5-End] COLLISION DISABLED (EndAttack)"));

        // 모든 콜리전 볼륨(히트박스)을 비활성화합니다.
        for (UShapeComponent* Collider : ActiveColliders)
        {
            if (Collider)
            {
                Collider->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            }
        }
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
    // 캐릭터를 거치지 않고 직접 SetForm을 호출합니다.
    SetForm(Def);
}

// [새 함수] 콜리전 풀 초기화
void UAttackComponent::InitializeColliderPool(int32 PoolSize)
{
    if (!GetOwner()) return;

    for (int32 i = 0; i < PoolSize; ++i)
    {
        // 1. 박스 생성
        if (UBoxComponent* Box = CreateNewBoxCollider())
        {
            BoxColliderPool.Add(Box);
        }

        // 2. 구체 생성
        if (USphereComponent* Sphere = CreateNewSphereCollider())
        {
            SphereColliderPool.Add(Sphere);
        }
    }
}

// [새 함수] 풀에서 박스 가져오기
UBoxComponent* UAttackComponent::GetPooledBoxCollider()
{
    // 현재 활성화되지 않은 (ActiveColliders에 없는) 콜리전 탐색
    for (UBoxComponent* Box : BoxColliderPool)
    {
        if (Box && !ActiveColliders.Contains(Box))
        {
            return Box;
        }
    }

    // 부족하므로 새로 생성
    UBoxComponent* NewBox = CreateNewBoxCollider();
    BoxColliderPool.Add(NewBox);
    return NewBox;

}

// [새 함수] 풀에서 구체 가져오기
USphereComponent* UAttackComponent::GetPooledSphereCollider()
{
    for (USphereComponent* Sphere : SphereColliderPool)
    {
        if (Sphere && !ActiveColliders.Contains(Sphere))
        {
            return Sphere;
        }
    }

    // 부족 → 새로 생성
    USphereComponent* NewSphere = CreateNewSphereCollider();
    SphereColliderPool.Add(NewSphere);
    return NewSphere;
}

// [수정 후] DeactivateAllColliders (DestroyComponent 제거)
void UAttackComponent::DeactivateAllColliders()
{
    USkeletalMeshComponent* Mesh = GetMesh();
    for (UShapeComponent* Collider : ActiveColliders)
    {
        if (!Collider) continue;

        // 1) 충돌 비활성화
        Collider->SetCollisionEnabled(ECollisionEnabled::NoCollision);

        // 2) 소켓에서 분리 (상대 좌표 유지한 채)
        //    KeepRelativeTransform 사용해야 다음 Attach 시 서로 영향을 안 줌
        Collider->DetachFromComponent(FDetachmentTransformRules::KeepRelativeTransform);

        // 3) 풀 복귀를 위해 초기화
        //    상대 좌표 초기화는 Detach 상태에서도 의미 있음 (다음 Attach에 적용됨)
        Collider->SetRelativeLocation(FVector::ZeroVector);
        Collider->SetRelativeRotation(FRotator::ZeroRotator);

        // 크기 초기화 (박스/스피어 모두)
        if (UBoxComponent* Box = Cast<UBoxComponent>(Collider))
        {
            Box->SetBoxExtent(FVector::ZeroVector);
        }
        else if (USphereComponent* Sphere = Cast<USphereComponent>(Collider))
        {
            Sphere->SetSphereRadius(0.0f);
        }
    }
    ActiveColliders.Empty(); // 활성 목록만 비움 (풀은 유지)
}



UBoxComponent* UAttackComponent::CreateNewBoxCollider()
{
    AActor* Owner = GetOwner();
    if (!Owner) return nullptr;

    UBoxComponent* NewBox = NewObject<UBoxComponent>(Owner);
    if (!NewBox) return nullptr;

    NewBox->RegisterComponent();
    NewBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    NewBox->OnComponentBeginOverlap.AddDynamic(this, &UAttackComponent::OnAttackOverlap);
    NewBox->SetCollisionObjectType(ECC_WorldDynamic);
    NewBox->SetCollisionResponseToAllChannels(ECR_Ignore);
    NewBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    NewBox->SetHiddenInGame(false);

    return NewBox;
}

USphereComponent* UAttackComponent::CreateNewSphereCollider()
{
    AActor* Owner = GetOwner();
    if (!Owner) return nullptr;

    USphereComponent* NewSphere = NewObject<USphereComponent>(Owner);
    if (!NewSphere) return nullptr;

    NewSphere->RegisterComponent();
    NewSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    NewSphere->OnComponentBeginOverlap.AddDynamic(this, &UAttackComponent::OnAttackOverlap);
    NewSphere->SetCollisionObjectType(ECC_WorldDynamic);
    NewSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
    NewSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    NewSphere->SetHiddenInGame(false);

    return NewSphere;
}

