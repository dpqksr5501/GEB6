// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Decorators/BTDecorator_BlackboardBase.h"
#include "IsGreaterThan.generated.h"

/**
 * �������� Float �� (BlackboardKey)�� 'CompareValue'���� ũ�� true�� ��ȯ�մϴ�.
 * (Inverse üũ �� '�۰ų� ����' (<=)�� �����մϴ�.)
 */
UCLASS()
class KHU_GEB_API UIsGreaterThan : public UBTDecorator_BlackboardBase
{
	GENERATED_BODY()

public:
	UIsGreaterThan();

protected:
	/** ���� ������ �Ǵ� ��. */
	UPROPERTY(EditAnywhere, Category = "Condition")
	float CompareValue;

	/** ���� ���� �˻縦 �����ϴ� �Լ��Դϴ�. */
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;

	/** �����Ϳ��� ��� �̸��� �� �˱� ���� ǥ���ϱ� ���� �Լ��Դϴ�. */
	virtual FString GetStaticDescription() const override;
};
