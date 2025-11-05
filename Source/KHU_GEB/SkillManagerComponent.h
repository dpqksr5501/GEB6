// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SkillDefinition.h"
#include "SkillManagerComponent.generated.h"

class USkillBase;
class USkillSet;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class KHU_GEB_API USkillManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	USkillManagerComponent();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TMap<ESkillSlot, TObjectPtr<USkillBase>> Equipped;
/*
protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
*/
public:
	UFUNCTION(BlueprintCallable)
	void EquipFromSkillSet(USkillSet* Set);

	UFUNCTION(BlueprintCallable)
	void ClearAll();

	// 입력 바인딩에서 호출
	UFUNCTION(BlueprintCallable) bool TryActivate(ESkillSlot Slot);
	UFUNCTION(BlueprintCallable) bool TryStop(ESkillSlot Slot);
};
