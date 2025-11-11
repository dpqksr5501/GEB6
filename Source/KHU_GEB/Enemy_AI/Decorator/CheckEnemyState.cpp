// Fill out your copyright notice in the Description page of Project Settings.


#include "CheckEnemyState.h"
#include "BehaviorTree/BlackboardComponent.h"

UCheckEnemyState::UCheckEnemyState()
{
	NodeName = TEXT("Check Enemy State");

	// 상속받은 'BlackboardKey' 변수에 Enum 필터를 설정합니다.
	BlackboardKey.AddEnumFilter(this, GET_MEMBER_NAME_CHECKED(UCheckEnemyState, BlackboardKey), StaticEnum<EEnemyState>());
}

bool UCheckEnemyState::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	const UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (BlackboardComp == nullptr)
	{
		return false;
	}

	// 상속받은 'BlackboardKey'를 사용하여 값을 가져옵니다.
	const uint8 CurrentStateValue = BlackboardComp->GetValueAsEnum(BlackboardKey.SelectedKeyName);

	const EEnemyState CurrentState = static_cast<EEnemyState>(CurrentStateValue);

	return CurrentState == DesiredState;
}