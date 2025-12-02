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
    /** * Target(Player) 액터를 읽어올 블랙보드 키입니다. (입력)
     * UBTService_BlackboardBase에서 상속받은 'BlackboardKey'는 TargetDistance (출력) 용으로 사용합니다.
     */
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector TargetActorKey;

    /** 이 서비스가 호출될 때마다 실행되는 함수입니다. */
    virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
};
