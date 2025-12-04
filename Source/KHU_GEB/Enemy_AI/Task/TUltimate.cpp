// Fill out your copyright notice in the Description page of Project Settings.

#include "Enemy_AI/Task/TUltimate.h"
#include "AIController.h"
#include "GameFramework/Character.h"
#include "Animation/AnimInstance.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Engine/World.h"
#include "Enemy_AI/Enemy_Base.h"
#include "FormDefinition.h"
#include "Skills/SkillBase.h"
#include "Enemy_AI/Enemy_Special.h"
#include "Skills/Skill_Ultimate.h"

UTUltimate::UTUltimate()
{
	NodeName = TEXT("Ultimate");

	// TickTask 함수가 호출되도록 bNotifyTick을 true로 설정합니다.
	bNotifyTick = true;

	// 초기화
	CurrentMontage = nullptr;
}

EBTNodeResult::Type UTUltimate::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	// 부모 클래스의 ExecuteTask를 먼저 호출합니다.
	Super::ExecuteTask(OwnerComp, NodeMemory);

	BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (!BlackboardComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("TUltimate: BlackboardComponent is missing"));
		return EBTNodeResult::Failed;
	}

	// AI 컨트롤러와 컨트롤러가 조종하는 캐릭터를 가져옵니다.
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController)
	{
		UE_LOG(LogTemp, Warning, TEXT("TUltimate: AIController is missing"));
		return EBTNodeResult::Failed;
	}

	ACharacter* Character = Cast<ACharacter>(AIController->GetPawn());
	if (!Character)
	{
		UE_LOG(LogTemp, Warning, TEXT("TUltimate: Character is missing"));
		return EBTNodeResult::Failed;
	}

	// Enemy_Base로 캐스팅
	AEnemy_Base* EnemyBase = Cast<AEnemy_Base>(Character);
	if (!EnemyBase)
	{
		UE_LOG(LogTemp, Warning, TEXT("TUltimate: EnemyBase casting failed"));
		return EBTNodeResult::Failed;
	}

	// DefaultFormDef 확인
	if (!EnemyBase->DefaultFormDef)
	{
		UE_LOG(LogTemp, Warning, TEXT("TUltimate: DefaultFormDef is missing"));
		return EBTNodeResult::Failed;
	}

	// 애님 인스턴스 가져오기
	UAnimInstance* AnimInstance = Character->GetMesh() ? Character->GetMesh()->GetAnimInstance() : nullptr;
	if (!AnimInstance)
	{
		UE_LOG(LogTemp, Warning, TEXT("TUltimate: AnimInstance is missing"));
		return EBTNodeResult::Failed;
	}

	// [추가] Special Enemy인 경우 델리게이트 구독
	if (AEnemy_Special* EnemySpecial = Cast<AEnemy_Special>(EnemyBase))
	{
		// Ultimate 스킬 컴포넌트 가져오기
		if (USkillBase* SkillBase = EnemyBase->Equipped.FindRef(ESkillSlot::Ultimate))
		{
			if (USkill_Ultimate* UltimateSkill = Cast<USkill_Ultimate>(SkillBase))
			{
				CachedSpecialUltimate = UltimateSkill;

				// 델리게이트 구독 (중복 방지를 위해 먼저 제거)
				UltimateSkill->OnSpecialUltimateCompleted.RemoveDynamic(
					this, &UTUltimate::OnSpecialUltimateCompleted);
				UltimateSkill->OnSpecialUltimateCompleted.AddDynamic(
					this, &UTUltimate::OnSpecialUltimateCompleted);

				UE_LOG(LogTemp, Log,
					TEXT("TUltimate: Subscribed to Special Ultimate completion delegate"));
			}
		}
	}

	// 스킬 활성화 (SkillSlotToActivate가 None이 아닌 경우)
	if (SkillSlotToActivate != ESkillSlot::Ultimate) // None 값이 없어서 Active가 아닌 경우로 체크
	{
		UE_LOG(LogTemp, Log, TEXT("TUltimate: No Ultimate slot specified"));
	}
	else
	{
		AEnemy_Base* Caster = Cast<AEnemy_Base>(AIController->GetPawn());
		if (!Caster) return EBTNodeResult::Failed;

		UE_LOG(LogTemp, Log, TEXT("TUltimate: Activating skill from Enemy_Base"));
		Caster->ActivateUltimate(); // Enemy_base의 얼티밋 스킬 실행
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

void UTUltimate::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	// 부모 클래스의 TickTask를 먼저 호출합니다.
	Super::TickTask(OwnerComp, NodeMemory, DeltaSeconds);

	// 유효성 검사
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		UE_LOG(LogTemp, Warning, TEXT("TUltimate: AIController is missing in TickTask"));
		return;
	}

	ACharacter* Character = Cast<ACharacter>(AIController->GetPawn());
	if (!Character)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		UE_LOG(LogTemp, Warning, TEXT("TUltimate: Character is missing in TickTask"));
		return;
	}

	// [수정] Special Enemy는 델리게이트로 완료 처리 (Tick에서 체크 안함)
	if (CachedSpecialUltimate.IsValid())
	{
		// Special은 OnSpecialUltimateCompleted 콜백에서 처리됨
		// 여기서는 스킬이 비정상적으로 중단되었는지만 체크
		if (!CachedSpecialUltimate->IsSpecialUltimateActive())
		{
			// 스킬이 이미 종료됨 (델리게이트가 호출되지 않았다면)
			UE_LOG(LogTemp, Warning,
				TEXT("TUltimate: Special Ultimate inactive but delegate not called"));

			if (BlackboardComp)
			{
				BlackboardComp->SetValueAsEnum("EnemyState", (uint8)EEnemyState::EES_Idle);
			}
			FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		}
		return;
	}

	UAnimInstance* AnimInstance = Character->GetMesh() ? Character->GetMesh()->GetAnimInstance() : nullptr;
	if (!AnimInstance)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		UE_LOG(LogTemp, Warning, TEXT("TUltimate: AnimInstance is missing in TickTask"));
		return;
	}

	AEnemy_Special* EnemySpecial = Cast<AEnemy_Special>(Character);
	if (!EnemySpecial)
	{
		// 몽타주가 재생 중인지 확인
		if (!AnimInstance->Montage_IsPlaying(CurrentMontage))
		{
			// 몽타주 재생이 끝나면 상태를 Idle로 변경하고 태스크 성공
			if (BlackboardComp)
			{
				BlackboardComp->SetValueAsEnum("EnemyState", (uint8)EEnemyState::EES_Idle);
			}
			FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
			UE_LOG(LogTemp, Log, TEXT("TUltimate: SkillMontage finished, returning to Idle"));
		}
	}
	else {
		UE_LOG(LogTemp, Log, TEXT("TUltimate: Special is not done by montage"));
	}
}

