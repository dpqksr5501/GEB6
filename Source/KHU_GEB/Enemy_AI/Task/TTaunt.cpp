// Fill out your copyright notice in the Description page of Project Settings.

#include "Enemy_AI/Task/TTaunt.h"
#include "AIController.h"
#include "GameFramework/Character.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Engine/World.h"
#include "Enemy_AI/Enemy_Base.h"
#include "JumpComponent.h"

UTTaunt::UTTaunt()
{
	NodeName = TEXT("Taunt");
	bNotifyTick = true;

	TauntStartTime = 0.f;
	CachedCharacter = nullptr;
	CachedJumpComp = nullptr;
	TauntDuration = 2.1f;
}

EBTNodeResult::Type UTTaunt::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	Super::ExecuteTask(OwnerComp, NodeMemory);

	// 1. AI 컨트롤러 획득
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TTaunt] AIController is missing"));
		return EBTNodeResult::Failed;
	}

	// 2. 캐릭터 획득
	CachedCharacter = Cast<ACharacter>(AIController->GetPawn());
	if (!CachedCharacter)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TTaunt] Character is missing"));
		return EBTNodeResult::Failed;
	}

	// 3. Enemy_Base로 캐스팅하여 JumpComponent 획득
	if (AEnemy_Base* Enemy = Cast<AEnemy_Base>(CachedCharacter))
	{
		CachedJumpComp = Enemy->JumpComp;
		if (!CachedJumpComp)
		{
			UE_LOG(LogTemp, Error, TEXT("[TTaunt] JumpComponent is null! Enemy: %s"), 
				*Enemy->GetName());
			return EBTNodeResult::Failed;
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[TTaunt] Character is not Enemy_Base! Class: %s"), 
			*CachedCharacter->GetClass()->GetName());
		return EBTNodeResult::Failed;
	}

	// 4. 땅 위에서만 실행 가능 (Guard는 지상 스킬)
	if (!CachedJumpComp->IsOnGround())
	{
		UE_LOG(LogTemp, Warning, TEXT("[TTaunt] Character is not on ground"));
		return EBTNodeResult::Failed;
	}

	// 5. JumpComponent의 HandleSpacePressed 호출 (폼에 맞는 도발 실행)
	// CurrentForm이 Guard이므로 내부적으로 HandleGuardPressed가 호출됨
	CachedJumpComp->HandleSpacePressed();
	
	TauntStartTime = OwnerComp.GetWorld()->GetTimeSeconds();
	
	UE_LOG(LogTemp, Log, TEXT("[TTaunt] Taunt initiated via JumpComponent"));
	
	return EBTNodeResult::InProgress;
}

void UTTaunt::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickTask(OwnerComp, NodeMemory, DeltaSeconds);

	// 유효성 검사
	if (!CachedCharacter || !CachedJumpComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TTaunt] Cached references invalid in TickTask"));
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	UWorld* World = OwnerComp.GetWorld();
	if (!World)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	// 도발(끌어당김) 지속 시간 체크
	const float CurrentTime = World->GetTimeSeconds();
	const float ElapsedTime = CurrentTime - TauntStartTime;
	// TauntDuration만큼 대기 후 완료
	// JumpComponent의 GuardPullDuration(2.0초) + 여유시간(0.1초)
	if (ElapsedTime >= TauntDuration)
	{
		UE_LOG(LogTemp, Log, TEXT("[TTaunt] Taunt completed (%.2f seconds elapsed)"), ElapsedTime);
		
		// HandleSpaceReleased 호출 (필요시 정리)
		CachedJumpComp->HandleSpaceReleased();
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	// 아직 도발 진행 중 - JumpComponent의 Tick에서 끌어당김 처리 중
	// 여기서는 단순히 시간만 체크
}

EBTNodeResult::Type UTTaunt::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	// HandleSpaceReleased 호출하여 정리
	if (CachedJumpComp)
	{
		CachedJumpComp->HandleSpaceReleased();
		UE_LOG(LogTemp, Log, TEXT("[TTaunt] HandleSpaceReleased on abort"));
	}

	// 캐시 정리
	CachedCharacter = nullptr;
	CachedJumpComp = nullptr;
	TauntStartTime = 0.f;

	return Super::AbortTask(OwnerComp, NodeMemory);
}

