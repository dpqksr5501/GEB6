// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/BlackboardComponent.h" // FBlackboardKeySelector 사용을 위해 필요
#include "TApproach.generated.h"

// 전방 선언
class AAIController;
class APawn;
class UCharacterMovementComponent;

/**
 * TargetDistance를 확인하며 Target에게 접근하는 노드
 */
UCLASS()
class KHU_GEB_API UTApproach : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UTApproach();

	/** BP에서 설정할 정지 거리 */
	UPROPERTY(EditAnywhere, Category = "Task Properties")
	float StopDistance = 100.0f;

	/** BP에서 설정할 걷기 속도 */
	UPROPERTY(EditAnywhere, Category = "Task Properties")
	float WalkSpeed = 300.0f;

	/** 타겟 액터를 가져올 블랙보드 키 */
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector TargetKey;

	/** 타겟과의 거리를 가져올 블랙보드 키 */
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector TargetDistanceKey;

protected:
	/** 노드 실행 시작 */
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	/** 매 프레임 틱 */
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	/** 태스크가 중단될 때 호출됨 */
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	UPROPERTY()
	TObjectPtr<AAIController> AIController;

	UPROPERTY()
	TObjectPtr<APawn> ControlledPawn;

	UPROPERTY()
	TObjectPtr<UCharacterMovementComponent> MovementComponent;

	UPROPERTY()
	UBlackboardComponent* BlackboardComp;

	UPROPERTY()
	float CurrentDistance = 0.0f;
};