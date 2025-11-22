// Fill out your copyright notice in the Description page of Project Settings.


#include "ManaComponent.h"
#include "GameFramework/Actor.h"

UManaComponent::UManaComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	// 에디터에서 값 바꾸기 전 기본값
	MaxMana = 100.f;
	RegenPerSecond = 5.f;
	CurrentMana = MaxMana;
}

void UManaComponent::BeginPlay()
{
	Super::BeginPlay();

	// 런타임 시작 시 한 번 더 안전하게 초기화
	CurrentMana = FMath::Max(0.f, MaxMana);
	HandleManaChanged();
}

void UManaComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// 초당 RegenPerSecond만큼 회복
	if (RegenPerSecond > 0.f && CurrentMana < MaxMana)
	{
		const float Delta = RegenPerSecond * DeltaTime;
		AddMana(Delta);
	}
}

void UManaComponent::SetMaxMana(float NewMaxMana, bool bResetCurrent)
{
	MaxMana = FMath::Max(0.f, NewMaxMana);

	if (bResetCurrent)
	{
		CurrentMana = MaxMana;
	}
	else
	{
		CurrentMana = FMath::Clamp(CurrentMana, 0.f, MaxMana);
	}

	HandleManaChanged();
}

void UManaComponent::AddMana(float DeltaMana)
{
	const float OldMana = CurrentMana;
	CurrentMana = FMath::Clamp(CurrentMana + DeltaMana, 0.f, MaxMana);

	if (!FMath::IsNearlyEqual(OldMana, CurrentMana))
	{
		HandleManaChanged();
	}
}

bool UManaComponent::ConsumeMana(float Amount)
{
	if (Amount <= 0.f)
	{
		return true; // 0 이하면 그냥 통과
	}

	if (!HasEnoughMana(Amount))
	{
		return false;
	}

	CurrentMana = FMath::Clamp(CurrentMana - Amount, 0.f, MaxMana);
	HandleManaChanged();
	return true;
}

bool UManaComponent::HasEnoughMana(float Amount) const
{
	if (Amount <= 0.f)
	{
		return true;
	}
	return CurrentMana >= Amount;
}

void UManaComponent::HandleManaChanged()
{
	OnManaChanged.Broadcast(CurrentMana, MaxMana);
}
