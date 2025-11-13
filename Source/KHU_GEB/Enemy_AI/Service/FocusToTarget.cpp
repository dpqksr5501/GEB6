// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemy_AI/Service/FocusToTarget.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"

UFocusToTarget::UFocusToTarget()
{
	NodeName = TEXT("Focus To Target");
	bNotifyTick = true;
}

void UFocusToTarget::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);
	AAIController* AICon = OwnerComp.GetAIOwner();
	if (AICon == nullptr) {
		UE_LOG(LogTemp, Warning, TEXT("FocusToTarget: AICon is null"));
	}
	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();

	AActor* TargetActor = Cast<AActor>(BlackboardComp->GetValueAsObject(GetSelectedBlackboardKey()));
	if(TargetActor == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("FocusToTarget: TargetActor is null"));
		return;
	}
	else {
		FVector TargetLocation = TargetActor->GetActorLocation();
		AICon->SetFocalPoint(TargetLocation);
	}
}