// Fill out your copyright notice in the Description page of Project Settings.


#include "JumpComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"
#include "KHU_GEBCharacter.h"
#include "Enemy_AI/EnemyAnimIntance.h"
#include "Enemy_AI/Enemy_Dragon.h"

UJumpComponent::UJumpComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UJumpComponent::BeginPlay()
{
	Super::BeginPlay();

	CachedCharacter = Cast<ACharacter>(GetOwner());

	// 기본적으로 Tick은 끄고, 회전이 필요할 때만 켭니다.
	SetComponentTickEnabled(false);

	if (CachedCharacter)
	{
		// 착지 델리게이트
		CachedCharacter->LandedDelegate.AddDynamic(this, &UJumpComponent::OnCharacterLanded);

		// 기본 중력 값 캐시
		if (UCharacterMovementComponent* MoveComp = CachedCharacter->GetCharacterMovement())
		{
			DefaultGravityScale = MoveComp->GravityScale;
		}

		// MeshRoot 찾기
		if (AKHU_GEBCharacter* Player = Cast<AKHU_GEBCharacter>(CachedCharacter))
		{
			SwiftSpinRoot = Player->GetMeshRoot();
		}
		else if (AEnemy_Base* Enemy = Cast<AEnemy_Base>(CachedCharacter))
		{
			SwiftSpinRoot = Enemy->GetMeshRoot();
		}

		// 혹시 MeshRoot를 못 찾았으면 마지막 fallback으로 Mesh 사용
		if (!SwiftSpinRoot)
		{
			SwiftSpinRoot = CachedCharacter->GetMesh();
		}
	}
}

void UJumpComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// === Range 락온 거리(1200) 보정 ===
	if (CurrentForm == EFormType::Range && bRangeLockOnAdjusting)
	{
		AKHU_GEBCharacter* Player = Cast<AKHU_GEBCharacter>(CachedCharacter);
		if (!Player)
		{
			bRangeLockOnAdjusting = false;
		}
		else if (AActor* Target = Player->GetLockOnTarget())
		{
			UCharacterMovementComponent* MoveComp = CachedCharacter->GetCharacterMovement();
			if (!MoveComp)
			{
				bRangeLockOnAdjusting = false;
			}
			else
			{
				const FVector OwnerLoc = CachedCharacter->GetActorLocation();
				const FVector TargetLoc = Target->GetActorLocation();

				FVector FlatOwner(OwnerLoc.X, OwnerLoc.Y, 0.f);
				const FVector FlatTarget(TargetLoc.X, TargetLoc.Y, 0.f);

				FVector ToOwner = FlatOwner - FlatTarget; // Target → Owner
				float CurrentDist = ToOwner.Size();

				const float DesiredDist = RangeLockOnDistance;
				const float Tolerance = 5.f; // 1200 ± 5 안이면 도착으로 간주

				if (CurrentDist < KINDA_SMALL_NUMBER)
				{
					// 거의 같은 위치면 그냥 끝
					bRangeLockOnAdjusting = false;
				}
				else
				{
					const float Diff = CurrentDist - DesiredDist;

					// 이미 거의 1200이면 딱 맞춰놓고 XY 속도 정지
					if (FMath::Abs(Diff) <= Tolerance)
					{
						FVector Dir = ToOwner / CurrentDist; // 바깥 방향
						FVector NewFlatOwner = FlatTarget + Dir * DesiredDist;
						FVector NewLoc(NewFlatOwner.X, NewFlatOwner.Y, OwnerLoc.Z);

						CachedCharacter->SetActorLocation(NewLoc, true);

						// XY 속도 정지 (Z는 그대로 유지)
						FVector Vel = MoveComp->Velocity;
						Vel.X = 0.f;
						Vel.Y = 0.f;
						MoveComp->Velocity = Vel;

						bRangeLockOnAdjusting = false;
					}
					else
					{
						// 한 프레임에 이동할 거리 (넘치지 않게 클램프)
						float Step = RangeLockOnHorizontalSpeed * RangeLockOnSpeedMultiplier * DeltaTime;
						Step = FMath::Min(Step, FMath::Abs(Diff));

						// 가까운 경우: 바깥으로 (Target → Owner)
						// 먼   경우: 안으로  (Owner → Target)
						FVector Dir;
						if (Diff < 0.f) // 현재 < 원하는 거리 → 바깥으로 밀어냄
						{
							Dir = ToOwner / CurrentDist; // Target → Owner
						}
						else // 현재 > 원하는 거리 → 안으로 당김
						{
							Dir = (FlatTarget - FlatOwner).GetSafeNormal(); // Owner → Target
						}

						FVector NewFlatOwner = FlatOwner + Dir * Step;
						FVector NewLoc(NewFlatOwner.X, NewFlatOwner.Y, OwnerLoc.Z);

						CachedCharacter->SetActorLocation(NewLoc, true);
					}
				}
			}
		}
		else
		{
			// 락온이 풀렸으면 보정도 중단
			bRangeLockOnAdjusting = false;
		}

		// Range 보정이 끝났고, Swift/Guard도 안 돌면 Tick 끄기
		if (!bRangeLockOnAdjusting && !bSwiftSpinning && !bGuardPullActive)
		{
			SetComponentTickEnabled(false);
		}
	}

	// Swift 2단 점프 회전 처리 (Actor 중심 회전)
	// Swift 2단 점프 회전 처리 (MeshRoot 회전)
	if (bSwiftSpinning && SwiftSpinRoot)
	{
		SwiftSpinElapsed += DeltaTime;

		const float Duration = FMath::Max(SwiftSpinDuration, KINDA_SMALL_NUMBER);
		const float Alpha = FMath::Clamp(SwiftSpinElapsed / Duration, 0.f, 1.f);

		const float Angle = 360.f * Alpha; // 한 바퀴

		FRotator NewRot = SwiftStartRootRotation;
		NewRot.Pitch -= Angle;

		SwiftSpinRoot->SetRelativeRotation(NewRot);

		if (SwiftSpinElapsed >= SwiftSpinDuration) { StopSwiftSpin(/*bResetRotation=*/true); }
	}

	// Guard 지속형 끌어당김
	if (bGuardPullActive)
	{
		UE_LOG(LogTemp, Log, TEXT("Guard Pull Active Tick"));
		UWorld* World = GetWorld();
		if (!World)
		{
			bGuardPullActive = false;
			GuardPullTargets.Reset();
			return;
		}

		GuardPullElapsed += DeltaTime;

		const FVector PlayerLoc = CachedCharacter->GetActorLocation();

		for (int32 i = GuardPullTargets.Num() - 1; i >= 0; --i)
		{
			ACharacter* Target = GuardPullTargets[i].Get();
			if (!Target)
			{
				GuardPullTargets.RemoveAt(i);
				continue;
			}

			FVector TargetLoc = Target->GetActorLocation();

			// 플레이어 기준 방향/거리
			FVector ToPlayer = PlayerLoc - TargetLoc;
			const float Distance = ToPlayer.Size();

			if (Distance <= KINDA_SMALL_NUMBER) continue;

			FVector Dir = ToPlayer / Distance;

			// 플레이어 근처, 약간 띄운 위치를 목표로
			FVector Anchor = PlayerLoc;
			Anchor.Z += GuardLiftHeight;

			const FVector DesiredLoc = Anchor - Dir * GuardEndDistance;

			// 부드럽게 끌려오도록 보간
			const FVector NewLoc = FMath::VInterpTo(
				TargetLoc,
				DesiredLoc,
				DeltaTime,
				GuardPullInterpSpeed
			);

			// 충돌 고려해서 위치 이동
			Target->SetActorLocation(NewLoc, true);
		}

		// 지속 시간 끝나면 끌어당김 종료
		if (GuardPullElapsed >= GuardPullDuration || GuardPullTargets.Num() == 0)
		{
			bGuardPullActive = false;
			GuardPullTargets.Reset();

			// 만약 Swift 회전도 안 돌고 있다면, 그때만 Tick 끄기
			if (!bSwiftSpinning)
			{
				SetComponentTickEnabled(false);
			}
		}
	}
}

