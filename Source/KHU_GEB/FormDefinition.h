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

// [새로 추가] 히트박스 모양을 정의할 Enum
UENUM(BlueprintType)
enum class EHitboxShape : uint8
{
	Box,
	Sphere
};

// [새로 추가] 개별 히트박스 설정을 위한 구조체
USTRUCT(BlueprintType)
struct FHitboxConfig
{
	GENERATED_BODY()

	/** 이 히트박스를 부착할 스켈레탈 메시 소켓 이름 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName SocketName = NAME_None;

	/** 히트박스 모양 (박스 또는 구체) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EHitboxShape Shape = EHitboxShape::Box;

	/**
	* 모양에 따른 크기 값:
	* - Box: BoxExtent (FVector(X, Y, Z))
	* - Sphere: Radius (FVector(Radius, 0, 0))
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FVector Size = FVector(32.f);

	/** 소켓 위치를 기준으로 한 추가적인 위치 오프셋입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transform")
	FVector RelativeLocation = FVector::ZeroVector;

	/** 소켓 회전을 기준으로 한 추가적인 회전 오프셋입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transform")
	FRotator RelativeRotation = FRotator::ZeroRotator;
};



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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack")
	TArray<FHitboxConfig> Hitboxes;

	/** 이 폼일 때의 기본 최대 이동 속도입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = "0.0"))
	float BaseWalkSpeed = 600.f; // 기본값 600으로 설정

	/** 이 폼일 때의 기본 가속도입니다. (0이면 캐릭터 무브먼트 컴포넌트의 기본값을 사용) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = "0.0"))
	float BaseAcceleration = 0.f; // 0.0f으로 설정하여 기본값을 유지하도록 함
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