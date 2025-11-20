// Fill out your copyright notice in the Description page of Project Settings.

#include "Enemy_AI/Task/TAttack.h"
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

UTAttack::UTAttack()
{
	// 노드 이름을 "Attack"으로 설정합니다.
	NodeName = TEXT("Attack");

	// TickTask 함수가 호출되도록 bNotifyTick을 true로 설정합니다.
	bNotifyTick = true;

	// 초기화
	CurrentMontage = nullptr;
	CachedWeaponComp = nullptr;
}

EBTNodeResult::Type UTAttack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	// 부모 클래스의 ExecuteTask를 먼저 호출합니다.
	Super::ExecuteTask(OwnerComp, NodeMemory);
	
	BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (!BlackboardComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("TAttack: BlackboardComponent is missing"));
		return EBTNodeResult::Failed;
	}

	// AI 컨트롤러와 컨트롤러가 빙의 중인 캐릭터를 가져옵니다.
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController)
	{
		UE_LOG(LogTemp, Warning, TEXT("TAttack: AIController is missing"));
		return EBTNodeResult::Failed;
	}

	ACharacter* Character = Cast<ACharacter>(AIController->GetPawn());
	if (!Character)
	{
		UE_LOG(LogTemp, Warning, TEXT("TAttack: Character is missing"));
		return EBTNodeResult::Failed;
	}

	// Enemy_Base로 캐스팅
	AEnemy_Base* EnemyBase = Cast<AEnemy_Base>(Character);
	if (!EnemyBase)
	{
		UE_LOG(LogTemp, Warning, TEXT("TAttack: EnemyBase casting failed"));
		return EBTNodeResult::Failed;
	}

	// WeaponComponent를 가져와서 캐시
	CachedWeaponComp = EnemyBase->FindComponentByClass<UWeaponComponent>();
	if (!CachedWeaponComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("TAttack: WeaponComponent is missing"));
		return EBTNodeResult::Failed;
	}

	// DefaultFormDef 확인
	if (!EnemyBase->DefaultFormDef)
	{
		UE_LOG(LogTemp, Warning, TEXT("TAttack: DefaultFormDef is missing"));
		return EBTNodeResult::Failed;
	}

	// AttackMontages 배열 확인
	const TArray<FAttackStep>& AttackMontages = EnemyBase->DefaultFormDef->AttackMontages;
	if (AttackMontages.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("TAttack: No AttackMontages found in DefaultFormDef"));
		return EBTNodeResult::Failed;
	}

	// 첫 번째 공격 몽타주의 FullBody 버전 가져오기
	const FAttackStep& FirstAttack = AttackMontages[0];
	CurrentMontage = FirstAttack.Montage_FullBody;

	if (!CurrentMontage)
	{
		UE_LOG(LogTemp, Warning, TEXT("TAttack: Montage_FullBody is missing in first AttackStep"));
		return EBTNodeResult::Failed;
	}

	// 애님 인스턴스 가져오기
	UAnimInstance* AnimInstance = Character->GetMesh() ? Character->GetMesh()->GetAnimInstance() : nullptr;
	if (!AnimInstance)
	{
		UE_LOG(LogTemp, Warning, TEXT("TAttack: AnimInstance is missing"));
		return EBTNodeResult::Failed;
	}

	// 기존 바인딩 제거 (중복 방지)
	AnimInstance->OnPlayMontageNotifyBegin.RemoveDynamic(this, &UTAttack::OnNotifyBeginReceived);
	AnimInstance->OnPlayMontageNotifyEnd.RemoveDynamic(this, &UTAttack::OnNotifyEndReceived);

	// 새로 바인딩
	AnimInstance->OnPlayMontageNotifyBegin.AddDynamic(this, &UTAttack::OnNotifyBeginReceived);
	AnimInstance->OnPlayMontageNotifyEnd.AddDynamic(this, &UTAttack::OnNotifyEndReceived);

	// 몽타주 재생
	float MontageLength = AnimInstance->Montage_Play(CurrentMontage, 1.0f);
	if (MontageLength <= 0.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("TAttack: Failed to play montage"));
		return EBTNodeResult::Failed;
	}

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

	// 유효성 검사
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		UE_LOG(LogTemp, Warning, TEXT("TAttack: AIController is missing in TickTask"));
		return;
	}

	ACharacter* Character = Cast<ACharacter>(AIController->GetPawn());
	if (!Character)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		UE_LOG(LogTemp, Warning, TEXT("TAttack: Character is missing in TickTask"));
		return;
	}

	UAnimInstance* AnimInstance = Character->GetMesh() ? Character->GetMesh()->GetAnimInstance() : nullptr;
	if (!AnimInstance)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		UE_LOG(LogTemp, Warning, TEXT("TAttack: AnimInstance is missing in TickTask"));
		return;
	}

	if (!CurrentMontage)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		UE_LOG(LogTemp, Warning, TEXT("TAttack: CurrentMontage is missing in TickTask"));
		return;
	}

	// 몽타주가 재생 중인지 확인
	if (!AnimInstance->Montage_IsPlaying(CurrentMontage))
	{
		// 몽타주 종료 시 델리게이트 정리
		AnimInstance->OnPlayMontageNotifyBegin.RemoveDynamic(this, &UTAttack::OnNotifyBeginReceived);
		AnimInstance->OnPlayMontageNotifyEnd.RemoveDynamic(this, &UTAttack::OnNotifyEndReceived);

		// 몽타주 재생이 끝났으므로 태스크 완료
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}

