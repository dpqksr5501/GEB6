// Fill out your copyright notice in the Description page of Project Settings.

#include "TApproach.h"
#include "AIController.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Navigation/PathFollowingComponent.h" // EPathFollowingRequestResult::Type 사용
#include "Engine.h" // GEngine (디버그 메시지) 사용을 위해 필요

UTApproach::UTApproach()
{
	NodeName = "TApproach";

	// TickTask를 매 프레임 호출하기 위해 반드시 true로 설정
	bNotifyTick = true;

	// 멤버 변수(AIController 등)를 안전하게 저장하기 위해 true로 설정
	bCreateNodeInstance = true;
}

EBTNodeResult::Type UTApproach::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	// AIController, Controlled Pawn, Character Movement Component 가져와 변수로 저장
	AIController = OwnerComp.GetAIOwner();
	BlackboardComp = OwnerComp.GetBlackboardComponent();

	if (!AIController || !BlackboardComp)
	{
		return EBTNodeResult::Failed;
	}

	ControlledPawn = AIController->GetPawn();
	if (!ControlledPawn)
	{
		return EBTNodeResult::Failed;
	}

	MovementComponent = Cast<UCharacterMovementComponent>(ControlledPawn->GetComponentByClass(UCharacterMovementComponent::StaticClass()));
	if (!MovementComponent)
	{
		return EBTNodeResult::Failed;
	}

	// BlackBoard에서 "TargetDistance" 값(float) 가져와 CurrentDistance 변수 갱신
	CurrentDistance = BlackboardComp->GetValueAsFloat(TargetDistanceKey.SelectedKeyName);

	// If 분기를 통해 CurrentDistance <= Stopdistance 검사
	if (CurrentDistance <= StopDistance)
	{
		// 이미 범위 내에 있으므로 즉시 성공
		return EBTNodeResult::Succeeded;
	}
	else
	{
		// 타겟 액터 가져오기
		AActor* TargetActor = Cast<AActor>(BlackboardComp->GetValueAsObject(TargetKey.SelectedKeyName));
		if (!TargetActor)
		{
			return EBTNodeResult::Failed;
		}

		// MoveToActor를 여기서 한 번만 호출
		EPathFollowingRequestResult::Type MoveResult = AIController->MoveToActor(TargetActor, 1.0f);

		if (MoveResult == EPathFollowingRequestResult::RequestSuccessful)
		{
			// 이동 요청 성공. TickTask에서 거리 검사 시작
			// 상태 변환
			BlackboardComp->SetValueAsEnum("EnemyState", (uint8)EEnemyState::EES_Moving);
			return EBTNodeResult::InProgress;
		}
		else
		{
			// 경로 탐색 실패 등
			return EBTNodeResult::Failed;
		}
	}
}

void UTApproach::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	// TickTask는 오직 거리 검사만 수행
	BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (!BlackboardComp)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}
	CurrentDistance = BlackboardComp->GetValueAsFloat(TargetDistanceKey.SelectedKeyName);
	if (CurrentDistance <= StopDistance)
	{
		// 목표 범위 도달. 이동 중지 (선택적이지만 안전함)
		if (AIController)
		{
			AIController->StopMovement();
		}

		// 태스크 성공으로 종료
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
	// else: 아직 범위 밖. 다음 틱까지 InProgress 상태 유지
}

EBTNodeResult::Type UTApproach::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	// 이 태스크가 다른 브랜치에 의해 중단(Abort)되었을 때
	// AI가 즉시 멈추도록 보장
	if (AIController)
	{
		AIController->StopMovement();
	}

	return EBTNodeResult::Aborted;
}