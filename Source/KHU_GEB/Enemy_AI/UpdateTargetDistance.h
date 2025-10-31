// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Services/BTService_BlackboardBase.h"
#include "UpdateTargetDistance.generated.h"

/**
 * 
 */
UCLASS()
class KHU_GEB_API UUpdateTargetDistance : public UBTService_BlackboardBase
{
	GENERATED_BODY()
public:
    UUpdateTargetDistance();

protected:
    /** * Target(Player) ���͸� �о�� ������ Ű�Դϴ�. (�Է�)
     * UBTService_BlackboardBase���� ��ӹ��� 'BlackboardKey'�� TargetDistance (���) ������ ����մϴ�.
     */
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector TargetActorKey;

    /** �� ���񽺰� ȣ��� ������ ����Ǵ� �Լ��Դϴ�. */
    virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
};