EBTNodeResult::Type UTAttack::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	// AI 컨트롤러와 캐릭터를 가져옵니다.
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (AIController)
	{
		ACharacter* Character = Cast<ACharacter>(AIController->GetPawn());
		if (Character)
		{
			UAnimInstance* AnimInstance = Character->GetMesh() ? Character->GetMesh()->GetAnimInstance() : nullptr;
			if (AnimInstance && CurrentMontage)
			{
				// 델리게이트 해제 (AbortTask에서도 정리)
				AnimInstance->OnPlayMontageNotifyBegin.RemoveDynamic(this, &UTAttack::OnNotifyBeginReceived);
				AnimInstance->OnPlayMontageNotifyEnd.RemoveDynamic(this, &UTAttack::OnNotifyEndReceived);

				// 몽타주 중단
				AnimInstance->Montage_Stop(0.2f, CurrentMontage);
				UE_LOG(LogTemp, Log, TEXT("TAttack: Attack aborted, stopping montage"));
			}

			// WeaponComponent의 콜리전 강제 비활성화 (안전 장치)
			if (CachedWeaponComp)
			{
				CachedWeaponComp->DisableCollision();
				UE_LOG(LogTemp, Log, TEXT("TAttack: Disabled weapon collision on abort"));
			}
		}
	}

	// 변수 정리
	CurrentMontage = nullptr;
	CachedWeaponComp = nullptr;

	return Super::AbortTask(OwnerComp, NodeMemory);
}

// Notify 처리 함수들
UFUNCTION()
void UTAttack::OnNotifyBeginReceived(FName NotifyName, const FBranchingPointNotifyPayload& Payload)
{
    if (!CachedWeaponComp) return;

    if (NotifyName == TEXT("StartAttack"))
    {
        CachedWeaponComp->EnableCollision();
        UE_LOG(LogTemp, Log, TEXT("TAttack: StartAttack notify - Collision enabled"));
    }
    else if (NotifyName == TEXT("EndAttack"))
    {
        CachedWeaponComp->DisableCollision();
        UE_LOG(LogTemp, Log, TEXT("TAttack: EndAttack notify - Collision disabled"));
    }
}

UFUNCTION()
void UTAttack::OnNotifyEndReceived(FName NotifyName, const FBranchingPointNotifyPayload& Payload)
{
    // 필요시 구현
}