void UJumpComponent::SetForm(EFormType Form, const UFormDefinition* Def)
{
	CurrentForm = Form;
	CurrentFormDef = Def;

	bIsJumping = false;
	JumpCount = 0;

	// Range 거리 보정 중이었다면 취소
	bRangeLockOnAdjusting = false;

	//Guard 끌어당기기 쓰다가 다른 폼으로 변했을 시 취소하는 로직
	if (bGuardPullActive)
	{
		bGuardPullActive = false;
		GuardPullTargets.Reset();
		SetComponentTickEnabled(false);
	}

	// === Swift ===
	if (bSwiftSpinning) { StopSwiftSpin(/*bResetRotation=*/true); }

	if (CachedCharacter)
	{
		// 폼이 바뀔 때마다 중력은 항상 기본값으로 돌려놓기
		if (UCharacterMovementComponent* MoveComp = CachedCharacter->GetCharacterMovement())
		{
			MoveComp->GravityScale = DefaultGravityScale;
		}

		// 활강 변수 끄기
		if (AKHU_GEBCharacter* MyChar = Cast<AKHU_GEBCharacter>(CachedCharacter))
		{
			MyChar->bIsRangeGliding = false;
		}

		switch (CurrentForm)
		{
		case EFormType::Swift:
			// Swift: 더블 점프
			CachedCharacter->JumpMaxCount = 2;
			break;

		default:
			// 나머지 폼: 기본 1회 점프
			CachedCharacter->JumpMaxCount = 1;
			break;
		}
	}
}

bool UJumpComponent::IsOnGround() const
{
	if (!CachedCharacter) return false;

	if (const UCharacterMovementComponent* MoveComp = CachedCharacter->GetCharacterMovement())
	{
		return MoveComp->IsMovingOnGround();
	}
	return false;
}

void UJumpComponent::OnCharacterLanded(const FHitResult& Hit)
{
	// 착지하면 점프 상태/카운트/회전 모두 리셋
	bIsJumping = false;
	JumpCount = 0;

	// === 활강 모드 해제 (Player와 Enemy 모두 지원) ===
	
	// 1. Player인 경우
	if (AKHU_GEBCharacter* MyChar = Cast<AKHU_GEBCharacter>(CachedCharacter))
	{
		MyChar->bIsRangeGliding = false;
	}
	// 2. Enemy인 경우
	else if (USkeletalMeshComponent* Mesh = CachedCharacter->GetMesh())
	{
		if (UEnemyAnimIntance* EnemyAnim = Cast<UEnemyAnimIntance>(Mesh->GetAnimInstance()))
		{
			EnemyAnim->bIsFalling = false;
			EnemyAnim->bIsJumping = false;
		}
	}

	// Range에서 조정했던 중력 값 원복
	if (CachedCharacter)
	{
		if (UCharacterMovementComponent* MoveComp = CachedCharacter->GetCharacterMovement())
		{
			MoveComp->GravityScale = DefaultGravityScale;
		}
	}

	// Range 락온 거리 조정 중이면 종료
	bRangeLockOnAdjusting = false;

	// Swift 회전 중이었다면 종료 + 각도 복원
	if (bSwiftSpinning) { StopSwiftSpin(/*bResetRotation=*/true); }

	// Guard / Swift / Range 아무 것도 안 돌고 있으면 Tick 끄기
	if (!bSwiftSpinning && !bGuardPullActive && !bRangeLockOnAdjusting)
	{
		SetComponentTickEnabled(false);
	}
}

void UJumpComponent::HandleSpacePressed()
{
	if (!CachedCharacter) return;
	// 여기에 CurrentForm 로그 출력. 정수 말고 이름으로 가능?
	UE_LOG(LogTemp, Log, TEXT("Current Form: %d"), static_cast<int32>(CurrentForm));
	switch (CurrentForm)
	{
	case EFormType::Base:    HandleBasePressed();    break;
	case EFormType::Range:   HandleRangePressed();   break;
	case EFormType::Swift:   HandleSwiftPressed();   break;
	case EFormType::Guard:   HandleGuardPressed();   break;
	case EFormType::Special: HandleSpecialPressed(); break;
	default:
		HandleBasePressed();
		break;
	}
}

void UJumpComponent::HandleSpaceReleased()
{
	if (!CachedCharacter) return;

	switch (CurrentForm)
	{
	case EFormType::Base:    HandleBaseReleased();    break;
	case EFormType::Range:   HandleRangeReleased();   break;
	case EFormType::Swift:   HandleSwiftReleased();   break;
	case EFormType::Guard:   HandleGuardReleased();   break;
	case EFormType::Special: HandleSpecialReleased(); break;
	default:
		HandleBaseReleased();
		break;
	}
}

/* =============== Base: 일반 점프 =============== */

void UJumpComponent::HandleBasePressed()
{
	CachedCharacter->Jump();
	bIsJumping = true;
}

void UJumpComponent::HandleBaseReleased()
{
	CachedCharacter->StopJumping();
	bIsJumping = false;
}

/* =============== Range: 높이 점프 후 글라이드(비행) =============== */

