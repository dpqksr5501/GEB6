// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "FormDefinition.generated.h"

class USkeletalMesh;
class UAnimInstance;
class USkillSet;

UENUM(BlueprintType)
enum class EFormType : uint8 { Base, Range, Swift, Guard, Special };

UCLASS()
class KHU_GEB_API UFormDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EFormType FormType = EFormType::Base;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<USkeletalMesh> Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<UAnimInstance> AnimClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<USkillSet> SkillSet;
};

UCLASS(BlueprintType)
class KHU_GEB_API UFormSet : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TMap<EFormType, TObjectPtr<UFormDefinition>> Forms;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EFormType DefaultForm = EFormType::Base;

	const UFormDefinition* Find(EFormType T) const
	{
		if (const TObjectPtr<UFormDefinition>* P = Forms.Find(T)) return P->Get();
		return nullptr;
	}
};