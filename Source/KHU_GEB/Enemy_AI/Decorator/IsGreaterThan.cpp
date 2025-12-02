// Fill out your copyright notice in the Description page of Project Settings.


#include "IsGreaterThan.h"
#include "BehaviorTree/BlackboardComponent.h"

UIsGreaterThan::UIsGreaterThan()
{
	// 노드 이름 설정
	NodeName = TEXT("Is Greater Than");

	// 기본 비교 값 설정
	CompareValue = 0.0f;

	// Observer Abort 설정 - 블랙보드 값 변경 시 재평가
	bNotifyBecomeRelevant = true; // 이 node에 진입할 때 OnBecomeRelevant를 호출하게 된다.
	bNotifyTick = false; // Tick은 필요 없음
}

void UIsGreaterThan::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	// 블랙보드 키 변경 감지 설정
	UBlackboardData* BBAsset = GetBlackboardAsset();
	if (BBAsset)
	{
		BlackboardKey.ResolveSelectedKey(*BBAsset);
	}
}

void UIsGreaterThan::OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	Super::OnBecomeRelevant(OwnerComp, NodeMemory);

	// 노드가 활성화 될 때 부모 클래스에서 Observer를 등록
}

void UIsGreaterThan::OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	Super::OnCeaseRelevant(OwnerComp, NodeMemory);

	// 노드가 비활성화 될 때 부모 클래스에서 Observer를 해제
}

EBlackboardNotificationResult UIsGreaterThan::OnBlackboardKeyValueChange(const UBlackboardComponent& Blackboard, FBlackboard::FKey ChangedKeyID)
{
	// BlackboardKey 값이 변경될 때 마다 조건을 재평가
	return Super::OnBlackboardKeyValueChange(Blackboard, ChangedKeyID);
}

bool UIsGreaterThan::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (!BlackboardComp)
	{
		return false;
	}

	// 1. 블랙보드에서 (BlackboardKey에 연결된) Float 값을 읽어옵니다.
	const float BlackboardValue = BlackboardComp->GetValueAsFloat(BlackboardKey.SelectedKeyName);

	// 2. 비교: BlackboardValue > CompareValue
	return BlackboardValue > CompareValue;
}

FString UIsGreaterThan::GetStaticDescription() const
{
	// 에디터에 "BlackboardKey의 이름 > CompareValue" 형태로 표시됩니다.
	// 예: "LastAttackCounter > 5.0"
	return FString::Printf(TEXT("%s: > %.1f"), *Super::GetStaticDescription(), CompareValue);
}