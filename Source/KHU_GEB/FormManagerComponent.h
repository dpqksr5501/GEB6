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

	// === �� ���� ===
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Forms")
	EPlayerForm CurrentForm = EPlayerForm::Base;

	// �� �� ���� �������Ʈ Ŭ���� (�����Ϳ��� BP ����)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Forms")
	TMap<EPlayerForm, TSubclassOf<AKHU_GEBCharacter>> FormClasses;

	// �� �� �� ���� DataAsset (�����Ϳ��� 4�� DataAsset ����)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Forms")
	TMap<EPlayerForm, TSoftObjectPtr<UFormStatsData>> DefaultFormStats;

	// ����(���� ������)
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

public:
	// API
	UFUNCTION(BlueprintCallable, Category = "Forms") void InitializeForms(EPlayerForm StartForm);
	UFUNCTION(BlueprintCallable, Category = "Forms") void SwitchTo(EPlayerForm NewForm);

	// ���� I/O
	UFUNCTION(BlueprintCallable, Category = "Forms") FPlayerStateBundle BuildBundle() const;
	UFUNCTION(BlueprintCallable, Category = "Forms") void ApplyBundle(const FPlayerStateBundle& B);

	// ���� Owner(Character)�� ���� ��� �ݿ�
	UFUNCTION(BlueprintCallable, Category = "Forms") void ApplyStatsToOwner();
};