void UJumpComponent::HandleRangePressed()
{
	UCharacterMovementComponent* MoveComp = CachedCharacter->GetCharacterMovement();
	if (!MoveComp) return;

	// 1) 땅 위: 높이 비행 + 글라이드
	if (IsOnGround())
	{
		// --- Range 락온 거리 보정 플래그 초기화 ---
		bRangeLockOnAdjusting = false;

		// 락온 타겟이 있을 때만 거리 보정 시작
		if (AKHU_GEBCharacter* Player = Cast<AKHU_GEBCharacter>(CachedCharacter))
		{
			if (Player->GetLockOnTarget())
			{
				bRangeLockOnAdjusting = true;
				SetComponentTickEnabled(true);
			}
		}

		// 기본 점프의 RangeHighJumpMultiplier 배 만큼 위로 발사 (XY는 0)
		FVector LaunchVelocity = FVector::ZeroVector;
		const float JumpStrength = MoveComp->JumpZVelocity * RangeHighJumpMultiplier;
		LaunchVelocity.Z = JumpStrength;

		// XY는 유지(=0에서 시작), Z만 덮어씀
		CachedCharacter->LaunchCharacter(LaunchVelocity, false, true);

		// 글라이딩 느낌: 중력을 줄여서 천천히 떨어지게
		MoveComp->GravityScale = RangeGlideGravityScale;

		bIsJumping = true;
		JumpCount = 1;

		// === ABP 플래그 설정 (Player와 Enemy 모두 지원) ===
		if (AKHU_GEBCharacter* MyChar = Cast<AKHU_GEBCharacter>(CachedCharacter))
		{
			MyChar->bIsRangeGliding = true;
		}
		else if (AEnemy_Dragon* Enemy = Cast<AEnemy_Dragon>(CachedCharacter))
		{
			if (USkeletalMeshComponent* Mesh = Enemy->GetMesh())
			{
				if (UEnemyAnimIntance* EnemyAnim = Cast<UEnemyAnimIntance>(Mesh->GetAnimInstance()))
				{
					EnemyAnim->bIsJumping = true;
					EnemyAnim->bIsFalling = true;
				}
			}
		}
	}
	// 2) 공중: 급강하(착치) – 기존 "타겟 근처까지" 로직 그대로 유지
	else
	{
		FVector Velocity = MoveComp->Velocity;
		const float FallStrength = FMath::Max(
			MoveComp->JumpZVelocity * RangeHighJumpMultiplier,
			600.f
		);

		Velocity.Z = -FMath::Abs(FallStrength);

		if (AKHU_GEBCharacter* Player = Cast<AKHU_GEBCharacter>(CachedCharacter))
		{
			if (AActor* Target = Player->GetLockOnTarget())
			{
				const FVector OwnerLoc = CachedCharacter->GetActorLocation();
				const FVector TargetLoc = Target->GetActorLocation();

				const FVector FlatOwner(OwnerLoc.X, OwnerLoc.Y, 0.f);
				const FVector FlatTarget(TargetLoc.X, TargetLoc.Y, 0.f);

				FVector ToTarget = FlatTarget - FlatOwner; // Player → Target
				const float CurrentDist = ToTarget.Size();

				if (CurrentDist > KINDA_SMALL_NUMBER)
				{
					ToTarget /= CurrentDist;

					const float DesiredDist = RangeLockOnCloseDistance;

					if (CurrentDist > DesiredDist + 10.f)
					{
						Velocity.X = ToTarget.X * RangeLockOnHorizontalSpeed;
						Velocity.Y = ToTarget.Y * RangeLockOnHorizontalSpeed;
					}
				}
			}
		}

		MoveComp->Velocity = Velocity;
		MoveComp->GravityScale = RangeFastFallGravityScale;
	}
}

void UJumpComponent::HandleRangeReleased()
{

}

/* =============== Swift: 더블 점프 =============== */

void UJumpComponent::HandleSwiftPressed()
{
	if (!CachedCharacter) return;

	// 1. 땅 위: 첫 번째 점프
	if (IsOnGround())
	{
		// JumpMaxCount 출력
		UE_LOG(LogTemp, Log, TEXT("JumpMaxCount: %d"), CachedCharacter->JumpMaxCount);
		JumpCount = 1;
		CachedCharacter->Jump();
		bIsJumping = true;
		return;
	}

	// 2. 공중: 두 번째 점프
	if (JumpCount < 2)
	{
		JumpCount = 2;
		CachedCharacter->Jump();
		bIsJumping = true;

		// 회전 시작 세팅
		bSwiftSpinning = true;
		SwiftSpinElapsed = 0.f;

		if (SwiftSpinRoot)
		{
			SwiftStartRootRotation = SwiftSpinRoot->GetRelativeRotation();
		}

		if (UCharacterMovementComponent* MoveComp = CachedCharacter->GetCharacterMovement())
		{
			bSwiftSavedOrientRotationToMovement = MoveComp->bOrientRotationToMovement;
			MoveComp->bOrientRotationToMovement = false; // 회전 중엔 우리가 직접 회전 제어
		}

		SetComponentTickEnabled(true);
	}
}

