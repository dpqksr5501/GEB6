// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FireballProjectile.generated.h"

class USphereComponent;
class UProjectileMovementComponent;
class UNiagaraSystem;

UCLASS()
class KHU_GEB_API AFireballProjectile : public AActor
{
    GENERATED_BODY()

public:
    AFireballProjectile();

protected:
    virtual void BeginPlay() override;

    UFUNCTION()
    void OnHit(
        UPrimitiveComponent* HitComp,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComp,
        FVector NormalImpulse,
        const FHitResult& Hit);

    UFUNCTION()
    void OnBeginOverlap(
        UPrimitiveComponent* OverlappedComp,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex,
        bool bFromSweep,
        const FHitResult& SweepResult);

    /** 실제 폭발 처리(직접 데미지 + 범위 데미지 + 이펙트 + Destroy)를 담당 */
    void Explode(AActor* DirectHitActor);

public:
    /** 충돌용 구체 콜리전 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<USphereComponent> CollisionComp;

    /** 발사체 이동 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

    /** 폭발 이펙트 (선택) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FX")
    TObjectPtr<UNiagaraSystem> ExplosionNS;

    /** 구체에 직접 맞았을 때 들어갈 대미지 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage", meta = (ExposeOnSpawn = "true"))
    float DirectDamage = 0.f;

    /** 폭발 범위에 들어가는 대미지 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage", meta = (ExposeOnSpawn = "true"))
    float ExplosionDamage = 0.f;

    /** 폭발 반경 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage", meta = (ExposeOnSpawn = "true"))
    float ExplosionRadius = 200.f;
};
