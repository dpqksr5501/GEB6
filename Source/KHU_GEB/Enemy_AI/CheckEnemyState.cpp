// Fill out your copyright notice in the Description page of Project Settings.


#include "CheckEnemyState.h"
#include "BehaviorTree/BlackboardComponent.h"

UCheckEnemyState::UCheckEnemyState()
{
	NodeName = TEXT("Check Enemy State");

	// ��ӹ��� 'BlackboardKey' ������ Enum ���͸� �����մϴ�.
	BlackboardKey.AddEnumFilter(this, GET_MEMBER_NAME_CHECKED(UCheckEnemyState, BlackboardKey), StaticEnum<EEnemyState>());
}

bool UCheckEnemyState::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	const UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (BlackboardComp == nullptr)
	{
		return false;
	}

	// ��ӹ��� 'BlackboardKey'�� ����Ͽ� ���� �����ɴϴ�.
	const uint8 CurrentStateValue = BlackboardComp->GetValueAsEnum(BlackboardKey.SelectedKeyName);

	const EEnemyState CurrentState = static_cast<EEnemyState>(CurrentStateValue);

	return CurrentState == DesiredState;
}