void UJumpComponent::HandleSwiftReleased()
{
	CachedCharacter->StopJumping();
	bIsJumping = false;
}

/* =============== Guard: 앞으로 돌진 + 주변 몬스터 끌어당김 =============== */

void UJumpComponent::HandleGuardPressed()
{	
	// 쿨타임 중이거나 이미 끌어당기는 중이면 무시
	if (!bCanGuardPull || bGuardPullActive) return;
	UE_LOG(LogTemp, Log, TEXT("[JumpComponent] HandleGuardPressed: Attempting to start Guard Pull"));
	UWorld* World = GetWorld();
	if (!World) return;

	const FVector Center = CachedCharacter->GetActorLocation();

	TArray<FOverlapResult> Overlaps;

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(GuardPull), false, CachedCharacter);
	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);

	const bool bHitAny = World->OverlapMultiByObjectType(
		Overlaps,
		Center,
		FQuat::Identity,
		ObjectParams,
		FCollisionShape::MakeSphere(GuardPullRadius),
		QueryParams
	);

	GuardPullTargets.Reset();

	if (bHitAny)
	{
		for (const FOverlapResult& Result : Overlaps)
		{
			AActor* OtherActor = Result.GetActor();
			ACharacter* OtherChar = Cast<ACharacter>(OtherActor);
			if (!OtherChar) continue;

			// 자기 자신 제외
			if (OtherChar == CachedCharacter) continue;

			// 여기서 "적" 필터링 필요하면 태그/팀으로 필터
			// if (!OtherChar->ActorHasTag("Enemy")) continue;

			GuardPullTargets.Add(OtherChar);
		}
	}

	if (GuardPullTargets.Num() == 0)
	{
		// 끌어당길 대상이 없으면 쿨타임도 돌리지 않음
		return;
	}

	// === ABP 플래그 설정 (Player와 Enemy 모두 지원) ===
	
	// 1. Player인 경우 (KHU_GEBCharacter) - 기존 로직 유지
	if (AKHU_GEBCharacter* MyPlayer = Cast<AKHU_GEBCharacter>(CachedCharacter))
	{
		MyPlayer->bSpaceActionInput = true;
		FTimerHandle ResetTimerHandle;
		MyPlayer->GetWorldTimerManager().SetTimer(
			ResetTimerHandle,
			MyPlayer,
			&AKHU_GEBCharacter::AutoResetSpaceAction,
			0.15f, // 0.15초 딜레이
			false
		);

		MyPlayer->bIsMovementInputBlocked = true;

		//2.1초 뒤에 이동 잠금 해제 예약
		FTimerHandle MoveLockTimerHandle;
		MyPlayer->GetWorldTimerManager().SetTimer(
			MoveLockTimerHandle,
			MyPlayer,
			&AKHU_GEBCharacter::ReleaseMovementLock,
			2.1f,
			false
		);
		
		UE_LOG(LogTemp, Log, TEXT("[JumpComponent] HandleGuardPressed: Player ABP flags set"));
	}
	// 2. Enemy인 경우 (EnemyAnimInstance 사용)
	else if (USkeletalMeshComponent* Mesh = CachedCharacter->GetMesh())
	{	
		if (UEnemyAnimIntance* EnemyAnim = Cast<UEnemyAnimIntance>(Mesh->GetAnimInstance()))
		{
			EnemyAnim->bSpaceActionInput = true;

			// 0.15초 후 리셋
			FTimerHandle ResetTimerHandle;
			World->GetTimerManager().SetTimer(
				ResetTimerHandle,
				EnemyAnim,
				&UEnemyAnimIntance::ResetSpaceActionInput,
				0.15f,
				false
			);
		}
	}

	// 끌어당기기 시작
	bGuardPullActive = true;
	GuardPullElapsed = 0.f;

	SetComponentTickEnabled(true);

	// Special처럼 쿨타임 진입
	bCanGuardPull = false;
	World->GetTimerManager().SetTimer(
		GuardCooldownTimerHandle,
		this,
		&UJumpComponent::ResetGuardCooldown,
		GuardCooldownTime,
		false
	);
	
	UE_LOG(LogTemp, Log, TEXT("[JumpComponent] HandleGuardPressed: Guard pull started with %d targets"), 
		GuardPullTargets.Num());
}

