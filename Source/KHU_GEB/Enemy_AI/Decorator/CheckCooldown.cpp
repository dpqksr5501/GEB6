// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemy_AI/Decorator/CheckCooldown.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Engine/World.h" // GetTimeSeconds()를 위해 필요합니다.

UCheckCooldown::UCheckCooldown()
{
	// 노드 이름 설정
	NodeName = TEXT("Check Cooldown");

	// CompareValue의 기본값을 5초로 설정
	CompareValue = 5.0f;

	// 이 데코레이터는 블랙보드 '값'의 변경을 감지하는 것이 아니라
	// 실행 흐름이 도달했을 때 '시간'을 기준으로 체크합니다.
	//bObserveBlackboardValue = false;
}

bool UCheckCooldown::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	// 상위 클래스의 함수를 먼저 호출 (필수적인 초기화가 있을 수 있음)
	// UBTDecorator_BlackboardBase는 CalculateRawConditionValue가 아닌
	// PerformConditionCheck() 등에서 키 유효성을 검사하므로 직접 로직을 구현합니다.

	const UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (!BlackboardComp)
	{
		// 블랙보드 컴포넌트가 없으면 실패 처리
		return false;
	}

	// 에디터에서 선택된 블랙보드 키가 유효한지 확인
	if (!GetSelectedBlackboardKey().IsValid())
	{
		// 키가 선택되지 않았으면 실패 처리
		return false;
	}

	// 현재 게임 월드의 시간을 가져옵니다.
	const float CurrentTime = OwnerComp.GetWorld()->GetTimeSeconds();

	// 블랙보드에서 선택된 키(예: LastActionTime)의 값을 float으로 가져옵니다.
	const float LastTime = BlackboardComp->GetValueAsFloat(GetSelectedBlackboardKey());

	// 현재 시간과 마지막 실행 시간의 차이(경과 시간)를 계산합니다.
	const float ElapsedTime = CurrentTime - LastTime;

	// 경과 시간이 설정된 쿨타임(CompareValue)보다 크거나 같으면,
	// 쿨타임이 지난 것(true)으로 판단하여 데코레이터를 통과시킵니다.
	return ElapsedTime >= CompareValue;
}