// Fill out your copyright notice in the Description page of Project Settings.

#include "TMoveTo.h"
#include "AIController.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Navigation/PathFollowingComponent.h" // EPathFollowingRequestResult::Type 사용
#include "Engine.h" // GEngine (디버그 메시지) 사용을 위해 필요

UTMoveTo::UTMoveTo()
{
	NodeName = "TMoveTo";

	// TickTask를 매 프레임 호출하기 위해 반드시 true로 설정
	bNotifyTick = true;

	// 멤버 변수(AIController 등)를 안전하게 저장하기 위해 true로 설정
	bCreateNodeInstance = true;

	// Vector 타입의 키만 허용
	TargetLocationKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UTMoveTo, TargetLocationKey));
}

EBTNodeResult::Type UTMoveTo::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
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

	// BlackBoard에서 Vector 타입의 목표 위치 가져오기
	FVector TargetLocation = BlackboardComp->GetValueAsVector(TargetLocationKey.SelectedKeyName);
	
	// 목표 위치가 유효한지 확인
	if (TargetLocation == FVector::ZeroVector)
	{
		return EBTNodeResult::Failed;
	}

	// 현재 거리 계산
	FVector CurrentLocation = ControlledPawn->GetActorLocation();
	CurrentDistance = FVector::Dist(CurrentLocation, TargetLocation);

	// If 분기를 통해 CurrentDistance <= StopDistance 검사
	if (CurrentDistance <= StopDistance)
	{
		// 이미 범위 내에 있으므로 즉시 성공
		return EBTNodeResult::Succeeded;
	}
	else
	{
		// 거짓일 경우 (범위 밖에 있으므로 이동 시작)
		MovementComponent->MaxWalkSpeed = WalkSpeed;

		// 상태 변환
		BlackboardComp->SetValueAsEnum("EnemyState", (uint8)EEnemyState::EES_Moving);
		
		// TickTask에서 직접 이동 처리를 시작
		return EBTNodeResult::InProgress;
	}
}

void UTMoveTo::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (!BlackboardComp || !ControlledPawn || !AIController)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	// 목표 위치와 현재 거리 계산
	FVector TargetLocation = BlackboardComp->GetValueAsVector(TargetLocationKey.SelectedKeyName);
	FVector CurrentLocation = ControlledPawn->GetActorLocation();
	CurrentDistance = FVector::Dist(CurrentLocation, TargetLocation);

	if (CurrentDistance <= StopDistance)
	{
		// 목표 범위 도달. 이동 입력 중지
		AIController->SetIgnoreMoveInput(false);
		
		// 태스크 성공으로 종료
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
	else
	{
		// 목표 방향 계산 (정규화된 벡터)
		FVector Direction = (TargetLocation - CurrentLocation).GetSafeNormal();
		
		// Z축 성분 제거 (평면 이동만)
		Direction.Z = 0.0f;
		Direction = Direction.GetSafeNormal();

		// 방향 벡터를 이용해 이동 입력 적용
		if (!Direction.IsZero())
		{
			// Pawn의 이동 입력에 직접 방향 벡터 적용
			ControlledPawn->AddMovementInput(Direction, 1.0f);
		}
	}
}

EBTNodeResult::Type UTMoveTo::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	// 이 태스크가 다른 브랜치에 의해 중단(Abort)되었을 때
	// 이동 입력 중지
	if (AIController)
	{
		AIController->SetIgnoreMoveInput(false);
	}

	return EBTNodeResult::Aborted;
}
