// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FormDefinition.h"
#include "FormManagerComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnFormChanged, EFormType, NewForm, const UFormDefinition*, Def);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class KHU_GEB_API UFormManagerComponent : public UActorComponent
{
	GENERATED_BODY()

	UPROPERTY(Transient) TWeakObjectPtr<class USkeletalMeshComponent> CachedMesh;

public:
	UFormManagerComponent();

	const UFormDefinition* FindDef(EFormType T) const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Form")
	TObjectPtr<UFormSet> FormSet;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Form")
	EFormType CurrentForm = EFormType::Base;

	UPROPERTY(BlueprintAssignable) FOnFormChanged OnFormChanged;

private:
	
	void ApplyMesh(const UFormDefinition* Def);	

	void FindMesh();

public:
	UFUNCTION(BlueprintCallable, Category = "Form")
	void InitializeForms();

	UFUNCTION(BlueprintCallable, Category = "Form")
	void SwitchTo(EFormType NewForm);

	// 현재 폼 타입을 반환하는 헬퍼 함수 (타격 효과음을 위한 헬퍼 함수)
	UFUNCTION(BlueprintPure, Category = "Form")
	EFormType GetCurrentFormType() const { return CurrentForm; }
};