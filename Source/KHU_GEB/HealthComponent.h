// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HealthComponent.generated.h"

class AActor;
class USkillBase;

// 스킬/공격에서 HealthComponent로 전달할 데미지 정보
USTRUCT(BlueprintType)
struct FDamageSpec
{
	GENERATED_BODY();

	// 스킬에서 계산한 “원래 데미지”
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float RawDamage = 0.f;

	// 방어력 무시 여부 (Swift, Special 도트 등)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIgnoreDefense = false;

	// 주기적(도트/힐틱) 데미지인지 여부
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bPeriodic = false;

	// 이 데미지가 콤보/다단히트 중 몇 타인지 표현하고 싶을 때
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 HitCount = 1;

	// 누가 때렸는지
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TWeakObjectPtr<AActor> Instigator;

	// 어떤 스킬에서 온 데미지인지
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TWeakObjectPtr<USkillBase> SourceSkill;
};

// HealthComponent가 최종적으로 적용한 데미지를 알려주는 이벤트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(
	FOnDamageApplied,
	float, NewHealth,          // 적용 후 체력
	float, FinalDamage,        // 최종 반영된 데미지
	float, RawDamage,          // 스킬에서 보낸 원본 데미지
	AActor*, InstigatorActor,  // 공격 주체
	USkillBase*, SourceSkill   // 공격한 스킬
);

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
	// 실제로 체력 감소를 적용하는 함수
	UFUNCTION(BlueprintCallable, Category = "Health")
	float ApplyDamageSpec(const FDamageSpec& Spec);

	UFUNCTION(BlueprintCallable, Category = "Health")
	void InitializeHealth(float InMaxHealth, float InStartHealth = -1.f);

	UFUNCTION(BlueprintCallable, Category = "Health")
	void ReduceHealth(float Amount);            // 체력 감소 (피해)

	UFUNCTION(BlueprintCallable, Category = "Health")
	void AddHealth(float Amount);               // 체력 회복

	UFUNCTION(BlueprintCallable, Category = "Health")
	void SetMaxHealth(float NewMaxHealth, bool bClampCurrentToNewMax = true);

	UFUNCTION(BlueprintPure, Category = "Health")
	float GetHealthPercent() const { return MaxHealth > 0.f ? Health / MaxHealth : 0.f; }


private:
	void ApplyHealth(float NewHealth, float Delta);
	void HandleDeathIfNeeded();



};
