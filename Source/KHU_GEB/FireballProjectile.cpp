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
    // 필요하다면 소환자 팀 등은 Ignore로 바꿔주세요.

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
            // 직접 맞은 대상은 폭발 데미지 중복 방지 (원하면 빼도 됨)
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
        // 폭발 범위를 디버그 구로 표시 (2초 동안)
        DrawDebugSphere(
            World,
            GetActorLocation(),
            ExplosionRadius,
            32,
            FColor::Red,
            false,
            2.0f,   // 지속 시간
            0,
            2.0f    // 선 두께
        );
#endif
    }



    // 3) 폭발 이펙트
    if (ExplosionNS)
    {
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(
            World,
            ExplosionNS,
            GetActorLocation(),
            GetActorRotation());
    }

    // 4) 화염구 제거
    Destroy();
}