EBTNodeResult::Type UTUltimate::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	// [추가] 델리게이트 구독 해제
	if (CachedSpecialUltimate.IsValid())
	{
		if (USkill_Ultimate* UltSkill = CachedSpecialUltimate.Get())
		{
			UltSkill->OnSpecialUltimateCompleted.RemoveDynamic(
				this, &UTUltimate::OnSpecialUltimateCompleted);
		}
		CachedSpecialUltimate = nullptr;
	}

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
				UE_LOG(LogTemp, Log, TEXT("TUltimate: Skill aborted, stopping montage"));
			}
		}
	}

	// 상태 정리
	CurrentMontage = nullptr;

	return Super::AbortTask(OwnerComp, NodeMemory);
}

// [추가] Special 궁극기 완료 콜백
void UTUltimate::OnSpecialUltimateCompleted()
{
	UE_LOG(LogTemp, Log,
		TEXT("TUltimate: OnSpecialUltimateCompleted called - Finishing task"));

	// BehaviorTreeComponent 가져오기 (FinishLatentTask는 컴포넌트가 필요)
	if (CachedSpecialUltimate.IsValid())
	{
		AActor* Owner = CachedSpecialUltimate->GetOwner();
		if (APawn* PawnOwner = Cast<APawn>(Owner))
		{
			if (AAIController* AIController = Cast<AAIController>(PawnOwner->GetController()))
			{
				if (UBehaviorTreeComponent* BTComp = Cast<UBehaviorTreeComponent>(
					AIController->BrainComponent))
				{
					if (BlackboardComp)
					{
						BlackboardComp->SetValueAsEnum("EnemyState", (uint8)EEnemyState::EES_Idle);
					}

					// 태스크 완료
					FinishLatentTask(*BTComp, EBTNodeResult::Succeeded);

					// 델리게이트 구독 해제
					if (USkill_Ultimate* UltSkill = CachedSpecialUltimate.Get())
					{
						UltSkill->OnSpecialUltimateCompleted.RemoveDynamic(
							this, &UTUltimate::OnSpecialUltimateCompleted);
					}

					CachedSpecialUltimate = nullptr;
				}
			}
		}
	}
}