// Fill out your copyright notice in the Description page of Project Settings.


#include "FormManagerComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "KHU_GEBCharacter.h"
#include "FormStatsData.h"
#include "PlayerStateBundle.h"
#include "PlayerStatsComponent.h"

// Sets default values for this component's properties
UFormManagerComponent::UFormManagerComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

}


// Called when the game starts
void UFormManagerComponent::BeginPlay()
{
	Super::BeginPlay();

    if (!Stats)
    {
        Stats = NewObject<UPlayerStatsComponent>(GetOwner(), TEXT("PlayerStats"));
        Stats->RegisterComponent();
    }

	// DefaultFormStats → Stats 초기화(처음 한 번)
	if (Stats && Stats->FormStats.Num() == 0)
	{
		for (const auto& P : DefaultFormStats)
		{
			FFormStatState S; S.StatsData = P.Value;
			Stats->FormStats.Add(P.Key, S);
			Stats->RecalcForForm(P.Key);
		}
		if (Stats->MaxHealth <= 0.f) Stats->MaxHealth = 120.f;
		if (Stats->Health <= 0.f) Stats->Health = Stats->MaxHealth;
	}
}

/*
// Called every frame
void UFormManagerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}
*/

AKHU_GEBCharacter* UFormManagerComponent::GetOwnerChar() const
{
    return Cast<AKHU_GEBCharacter>(GetOwner());
}

FTransform UFormManagerComponent::GetSafeSpawnTM(const FTransform& Around) const
{
    FVector Loc = Around.GetLocation();
    FHitResult Hit; FVector S = Loc + FVector(0, 0, 1000), E = Loc - FVector(0, 0, 2000);
    if (GetWorld()->LineTraceSingleByChannel(Hit, S, E, ECC_Visibility))
    {
        float HalfH = 96.f;
        if (auto* C = GetOwnerChar()) if (auto* Cap = C->GetCapsuleComponent()) HalfH = Cap->GetScaledCapsuleHalfHeight();
        Loc = Hit.Location + FVector(0, 0, HalfH + 2);
    }
    FTransform Out = Around; Out.SetLocation(Loc); return Out;
}

void UFormManagerComponent::InitializeForms(EPlayerForm StartForm)
{
    CurrentForm = StartForm;
    // 시작 Pawn(이 매니저의 Owner)에 스탯 반영
    ApplyStatsToOwner();
}

void UFormManagerComponent::ApplyStatsToOwner()
{
    if (!Stats) return;
    if (auto* C = GetOwnerChar())
    {
        if (auto* Move = C->GetCharacterMovement())
            Move->MaxWalkSpeed = Stats->GetMoveSpeed(CurrentForm);
        // 쿨타임 시스템이 있으면 여기서 반영
        // AbilitySystem->SetGlobalCooldown( Stats->GetSkillCooldown(CurrentForm) );
    }
}

FPlayerStateBundle UFormManagerComponent::BuildBundle() const
{
    FPlayerStateBundle B;
    if (Stats)
    {
        B.MaxHP = Stats->MaxHealth; B.HP = FMath::Clamp(Stats->Health, 0.f, Stats->MaxHealth);
        for (const auto& P : Stats->FormStats)
        {
            FFormStatProgressSave S;
            S.Form = P.Key;
            S.AttackLevel = P.Value.Progress.AttackLevel;
            S.DefenseLevel = P.Value.Progress.DefenseLevel;
            S.MoveSpeedLevel = P.Value.Progress.MoveSpeedLevel;
            S.SkillCooldownLevel = P.Value.Progress.SkillCooldownLevel;
            B.FormProgresses.Add(S);
        }
    }
    return B;
}

void UFormManagerComponent::ApplyBundle(const FPlayerStateBundle& B)
{
    if (!Stats) return;
    Stats->MaxHealth = B.MaxHP;
    Stats->Health = FMath::Clamp(B.HP, 0.f, B.MaxHP);
    for (const auto& S : B.FormProgresses)
    {
        if (auto* St = Stats->FormStats.Find(S.Form))
        {
            St->Progress.AttackLevel = S.AttackLevel;
            St->Progress.DefenseLevel = S.DefenseLevel;
            St->Progress.MoveSpeedLevel = S.MoveSpeedLevel;
            St->Progress.SkillCooldownLevel = S.SkillCooldownLevel;
            Stats->RecalcForForm(S.Form);
        }
    }
    // 현재 폼 기준 즉시 반영
    ApplyStatsToOwner();
}

