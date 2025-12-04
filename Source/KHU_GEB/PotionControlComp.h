// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PotionControlComp.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPotionConsume);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class KHU_GEB_API UPotionControlComp : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UPotionControlComp();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;


	UPROPERTY(BlueprintAssignable, Category = "Health")
	FOnPotionConsume OnPotionConsume;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float PotionCount = 0;

	UFUNCTION(BlueprintCallable)
	void AddPotion();

	UFUNCTION(BlueprintCallable)
	void ConsumePotion();

		
};
