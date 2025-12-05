// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FormDefinition.h"
#include "Engine/EngineTypes.h"
#include "JumpComponent.generated.h"

class ACharacter;
class USceneComponent;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class KHU_GEB_API UJumpComponent : public UActorComponent
{
	GENERATED_BODY()

protected:
	UPROPERTY(Transient)
	ACharacter* CachedCharacter = nullptr;

	UPROPERTY(Transient)
	EFormType CurrentForm = EFormType::Base;

	UPROPERTY(Transient)
	const UFormDefinition* CurrentFormDef = nullptr;

	// === 공통 ===

	bool bIsJumping = false;
	int32 JumpCount = 0;

	/** 캐릭터의 기본 중력 값 */
	float DefaultGravityScale = 1.0f;

	// === Range ===

	/** Range 폼에서 상승 높이 배수 (기본 점프 대비) */
	UPROPERTY(EditAnywhere, Category = "Jump|Range")
	float RangeHighJumpMultiplier = 0.38f;

	/** Range 글라이드(서서히 하강) 중 중력 스케일 */
	UPROPERTY(EditAnywhere, Category = "Jump|Range")
	float RangeGlideGravityScale = 0.1f;

	/** Range 급강하(착치) 중 중력 스케일 */
	UPROPERTY(EditAnywhere, Category = "Jump|Range")
	float RangeFastFallGravityScale = 3.0f;

	/** 락온 상태에서 상승 시 타겟과 벌릴 목표 거리(수평) – 기본 1200 */
	UPROPERTY(EditAnywhere, Category = "Jump|Range")
	float RangeLockOnDistance = 1200.f;

	/** 락온 상태에서 급강하 시 타겟 근처까지 좁힐 거리(수평) – 기본 150 */
	UPROPERTY(EditAnywhere, Category = "Jump|Range")
	float RangeLockOnCloseDistance = 300.f;

	/** 락온 상태에서 수평 이동에 사용할 속도 */
	UPROPERTY(EditAnywhere, Category = "Jump|Range")
	float RangeLockOnHorizontalSpeed = 4000.f;

	/** 락온 상태에서 수평 이동에 사용할 속도 배수 */
	UPROPERTY(EditAnywhere, Category = "Jump|Range")
	float RangeLockOnSpeedMultiplier = 0.25f;

	/** Range 락온 거리(1200)로 보정 중인지 여부 */
	bool bRangeLockOnAdjusting = false;

	/** Range 급강하 후 착지 스턴 여부 플래그 */
	bool bRangeStompPending = false;

	/** Range 착지 스턴 반경 */
	UPROPERTY(EditAnywhere, Category = "Jump|Range|Stomp")
	float RangeStompRadius = 400.f;

	/** Range 착지 스턴 지속 시간(초) */
	UPROPERTY(EditAnywhere, Category = "Jump|Range|Stomp")
	float RangeStompStunDuration = 2.0f;

	/** Range 착지 스턴 디버그 표시 여부 */
	UPROPERTY(EditAnywhere, Category = "Jump|Range|Stomp|Debug")
	bool bDrawRangeStompDebug = false;

	/** Range 착지 스턴 디버그 색상 */
	UPROPERTY(EditAnywhere, Category = "Jump|Range|Stomp|Debug")
	FColor RangeStompDebugColor = FColor::Red;

	// 스턴 전에 MaxWalkSpeed 저장해 둘 맵
	TMap<TWeakObjectPtr<ACharacter>, float> RangeStunOriginalSpeeds;

	// === Swift ===

	/** 회전의 축이 되는 루트 (캐릭터 중심에 있는 MeshRoot) */
	UPROPERTY(Transient)
	USceneComponent* SwiftSpinRoot = nullptr;

	/** 두 번째 점프 동안 한 바퀴 도는 데 걸리는 시간(초) */
	UPROPERTY(EditAnywhere, Category = "Jump|Swift")
	float SwiftSpinDuration = 0.3f;

	/** 현재 Swift 2단 점프 회전 중인지 여부 */
	bool bSwiftSpinning = false;

	/** 회전이 시작된 뒤 경과 시간(초) */
	float SwiftSpinElapsed = 0.f;

