// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PlayerEnums.h"
#include "FormManagerComponent.generated.h"

class AKHU_GEBCharacter;
class UPlayerStatsComponent;
class UFormStatsData;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class KHU_GEB_API UFormManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UFormManagerComponent();

	// === 폼 상태 ===
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Forms")
	EPlayerForm CurrentForm = EPlayerForm::Base;

	// 폼 → 폼용 블루프린트 클래스 (에디터에서 BP 지정)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Forms")
	TMap<EPlayerForm, TSubclassOf<AKHU_GEBCharacter>> FormClasses;

	// 폼 → 폼 스탯 DataAsset (에디터에서 4개 DataAsset 지정)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Forms")
	TMap<EPlayerForm, TSoftObjectPtr<UFormStatsData>> DefaultFormStats;

	// 스탯(공통 소유자)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	UPlayerStatsComponent* Stats;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	// virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	AKHU_GEBCharacter* GetOwnerChar() const;
	FTransform GetSafeSpawnTM(const FTransform& Around) const;
	FTransform GetInPlaceOrNearbyTM(AKHU_GEBCharacter* OldPawn, TSubclassOf<AKHU_GEBCharacter> NewCls) const;

public:
	// API
	UFUNCTION(BlueprintCallable, Category = "Forms") void InitializeForms(EPlayerForm StartForm);
	UFUNCTION(BlueprintCallable, Category = "Forms") void SwitchTo(EPlayerForm NewForm);

	// 번들 I/O
	UFUNCTION(BlueprintCallable, Category = "Forms") FPlayerStateBundle BuildBundle() const;
	UFUNCTION(BlueprintCallable, Category = "Forms") void ApplyBundle(const FPlayerStateBundle& B);

	// 현재 Owner(Character)에 스탯 즉시 반영
	UFUNCTION(BlueprintCallable, Category = "Forms") void ApplyStatsToOwner();
};