#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
// ECharacterState 열거형을 사용하기 위해 MonsterAnimInstanceBase.h를 포함합니다.
#include "MonsterAnimInstanceBase.h" 
#include "MyAnimDataProvider.generated.h"

UINTERFACE(MinimalAPI)
class UMyAnimDataProvider : public UInterface
{
    GENERATED_BODY()
};

/**
 * 애님 인스턴스에 필수 데이터를 제공하는 모든 클래스(플레이어, 몬스터 등)가
 * 반드시 구현해야 하는 함수의 목록입니다.
 */
class KHU_GEB_API IMyAnimDataProvider
{
    GENERATED_BODY()

public:
    // 애님 인스턴스가 이 함수들을 호출할 것입니다.

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "AnimDataProvider")
    float GetAnimSpeed() const;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "AnimDataProvider")
    ECharacterState GetAnimCharacterState() const;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "AnimDataProvider")
    bool GetAnimIsFalling() const;

    /**
     * 방금 점프 입력이 있었는지 반환합니다.
     * @param bConsumeInput true일 경우, 신호를 읽은 후 리셋(소모)합니다.
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "AnimDataProvider")
    bool GetAnimJumpInput(bool bConsumeInput);
};