void UJumpComponent::HandleGuardReleased()
{

}

/* =============== Special: 뒤로 블링크 =============== */

void UJumpComponent::HandleSpecialPressed()
{
	UE_LOG(LogTemp, Log, TEXT("[JumpComponent] HandleSpecialPressed: Attempting to perform Special Blink"));
	UWorld* World = GetWorld();
	if (!World) return;

	// 혹시 Swift 회전 중이면 정리
	if (bSwiftSpinning) { StopSwiftSpin(/*bResetRotation=*/true); }

	// 쿨타임 중이면 바로 리턴
	if (!bCanSpecialBlink) return;

	const FVector StartLocation = CachedCharacter->GetActorLocation();
	const FRotator CurrentRot = CachedCharacter->GetActorRotation();

	// 앞으로 방향 (Yaw 기준)
	FVector Forward = CurrentRot.Vector();
	Forward.Z = 0.f;
	Forward = Forward.GetSafeNormal();
	if (Forward.IsNearlyZero())
	{
		Forward = CachedCharacter->GetActorForwardVector();
		Forward.Z = 0.f;
		Forward.Normalize();
	}

	const float BlinkDistance = SpecialBlinkDistance;
	FVector DesiredEnd = StartLocation + Forward * BlinkDistance;

	// 캡슐 정보
	UCapsuleComponent* Capsule = CachedCharacter->GetCapsuleComponent();
	float CapsuleRadius = 34.f;
	float CapsuleHalfHeight = 88.f;
	if (Capsule) { Capsule->GetScaledCapsuleSize(CapsuleRadius, CapsuleHalfHeight); }

	// 장애물 스윕
	FHitResult HitResult;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(SpecialBlink), false, CachedCharacter);

	const bool bHit = World->SweepSingleByChannel(
		HitResult,
		StartLocation,
		DesiredEnd,
		FQuat::Identity,
		ECC_Pawn,
		FCollisionShape::MakeCapsule(CapsuleRadius, CapsuleHalfHeight),
		Params
	);

	FVector TargetLocation = DesiredEnd;

	if (bHit)
	{
		const FVector ImpactPoint = HitResult.ImpactPoint;
		const FVector BackOff = Forward * CapsuleRadius;

		TargetLocation = ImpactPoint - BackOff;
		TargetLocation.Z = StartLocation.Z + SpecialBlinkHeightOffset;
	}
	else { TargetLocation.Z = StartLocation.Z + SpecialBlinkHeightOffset; }

	const bool bTeleported = CachedCharacter->TeleportTo(
		TargetLocation,
		CurrentRot,
		false,
		true
	);

	if (bTeleported)
	{
		// 블링크 성공 → 쿨타임 진입
		bCanSpecialBlink = false;

		World->GetTimerManager().SetTimer(
			SpecialBlinkCooldownHandle,
			this,
			&UJumpComponent::ResetSpecialBlinkCooldown,
			SpecialBlinkCooldown,
			false  // 반복 아님
		);
	}

	bIsJumping = false;
	JumpCount = 0;
}

void UJumpComponent::HandleSpecialReleased()
{

}

/* =============== 추가 =============== */

void UJumpComponent::StopSwiftSpin(bool bResetRotation)
{
	if (CachedCharacter)
	{
		if (UCharacterMovementComponent* MoveComp = CachedCharacter->GetCharacterMovement())
		{
			MoveComp->bOrientRotationToMovement = bSwiftSavedOrientRotationToMovement;
		}
	}

	if (bResetRotation && SwiftSpinRoot)
	{
		SwiftSpinRoot->SetRelativeRotation(SwiftStartRootRotation);
	}

	bSwiftSpinning = false;
	SwiftSpinElapsed = 0.f;
	
	if (!bGuardPullActive && !bRangeLockOnAdjusting)
	{
		SetComponentTickEnabled(false);
	}
}

void UJumpComponent::ResetGuardCooldown()
{
	bCanGuardPull = true;
}

void UJumpComponent::ResetSpecialBlinkCooldown()
{
	bCanSpecialBlink = true;
}