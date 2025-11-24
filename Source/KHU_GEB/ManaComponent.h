// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ManaComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnManaChanged, float, CurrentMana, float, MaxMana);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class KHU_GEB_API UManaComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UManaComponent();

	/** 현재 최대 마나 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mana")
	float MaxMana = 100.f;

	/** 초당 회복되는 마나량 (0이면 회복 안 함) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mana")
	float RegenPerSecond = 5.f;

	/** 현재 마나 (런타임 전용) */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Mana")
	float CurrentMana;

	/** 마나가 변경되었을 때 브로드캐스트 (UI 갱신 용도 등) */
	UPROPERTY(BlueprintAssignable, Category = "Mana")
	FOnManaChanged OnManaChanged;

private:
	/** 0보다 크면 자연 회복을 하지 않음 (참조 카운트 방식) */
	int32 RegenBlockCount = 0;
	bool IsRegenBlocked() const { return RegenBlockCount > 0; }

protected:
	virtual void BeginPlay() override;

	/** 내부에서 Current/Max가 바뀔 때마다 불러서 델리게이트 브로드캐스트 */
	void HandleManaChanged();

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** 마나 재생 차단 카운터 */
	void AddRegenBlock();
	void RemoveRegenBlock();

	/** 최대 마나를 설정 (옵션으로 현재 마나도 같이 리셋) */
	UFUNCTION(BlueprintCallable, Category = "Mana")
	void SetMaxMana(float NewMaxMana, bool bResetCurrent = true);

	/** 양수면 회복, 음수면 감소 (Clamp 0 ~ MaxMana) */
	UFUNCTION(BlueprintCallable, Category = "Mana")
	void AddMana(float DeltaMana);

	/** Amount만큼 마나를 소모. 부족하면 false 리턴 */
	UFUNCTION(BlueprintCallable, Category = "Mana")
	bool ConsumeMana(float Amount);

	/** Amount만큼 쓸 수 있는지 여부만 확인 */
	UFUNCTION(BlueprintPure, Category = "Mana")
	bool HasEnoughMana(float Amount) const;

	UFUNCTION(BlueprintPure, Category = "Mana")
	float GetCurrentMana() const { return CurrentMana; }

	UFUNCTION(BlueprintPure, Category = "Mana")
	float GetManaRatio() const { return (MaxMana > 0.f) ? (CurrentMana / MaxMana) : 0.f; }

};