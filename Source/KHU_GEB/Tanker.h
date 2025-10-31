// Fill out your copyright notice in the Description page of Project Settings.
//Tanker.h

#pragma once

#include "CoreMinimal.h"
#include "MonsterBase.h"
#include "Tanker.generated.h"

/**
 * Tanker ĳ���� Ŭ����
 */
UCLASS()
class KHU_GEB_API ATanker : public AMonsterBase
{
    GENERATED_BODY()

public:
    // ������
    ATanker();

protected:
    /** ���� �������� ���θ� ��Ÿ���� ����. �������Ʈ���� �а� �� �� �ֵ��� �����մϴ�. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tanker|State", meta = (AllowPrivateAccess = "true"))
    bool bIsInvincible; // ���� ���� ���� ����


public:
    /** �������Ʈ���� ȣ���Ͽ� ���� ���¸� Ȱ��ȭ�ϴ� �Լ� */
    UFUNCTION(BlueprintCallable, Category = "Tanker|State")
    void ActivateInvincibility();

    /** �������Ʈ���� ȣ���Ͽ� ���� ���¸� ��Ȱ��ȭ�ϴ� �Լ� */
    UFUNCTION(BlueprintCallable, Category = "Tanker|State")
    void DeactivateInvincibility();

    /** �������Ʈ���� ���� ���� �������� Ȯ���ϴ� �Լ� */
    UFUNCTION(BlueprintPure, Category = "Tanker|State")
    bool IsInvincible() const;
};