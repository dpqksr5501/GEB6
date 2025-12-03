// Fill out your copyright notice in the Description page of Project Settings.😄

#include "Tanker.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

// 생성자
ATanker::ATanker()
{
    bIsInvincible = false;
}

// 무적 활성화 함수 구현
void ATanker::ActivateInvincibility()
{
    bIsInvincible = true;
    UE_LOG(LogTemp, Log, TEXT("Tanker %s activated invincibility."), *GetName());
}

// 무적 비활성화 함수 구현
void ATanker::DeactivateInvincibility()
{
    bIsInvincible = false;
    UE_LOG(LogTemp, Log, TEXT("Tanker %s deactivated invincibility."), *GetName());
}

// 현재 무적 상태인지 반환하는 함수 구현
bool ATanker::IsInvincible() const
{
    return bIsInvincible;
}