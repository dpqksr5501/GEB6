// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemy_AI/Task/TAttack.h"
#include "AIController.h"
#include "GameFramework/Character.h"
#include "Animation/AnimInstance.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Engine/World.h"

UTAttack::UTAttack()
{
	// 노드 이름을 "Attack"으로 설정합니다.
	NodeName = TEXT("Attack");

	// TickTask 함수가 호출되도록 bNotifyTick을 true로 설정합니다.
	bNotifyTick = true;
}

EBTNodeResult::Type UTAttack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	// 부모 클래스의 ExecuteTask를 먼저 호출합니다.
	Super::ExecuteTask(OwnerComp, NodeMemory);
	BlackboardComp = OwnerComp.GetBlackboardComponent();

	if (!BlackboardComp)
	{
		return EBTNodeResult::Failed;
	}

	// AI 컨트롤러와 컨트롤러가 빙의 중인 캐릭터를 가져옵니다.
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (AIController == nullptr)
	{
		return EBTNodeResult::Failed;
	}

	ACharacter* Character = Cast<ACharacter>(AIController->GetPawn());
	if (Character == nullptr)
	{
		return EBTNodeResult::Failed;
	}

	// 캐릭터의 애님 인스턴스를 가져옵니다.
	UAnimInstance* AnimInstance = Character->GetMesh() ? Character->GetMesh()->GetAnimInstance() : nullptr;
	if (AnimInstance == nullptr)
	{
		return EBTNodeResult::Failed;
	}

	// AttackMontage 변수가 BT 에디터에서 설정되었는지 확인합니다.
	if (AttackMontage == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("TAttack: AttackMontage가 설정되지 않았습니다."));
		return EBTNodeResult::Failed;
	}

	// 몽타주를 재생합니다.
	AnimInstance->Montage_Play(AttackMontage);

	// 몽타주가 재생되는 동안 태스크가 계속 실행 중이어야 하므로 InProgress를 반환합니다.
	// TickTask에서 몽타주 종료를 감지할 것입니다.
	// 상태 변환
	BlackboardComp->SetValueAsEnum("EnemyState", (uint8)EEnemyState::EES_Attacking);
	BlackboardComp->SetValueAsFloat(LastActionTimeKey.SelectedKeyName, OwnerComp.GetWorld()->GetTimeSeconds());
	BlackboardComp->SetValueAsFloat("LastActionTime", OwnerComp.GetWorld()->GetTimeSeconds());
	return EBTNodeResult::InProgress;
}

void UTAttack::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	// 부모 클래스의 TickTask를 먼저 호출합니다.
	Super::TickTask(OwnerComp, NodeMemory, DeltaSeconds);

	// AI 컨트롤러와 캐릭터, 애님 인스턴스를 다시 가져옵니다.
	// (ExecuteTask 이후에 유효하지 않게 될 수도 있으므로 매번 확인하는 것이 안전합니다.)
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (AIController == nullptr)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	ACharacter* Character = Cast<ACharacter>(AIController->GetPawn());
	if (Character == nullptr)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	UAnimInstance* AnimInstance = Character->GetMesh() ? Character->GetMesh()->GetAnimInstance() : nullptr;
	if (AnimInstance == nullptr)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	if (AttackMontage == nullptr)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}
	
	// 지정된 몽타주가 현재 재생 중인지 확인합니다.
	if (!AnimInstance->Montage_IsPlaying(AttackMontage))
	{
		// 몽타주 재생이 끝났으므로, 태스크를 성공으로 완료시킵니다.
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}

	// 몽타주가 아직 재생 중이라면 아무것도 하지 않습니다. (태스크는 InProgress 상태 유지)
}