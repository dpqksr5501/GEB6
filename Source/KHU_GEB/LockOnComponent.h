// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LockOnComponent.generated.h"

class UHealthComponent;

/** 락온 상태 변경 델리게이트 (UI 연동용) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLockOnTargetChanged, AActor*, NewTarget);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class KHU_GEB_API ULockOnComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	ULockOnComponent();

	/** 락온 반경 (캐릭터 중심 구) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn")
	float LockOnRadius;

	/** 타겟까지 최대 거리 (반경보다 살짝 크게 둬도 됨) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn")
	float MaxLockOnDistance;

	/** 라인 트레이스로 시야 체크할 때 사용할 채널 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn")
	TEnumAsByte<ECollisionChannel> VisibilityChannel;

	/** 화면 중심에서의 최대 허용 거리 (픽셀) – 너무 옆에 있는 건 무시 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn")
	float MaxScreenDistanceFromCenter;

	/** 회전 보간 속도 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn")
	float RotationInterpSpeed;

	/** 타겟이 바뀌었을 때 (UI용) */
	UPROPERTY(BlueprintAssignable, Category = "LockOn")
	FOnLockOnTargetChanged OnLockOnTargetChanged;

protected:
	/** 현재 타겟 */
	TWeakObjectPtr<AActor> CurrentTarget;

	/** 캐시된 PlayerController */
	TWeakObjectPtr<APlayerController> CachedPC;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	/** 타겟 후보들을 검색해서 최적의 타겟을 찾음 */
	AActor* FindBestTarget();

	/** 후보 수집 (캐릭터 중심 구) */
	void CollectCandidates(TArray<AActor*>& OutCandidates) const;

	/** 타겟 유효성 체크 (사망/거리/시야) */
	bool IsTargetValid(AActor* Target) const;

	/** 화면 중심과의 거리 계산 (픽셀 단위) */
	bool CalcScreenDistanceToCenter(AActor* Target, float& OutDistance) const;

	/** 컨트롤러 회전 보간 */
	void UpdateControlRotation(float DeltaTime);

	/** 캐릭터 회전 모드 변경 (락온 on/off) */
	void ApplyCharacterRotationMode(bool bEnableLockOn);

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** 락온 토글 (있으면 해제, 없으면 찾기) */
	UFUNCTION(BlueprintCallable, Category = "LockOn")
	void ToggleLockOn();

	/** 강제 해제 */
	UFUNCTION(BlueprintCallable, Category = "LockOn")
	void ClearLockOn();

	/** 현재 락온 중인지 */
	UFUNCTION(BlueprintCallable, Category = "LockOn")
	bool IsLockedOn() const { return CurrentTarget.IsValid(); }

	/** 현재 타겟 가져오기 */
	UFUNCTION(BlueprintCallable, Category = "LockOn")
	AActor* GetCurrentTarget() const { return CurrentTarget.Get(); }

	/** 화면 기준 좌/우 타겟 전환 (나중에 구현해도 됨) */
	UFUNCTION(BlueprintCallable, Category = "LockOn")
	void SwitchTarget(bool bRight);
		
};
