// Fill out your copyright notice in the Description page of Project Settings.

#include "Enemy_AI/Task/TCombo_Attack.h"
#include "AIController.h"
#include "GameFramework/Character.h"
#include "Animation/AnimInstance.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Engine/World.h"
#include "Enemy_AI/Enemy_Base.h"
#include "WeaponComponent.h"
#include "FormDefinition.h"
#include "Animation/AnimNotifies/AnimNotify.h"

UTCombo_Attack::UTCombo_Attack()
{
	// 노드 이름을 "Combo Attack"으로 설정합니다.
	NodeName = TEXT("Combo Attack");

	// TickTask 함수가 호출되도록 bNotifyTick을 true로 설정합니다.
	bNotifyTick = true;

	// 초기화
	CurrentMontage = nullptr;
	CachedWeaponComp = nullptr;
	CachedEnemyBase = nullptr;
	CachedAnimInstance = nullptr;
	BlackboardComp = nullptr;
	
	CurrentComboIndex = 0;
	ExecutedComboCount = 0;
	bWaitingForNextCombo = false;
}

EBTNodeResult::Type UTCombo_Attack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	// 부모 클래스의 ExecuteTask를 먼저 호출합니다.
	Super::ExecuteTask(OwnerComp, NodeMemory);
	
	// 콤보 카운터 초기화
	CurrentComboIndex = 0;
	ExecutedComboCount = 0;
	bWaitingForNextCombo = false;
	
	BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (!BlackboardComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("TCombo_Attack: BlackboardComponent is missing"));
		return EBTNodeResult::Failed;
	}

	// AI 컨트롤러와 캐릭터를 가져옵니다
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController)
	{
		UE_LOG(LogTemp, Warning, TEXT("TCombo_Attack: AIController is missing"));
		return EBTNodeResult::Failed;
	}

	ACharacter* Character = Cast<ACharacter>(AIController->GetPawn());
	if (!Character)
	{
		UE_LOG(LogTemp, Warning, TEXT("TCombo_Attack: Character is missing"));
		return EBTNodeResult::Failed;
	}

	// Enemy_Base로 캐스팅하여 캐시
	CachedEnemyBase = Cast<AEnemy_Base>(Character);
	if (!CachedEnemyBase)
	{
		UE_LOG(LogTemp, Warning, TEXT("TCombo_Attack: Enemy_Base casting failed"));
		return EBTNodeResult::Failed;
	}

	// WeaponComponent를 가져와서 캐시
	CachedWeaponComp = CachedEnemyBase->FindComponentByClass<UWeaponComponent>();
	if (!CachedWeaponComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("TCombo_Attack: WeaponComponent is missing"));
		return EBTNodeResult::Failed;
	}

	// DefaultFormDef 확인
	if (!CachedEnemyBase->DefaultFormDef)
	{
		UE_LOG(LogTemp, Warning, TEXT("TCombo_Attack: DefaultFormDef is missing"));
		return EBTNodeResult::Failed;
	}

	// AttackMontages 배열 확인
	const TArray<FAttackStep>& AttackMontages = CachedEnemyBase->DefaultFormDef->AttackMontages;
	if (AttackMontages.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("TCombo_Attack: No AttackMontages found in DefaultFormDef"));
		return EBTNodeResult::Failed;
	}

	// AnimInstance 가져와서 캐시
	CachedAnimInstance = Character->GetMesh() ? Character->GetMesh()->GetAnimInstance() : nullptr;
	if (!CachedAnimInstance)
	{
		UE_LOG(LogTemp, Warning, TEXT("TCombo_Attack: AnimInstance is missing"));
		return EBTNodeResult::Failed;
	}

	// 애님 노티파이 델리게이트 바인딩
	CachedAnimInstance->OnPlayMontageNotifyBegin.RemoveDynamic(this, &UTCombo_Attack::OnNotifyBeginReceived);
	CachedAnimInstance->OnPlayMontageNotifyEnd.RemoveDynamic(this, &UTCombo_Attack::OnNotifyEndReceived);
	CachedAnimInstance->OnPlayMontageNotifyBegin.AddDynamic(this, &UTCombo_Attack::OnNotifyBeginReceived);
	CachedAnimInstance->OnPlayMontageNotifyEnd.AddDynamic(this, &UTCombo_Attack::OnNotifyEndReceived);

	// 상태를 공격 중으로 변경
	BlackboardComp->SetValueAsEnum("EnemyState", (uint8)EEnemyState::EES_Attacking);
	BlackboardComp->SetValueAsFloat(LastActionTimeKey.SelectedKeyName, OwnerComp.GetWorld()->GetTimeSeconds());
	
	UE_LOG(LogTemp, Log, TEXT("TCombo_Attack: Combo attack started (Target: %d, Available montages: %d)"), 
		TargetComboCount, AttackMontages.Num());

	// 첫 번째 콤보 공격 시작
	PlayNextComboAttack(OwnerComp);
	
	return EBTNodeResult::InProgress;
}

