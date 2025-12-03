// Fill out your copyright notice in the Description page of Project Settings.

#include "Enemy_AI/Task/TGlide.h"
#include "AIController.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Engine/World.h"
#include "Enemy_AI/Enemy_Base.h"
#include "Enemy_AI/EnemyAnimIntance.h"
#include "JumpComponent.h"

UTGlide::UTGlide()
{
	NodeName = TEXT("Glide");
	bNotifyTick = true; // 착지까지 대기하므로 Tick 필요

	GlideStartTime = 0.f;
	CachedCharacter = nullptr;
	CachedJumpComp = nullptr;
	CachedAnimInstance = nullptr;
	GlideTimeout = 10.0f;
}

EBTNodeResult::Type UTGlide::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	Super::ExecuteTask(OwnerComp, NodeMemory);

	// 1. AI 컨트롤러 획득
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TGlide] AIController is missing"));
		return EBTNodeResult::Failed;
	}

	// 2. 캐릭터 획득
	CachedCharacter = Cast<ACharacter>(AIController->GetPawn());
	if (!CachedCharacter)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TGlide] Character is missing"));
		return EBTNodeResult::Failed;
	}

	// 3. CharacterMovementComponent 확인
	UCharacterMovementComponent* MovementComp = CachedCharacter->GetCharacterMovement();
	if (!MovementComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TGlide] CharacterMovementComponent is missing"));
		return EBTNodeResult::Failed;
	}

	// 4. Enemy_Base로 캐스팅하여 JumpComponent 획득
	if (AEnemy_Base* Enemy = Cast<AEnemy_Base>(CachedCharacter))
	{
		CachedJumpComp = Enemy->JumpComp;
		if (!CachedJumpComp)
		{
			UE_LOG(LogTemp, Error, TEXT("[TGlide] JumpComponent is null! Enemy: %s"), 
				*Enemy->GetName());
			return EBTNodeResult::Failed;
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[TGlide] Character is not Enemy_Base! Class: %s"), 
			*CachedCharacter->GetClass()->GetName());
		return EBTNodeResult::Failed;
	}

	// 5. AnimInstance 획득
	if (USkeletalMeshComponent* Mesh = CachedCharacter->GetMesh())
	{
		CachedAnimInstance = Cast<UEnemyAnimIntance>(Mesh->GetAnimInstance());
		if (!CachedAnimInstance)
		{
			UE_LOG(LogTemp, Warning, TEXT("[TGlide] EnemyAnimInstance casting failed"));
		}
	}

	// 6. 땅 위에서만 실행 가능
	if (!CachedJumpComp->IsOnGround())
	{
		UE_LOG(LogTemp, Warning, TEXT("[TGlide] Character is not on ground"));
		return EBTNodeResult::Failed;
	}

	// 7. AnimInstance 플래그 설정 (점프 시작)
	if (CachedAnimInstance)
	{
		CachedAnimInstance->SetIsJumping(true);
		UE_LOG(LogTemp, Log, TEXT("[TGlide] SetIsJumping(true)"));
	}

	// 8. JumpComponent의 HandleSpacePressed 호출 (폼에 맞는 글라이드 실행)
	// CurrentForm이 Range이므로 HandleRangePressed가 호출됨
	// - LaunchCharacter로 위로 발사
	// - GravityScale을 RangeGlideGravityScale(0.1)로 설정 → 천천히 하강
	// - bIsRangeGliding = true 설정
	CachedJumpComp->HandleSpacePressed();
	
	GlideStartTime = OwnerComp.GetWorld()->GetTimeSeconds();
	
	UE_LOG(LogTemp, Log, TEXT("[TGlide] Glide initiated via JumpComponent"));
	
	return EBTNodeResult::InProgress;
}

void UTGlide::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickTask(OwnerComp, NodeMemory, DeltaSeconds);

	// 유효성 검사
	if (!CachedCharacter || !CachedJumpComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TGlide] Cached references invalid in TickTask"));
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	UCharacterMovementComponent* MovementComp = CachedCharacter->GetCharacterMovement();
	if (!MovementComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TGlide] MovementComponent is invalid in TickTask"));
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	UWorld* World = OwnerComp.GetWorld();
	if (!World)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	// 상하 속도가 0이하 (하강중)이라면 Task 종료
	if (MovementComp->Velocity.Z < 0.f)
	{
		UE_LOG(LogTemp, Log, TEXT("[TGlide] Character started descending, finishing Glide task"));
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}
}

EBTNodeResult::Type UTGlide::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	// HandleSpaceReleased 호출하여 정리
	if (CachedJumpComp)
	{
		CachedJumpComp->HandleSpaceReleased();
		UE_LOG(LogTemp, Log, TEXT("[TGlide] HandleSpaceReleased on abort"));
	}

	// AnimInstance 플래그 즉시 리셋
	if (CachedAnimInstance)
	{
		CachedAnimInstance->SetIsJumping(false);
		UE_LOG(LogTemp, Log, TEXT("[TGlide] Flags reset on abort"));
	}

	// 캐시 정리
	CachedCharacter = nullptr;
	CachedJumpComp = nullptr;
	CachedAnimInstance = nullptr;
	GlideStartTime = 0.f;

	return Super::AbortTask(OwnerComp, NodeMemory);
}

