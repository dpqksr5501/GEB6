// Fill out your copyright notice in the Description page of Project Settings.

#include "Enemy_AI/Task/TBlink.h"
#include "AIController.h"
#include "GameFramework/Character.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Enemy_AI/Enemy_Base.h"
#include "JumpComponent.h"

UTBlink::UTBlink()
{
	NodeName = TEXT("Blink");
	bNotifyTick = false; // Blink는 즉시 완료되므로 Tick 불필요

	CachedCharacter = nullptr;
	CachedJumpComp = nullptr;
}

EBTNodeResult::Type UTBlink::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	Super::ExecuteTask(OwnerComp, NodeMemory);

	// 1. AI 컨트롤러 획득
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TBlink] AIController is missing"));
		return EBTNodeResult::Failed;
	}

	// 2. 캐릭터 획득
	CachedCharacter = Cast<ACharacter>(AIController->GetPawn());
	if (!CachedCharacter)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TBlink] Character is missing"));
		return EBTNodeResult::Failed;
	}

	// 3. Enemy_Base로 캐스팅하여 JumpComponent 획득
	if (AEnemy_Base* Enemy = Cast<AEnemy_Base>(CachedCharacter))
	{
		CachedJumpComp = Enemy->JumpComp;
		if (!CachedJumpComp)
		{
			UE_LOG(LogTemp, Error, TEXT("[TBlink] JumpComponent is null! Enemy: %s"), 
				*Enemy->GetName());
			return EBTNodeResult::Failed;
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[TBlink] Character is not Enemy_Base! Class: %s"), 
			*CachedCharacter->GetClass()->GetName());
		return EBTNodeResult::Failed;
	}

	// 4. 땅 위에서만 실행 가능
	if (!CachedJumpComp->IsOnGround())
	{
		UE_LOG(LogTemp, Warning, TEXT("[TBlink] Character is not on ground"));
		return EBTNodeResult::Failed;
	}

	// 5. JumpComponent의 HandleSpacePressed 호출 (폼에 맞는 블링크 실행)
	// CurrentForm이 Special이므로 내부적으로 HandleSpecialPressed가 호출됨
	// HandleSpecialPressed는:
	// - 쿨타임 체크 (bCanSpecialBlink)
	// - 앞으로 BlinkDistance만큼 텔레포트
	// - 성공/실패 여부는 내부에서 처리
	CachedJumpComp->HandleSpacePressed();
	
	UE_LOG(LogTemp, Log, TEXT("[TBlink] Blink executed via JumpComponent"));
	
	// Blink는 즉시 완료
	// HandleSpecialPressed 내부에서 쿨타임 체크 및 텔레포트 실행
	// 쿨타임 중이거나 실패해도 이미 처리됨
	return EBTNodeResult::Succeeded;
}

