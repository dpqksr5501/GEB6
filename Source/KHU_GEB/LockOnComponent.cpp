// Fill out your copyright notice in the Description page of Project Settings.


#include "LockOnComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/OverlapResult.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "HealthComponent.h"
#include "KHU_GEBCharacter.h"
#include "GameFramework/SpringArmComponent.h"

ULockOnComponent::ULockOnComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	LockOnRadius = 1500.f;
	MaxLockOnDistance = 2000.f;
	VisibilityChannel = ECC_Visibility;
	MaxScreenDistanceFromCenter = 600.f; // 화면 중앙에서 대략 이 정도까지만 허용
	RotationInterpSpeed = 10.f;
}

void ULockOnComponent::BeginPlay()
{
	Super::BeginPlay();

	if (APawn* PawnOwner = Cast<APawn>(GetOwner()))
	{
		CachedPC = Cast<APlayerController>(PawnOwner->GetController());
	}
}

void ULockOnComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!CurrentTarget.IsValid()) return;

	AActor* Target = CurrentTarget.Get();
	if (!IsTargetValid(Target))
	{
		ClearLockOn();
		return;
	}

	// --- Range 조준 중이면 카메라 회전은 Range 스킬에 맡긴다 ---
	if (AActor* Owner = GetOwner())
	{
		if (AKHU_GEBCharacter* PlayerChar = Cast<AKHU_GEBCharacter>(Owner))
		{
			if (PlayerChar->IsRangeAiming())
			{
				// 락온 타겟은 유지하지만, 카메라 회전은 건드리지 않음
				return;
			}
		}
	}

	UpdateControlRotation(DeltaTime);
}

void ULockOnComponent::ToggleLockOn()
{
	if (CurrentTarget.IsValid())
	{
		ClearLockOn();
		return;
	}

	AActor* NewTarget = FindBestTarget();
	if (NewTarget)
	{
		CurrentTarget = NewTarget;
		ApplyCharacterRotationMode(true);
		OnLockOnTargetChanged.Broadcast(NewTarget);
	}
}

void ULockOnComponent::ClearLockOn()
{
	if (CurrentTarget.IsValid())
	{
		CurrentTarget.Reset();
		ApplyCharacterRotationMode(false);
		OnLockOnTargetChanged.Broadcast(nullptr);
	}
}

void ULockOnComponent::SwitchTarget(bool bRight)
{
	// 1차 버전에서는 그냥 다시 FindBestTarget() 해서 재선택하는 정도로만 구현해도 됨
	AActor* NewTarget = FindBestTarget();
	if (NewTarget && NewTarget != CurrentTarget.Get())
	{
		CurrentTarget = NewTarget;
		OnLockOnTargetChanged.Broadcast(NewTarget);
	}
}

void ULockOnComponent::CollectCandidates(TArray<AActor*>& OutCandidates) const
{
	OutCandidates.Reset();

	if (!GetWorld()) return;

	AActor* Owner = GetOwner();
	if (!Owner) return;

	FVector Center = Owner->GetActorLocation();

	TArray<FOverlapResult> Overlaps;

	FCollisionObjectQueryParams ObjQueryParams;
	ObjQueryParams.AddObjectTypesToQuery(ECC_Pawn); // Pawn만 대상

	FCollisionShape Sphere = FCollisionShape::MakeSphere(LockOnRadius);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(LockOnOverlap), false, Owner);

	bool bHit = GetWorld()->OverlapMultiByObjectType(
		Overlaps,
		Center,
		FQuat::Identity,
		ObjQueryParams,
		Sphere,
		QueryParams
	);

	if (!bHit) return;

	for (const FOverlapResult& Result : Overlaps)
	{
		AActor* OverlappedActor = Result.GetActor();
		if (!OverlappedActor || OverlappedActor == Owner) continue;

		// 체력 컴포넌트가 있고, 살아있는 대상만
		if (UHealthComponent* HealthComp = OverlappedActor->FindComponentByClass<UHealthComponent>())
		{
			if (!HealthComp->IsDead())   // 함수 이름은 프로젝트에 맞게 수정
			{
				OutCandidates.Add(OverlappedActor);
			}
		}
	}
}

bool ULockOnComponent::CalcScreenDistanceToCenter(AActor* Target, float& OutDistance) const
{
	OutDistance = FLT_MAX;

	if (!CachedPC.IsValid() || !Target) return false;

	FVector2D ViewportSize(0.f, 0.f);
	if (!GEngine || !GEngine->GameViewport) return false;
	GEngine->GameViewport->GetViewportSize(ViewportSize);

	if (ViewportSize.X <= 0.f || ViewportSize.Y <= 0.f) return false;

	FVector2D ScreenPos;
	bool bProjected = CachedPC->ProjectWorldLocationToScreen(Target->GetActorLocation(), ScreenPos);
	if (!bProjected) return false;

	const FVector2D ScreenCenter = ViewportSize * 0.5f;
	OutDistance = FVector2D::Distance(ScreenPos, ScreenCenter);

	// 화면 안에 있는지 대략적인 체크 (조금 여유있게)
	if (ScreenPos.X < -100.f || ScreenPos.X > ViewportSize.X + 100.f ||
		ScreenPos.Y < -100.f || ScreenPos.Y > ViewportSize.Y + 100.f)
	{
		return false;
	}

	return true;
}

