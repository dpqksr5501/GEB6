// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "EnemyState.h"
#include "TCombo_Attack.generated.h"

class UWeaponComponent;
class AEnemy_Base;

/**
 * AI가 WeaponComponent를 사용하여 콤보 공격을 수행하는 태스크입니다.
 * DefaultFormDef의 공격 몽타주들을 순차적으로 재생하며, 지정된 콤보 횟수만큼 공격합니다.
 * 
 * 예시: 
 * - 몽타주가 3개(1타, 2타, 3타)이고 TargetComboCount가 5이면: 1→2→3→1→2 순서로 공격
 * - 몽타주가 3개이고 TargetComboCount가 6이면: 1→2→3→1→2→3 순서로 공격
 */
UCLASS()
class KHU_GEB_API UTCombo_Attack : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UTCombo_Attack();

	/** 
	 * [에디터 노출] 수행할 콤보 공격 횟수
	 * 이 값만큼 공격을 순차적으로 실행합니다.
	 * 예: 3이면 3번 공격, 5이면 5번 공격 (몽타주가 모자라면 순환)
	 */
	UPROPERTY(EditAnywhere, Category = "Combo Attack", meta = (ClampMin = "1", ClampMax = "10"))
	int32 TargetComboCount = 3;

	/** 블랙보드 키 설정 */
	UPROPERTY(EditAnywhere, Category = "Combo Attack")
	FBlackboardKeySelector LastActionTimeKey;

protected:
	/** 태스크가 시작될 때 호출됩니다. 첫 번째 콤보 공격 몽타주를 재생합니다. */
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	/** 태스크가 실행 중인 동안 매 틱 호출됩니다. 몽타주 종료 후 다음 콤보로 진행합니다. */
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	/** 태스크가 중단될 때 호출됩니다. */
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	/** 애님 노티파이 Begin 수신 (StartAttack, EndAttack 등) */
	UFUNCTION()
	void OnNotifyBeginReceived(FName NotifyName, const FBranchingPointNotifyPayload& Payload);

	/** 애님 노티파이 End 수신 (필요시 확장용) */
	UFUNCTION()
	void OnNotifyEndReceived(FName NotifyName, const FBranchingPointNotifyPayload& Payload);

private:
	/** 다음 콤보 공격 몽타주를 재생합니다 */
	void PlayNextComboAttack(UBehaviorTreeComponent& OwnerComp);

	/** 현재 콤보 인덱스에 해당하는 몽타주를 가져옵니다 (순환 방식) */
	UAnimMontage* GetMontageForCurrentCombo() const;

	/** 모든 참조와 델리게이트를 정리합니다 */
	void CleanupReferences();

private:
	/** 블랙보드 컴포넌트 참조 */
	UPROPERTY()
	UBlackboardComponent* BlackboardComp;

	/** 현재 재생 중인 몽타주 */
	UPROPERTY()
	UAnimMontage* CurrentMontage;

	/** WeaponComponent 참조 (캐시용) */
	UPROPERTY()
	UWeaponComponent* CachedWeaponComp;

	/** Enemy_Base 참조 (캐시용) */
	UPROPERTY()
	AEnemy_Base* CachedEnemyBase;

	/** AnimInstance 참조 (캐시용) */
	UPROPERTY()
	UAnimInstance* CachedAnimInstance;

	/** 현재 몇 번째 콤보를 실행 중인지 (0부터 시작) */
	int32 CurrentComboIndex;

	/** 지금까지 수행한 콤보 공격 횟수 */
	int32 ExecutedComboCount;

	/** 현재 몽타주가 재생 완료되었는지 여부 */
	bool bWaitingForNextCombo;
};
