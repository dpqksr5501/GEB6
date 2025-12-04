// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HealthComponent.generated.h"

class AActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FOnDamageApplied, float, NewHealth, float, RawDamage,
	float, FinalDamage, AActor*, InstigatorActor, AActor*, DamageCauser);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHealthChanged, float, NewHealth, float, Delta);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDeath);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class KHU_GEB_API UHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UHealthComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxHealth;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Health;

	UPROPERTY(BlueprintAssignable, Category = "Health")
	FOnHealthChanged OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "Health")
	FOnDeath OnDeath;

	UPROPERTY(BlueprintAssignable, Category = "Health")
	FOnDamageApplied OnDamageApplied;

public:
	UFUNCTION(BlueprintCallable, Category = "Health")
	void InitializeHealth(float InMaxHealth, float InStartHealth = -1.f);

	/** 전투 로직에서 사용하는 유일한 데미지 진입점 (Actor에서만 호출) */
	UFUNCTION(BlueprintCallable, Category = "Health")
	float ApplyDamage(
		float   RawDamage,
		AActor* InstigatorActor = nullptr,
		AActor* DamageCauser = nullptr
	);

	UFUNCTION(BlueprintCallable, Category = "Health")
	void ReduceHealth(float Amount);	// 체력 감소 (피해)

	UFUNCTION(BlueprintCallable, Category = "Health")
	void AddHealth(float Amount);		// 체력 회복

	UFUNCTION(BlueprintCallable, Category = "Health")
	void SetMaxHealth(float NewMaxHealth, bool bClampCurrentToNewMax = true);

	UFUNCTION(BlueprintPure, Category = "Health")
	float GetHealthPercent() const { return MaxHealth > 0.f ? Health / MaxHealth : 0.f; }

	UFUNCTION(BlueprintPure, Category = "Health")
	bool IsDead() const;

private:
	void ApplyHealth(float NewHealth, float Delta);
	void HandleDeathIfNeeded();

};
