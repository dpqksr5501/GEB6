// Fill out your copyright notice in the Description page of Project Settings.


#include "MonsterAnimInstanceBase.h"
#include "Animation/AnimInstance.h"
#include "MonsterBase.h"

void UMonsterAnimInstanceBase::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	OwningMonster = Cast<AMonsterBase>(TryGetPawnOwner());
}

void UMonsterAnimInstanceBase::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (OwningMonster)
	{
		Speed = OwningMonster->GetVelocity().Size();
		CharacterState = OwningMonster->GetCharacterState();
	}
}