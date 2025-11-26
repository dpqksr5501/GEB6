// Fill out your copyright notice in the Description page of Project Settings.

#include "Enemy_AI/Task/TJump.h"
#include "AIController.h"
#include "GameFramework/Character.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Engine/World.h"
#include "Enemy_AI/Enemy_Base.h"
#include "JumpComponent.h"
#include "MonsterAnimInstanceBase.h"

UTJump::UTJump()
{
	NodeName = TEXT("Jump");
	bNotifyTick = true;

	JumpStartTime = 0.f;
	CachedJumpComp = nullptr;
	CachedEnemy = nullptr;
	CachedAnimInstance = nullptr;
}

EBTNodeResult::Type UTJump::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	Super::ExecuteTask(OwnerComp, NodeMemory);

	// 1. AI 컨트롤러 획득
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController)
	{
		UE_LOG(LogTemp, Warning, TEXT("TJump: AIController is missing"));
		return EBTNodeResult::Failed;
	}

	// 2. 캐릭터 획득
	ACharacter* Character = Cast<ACharacter>(AIController->GetPawn());
	if (!Character)
	{
		UE_LOG(LogTemp, Warning, TEXT("TJump: Character is missing"));
		return EBTNodeResult::Failed;
	}

	// 3. Enemy_Base로 캐스팅
	CachedEnemy = Cast<AEnemy_Base>(Character);
	if (!CachedEnemy)
	{
		UE_LOG(LogTemp, Warning, TEXT("TJump: Enemy_Base casting failed"));
		return EBTNodeResult::Failed;
	}

	// 4. JumpComponent 획득
	CachedJumpComp = CachedEnemy->JumpComp;
	if (!CachedJumpComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("TJump: JumpComponent is missing"));
		return EBTNodeResult::Failed;
	}

	// 5. AnimInstance 획득 및 캐시
	if (USkeletalMeshComponent* Mesh = Character->GetMesh())
	{
		CachedAnimInstance = Cast<UMonsterAnimInstanceBase>(Mesh->GetAnimInstance());
		if (!CachedAnimInstance)
		{
			UE_LOG(LogTemp, Warning, TEXT("TJump: AnimInstance casting failed"));
			return EBTNodeResult::Failed;
		}
	}

	// 6. 이미 땅에 있는지 확인 (불필요한 점프 방지)
	if (!CachedJumpComp->IsOnGround())
	{
		UE_LOG(LogTemp, Log, TEXT("TJump: Already in air, waiting for landing"));
		JumpStartTime = OwnerComp.GetWorld()->GetTimeSeconds();
		return EBTNodeResult::InProgress;
	}

	// 7. ABP 플래그 설정 (애니메이션 전환 트리거)
	if (CachedAnimInstance)
	{
		CachedAnimInstance->bJumpInput_Anim = true;
	}

	// 8. 점프 실행
	CachedJumpComp->HandleSpacePressed();

	// 9. 타이머 시작
	JumpStartTime = OwnerComp.GetWorld()->GetTimeSeconds();

	UE_LOG(LogTemp, Log, TEXT("TJump: Jump started for %s"), *CachedEnemy->GetName());

	return EBTNodeResult::InProgress;
}

void UTJump::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickTask(OwnerComp, NodeMemory, DeltaSeconds);

	// 유효성 검사
	if (!CachedJumpComp || !CachedEnemy)
	{
		UE_LOG(LogTemp, Warning, TEXT("TJump: Cached components are invalid in TickTask"));
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	UWorld* World = OwnerComp.GetWorld();
	if (!World)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	// 1. 타임아웃 체크
	const float CurrentTime = World->GetTimeSeconds();
	const float ElapsedTime = CurrentTime - JumpStartTime;

	if (ElapsedTime >= JumpTimeout)
	{
		UE_LOG(LogTemp, Warning, TEXT("TJump: Jump timeout (%.2f seconds elapsed)"), ElapsedTime);
		CachedJumpComp->HandleSpaceReleased();
		
		// ABP 플래그 리셋
		if (CachedAnimInstance)
		{
			CachedAnimInstance->bJumpInput_Anim = false;
		}
		
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	// 2. 착지 확인
	if (CachedJumpComp->IsOnGround())
	{
		UE_LOG(LogTemp, Log, TEXT("TJump: Landed successfully (%.2f seconds)"), ElapsedTime);
		CachedJumpComp->HandleSpaceReleased();
		
		// ABP 플래그 리셋
		if (CachedAnimInstance)
		{
			CachedAnimInstance->bJumpInput_Anim = false;
		}
		
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	// 여전히 공중에 있음 - 계속 대기
}

EBTNodeResult::Type UTJump::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	// 점프 중단 처리
	if (CachedJumpComp)
	{
		CachedJumpComp->HandleSpaceReleased();
		UE_LOG(LogTemp, Log, TEXT("TJump: Jump aborted"));
	}

	// ABP 플래그 리셋
	if (CachedAnimInstance)
	{
		CachedAnimInstance->bJumpInput_Anim = false;
		UE_LOG(LogTemp, Log, TEXT("TJump: Reset bJumpInput_Anim on abort"));
	}

	// 캐시 정리
	CachedJumpComp = nullptr;
	CachedEnemy = nullptr;
	CachedAnimInstance = nullptr;
	JumpStartTime = 0.f;

	return Super::AbortTask(OwnerComp, NodeMemory);
}

