// Fill out your copyright notice in the Description page of Project Settings.


#include "IsGreaterThan.h"
#include "BehaviorTree/BlackboardComponent.h"

UIsGreaterThan::UIsGreaterThan()
{
	// 노드 이름 설정
	NodeName = TEXT("Is Greater Than");

	// 기본 비교 값 설정
	CompareValue = 0.0f;
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
	UE_LOG(LogTemp, Display, TEXT("BlackboardValue: %f"), BlackboardValue);

	// 2. 비교: BlackboardValue > CompareValue
	return BlackboardValue > CompareValue;
}

FString UIsGreaterThan::GetStaticDescription() const
{
	// 에디터에 "BlackboardKey의 이름 > CompareValue" 형태로 표시됩니다.
	// 예: "LastAttackCounter > 5.0"
	return FString::Printf(TEXT("%s: > %.1f"), *Super::GetStaticDescription(), CompareValue);
}