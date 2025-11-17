// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SkillDefinition.h"
#include "SkillBase.generated.h"

UCLASS( Abstract, Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class KHU_GEB_API USkillBase : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	USkillBase();

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	ESkillSlot Slot = ESkillSlot::Active;
/*
protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
*/
	// SkillDefinition에서 주입
	virtual void InitializeFromDefinition(const USkillDefinition* Def) { /* Params 복사 등 */ }

public:
	UFUNCTION(BlueprintCallable, Category = "Skill")
	virtual bool CanActivate() const { return true; }

	UFUNCTION(BlueprintCallable, Category = "Skill")
	virtual void ActivateSkill();

	UFUNCTION(BlueprintCallable, Category = "Skill")
	virtual void StopSkill();
};