bool ULockOnComponent::IsTargetValid(AActor* Target) const
{
	if (!Target || !GetOwner()) return false;

	// HealthComponent로 사망 체크
	if (UHealthComponent* HealthComp = Target->FindComponentByClass<UHealthComponent>())
	{
		if (HealthComp->IsDead())
		{
			return false;
		}
	}

	const float DistSq = FVector::DistSquared(Target->GetActorLocation(), GetOwner()->GetActorLocation());
	if (DistSq > MaxLockOnDistance * MaxLockOnDistance) return false;

	// 카메라 기준 시야각/가림 체크
	if (!CachedPC.IsValid()) return true; // 일단 컨트롤러 없으면 대충 통과

	FVector CamLoc;
	FRotator CamRot;
	CachedPC->GetPlayerViewPoint(CamLoc, CamRot);

	const FVector DirToTarget = (Target->GetActorLocation() - CamLoc).GetSafeNormal();
	const float Dot = FVector::DotProduct(CamRot.Vector(), DirToTarget);

	// 0보다 작으면 카메라 뒤쪽 (90도 이상) → 무효
	if (Dot < 0.f) return false;

	// 라인 트레이스로 가림 체크
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(LockOnVisibility), true, GetOwner());
	bool bHit = GetWorld()->LineTraceSingleByChannel(
		Hit,
		CamLoc,
		Target->GetActorLocation(),
		VisibilityChannel,
		Params
	);

	if (bHit && Hit.GetActor() != Target)
	{
		// 다른 것에 가려져 있음
		return false;
	}

	return true;
}

AActor* ULockOnComponent::FindBestTarget()
{
	TArray<AActor*> Candidates;
	CollectCandidates(Candidates);

	if (!CachedPC.IsValid() || Candidates.Num() == 0) return nullptr;

	AActor* BestTarget = nullptr;
	float BestScreenDist = FLT_MAX;

	for (AActor* Candidate : Candidates)
	{
		if (!IsTargetValid(Candidate)) continue;

		float ScreenDist = 0.f;
		if (!CalcScreenDistanceToCenter(Candidate, ScreenDist)) continue;

		if (ScreenDist > MaxScreenDistanceFromCenter) continue;

		if (ScreenDist < BestScreenDist)
		{
			BestScreenDist = ScreenDist;
			BestTarget = Candidate;
		}
	}

	return BestTarget;
}

void ULockOnComponent::UpdateControlRotation(float DeltaTime)
{
	if (!CachedPC.IsValid() || !CurrentTarget.IsValid()) return;

	AActor* Owner = GetOwner();
	AActor* Target = CurrentTarget.Get();
	if (!Owner || !Target) return;

	FVector OwnerLoc = Owner->GetActorLocation();
	FVector TargetLoc = Target->GetActorLocation();

	FVector ToTarget = TargetLoc - OwnerLoc;
	ToTarget.Z = 0.f;

	if (ToTarget.IsNearlyZero()) return;

	// 이 회전에서 우리는 Yaw만 사용할 것
	const FRotator DesiredYawRot = ToTarget.Rotation(); // Pitch=0, Roll=0, Yaw만 의미 있음

	const FRotator CurrentRot = CachedPC->GetControlRotation();

	// Pitch/ Roll은 그대로 두고, Yaw만 타겟 쪽으로 보간
	FRotator TargetRot = CurrentRot;
	TargetRot.Yaw = DesiredYawRot.Yaw;

	const FRotator NewRot = FMath::RInterpTo(CurrentRot, TargetRot, DeltaTime, RotationInterpSpeed);
	CachedPC->SetControlRotation(NewRot);
}

void ULockOnComponent::ApplyCharacterRotationMode(bool bEnableLockOn)
{
	ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
	if (!OwnerChar) return;

	// 플레이어 캐릭터라면 회전 모드 갱신을 캐릭터에게 위임
	if (AKHU_GEBCharacter* PlayerChar = Cast<AKHU_GEBCharacter>(OwnerChar))
	{
		PlayerChar->RefreshRotationMode();


		//카메라 위/오른쪽으로 약간 올려서 시야 확보하는 코드-----------------------------------------
		if (USpringArmComponent* Boom = PlayerChar->GetCameraBoom())
		{
			if (bEnableLockOn)
			{
				// 락온 켜짐: 설정해둔 오프셋(우측 상단) 적용
				Boom->SocketOffset = LockOnSocketOffset;
				//카메라 거리 늘어남
				PlayerChar->SetTargetCameraBoomLength(500.0f);
			}
			else
			{
				// 락온 꺼짐: 오프셋을 (0,0,0)으로 초기화 -> 정중앙 복귀
				Boom->SocketOffset = FVector::ZeroVector;
				//카메라 거리 다시 초기화(원래값으로)
				PlayerChar->SetTargetCameraBoomLength(PlayerChar->GetDefaultCameraBoomLength());
			}
		}
		//-------------------------------------------
	}
	else
	{
		// (AI 몬스터 등) 플레이어가 아닌 경우엔 기존 방식 유지
		if (UCharacterMovementComponent* MoveComp = OwnerChar->GetCharacterMovement())
		{
			if (bEnableLockOn)
			{
				MoveComp->bOrientRotationToMovement = false;
				OwnerChar->bUseControllerRotationYaw = true;
			}
			else
			{
				MoveComp->bOrientRotationToMovement = true;
				OwnerChar->bUseControllerRotationYaw = false;
			}
		}
	}
}
