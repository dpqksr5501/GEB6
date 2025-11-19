// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "FormDefinition.generated.h"

class USkeletalMesh;
class UAnimInstance;
class UAnimMontage;
class USkillSet;
class UFormStatData;
class UWeaponData;


UENUM(BlueprintType)
enum class EFormType : uint8 { Base, Range, Swift, Guard, Special };


USTRUCT(BlueprintType)
struct FAttackStep
{
	GENERATED_BODY()

	// 이 몽타주에서 Save/Reset 이 열리는 "프레임" 번호
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0"))
	int32 SaveFrame = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0"))
	int32 ResetFrame = 0;

	// 선택: 프레임 → 초 변환에 쓸 FPS(0이면 자동 추정)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0"))
	float OverrideFPS = 0.f; // 0: 자동, 그 외: 해당 값 사용(예: 30, 60)

	/** 캐릭터가 움직이는 중에 재생될 상체 전용 몽타주입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Montage")
	TObjectPtr<UAnimMontage> Montage_UpperBody = nullptr;

	/** 캐릭터가 가만히 있을 때 재생될 전신 몽타주입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Montage")
	TObjectPtr<UAnimMontage> Montage_FullBody = nullptr;
};

UCLASS()
class KHU_GEB_API UFormDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Type")
	EFormType FormType = EFormType::Base;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mesh")
	TObjectPtr<USkeletalMesh> Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
	TSubclassOf<UAnimInstance> AnimClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Attack")
	TArray<FAttackStep> AttackMontages;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Skill")
	TObjectPtr<UAnimMontage> SkillMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill")
	TObjectPtr<USkillSet> SkillSet;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	TObjectPtr<UFormStatData> StatData;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<UWeaponData> WeaponData;
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