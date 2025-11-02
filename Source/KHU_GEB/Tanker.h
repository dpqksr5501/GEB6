// Fill out your copyright notice in the Description page of Project Settings.
//Tanker.h

#pragma once

#include "CoreMinimal.h"
#include "MonsterBase.h"
#include "Tanker.generated.h"

/**
 * Tanker 캐릭터 클래스
 */
UCLASS()
class KHU_GEB_API ATanker : public AMonsterBase
{
    GENERATED_BODY()

public:
    // 생성자
    ATanker();

protected:
    /** 무적 상태인지 여부를 나타내는 변수. 블루프린트에서 읽고 쓸 수 있도록 설정합니다. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tanker|State", meta = (AllowPrivateAccess = "true"))
    bool bIsInvincible; // 무적 상태 변수 선언


public:
    /** 블루프린트에서 호출하여 무적 상태를 활성화하는 함수 */
    UFUNCTION(BlueprintCallable, Category = "Tanker|State")
    void ActivateInvincibility();

    /** 블루프린트에서 호출하여 무적 상태를 비활성화하는 함수 */
    UFUNCTION(BlueprintCallable, Category = "Tanker|State")
    void DeactivateInvincibility();

    /** 블루프린트에서 현재 무적 상태인지 확인하는 함수 */
    UFUNCTION(BlueprintPure, Category = "Tanker|State")
    bool IsInvincible() const;
};