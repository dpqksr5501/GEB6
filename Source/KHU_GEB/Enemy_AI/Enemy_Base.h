// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Animation/AnimMontage.h"
#include "Enemy_Base.generated.h"

UCLASS()
class KHU_GEB_API AEnemy_Base : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AEnemy_Base();

	UPROPERTY(EditDefaultsOnly, Category = "Enemy Action") // 평타
	UAnimMontage* AttackMontage;
	UPROPERTY(EditDefaultsOnly, Category = "Enemy Action") // 기본스킬
	UAnimMontage* SkillMontage;
	UPROPERTY(EditDefaultsOnly, Category = "Enemy Action") // 특수스킬
	UAnimMontage* SpecialSkillMontage;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UFUNCTION(BlueprintCallable, Category = "Enemy Action")
	virtual void PerformAttack();
	UFUNCTION(BlueprintCallable, Category = "Enemy Action")
	virtual void PerformSkill();
	UFUNCTION(BlueprintCallable, Category = "Enemy Action")
	virtual void PerformSpecialSkill();
};
