// Fill out your copyright notice in the Description page of Project Settings.


#include "UpdateTargetDistance.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"

UUpdateTargetDistance::UUpdateTargetDistance()
{
    // 서비스의 이름 설정
    NodeName = TEXT("Update Target Distance");

    // 서비스가 얼마나 자주 실행될지 (초) 설정 (0.5초마다)
    Interval = 0.016f;
    // 매번 약간의 시간차를 두어 여러 AI가 동시에 실행되는 것을 방지
    RandomDeviation = 0.1f;
}

void UUpdateTargetDistance::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

    // 필요한 컴포넌트 가져오기
    UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
    AAIController* AIController = OwnerComp.GetAIOwner();

    // 유효성 검사 (필수)
    if (!BlackboardComp || !AIController)
    {
        return;
    }

    APawn* ControllingPawn = AIController->GetPawn();
    if (!ControllingPawn)
    {
        return;
    }

    // 1. 입력 키 (Target) 에서 타겟 액터 읽어오기
    UObject* TargetObject = BlackboardComp->GetValueAsObject(TargetActorKey.SelectedKeyName);
    AActor* TargetActor = Cast<AActor>(TargetObject);

    if (!TargetActor)
    {
        // 타겟이 없으면 (예: 플레이어를 놓침), 거리를 유효하지 않은 값(-1.0)으로 설정
        BlackboardComp->SetValueAsFloat(BlackboardKey.SelectedKeyName, -1.0f);
        return;
    }

    // 2. 자신과 타겟 사이의 거리 계산
    float Distance = FVector::Dist(ControllingPawn->GetActorLocation(), TargetActor->GetActorLocation());

    // 3. 출력 키 (TargetDistance) 에 계산된 거리 값 쓰기
    // BlackboardKey는 UBTService_BlackboardBase 부모 클래스에 정의되어 있습니다.
    BlackboardComp->SetValueAsFloat(BlackboardKey.SelectedKeyName, Distance);
}