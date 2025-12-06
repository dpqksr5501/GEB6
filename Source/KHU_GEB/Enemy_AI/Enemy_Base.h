// Fill out your copyright notice in the Description page of Project Settings.

#pragma once


#include "CoreMinimal.h"
#include "Delegates/DelegateCombinations.h"
#include "GameFramework/Character.h"
#include "Animation/AnimMontage.h"
#include "Skills/SkillDefinition.h"
#include "EnemyState.h"
#include "FormDefinition.h"
#include "StatManagerComponent.h"
#include "Enemy_Base.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEnemyDiedDelegate); // 보스가 죽었는지 확인하는 델리게이트

class UBlackboardComponent;
class UHealthComponent;
class UWeaponComponent;
class UWeaponData;
class UFormDefinition;
class UJumpComponent;
class USkillBase;
class UCrowdControlComponent;
class AKHU_GEBCharacter;



UCLASS()
class KHU_GEB_API AEnemy_Base : public ACharacter
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh", meta = (AllowPrivateAccess = "true"))
	USceneComponent* MeshRoot;

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

	// 각 적 타입이 자신의 스킬을 발동하도록 오버라이드
	UFUNCTION(BlueprintCallable, Category = "Skill")
	virtual void ActivateSkill();

	// 각 적 타입이 자신의 궁극기를 발동하도록 오버라이드
	UFUNCTION(BlueprintCallable, Category = "Skill")
	virtual void ActivateUltimate();

	// 무기 데이터
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	TObjectPtr<UWeaponData> DefaultWeaponData;

	// 폼 데이터
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Form")
	TObjectPtr<UFormDefinition> DefaultFormDef;

	// 점프 컴포넌트
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jump")
	UJumpComponent* JumpComp;

	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	//죽었을 때 방송할 델리게이트
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnEnemyDiedDelegate OnEnemyDied;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(BlueprintReadWrite, Category = "Health")
	UHealthComponent* HealthComp;

	UPROPERTY(BlueprintReadWrite, Category = "Weapon")
	UWeaponComponent* WeaponComp;

public:
	// 죽음
	UFUNCTION(BlueprintCallable)
	virtual void OnDeath();

	// 상대가 적인지 판정하는 함수
	UFUNCTION(BlueprintPure, Category = "Combat")
	bool IsEnemyFor(const AActor* Other) const;

	FORCEINLINE USceneComponent* GetMeshRoot() const { return MeshRoot; }

	// 현재 AI가 노리고 있는 타겟 (없으면 nullptr)
	UFUNCTION(BlueprintPure, Category = "AI")
	AActor* GetCurrentTarget() const;

	// BlackboardComp를 AI 컨트롤러에서 할당해 줄 수 있도록 UPROPERTY 설정
	UPROPERTY(BlueprintReadWrite, Category = "AI")
	UBlackboardComponent* BlackboardComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CC")
	UCrowdControlComponent* CrowdControlComp;

	// 적 레벨(BP에서 적 타입별로 설정)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	int32 EnemyLevel = 1;

	// 이 적의 런타임 스탯 (공격/방어/이동속도 등)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	FFormRuntimeStats EnemyStats;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Form")
	EFormType EnemyFormType = EFormType::Base;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	EEnemyKind EnemyKind = EEnemyKind::Minion;

	// 런타임 Attack/Defense를 손쉽게 가져다 쓰기 위한 헬퍼
	UFUNCTION(BlueprintPure, Category = "Stats")
	float GetAttackStat() const { return EnemyStats.Attack; }

	UFUNCTION(BlueprintPure, Category = "Stats")
	float GetDefenseStat() const { return EnemyStats.Defense; }

	UFUNCTION()
	void SetLevel(int32 NewLevel);

protected:
	void HandleKilledBy(AActor* Killer);

};