	/** Swift 회전 시작 시 MeshRoot의 로컬 회전값 */
	FRotator SwiftStartRootRotation;

	/** 회전 시작 전에 Movement의 bOrientRotationToMovement 값 기억 */
	bool bSwiftSavedOrientRotationToMovement = false;

	// === Guard ===

	/** Guard 끌어당기기 반경 */
	UPROPERTY(EditAnywhere, Category = "Jump|Guard")
	float GuardPullRadius = 1500.f;

	/** 플레이어로부터 이 거리까지 가까워지면 멈춤 */
	UPROPERTY(EditAnywhere, Category = "Jump|Guard")
	float GuardEndDistance = 100.f;

	/** 끌려오는 동안 플레이어 위치보다 얼마나 띄울지 (Z 오프셋) */
	UPROPERTY(EditAnywhere, Category = "Jump|Guard")
	float GuardLiftHeight = 0.f;

	/** 끌려오는 동안의 보간 속도 (InterpTo 속도) */
	UPROPERTY(EditAnywhere, Category = "Jump|Guard")
	float GuardPullInterpSpeed = 1.f;

	/** 끌어당기기 지속 시간(초) */
	UPROPERTY(EditAnywhere, Category = "Jump|Guard")
	float GuardPullDuration = 3.f;

	/** Guard 사용 쿨타임(초) */
	UPROPERTY(EditAnywhere, Category = "Jump|Guard")
	float GuardCooldownTime = 10.f;

	/** 현재 Guard를 사용할 수 있는지 */
	bool bCanGuardPull = true;

	/** 현재 끌어당기는 중인지 */
	bool bGuardPullActive = false;

	/** 끌어당기기 경과 시간 */
	float GuardPullElapsed = 0.f;

	/** 끌어당기는 타겟들 */
	TArray<TWeakObjectPtr<ACharacter>> GuardPullTargets;

	/** Guard 쿨타임 타이머 */
	FTimerHandle GuardCooldownTimerHandle;

	// === Special ===

	/** Special 블링크 거리 */
	UPROPERTY(EditAnywhere, Category = "Jump|Special")
	float SpecialBlinkDistance = 600.f;

	/** 블링크 시 바닥과의 여유(살짝 띄우고 싶을 때) */
	UPROPERTY(EditAnywhere, Category = "Jump|Special")
	float SpecialBlinkHeightOffset = 0.f;

	/** Special 블링크 쿨타임 (초) */
	UPROPERTY(EditAnywhere, Category = "Jump|Special")
	float SpecialBlinkCooldown = 5.0f;

	/** 지금 블링크를 사용할 수 있는지 여부 */ 
	bool bCanSpecialBlink = true;

	/** 쿨타임용 타이머 핸들 */
	FTimerHandle SpecialBlinkCooldownHandle;

public:
	UJumpComponent();

	// Enemy가 사용하기 위해 Public으로 노출
	bool IsOnGround() const;

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// 폼별 처리
	void HandleBasePressed();
	void HandleBaseReleased();
	void HandleRangePressed();
	void HandleRangeReleased();
	void HandleSwiftPressed();
	void HandleSwiftReleased();
	void HandleGuardPressed();
	void HandleGuardReleased();
	void HandleSpecialPressed();
	void HandleSpecialReleased();

	AActor* GetCurrentTargetForOwner() const;
	/** 
	Range 급강하 착지 시 주변 적 스턴 처리 */
	void ApplyRangeLandingStun();

	/** Range 착지 스턴 해제 콜백 */
	UFUNCTION()	void EndRangeLandingStun(ACharacter* Target);

	// Swift 회전 종료 처리(필요시 각도 복원)
	void StopSwiftSpin(bool bResetRotation);

	void ResetGuardCooldown();

	// 쿨타임 종료 콜백
	void ResetSpecialBlinkCooldown();

public:
	// Spacebar 입력 핸들러
	void HandleSpacePressed();
	void HandleSpaceReleased();

	// 폼 변경 시 호출
	UFUNCTION() void SetForm(EFormType Form, const UFormDefinition* Def);

protected:
	// 착지 시점에 호출 (점프 카운트/회전 상태 초기화)
	UFUNCTION() void OnCharacterLanded(const FHitResult& Hit);

};