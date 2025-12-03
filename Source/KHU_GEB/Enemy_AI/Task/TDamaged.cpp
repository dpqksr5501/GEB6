// Fill out your copyright notice in the Description page of Project Settings.

#include "Enemy_AI/Task/TDamaged.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Animation/AnimMontage.h"
#include "AIController.h"
#include "GameFramework/Character.h"
#include "Animation/AnimInstance.h"
#include "Enemy_AI/Enemy_Base.h"
#include "FormDefinition.h"

UTDamaged::UTDamaged()
{
	NodeName = "TDamaged";

	// 멤버 변수(AIController 등)를 안전하게 저장하기 위해 true로 설정
	bCreateNodeInstance = true;
	// TickTask를 사용하기 위해 true로 설정
	bNotifyTick = true;

	CurrentMontage = nullptr;
}

EBTNodeResult::Type UTDamaged::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (!BlackboardComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("TDamaged: BlackboardComponent is missing"));
		return EBTNodeResult::Failed;
	}
	
	// AI Controller 가져오기
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController)
	{
		UE_LOG(LogTemp, Warning, TEXT("TDamaged: AIController is missing"));
		return EBTNodeResult::Failed;
	}

	// Character 가져오기
	ACharacter* Character = Cast<ACharacter>(AIController->GetPawn());
	if (!Character)
	{
		UE_LOG(LogTemp, Warning, TEXT("TDamaged: Character is missing"));
		return EBTNodeResult::Failed;
	}

	// Enemy_Base로 캐스팅
	AEnemy_Base* EnemyBase = Cast<AEnemy_Base>(Character);
	if (!EnemyBase)
	{
		UE_LOG(LogTemp, Warning, TEXT("TDamaged: EnemyBase casting failed"));
		return EBTNodeResult::Failed;
	}

	// DefaultFormDef 확인
	if (!EnemyBase->DefaultFormDef)
	{
		UE_LOG(LogTemp, Warning, TEXT("TDamaged: DefaultFormDef is missing"));
		return EBTNodeResult::Failed;
	}

	// HitReactMontage 가져오기 (TAttack의 DefaultFormDef->AttackMontages 패턴과 유사)
	CurrentMontage = EnemyBase->DefaultFormDef->HitReactMontage;

	if (!CurrentMontage)
	{
		UE_LOG(LogTemp, Warning, TEXT("TDamaged: HitReactMontage is missing in DefaultFormDef"));
		return EBTNodeResult::Failed;
	}

	// AnimInstance 가져오기
	UAnimInstance* AnimInstance = Character->GetMesh() ? Character->GetMesh()->GetAnimInstance() : nullptr;
	if (!AnimInstance)
	{
		UE_LOG(LogTemp, Warning, TEXT("TDamaged: AnimInstance is missing"));
		return EBTNodeResult::Failed;
	}

	// 몽타주 재생
	float MontageLength = AnimInstance->Montage_Play(CurrentMontage, 1.0f);
	if (MontageLength <= 0.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("TDamaged: Failed to play HitReactMontage"));
		return EBTNodeResult::Failed;
	}

	UE_LOG(LogTemp, Log, TEXT("TDamaged: Playing HitReactMontage from DefaultFormDef"));
	return EBTNodeResult::InProgress;
}

void UTDamaged::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	// Montage 확인을 위해 AIController > Character > AnimInstance 가져오기
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		UE_LOG(LogTemp, Warning, TEXT("TDamaged: AIController is missing in TickTask"));
		return;
	}

	ACharacter* Character = Cast<ACharacter>(AIController->GetPawn());
	if (!Character)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		UE_LOG(LogTemp, Warning, TEXT("TDamaged: Character is missing in TickTask"));
		return;
	}

	UAnimInstance* AnimInstance = Character->GetMesh() ? Character->GetMesh()->GetAnimInstance() : nullptr;
	if (!AnimInstance)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		UE_LOG(LogTemp, Warning, TEXT("TDamaged: AnimInstance is missing in TickTask"));
		return;
	}

	if (!CurrentMontage)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		UE_LOG(LogTemp, Warning, TEXT("TDamaged: CurrentMontage is missing in TickTask"));
		return;
	}

	// 몽타주가 재생 중인지 확인
	if (!AnimInstance->Montage_IsPlaying(CurrentMontage))
	{
		// 몽타주 재생이 끝나면 상태를 Idle로 변경하고 Task 성공
		BlackboardComp->SetValueAsEnum("EnemyState", (uint8)EEnemyState::EES_Idle);
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		UE_LOG(LogTemp, Log, TEXT("TDamaged: HitReactMontage finished, returning to Idle"));
	}
}