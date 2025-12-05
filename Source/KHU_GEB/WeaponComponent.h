// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WeaponData.h" //추가
#include "WeaponComponent.generated.h"

class UBoxComponent;
class USphereComponent;
class UShapeComponent;
class UWeaponData;

/**
 * 무기의 물리적인 콜리전과 오버랩 이벤트를 관리하는 컴포넌트입니다.
 * UAttackComponent는 '콤보 로직'만 담당하고, 이 컴포넌트가 '물리 판정'을 담당합니다.
 */
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class KHU_GEB_API UWeaponComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UWeaponComponent();

	// UAttackComponent의 애님 노티파이(StartAttack)에서 호출될 함수
	void EnableCollision();

	// UAttackComponent의 애님 노티파이(EndAttack)에서 호출될 함수
	void DisableCollision();

	/**
	 * 캐릭터의 OnFormChanged가 호출될 때, 이 함수를 호출하여
	 * 현재 폼에 맞는 무기(의 히트박스)를 설정합니다.
	 */
	void SetWeaponDefinition(const UWeaponData* Def);

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	/** 미리 생성해 둔 구체 콜리전 풀 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<USphereComponent>> SphereColliderPool;

	/** 미리 생성해 둔 박스 콜리전 풀 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UBoxComponent>> BoxColliderPool;

	/** 현재 폼에서 '활성화'되어 사용 중인 콜리전 목록 (풀의 일부) */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UShapeComponent>> ActiveColliders;

	/** 이 스윙(Enable~Disable 사이)에서 이미 맞은 액터 목록 (중복 히트 방지) */
	UPROPERTY(Transient)
	TArray<TObjectPtr<AActor>> HitActorsThisSwing;

	/** 현재 장착된 무기 데이터 (캐시용) */
	UPROPERTY(Transient)
	const UWeaponData* CurrentWeaponDef = nullptr;

	/** 소유자(캐릭터)의 스켈레탈 메시 (캐시용) */
	UPROPERTY(Transient)
	TWeakObjectPtr<USkeletalMeshComponent> CachedMesh;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// [그룹 1] Base, Guard, Swift 폼일 때 재생할 사운드
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Sound")
	USoundBase* MeleeHitSound;

	// [그룹 2] Range, Special 폼일 때 재생할 사운드
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Sound")
	USoundBase* HeavyHitSound;

private:
	/** 풀에서 사용 가능한 박스 콜리전을 가져옵니다. */
	UBoxComponent* GetPooledBoxCollider();
	/** 풀에서 사용 가능한 구체 콜리전을 가져옵니다. */
	USphereComponent* GetPooledSphereCollider();
	/** 모든 활성 콜리전의 사용을 중지하고 풀로 되돌립니다. */
	void DeactivateAllColliders();
	/** 콜리전 풀을 생성합니다. (내부용) */
	UBoxComponent* CreateNewBoxCollider();
	USphereComponent* CreateNewSphereCollider();
	void InitializeColliderPool(int32 PoolSize = 5);

	/** [핵심] UAttackComponent에서 가져온 오버랩 핸들러 */
	UFUNCTION()
	void OnAttackOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};
