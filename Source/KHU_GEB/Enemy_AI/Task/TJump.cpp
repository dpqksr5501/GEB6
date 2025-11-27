// Fill out your copyright notice in the Description page of Project Settings.

#include "Enemy_AI/Task/TJump.h"
#include "AIController.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Engine/World.h"
#include "Enemy_AI/Enemy_Base.h"
#include "JumpComponent.h"

UTJump::UTJump()
{
	NodeName = TEXT("Jump");
	bNotifyTick = true;

	JumpStartTime = 0.f;
	bSecondJumpExecuted = false;
	CachedCharacter = nullptr;
	CachedJumpComp = nullptr;
	JumpTimeout = 5.0f;
	bEnableDoubleJump = false;
}

EBTNodeResult::Type UTJump::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	Super::ExecuteTask(OwnerComp, NodeMemory);

	// 1. AI 컨트롤러 획득
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TJump] AIController is missing"));
		return EBTNodeResult::Failed;
	}

	// 2. 캐릭터 획득
	CachedCharacter = Cast<ACharacter>(AIController->GetPawn());
	if (!CachedCharacter)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TJump] Character is missing"));
		return EBTNodeResult::Failed;
	}

	// 3. CharacterMovementComponent 확인
	UCharacterMovementComponent* MovementComp = CachedCharacter->GetCharacterMovement();
	if (!MovementComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TJump] CharacterMovementComponent is missing"));
		return EBTNodeResult::Failed;
	}

	// 4. Enemy_Base로 캐스팅하여 JumpComponent 획득
	if (AEnemy_Base* Enemy = Cast<AEnemy_Base>(CachedCharacter))
	{
		CachedJumpComp = Enemy->JumpComp;
		if (!CachedJumpComp)
		{
			UE_LOG(LogTemp, Error, TEXT("[TJump] JumpComponent is null! Enemy: %s"), 
				*Enemy->GetName());
			return EBTNodeResult::Failed;
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[TJump] Character is not Enemy_Base! Class: %s"), 
			*CachedCharacter->GetClass()->GetName());
		return EBTNodeResult::Failed;
	}

	// 5. 땅 위에서만 실행 가능
	if (!CachedJumpComp->IsOnGround())
	{
		UE_LOG(LogTemp, Warning, TEXT("[TJump] Character is not on ground"));
		return EBTNodeResult::Failed;
	}

	// 6. JumpComponent의 HandleSpacePressed 호출 (폼에 맞는 점프 실행)
	CachedJumpComp->HandleSpacePressed();
	
	JumpStartTime = OwnerComp.GetWorld()->GetTimeSeconds();
	bSecondJumpExecuted = false; // 2단 점프 플래그 초기화
	
	if (bEnableDoubleJump)
	{
		UE_LOG(LogTemp, Log, TEXT("[TJump] First jump initiated (Double Jump Enabled)"));
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("[TJump] Jump initiated via JumpComponent"));
	}
	
	return EBTNodeResult::InProgress;
}

void UTJump::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickTask(OwnerComp, NodeMemory, DeltaSeconds);

	// 유효성 검사
	if (!CachedCharacter || !CachedJumpComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TJump] Cached references invalid in TickTask"));
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	UCharacterMovementComponent* MovementComp = CachedCharacter->GetCharacterMovement();
	if (!MovementComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TJump] MovementComponent is invalid in TickTask"));
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	UWorld* World = OwnerComp.GetWorld();
	if (!World)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	// 타임아웃 체크
	const float CurrentTime = World->GetTimeSeconds();
	const float ElapsedTime = CurrentTime - JumpStartTime;

	if (ElapsedTime >= JumpTimeout)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TJump] Jump timeout (%.2f seconds elapsed)"), ElapsedTime);
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	// 점프 직후 최소 대기 시간 (0.1초) - 너무 빨리 착지 체크하지 않도록
	const float MinAirTime = 0.1f;
	const bool bHasMinAirTime = ElapsedTime >= MinAirTime;

	// === 2단 점프 로직 ===
	// bEnableDoubleJump가 true이고, 아직 2단 점프를 실행하지 않았으며,
	// Z속도가 음수(하강 시작)이면 2단 점프 실행
	if (bEnableDoubleJump && !bSecondJumpExecuted && bHasMinAirTime)
	{
		const float VerticalSpeed = MovementComp->Velocity.Z;
		
		// 최고점 도달 감지: Z속도가 음수로 전환 (하강 시작)
		if (VerticalSpeed < 0.f)
		{
			UE_LOG(LogTemp, Log, TEXT("[TJump] Peak reached (Z Velocity: %.2f), executing second jump"), VerticalSpeed);
			
			// 2단 점프 실행
			CachedJumpComp->HandleSpacePressed();
			bSecondJumpExecuted = true;
			
			UE_LOG(LogTemp, Log, TEXT("[TJump] Second jump executed"));
		}
	}

	// 착지 확인
	if (bHasMinAirTime && CachedJumpComp->IsOnGround())
	{
		// 추가 안전장치: 수직 속도가 거의 0에 가까운지 확인
		const float VerticalSpeed = FMath::Abs(MovementComp->Velocity.Z);
		if (VerticalSpeed < 50.f) // 거의 정지 상태
		{
			UE_LOG(LogTemp, Log, TEXT("[TJump] Landed successfully (%.2f seconds)"), ElapsedTime);
			
			// HandleSpaceReleased 호출 (필요시)
			CachedJumpComp->HandleSpaceReleased();
			
			FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
			return;
		}
	}
}

EBTNodeResult::Type UTJump::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	// HandleSpaceReleased 호출하여 정리
	if (CachedJumpComp)
	{
		CachedJumpComp->HandleSpaceReleased();
		UE_LOG(LogTemp, Log, TEXT("[TJump] HandleSpaceReleased on abort"));
	}

	// 캐시 정리
	CachedCharacter = nullptr;
	CachedJumpComp = nullptr;
	JumpStartTime = 0.f;
	bSecondJumpExecuted = false;

	return Super::AbortTask(OwnerComp, NodeMemory);
}

