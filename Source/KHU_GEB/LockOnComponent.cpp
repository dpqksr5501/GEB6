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

ULockOnComponent::ULockOnComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	LockOnRadius = 2000.f;
	MaxLockOnDistance = 2500.f;
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

	if (!CurrentTarget.IsValid())
	{
		return;
	}

	AActor* Target = CurrentTarget.Get();
	if (!IsTargetValid(Target))
	{
		ClearLockOn();
		return;
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
	if (DistSq > MaxLockOnDistance * MaxLockOnDistance)
	{
		return false;
	}

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

	if (!CachedPC.IsValid() || Candidates.Num() == 0)
	{
		return nullptr;
	}

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

	if (UCharacterMovementComponent* MoveComp = OwnerChar->GetCharacterMovement())
	{
		if (bEnableLockOn)
		{
			// 이동 방향 대신 컨트롤러(Yaw) 기준으로 회전
			MoveComp->bOrientRotationToMovement = false;
			OwnerChar->bUseControllerRotationYaw = true;
		}
		else
		{
			// 원래 세팅으로 복귀 (프로젝트 기본값에 맞게 조정)
			MoveComp->bOrientRotationToMovement = true;
			OwnerChar->bUseControllerRotationYaw = false;
		}
	}
}
