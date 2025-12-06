// Fill out your copyright notice in the Description page of Project Settings.


#include "FireballProjectile.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "DrawDebugHelpers.h"

AFireballProjectile::AFireballProjectile()
{
    PrimaryActorTick.bCanEverTick = false;

    // ========= Collision =========
    CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
    CollisionComp->InitSphereRadius(15.f);

    // 콜리전 확실히: QueryOnly + 대부분 Block
    CollisionComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    CollisionComp->SetCollisionObjectType(ECC_WorldDynamic);
    CollisionComp->SetCollisionResponseToAllChannels(ECR_Block);

    CollisionComp->SetNotifyRigidBodyCollision(true); // Hit 이벤트
    CollisionComp->SetGenerateOverlapEvents(true);    // Overlap 이벤트

    CollisionComp->OnComponentHit.AddDynamic(this, &AFireballProjectile::OnHit);
    CollisionComp->OnComponentBeginOverlap.AddDynamic(this, &AFireballProjectile::OnBeginOverlap);

    RootComponent = CollisionComp;

    // ========= Movement =========
    ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
    ProjectileMovement->UpdatedComponent = CollisionComp;
    ProjectileMovement->InitialSpeed = 1000.f;
    ProjectileMovement->MaxSpeed = 1000.f;
    ProjectileMovement->bRotationFollowsVelocity = true;
    ProjectileMovement->bShouldBounce = false;
    ProjectileMovement->ProjectileGravityScale = 1.f;

    InitialLifeSpan = 5.f; // 5초 후 자동 파괴
}

void AFireballProjectile::BeginPlay()
{
    Super::BeginPlay();

    // 시전자와의 충돌 무시
    if (AActor* InstActor = GetInstigator())
    {
        CollisionComp->IgnoreActorWhenMoving(InstActor, true);

        // Owner가 따로 있다면 이것도 무시 (보통 Owner == Instigator 이긴 함)
        if (AActor* OwnerActor = GetOwner())
        {
            if (OwnerActor != InstActor)
            {
                CollisionComp->IgnoreActorWhenMoving(OwnerActor, true);
            }
        }
    }
}

void AFireballProjectile::OnHit(
    UPrimitiveComponent* HitComp,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    FVector NormalImpulse,
    const FHitResult& Hit)
{
    UWorld* World = GetWorld();
    if (!World)
    {
        Destroy();
        return;
    }

    AActor* MyInstigator = GetInstigator();

    // 자기 자신 / 시전자 / Owner 에 닿았으면 폭발하지 않고 무시
    if (!OtherActor || OtherActor == this ||
        OtherActor == MyInstigator || OtherActor == GetOwner()) return;

    UE_LOG(LogTemp, Log, TEXT("[Fireball] OnHit -> %s"), *GetNameSafe(OtherActor));
    Explode(OtherActor);
}

void AFireballProjectile::OnBeginOverlap(
    UPrimitiveComponent* OverlappedComp,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex,
    bool bFromSweep,
    const FHitResult& SweepResult)
{
    UE_LOG(LogTemp, Log, TEXT("[Fireball] OnBeginOverlap -> %s"), *GetNameSafe(OtherActor));
    Explode(OtherActor);
}

void AFireballProjectile::Explode(AActor* DirectHitActor)
{
    UWorld* World = GetWorld();
    if (!World)
    {
        Destroy();
        return;
    }

    AActor* MyInstigator = GetInstigator();

    UE_LOG(LogTemp, Log, TEXT("[Fireball] Explode at %s. DirectHit=%s"),
        *GetNameSafe(this),
        *GetNameSafe(DirectHitActor));

    // 1) 직접 맞은 대상에게 DirectDamage
    if (DirectHitActor && DirectHitActor != this && DirectHitActor != MyInstigator)
    {
        if (DirectDamage > 0.f)
        {
            UGameplayStatics::ApplyDamage(
                DirectHitActor,
                DirectDamage,
                MyInstigator ? MyInstigator->GetInstigatorController() : nullptr,
                this,
                nullptr);
        }
    }

    // 2) 폭발 데미지 (범위)
    if (ExplosionDamage > 0.f && ExplosionRadius > 0.f)
    {
        TArray<AActor*> IgnoreActors;
        IgnoreActors.Add(this);
        if (MyInstigator)
        {
            IgnoreActors.Add(MyInstigator);
        }
        if (DirectHitActor)
        {
            // 직접 맞은 대상은 폭발 데미지 중복 방지
            IgnoreActors.Add(DirectHitActor);
        }

        UGameplayStatics::ApplyRadialDamage(
            World,
            ExplosionDamage,
            GetActorLocation(),
            ExplosionRadius,
            nullptr,
            IgnoreActors,
            this,
            MyInstigator ? MyInstigator->GetInstigatorController() : nullptr,
            true);

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
        // 폭발 범위를 디버그 '원'으로 표시 (XY 평면)
        DrawDebugCircle(
            World,
            GetActorLocation(),
            ExplosionRadius,
            32,
            FColor::Red,
            false,
            2.0f,
            0,
            2.0f,
            FVector(1.f, 0.f, 0.f),
            FVector(0.f, 1.f, 0.f),
            false
        );
#endif
    }

    // 3) 폭발 이펙트 (나이아가라)
    if (ExplosionNS)
    {
        float Scale = 1.f;
        if (ExplosionReferenceRadius > 0.f && ExplosionRadius > 0.f)
        {
            Scale = ExplosionRadius / ExplosionReferenceRadius;
        }

        UNiagaraFunctionLibrary::SpawnSystemAtLocation(
            World,
            ExplosionNS,
            GetActorLocation(),
            GetActorRotation(),
            FVector(Scale, Scale, 1.f) // 바닥 원판처럼 XY만 스케일
        );
    }

    // 4) 화염구 제거
    Destroy();
}