void UFormManagerComponent::SwitchTo(EPlayerForm NewForm)
{
    if (NewForm == CurrentForm) { ApplyStatsToOwner(); return; }

    auto* OwnerChar = GetOwnerChar(); if (!OwnerChar) return;
    APlayerController* PC = Cast<APlayerController>(OwnerChar->GetController()); if (!PC) return;

    TSubclassOf<AKHU_GEBCharacter>* Cls = FormClasses.Find(NewForm);
    if (!Cls || !*Cls) return;

    // 1) 번들 추출
    const FPlayerStateBundle Bundle = BuildBundle();

    // 2) 새 Pawn 스폰
    const FTransform TM = GetInPlaceOrNearbyTM(OwnerChar, *Cls);
    AKHU_GEBCharacter* NewPawn = GetWorld()->SpawnActorDeferred<AKHU_GEBCharacter>(
        *Cls, TM, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
    UGameplayStatics::FinishSpawningActor(NewPawn, TM);

    // 3) 새 Pawn의 매니저에 번들 주입 + 폼 지정 + 즉시 반영
    if (auto* NewMgr = NewPawn->FindComponentByClass<UFormManagerComponent>())
    {
        NewMgr->CurrentForm = NewForm;
        // DefaultFormStats 초기화는 NewMgr.BeginPlay에서 이미 됨 → 번들로 덮어쓰기
        NewMgr->ApplyBundle(Bundle);
    }

    // 4) 포제션 + 페이드
    if (APlayerCameraManager* CM = PC->PlayerCameraManager) CM->StartCameraFade(0, 1, 0.08f, FLinearColor::Black, false, true);
    PC->UnPossess(); PC->Possess(NewPawn);
    if (APlayerCameraManager* CM = PC->PlayerCameraManager) CM->StartCameraFade(1, 0, 0.08f, FLinearColor::Black, false, true);

    // 5) 상태 갱신 & 기존 Pawn 제거
    CurrentForm = NewForm;
    OwnerChar->Destroy();
}

static bool TryCapsuleAt(UWorld* W, const FVector& Loc, float Radius, float HalfH, ECollisionChannel Chan)
{
    FCollisionQueryParams Q; Q.bTraceComplex = false; Q.bReturnPhysicalMaterial = false;
    return !W->OverlapBlockingTestByChannel(Loc, FQuat::Identity, Chan,
        FCollisionShape::MakeCapsule(Radius, HalfH), Q);
}

FTransform UFormManagerComponent::GetInPlaceOrNearbyTM(AKHU_GEBCharacter* OldPawn, TSubclassOf<AKHU_GEBCharacter> NewCls) const
{
    FTransform TM = OldPawn->GetActorTransform();
    UWorld* W = GetWorld();
    if (!W) return GetSafeSpawnTM(TM);

    // 새 폼의 캡슐 크기 예측
    float NewRadius = 42.f, NewHalf = 96.f;
    if (const AKHU_GEBCharacter* CDO = NewCls ? Cast<AKHU_GEBCharacter>(NewCls->GetDefaultObject()) : nullptr)
    {
        if (const UCapsuleComponent* CapCDO = CDO->GetCapsuleComponent())
        {
            NewRadius = CapCDO->GetUnscaledCapsuleRadius() * CDO->GetActorScale3D().Z;
            NewHalf = CapCDO->GetUnscaledCapsuleHalfHeight() * CDO->GetActorScale3D().Z;
        }
    }

    // 1) 제자리 시도(스윕/오버랩 체크)
    const FVector BaseLoc = OldPawn->GetActorLocation();
    if (TryCapsuleAt(W, BaseLoc, NewRadius, NewHalf, ECC_Pawn))
    {
        TM.SetLocation(BaseLoc);
        return TM;
    }

    // 2) 근처 소폭 보정(반경 50~120cm 정도 스파이럴 탐색)
    static const float Radii[] = { 50.f, 75.f, 100.f, 120.f };
    for (float R : Radii)
    {
        for (int i = 0;i < 12;i++)
        {
            const float Ang = (PI * 2.f / 12.f) * i;
            const FVector Cand = BaseLoc + FVector(FMath::Cos(Ang), FMath::Sin(Ang), 0.f) * R;
            if (TryCapsuleAt(W, Cand, NewRadius, NewHalf, ECC_Pawn))
            {
                TM.SetLocation(Cand); return TM;
            }
        }
    }

    // 3) 최후: 기존 바닥 스냅 방식
    return GetSafeSpawnTM(TM);
}
