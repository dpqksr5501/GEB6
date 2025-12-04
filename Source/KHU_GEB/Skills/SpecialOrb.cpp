// Fill out your copyright notice in the Description page of Project Settings.


#include "SpecialOrb.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "HealthComponent.h"
#include "KHU_GEBCharacter.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"

ASpecialOrb::ASpecialOrb()
{
    PrimaryActorTick.bCanEverTick = false;

    // 루트
    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    RootComponent = Root;

    // 충돌
    Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
    Collision->SetupAttachment(Root);
    Collision->InitSphereRadius(50.f);

    Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Collision->SetCollisionProfileName(TEXT("OverlapAllDynamic"));

    // 메쉬
    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    Mesh->SetupAttachment(Collision);
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    // 나이아가라 FX 컴포넌트
    OrbFX = CreateDefaultSubobject<UNiagaraComponent>(TEXT("OrbFX"));
    OrbFX->SetupAttachment(Root);
    OrbFX->bAutoActivate = false; // 에셋 세팅 후 BeginPlay에서 켜줄 예정

    // 체력
    HealthComp = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComp"));
}

void ASpecialOrb::BeginPlay()
{
    Super::BeginPlay();

    if (HealthComp)
    {
        HealthComp->InitializeHealth(MaxHealth, MaxHealth);
        HealthComp->OnDeath.AddDynamic(this, &ASpecialOrb::HandleDeath);
    }

    // ★ 파티클 에셋이 지정되어 있으면 OrbFX에 할당 후 활성화
    if (OrbFX && OrbFXTemplate)
    {
        OrbFX->SetAsset(OrbFXTemplate);
        OrbFX->Activate(true);
    }
}

float ASpecialOrb::TakeDamage(
	float DamageAmount,
	FDamageEvent const& DamageEvent,
	AController* EventInstigator,
	AActor* DamageCauser)
{
	const float ActualDamage =
		Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	if (!HealthComp || ActualDamage <= 0.f)
	{
		return 0.f;
	}

	// 공격자 Actor 찾기 (다른 캐릭터들과 동일한 패턴) :contentReference[oaicite:3]{index=3}
	AActor* InstigatorActor = nullptr;
	if (EventInstigator)
	{
		InstigatorActor = EventInstigator->GetPawn();
	}
	if (!InstigatorActor)
	{
		InstigatorActor = DamageCauser;
	}

	// 옵션: 플레이어만 오브를 파괴할 수 있게 제한
	if (bOnlyPlayerCanDamage)
	{
		if (!Cast<AKHU_GEBCharacter>(InstigatorActor))
		{
			// 플레이어가 아니라면 데미지 무시
			return 0.f;
		}
	}

	// 실제 체력 감소는 HealthComponent가 처리
	return HealthComp->ApplyDamage(ActualDamage, InstigatorActor, DamageCauser); // :contentReference[oaicite:4]{index=4}
}

void ASpecialOrb::HandleDeath()
{
	// Skill_Ultimate 에서 Orb->OnDestroyed 로 남은 개수 관리 중 :contentReference[oaicite:5]{index=5}
	Destroy();
}
