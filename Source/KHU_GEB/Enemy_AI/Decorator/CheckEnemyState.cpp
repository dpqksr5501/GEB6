// Fill out your copyright notice in the Description page of Project Settings.


#include "CheckEnemyState.h"
#include "BehaviorTree/BlackboardComponent.h"

UCheckEnemyState::UCheckEnemyState()
{
	NodeName = TEXT("Check Enemy State");

	// 상속받은 'BlackboardKey' 변수에 Enum 필터를 설정합니다.
	BlackboardKey.AddEnumFilter(this, GET_MEMBER_NAME_CHECKED(UCheckEnemyState, BlackboardKey), StaticEnum<EEnemyState>());

	// Observer Abort 설정 - 블랙보드 값 변경 시 재평가
	bNotifyBecomeRelevant = true; // 이 node에 진입할 때 OnBecomeRelevant를 호출하게 된다.
	bNotifyTick = false; // Tick은 필요 없음
}

void UCheckEnemyState::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	// 블랙보드 키 변경 감지 설정
	UBlackboardData* BBAsset = GetBlackboardAsset();
	if (BBAsset)
	{
		BlackboardKey.ResolveSelectedKey(*BBAsset);
	}
}

void UCheckEnemyState::OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	Super::OnBecomeRelevant(OwnerComp, NodeMemory);

	// 노드가 활성화 될 때 부모 클래스에서 Observer를 등록
}

void UCheckEnemyState::OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	Super::OnCeaseRelevant(OwnerComp, NodeMemory);

	// 노드가 비활성화 될 때 부모 클래스에서 Observer를 해제
}

EBlackboardNotificationResult UCheckEnemyState::OnBlackboardKeyValueChange(const UBlackboardComponent& Blackboard, FBlackboard::FKey ChangedKeyID)
{
	// BlackboardKey 값이 변경될 때 마다 조건을 재평가
	return Super::OnBlackboardKeyValueChange(Blackboard, ChangedKeyID);
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

FString UCheckEnemyState::GetStaticDescription() const
{
	// 에디터에 "BlackboardKey의 이름 == DesiredState" 형태로 표시됩니다.
	// 예: "EnemyState == Attacking"
	FString StateString = StaticEnum<EEnemyState>()->GetNameStringByValue((int64)DesiredState);
	return FString::Printf(TEXT("%s: == %s"), *Super::GetStaticDescription(), *StateString);
}