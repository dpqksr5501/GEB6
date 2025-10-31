// Fill out your copyright notice in the Description page of Project Settings.

#include "Tanker.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

// ������
ATanker::ATanker()
{
    bIsInvincible = false;
}

// ���� Ȱ��ȭ �Լ� ����
void ATanker::ActivateInvincibility()
{
    bIsInvincible = true;
    UE_LOG(LogTemp, Log, TEXT("Tanker %s activated invincibility."), *GetName());
}

// ���� ��Ȱ��ȭ �Լ� ����
void ATanker::DeactivateInvincibility()
{
    bIsInvincible = false;
    UE_LOG(LogTemp, Log, TEXT("Tanker %s deactivated invincibility."), *GetName());
}

// ���� ���� �������� ��ȯ�ϴ� �Լ� ����
bool ATanker::IsInvincible() const
{
    return bIsInvincible;
}