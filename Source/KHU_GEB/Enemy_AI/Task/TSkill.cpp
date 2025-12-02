// Fill out your copyright notice in the Description page of Project Settings.

#include "Enemy_AI/Task/TSkill.h"
#include "AIController.h"
#include "GameFramework/Character.h"
#include "Animation/AnimInstance.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Engine/World.h"
#include "Enemy_AI/Enemy_Base.h"
#include "FormDefinition.h"
#include "Skills/SkillBase.h"

UTSkill::UTSkill()
{
	NodeName = TEXT("Skill");

	// TickTask 함수가 호출되도록 bNotifyTick을 true로 설정합니다.
	bNotifyTick = true;

	// 초기화
	CurrentMontage = nullptr;
}

EBTNodeResult::Type UTSkill::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	// 부모 클래스의 ExecuteTask를 먼저 호출합니다.
	Super::ExecuteTask(OwnerComp, NodeMemory);
	
	BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (!BlackboardComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("TSkill: BlackboardComponent is missing"));
		return EBTNodeResult::Failed;
	}

	// AI 컨트롤러와 컨트롤러가 조종하는 캐릭터를 가져옵니다.
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController)
	{
		UE_LOG(LogTemp, Warning, TEXT("TSkill: AIController is missing"));
		return EBTNodeResult::Failed;
	}

	ACharacter* Character = Cast<ACharacter>(AIController->GetPawn());
	if (!Character)
	{
		UE_LOG(LogTemp, Warning, TEXT("TSkill: Character is missing"));
		return EBTNodeResult::Failed;
	}

	// Enemy_Base로 캐스팅
	AEnemy_Base* EnemyBase = Cast<AEnemy_Base>(Character);
	if (!EnemyBase)
	{
		UE_LOG(LogTemp, Warning, TEXT("TSkill: EnemyBase casting failed"));
		return EBTNodeResult::Failed;
	}

	// DefaultFormDef 확인
	if (!EnemyBase->DefaultFormDef)
	{
		UE_LOG(LogTemp, Warning, TEXT("TSkill: DefaultFormDef is missing"));
		return EBTNodeResult::Failed;
	}

	// 애님 인스턴스 가져오기
	UAnimInstance* AnimInstance = Character->GetMesh() ? Character->GetMesh()->GetAnimInstance() : nullptr;
	if (!AnimInstance)
	{
		UE_LOG(LogTemp, Warning, TEXT("TSkill: AnimInstance is missing"));
		return EBTNodeResult::Failed;
	}

	// 스킬 활성화 (SkillSlotToActivate가 None이 아닌 경우)
	if (SkillSlotToActivate != ESkillSlot::Active) // None 값이 없어서 Active가 아닌 경우로 체크
	{
		UE_LOG(LogTemp, Log, TEXT("TSkill: No skill slot specified"));
	}
	else
	{
		AEnemy_Base* Caster = Cast<AEnemy_Base>(AIController->GetPawn());
		if (!Caster) return EBTNodeResult::Failed;

		UE_LOG(LogTemp, Log, TEXT("TSkill: Activating skill from Enemy_Base"));
		Caster->ActivateSkill(); // Enemy 레벨의 스킬 실행
	}

	// 현재 시간 가져오기
	float CurrentTime = OwnerComp.GetWorld()->GetTimeSeconds();

	// 상태 변경
	BlackboardComp->SetValueAsEnum("EnemyState", (uint8)EEnemyState::EES_Attacking);
	
	// LastActionTime 설정
	BlackboardComp->SetValueAsFloat("LastActionTime", CurrentTime);
	BlackboardComp->SetValueAsFloat(CooldownKey.SelectedKeyName, CurrentTime);
	
	return EBTNodeResult::InProgress;
}

void UTSkill::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	// 부모 클래스의 TickTask를 먼저 호출합니다.
	Super::TickTask(OwnerComp, NodeMemory, DeltaSeconds);

	// 유효성 검사
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		UE_LOG(LogTemp, Warning, TEXT("TSkill: AIController is missing in TickTask"));
		return;
	}

	ACharacter* Character = Cast<ACharacter>(AIController->GetPawn());
	if (!Character)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		UE_LOG(LogTemp, Warning, TEXT("TSkill: Character is missing in TickTask"));
		return;
	}

	UAnimInstance* AnimInstance = Character->GetMesh() ? Character->GetMesh()->GetAnimInstance() : nullptr;
	if (!AnimInstance)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		UE_LOG(LogTemp, Warning, TEXT("TSkill: AnimInstance is missing in TickTask"));
		return;
	}

	// 몽타주가 재생 중인지 확인
	if (!AnimInstance->Montage_IsPlaying(CurrentMontage))
	{
		// 몽타주 재생이 끝나면 상태를 Idle로 변경하고 태스크 성공
		if (BlackboardComp)
		{
			BlackboardComp->SetValueAsEnum("EnemyState", (uint8)EEnemyState::EES_Idle);
		}
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		UE_LOG(LogTemp, Log, TEXT("TSkill: SkillMontage finished, returning to Idle"));
	}
}

EBTNodeResult::Type UTSkill::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	// AI 컨트롤러와 캐릭터를 가져옵니다.
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (AIController)
	{
		ACharacter* Character = Cast<ACharacter>(AIController->GetPawn());
		if (Character)
		{
			UAnimInstance* AnimInstance = Character->GetMesh() ? Character->GetMesh()->GetAnimInstance() : nullptr;
			if (AnimInstance)
			{
				// 몽타주 중단
				CurrentMontage = AnimInstance->GetCurrentActiveMontage();
				if (CurrentMontage) {
					AnimInstance->Montage_Stop(0.2f, CurrentMontage);
				}
				UE_LOG(LogTemp, Log, TEXT("TSkill: Skill aborted, stopping montage"));
			}
		}
	}

	// 상태 정리
	CurrentMontage = nullptr;

	return Super::AbortTask(OwnerComp, NodeMemory);
}