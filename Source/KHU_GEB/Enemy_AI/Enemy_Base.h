// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Animation/AnimMontage.h"
#include "SkillDefinition.h"
#include "SkillBase.h"
#include "EnemyState.h"
#include "Enemy_Base.generated.h"

class UBlackboardComponent;
class UHealthComponent;
class UWeaponComponent;
class UWeaponData;
class UFormDefinition;

UCLASS()
class KHU_GEB_API AEnemy_Base : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AEnemy_Base();

	// 블루프린트에서 직접 스킬 인스턴스를 할당 (런타임에 이미 생성된 스킬 객체)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
	TMap<ESkillSlot, TObjectPtr<USkillBase>> Equipped;

	// 블루프린트에서 스킬 클래스를 설정 (생성할 스킬의 클래스 타입)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
	TMap<ESkillSlot, TSubclassOf<USkillBase>> SkillClasses;

	// SkillClasses를 기반으로 Equipped 맵 초기화
	UFUNCTION(BlueprintCallable, Category = "Skill")
	void InitializeSkills();

	// 무기 데이터
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	TObjectPtr<UWeaponData> DefaultWeaponData;

	// 폼 데이터
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	TObjectPtr<UFormDefinition> DefaultFormDef;

	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// BlackboardComp를 AI 컨트롤러에서 할당해 줄 수 있도록 UPROPERTY 설정
	UPROPERTY(BlueprintReadWrite, Category = "AI")
	UBlackboardComponent* BlackboardComp;

	UPROPERTY(BlueprintReadWrite, Category = "Health")
	UHealthComponent* HealthComp;

	UPROPERTY(BlueprintReadWrite, Category = "Weapon")
	UWeaponComponent* WeaponComp;

	// 죽음
	UFUNCTION()
	void OnDeath();
};
