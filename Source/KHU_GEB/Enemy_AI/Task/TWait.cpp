// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemy_AI/Task/TWait.h"
#include "BehaviorTree/BlackboardComponent.h"

UTWait::UTWait()
{
	NodeName = "TWait";

	// 멤버 변수(AIController 등)를 안전하게 저장하기 위해 true로 설정
	bCreateNodeInstance = true;
}

EBTNodeResult::Type UTWait::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) {
	BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (!BlackboardComp)
	{
		return EBTNodeResult::Failed;
	}
	// 상태 변환
	BlackboardComp->SetValueAsEnum("EnemyState", (uint8)EEnemyState::EES_Idle);

	return EBTNodeResult::Succeeded;
}