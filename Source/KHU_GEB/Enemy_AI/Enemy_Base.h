// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Animation/AnimMontage.h"
#include "Skills/SkillDefinition.h"
#include "EnemyState.h"
#include "Enemy_Base.generated.h"

class UBlackboardComponent;
class UHealthComponent;
class UWeaponComponent;
class UWeaponData;
class UFormDefinition;
class UJumpComponent;
class USkillBase;

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

};