// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemy_AI/Bullet.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "KHU_GEBCharacter.h"
#include "HealthComponent.h"
#include "Enemy_AI/Enemy_Base.h"

// Sets default values
ABullet::ABullet()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	// ========= Collision 컴포넌트 생성 =========
	CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	CollisionComp->InitSphereRadius(15.0f);
	
	// 콜리전 설정: Overlap으로 모든 것과 충돌
	CollisionComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionComp->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionComp->SetCollisionResponseToAllChannels(ECR_Overlap);
	CollisionComp->SetGenerateOverlapEvents(true);
	
	// Overlap 이벤트에 함수 바인딩
	CollisionComp->OnComponentBeginOverlap.AddDynamic(this, &ABullet::OnAttackOverlap);
	
	RootComponent = CollisionComp;

	// ========= ProjectileMovement 컴포넌트 생성 =========
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->UpdatedComponent = CollisionComp;
	ProjectileMovement->InitialSpeed = 1000.0f;
	ProjectileMovement->MaxSpeed = 1000.0f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = false;
	ProjectileMovement->ProjectileGravityScale = 0.0f; // 중력 없이 직선으로 날아감

	// 3초 후 자동 파괴
	InitialLifeSpan = 3.0f;
}

// Called when the game starts or when spawned
void ABullet::BeginPlay()
{
	Super::BeginPlay();
	
	// 발사자(Owner)와의 충돌 무시
	if (AActor* OwnerActor = GetOwner())
	{
		CollisionComp->IgnoreActorWhenMoving(OwnerActor, true);
	}

	// Instigator와의 충돌 무시
	if (AActor* InstigatorActor = GetInstigator())
	{
		CollisionComp->IgnoreActorWhenMoving(InstigatorActor, true);
	}
}

void ABullet::OnAttackOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	// 자기 자신이거나 유효하지 않은 액터면 무시
	if (!OtherActor || OtherActor == this)
	{
		return;
	}

	// Owner나 Instigator와 충돌하면 무시
	AActor* BulletOwner = GetOwner();
	if (OtherActor == BulletOwner || OtherActor == GetInstigator())
	{
		return;
	}

	// 1) Enemy끼리 충돌 무시
	if (BulletOwner && BulletOwner->IsA(AEnemy_Base::StaticClass()) &&
		OtherActor->IsA(AEnemy_Base::StaticClass()))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Bullet] Enemy hit Enemy, ignoring"));
		return;
	}

	// 2) HealthComponent 존재 여부 확인
	UHealthComponent* Health = OtherActor->FindComponentByClass<UHealthComponent>();
	if (!Health)
	{
		// HealthComponent가 없으면 데미지 적용 불가
		return;
	}

	// 3) 팀/타입 필터 (적이 아니면 무시)
	if (AEnemy_Base* OwnerEnemy = Cast<AEnemy_Base>(BulletOwner))
	{
		if (!OwnerEnemy->IsEnemyFor(OtherActor))
		{
			UE_LOG(LogTemp, Verbose,
				TEXT("[Bullet] Enemy bullet hit non-enemy: %s -> ignored"),
				*GetNameSafe(OtherActor));
			return;
		}
	}

	// 4) 데미지 적용 (WeaponComp와 동일한 방식)
	if (Damage <= 0.f)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Bullet] Damage is 0 or less, no damage applied"));
		Destroy();
		return;
	}

	APawn* OwnerPawn = Cast<APawn>(BulletOwner);
	AController* InstigatorController = OwnerPawn ? OwnerPawn->GetController() : nullptr;

	UGameplayStatics::ApplyDamage(
		OtherActor,
		Damage,  // ← Enemy_Minion_Range에서 전달받은 Attack 스탯
		InstigatorController,
		BulletOwner,
		UDamageType::StaticClass());

	UE_LOG(LogTemp, Log, 
		TEXT("[Bullet] %s hit %s for %.1f damage"), 
		*GetNameSafe(BulletOwner), 
		*OtherActor->GetName(), 
		Damage);

	// 피해를 줬으니 총알 제거
	Destroy();
}

