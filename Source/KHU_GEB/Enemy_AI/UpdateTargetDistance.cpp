// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemy_AI/UpdateTargetDistance.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"

UUpdateTargetDistance::UUpdateTargetDistance()
{
    // ������ �̸� ����
    NodeName = TEXT("Update Target Distance");

    // ���񽺰� �󸶳� ���� ������� (��) ���� (0.5�ʸ���)
    Interval = 0.5f;
    // �Ź� �ణ�� �ð����� �ξ� ���� AI�� ���ÿ� ����Ǵ� ���� ����
    RandomDeviation = 0.1f;
}

void UUpdateTargetDistance::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

    // �ʿ��� ������Ʈ ��������
    UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
    AAIController* AIController = OwnerComp.GetAIOwner();

    // ��ȿ�� �˻� (�ʼ�)
    if (!BlackboardComp || !AIController)
    {
        return;
    }

    APawn* ControllingPawn = AIController->GetPawn();
    if (!ControllingPawn)
    {
        return;
    }

    // 1. �Է� Ű (Target) ���� Ÿ�� ���� �о����
    UObject* TargetObject = BlackboardComp->GetValueAsObject(TargetActorKey.SelectedKeyName);
    AActor* TargetActor = Cast<AActor>(TargetObject);

    if (!TargetActor)
    {
        // Ÿ���� ������ (��: �÷��̾ ��ħ), �Ÿ��� ��ȿ���� ���� ��(-1.0)���� ����
        BlackboardComp->SetValueAsFloat(BlackboardKey.SelectedKeyName, -1.0f);
        return;
    }

    // 2. �ڽŰ� Ÿ�� ������ �Ÿ� ���
    float Distance = FVector::Dist(ControllingPawn->GetActorLocation(), TargetActor->GetActorLocation());

    // 3. ��� Ű (TargetDistance) �� ���� �Ÿ� �� ����
    // BlackboardKey�� UBTService_BlackboardBase �θ� Ŭ������ ���ǵǾ� �ֽ��ϴ�.
    BlackboardComp->SetValueAsFloat(BlackboardKey.SelectedKeyName, Distance);
}