void UTCombo_Attack::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickTask(OwnerComp, NodeMemory, DeltaSeconds);

	// 유효성 검사
	if (!CachedAnimInstance || !CurrentMontage)
	{
		UE_LOG(LogTemp, Warning, TEXT("TCombo_Attack: Missing required references in TickTask"));
		CleanupReferences();
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	// 현재 몽타주가 재생 중인지 확인
	if (!CachedAnimInstance->Montage_IsPlaying(CurrentMontage))
	{
		// 몽타주 재생이 끝났습니다
		ExecutedComboCount++;
		
		UE_LOG(LogTemp, Log, TEXT("TCombo_Attack: Combo %d completed (Target: %d)"), 
			ExecutedComboCount, TargetComboCount);

		// 목표 콤보 횟수에 도달했는지 확인
		if (ExecutedComboCount >= TargetComboCount)
		{
			// 콤보 공격 완료
			UE_LOG(LogTemp, Log, TEXT("TCombo_Attack: All combo attacks completed!"));
			CleanupReferences();
			FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
			return;
		}

		// 아직 더 공격해야 함 - 다음 콤보로 진행
		CurrentComboIndex++;
		PlayNextComboAttack(OwnerComp);
	}
}

EBTNodeResult::Type UTCombo_Attack::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UE_LOG(LogTemp, Log, TEXT("TCombo_Attack: Task aborted (Progress: %d/%d)"), 
		ExecutedComboCount, TargetComboCount);

	// 현재 재생 중인 몽타주 중단
	if (CachedAnimInstance && CurrentMontage)
	{
		CachedAnimInstance->Montage_Stop(0.2f, CurrentMontage);
	}

	// WeaponComponent의 콜리전 강제 비활성화 (안전 장치)
	if (CachedWeaponComp)
	{
		CachedWeaponComp->DisableCollision();
		UE_LOG(LogTemp, Log, TEXT("TCombo_Attack: Disabled weapon collision on abort"));
	}

	CleanupReferences();
	return Super::AbortTask(OwnerComp, NodeMemory);
}

void UTCombo_Attack::PlayNextComboAttack(UBehaviorTreeComponent& OwnerComp)
{
	if (!CachedEnemyBase || !CachedAnimInstance)
	{
		UE_LOG(LogTemp, Warning, TEXT("TCombo_Attack: Missing required references in PlayNextComboAttack"));
		CleanupReferences();
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	// 현재 콤보 인덱스에 해당하는 몽타주 가져오기 (순환)
	UAnimMontage* NextMontage = GetMontageForCurrentCombo();
	if (!NextMontage)
	{
		UE_LOG(LogTemp, Warning, TEXT("TCombo_Attack: No montage found for combo index %d"), 
			CurrentComboIndex);
		CleanupReferences();
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	CurrentMontage = NextMontage;

	// 몽타주 재생
	float MontageLength = CachedAnimInstance->Montage_Play(CurrentMontage, 1.0f);
	if (MontageLength <= 0.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("TCombo_Attack: Failed to play montage"));
		CleanupReferences();
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("TCombo_Attack: Combo %d started (Montage index: %d, Remaining: %d)"), 
		ExecutedComboCount + 1, 
		CurrentComboIndex % CachedEnemyBase->DefaultFormDef->AttackMontages.Num(),
		TargetComboCount - ExecutedComboCount - 1);
}

UAnimMontage* UTCombo_Attack::GetMontageForCurrentCombo() const
{
	if (!CachedEnemyBase || !CachedEnemyBase->DefaultFormDef)
	{
		return nullptr;
	}

	const TArray<FAttackStep>& AttackMontages = CachedEnemyBase->DefaultFormDef->AttackMontages;
	if (AttackMontages.Num() == 0)
	{
		return nullptr;
	}

	// 순환 인덱스 계산 (예: 몽타주 3개, 인덱스 5 → 실제 인덱스 2)
	int32 ActualIndex = CurrentComboIndex % AttackMontages.Num();
	const FAttackStep& AttackStep = AttackMontages[ActualIndex];

	// FullBody 몽타주 반환 (AI는 항상 정지 상태에서 공격하므로 FullBody 사용)
	return AttackStep.Montage_FullBody;
}

void UTCombo_Attack::CleanupReferences()
{
	// 애님 노티파이 델리게이트 해제
	if (CachedAnimInstance)
	{
		CachedAnimInstance->OnPlayMontageNotifyBegin.RemoveDynamic(this, &UTCombo_Attack::OnNotifyBeginReceived);
		CachedAnimInstance->OnPlayMontageNotifyEnd.RemoveDynamic(this, &UTCombo_Attack::OnNotifyEndReceived);
	}

	// 참조 정리
	CurrentMontage = nullptr;
	CachedWeaponComp = nullptr;
	CachedEnemyBase = nullptr;
	CachedAnimInstance = nullptr;
	BlackboardComp = nullptr;
}

void UTCombo_Attack::OnNotifyBeginReceived(FName NotifyName, const FBranchingPointNotifyPayload& Payload)
{
	if (!CachedWeaponComp)
	{
		return;
	}

	// StartAttack 노티파이: 무기 콜리전 활성화
	if (NotifyName == TEXT("StartAttack"))
	{
		CachedWeaponComp->EnableCollision();
		UE_LOG(LogTemp, Log, TEXT("TCombo_Attack: StartAttack notify - Collision enabled (Combo %d/%d)"), 
			ExecutedComboCount + 1, TargetComboCount);
	}
	// EndAttack 노티파이: 무기 콜리전 비활성화
	else if (NotifyName == TEXT("EndAttack"))
	{
		CachedWeaponComp->DisableCollision();
		UE_LOG(LogTemp, Log, TEXT("TCombo_Attack: EndAttack notify - Collision disabled (Combo %d/%d)"), 
			ExecutedComboCount + 1, TargetComboCount);
	}
}

void UTCombo_Attack::OnNotifyEndReceived(FName NotifyName, const FBranchingPointNotifyPayload& Payload)
{
	// 필요시 구현 (현재는 사용하지 않음)
}

