// Fill out your copyright notice in the Description page of Project Settings.

#include "Enemy_AI/Task/TAttack.h"
#include "AIController.h"
#include "GameFramework/Character.h"
#include "Animation/AnimInstance.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Engine/World.h"
#include "Enemy_AI/Enemy_Base.h"
#include "SkillManagerComponent.h"

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

	// Enemy_Base로 캐스팅하여 스킬 시스템에 접근
	AEnemy_Base* EnemyBase = Cast<AEnemy_Base>(Character);
	if (EnemyBase == nullptr)
	{
		return EBTNodeResult::Failed;	
	}

	// Equipped 배열에서 사용 가능한 스킬 찾기
	USkillBase* SkillToUse = nullptr;
	for (const auto& EquippedItem : EnemyBase->Equipped)
	{
		// 튜플의 두 번째 요소(스킬)를 가져오기
		USkillBase* Skill = EquippedItem.Get<1>();
		if (Skill != nullptr)
		{
			SkillToUse = Skill;
			break;
		}
	}

	// 스킬이 있으면 스킬 사용, 없으면 기존 몽타주 재생
	if (SkillToUse)
	{
		// 스킬 활성화
		SkillToUse->ActivateSkill();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("TAttack: Skill이 설정되지 않았습니다."));
	}
	// 캐릭터의 애님 인스턴스를 가져옵니다.
	UAnimInstance* AnimInstance = Character->GetMesh() ? Character->GetMesh()->GetAnimInstance() : nullptr;
	if (AnimInstance == nullptr)
	{
		return EBTNodeResult::Failed;
	}

	// UpperMontage 변수가 BT 에디터에서 설정되었는지 확인합니다.
	if (UpperMontage == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("TAttack: UpperMontage가 설정되지 않았습니다."));
		return EBTNodeResult::Failed;
	}
	if (FullMontage == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("TAttack: FullMontage가 설정되지 않았습니다."));
		return EBTNodeResult::Failed;
	}

	// 몽타주를 재생합니다.
	AnimInstance->Montage_Play(UpperMontage);
	AnimInstance->Montage_Play(FullMontage);

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
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (AIController == nullptr)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		UE_LOG(LogTemp, Warning, TEXT("TAttack: AIController is missing"));
		return;
	}

	ACharacter* Character = Cast<ACharacter>(AIController->GetPawn());
	if (Character == nullptr)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		UE_LOG(LogTemp, Warning, TEXT("TAttack: Character is missing"));
		return;
	}

	UAnimInstance* AnimInstance = Character->GetMesh() ? Character->GetMesh()->GetAnimInstance() : nullptr;
	if (AnimInstance == nullptr)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		UE_LOG(LogTemp, Warning, TEXT("TAttack: AnimInstance is missing"));
		return;
	}

	if (UpperMontage == nullptr || FullMontage == nullptr)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		UE_LOG(LogTemp, Warning, TEXT("TAttack: Montage is missing"));
		return;
	}
	
	// 지정된 몽타주가 현재 재생 중인지 확인합니다.
	if (!AnimInstance->Montage_IsPlaying(UpperMontage) && !AnimInstance->Montage_IsPlaying(FullMontage))
	{
		// 몽타주 재생이 끝났으므로, 태스크를 성공으로 완료시킵니다.
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}