// Fill out your copyright notice in the Description page of Project Settings.

// SpecialOrb.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SpecialOrb.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UHealthComponent;
class AKHU_GEBCharacter;
class UNiagaraComponent;
class UNiagaraSystem;

UCLASS()
class KHU_GEB_API ASpecialOrb : public AActor
{
    GENERATED_BODY()

public:
    ASpecialOrb();

protected:
    /** 루트 컴포넌트 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USceneComponent* Root;

    /** 충돌용 컴포넌트 (오브 크기) */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USphereComponent* Collision;

    /** 비주얼용 메쉬 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* Mesh;

    /** Orb 주변에 붙는 파티클(나이아가라) */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UNiagaraComponent* OrbFX;

    /** 체력 컴포넌트 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UHealthComponent* HealthComp;

    /** 오브 최대 체력 (기본 1) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Orb")
    float MaxHealth = 1.f;

    /** true 일 때 플레이어 공격만 받도록 제한 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Orb")
    bool bOnlyPlayerCanDamage = true;

    /** 사용할 나이아가라 이펙트 에셋 (BP에서 지정) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Orb|FX")
    TObjectPtr<UNiagaraSystem> OrbFXTemplate;

protected:
    virtual void BeginPlay() override;

    /** HealthComponent에서 죽음 이벤트가 들어왔을 때 호출 */
    UFUNCTION()
    void HandleDeath();

public:
    /** AActor::ApplyDamage 에 의해 호출되는 함수 */
    virtual float TakeDamage(
        float DamageAmount,
        struct FDamageEvent const& DamageEvent,
        AController* EventInstigator,
        AActor* DamageCauser
    ) override;

};