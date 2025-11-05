// Fill out your copyright notice in the Description page of Project Settings.

#include "FormManagerComponent.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"

UFormManagerComponent::UFormManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UFormManagerComponent::FindMesh()
{
	if (CachedMesh.IsValid()) return;
	AActor* Owner = GetOwner();
	if (!Owner) return;

	// 우선순위: ACharacter의 GetMesh → 일반 탐색
	if (auto* Ch = Cast<ACharacter>(Owner)) CachedMesh = Ch->GetMesh();
	if (!CachedMesh.IsValid()) CachedMesh = Owner->FindComponentByClass<USkeletalMeshComponent>();
}

const UFormDefinition* UFormManagerComponent::FindDef(EFormType T) const
{
	return (FormSet ? FormSet->Find(T) : nullptr);
}

void UFormManagerComponent::InitializeForms()
{
	FindMesh();
	if (!FormSet || !CachedMesh.IsValid()) return;

	CurrentForm = FormSet->DefaultForm;
	if (const UFormDefinition* Def = FindDef(CurrentForm))
	{
		ApplyMesh(Def);
		OnFormChanged.Broadcast(CurrentForm, Def);
	}
}

void UFormManagerComponent::SwitchTo(EFormType NewForm)
{
	FindMesh();
	if (!FormSet || !CachedMesh.IsValid()) return;
	if (NewForm == CurrentForm) return;

	if (const UFormDefinition* Def = FindDef(NewForm))
	{
		CurrentForm = NewForm;
		ApplyMesh(Def);
		OnFormChanged.Broadcast(NewForm, Def);
	}
}

void UFormManagerComponent::ApplyMesh(const UFormDefinition* Def)
{
	if (!Def || !CachedMesh.IsValid()) return;

	// 1) Mesh 교체 (있으면)
	if (Def->Mesh)
	{
		CachedMesh->SetSkeletalMesh(Def->Mesh);
	}

	// 2) AnimBP 교체 (있으면)
	if (Def->AnimClass)
	{
		// 안전하게 모드 지정 후 클래스 설정
		CachedMesh->SetAnimationMode(EAnimationMode::AnimationBlueprint);
		CachedMesh->SetAnimInstanceClass(Def->AnimClass);
	}

	// 참고: 필요 시 초기화(대개 자동 되지만 명시해도 무방)
	CachedMesh->InitAnim(true);
}
