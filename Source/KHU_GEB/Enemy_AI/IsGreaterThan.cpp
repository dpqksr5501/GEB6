// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemy_AI/IsGreaterThan.h"
#include "BehaviorTree/BlackboardComponent.h"

UIsGreaterThan::UIsGreaterThan()
{
	// ��� �̸� ����
	NodeName = TEXT("Is Greater Than");

	// �⺻ �� �� ����
	CompareValue = 0.0f;
}

bool UIsGreaterThan::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (!BlackboardComp)
	{
		return false;
	}

	// 1. �����忡�� (BlackboardKey�� �����) Float ���� �о�ɴϴ�.
	const float BlackboardValue = BlackboardComp->GetValueAsFloat(BlackboardKey.SelectedKeyName);
	UE_LOG(LogTemp, Display, TEXT("BlackboardValue: %f"), BlackboardValue);

	// 2. ��: BlackboardValue > CompareValue
	return BlackboardValue > CompareValue;
}

FString UIsGreaterThan::GetStaticDescription() const
{
	// �����Ϳ� "BlackboardKey�� �̸� > CompareValue" ���·� ǥ�õ˴ϴ�.
	// ��: "LastAttackCounter > 5.0"
	return FString::Printf(TEXT("%s: > %.1f"), *Super::GetStaticDescription(), CompareValue);
}