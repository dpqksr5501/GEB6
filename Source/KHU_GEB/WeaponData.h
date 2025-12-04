// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "WeaponData.generated.h"

/**
 *
 */

 // 1. FormDefinition.h에서 EHitboxShape ENUM을 이곳으로 이동
UENUM(BlueprintType)
enum class EHitboxShape : uint8
{
	Box,
	Sphere
};

// 2. FormDefinition.h에서 FHitboxConfig 구조체를 이곳으로 이동
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

/**
 * 무기의 콜리전(히트박스) 구성을 정의하는 데이터 에셋입니다.
 */
UCLASS(BlueprintType)
class KHU_GEB_API UWeaponData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// 3. UFormDefinition이 가지고 있던 Hitboxes 배열을 이 클래스가 소유합니다.
	/** 이 무기를 구성하는 히트박스 목록입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	TArray<FHitboxConfig> Hitboxes;
};
