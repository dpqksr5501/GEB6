// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemy_AI/Task/TDamaged.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Animation/AnimMontage.h"
#include "AIController.h"
#include "GameFramework/Character.h"

UTDamaged::UTDamaged()
{
	NodeName = "TDamaged";

	// 멤버 변수(AIController 등)를 안전하게 저장하기 위해 true로 설정
	bCreateNodeInstance = true;
	// TickTask를 사용하기 위해 true로 설정
	bNotifyTick = true;
}

EBTNodeResult::Type UTDamaged::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) {
	BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (!BlackboardComp)
	{
		return EBTNodeResult::Failed;
	}
	
	// Montage 재생을 위해 AIController > Character > AnimInstance 가져오기
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController)
	{
		UE_LOG(LogTemp, Warning, TEXT("TAttack: AIController is missing"));
		return EBTNodeResult::Failed;
	}
	ACharacter* Character = Cast<ACharacter>(AIController->GetPawn());
	if (!Character)
	{
		UE_LOG(LogTemp, Warning, TEXT("TDamaged: Character is missing"));
		return EBTNodeResult::Failed;
	}
	UAnimInstance* AnimInstance = Character->GetMesh() ? Character->GetMesh()->GetAnimInstance() : nullptr;
	if (AnimInstance && DamagedMontage)
	{
		// 상태 변환. 이미 Damaged가 되어야 발동되는 노드이긴 한데..
		// BlackboardComp->SetValueAsEnum("EnemyState", (uint8)EEnemyState::EES_Damaged);
		AnimInstance->Montage_Play(DamagedMontage);
	}
	else 
	{
		UE_LOG(LogTemp, Warning, TEXT("TDamaged: AnimInstance or DamagedMontage is missing"));
		return EBTNodeResult::Failed;
	}

	return EBTNodeResult::InProgress;
}

void UTDamaged::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) {
	// Montage 확인을 위해 AIController > Character > AnimInstance 가져오기
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController)
	{
		UE_LOG(LogTemp, Warning, TEXT("TAttack: AIController is missing"));
		return;
	}
	ACharacter* Character = Cast<ACharacter>(AIController->GetPawn());
	if (!Character)
	{
		UE_LOG(LogTemp, Warning, TEXT("TDamaged: Character is missing"));
		return;
	}
	UAnimInstance* AnimInstance = Character->GetMesh() ? Character->GetMesh()->GetAnimInstance() : nullptr;

	if (!AnimInstance->Montage_IsPlaying(DamagedMontage))
	{
		// 몽타주 재생이 끝나면 Task 성공
		BlackboardComp->SetValueAsEnum("EnemyState", (uint8)EEnemyState::EES_Idle);
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
	
}