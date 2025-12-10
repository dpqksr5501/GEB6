// Fill out your copyright notice in the Description page of Project Settings.


#include "WeaponComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h" // ApplyDamage용
#include "FormDefinition.h"			// EFormType
#include "KHU_GEBCharacter.h"		// GetMesh()를 위해 필요시
#include "HealthComponent.h"
#include "Enemy_AI/Enemy_Base.h"
#include "Enemy_AI/Enemy_Minion_Special.h"
#include "Skills/Skill_Ultimate.h"
#include "FormManagerComponent.h"
#include "NiagaraComponent.h"   
#include "NiagaraFunctionLibrary.h"

// Sets default values for this component's properties
UWeaponComponent::UWeaponComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}


// Called when the game starts
void UWeaponComponent::BeginPlay()
{
	Super::BeginPlay();

	// 소유자(캐릭터)의 메시를 찾아 캐시합니다.
	if (AActor* Owner = GetOwner())
	{
		if (ACharacter* OwnerCharacter = Cast<ACharacter>(Owner))
		{
			CachedMesh = OwnerCharacter->GetMesh();
		}
		else
		{
			// ACharacter가 아닌 경우 대비
			CachedMesh = Owner->FindComponentByClass<USkeletalMeshComponent>();
		}
	}

	// 콜리전 풀을 미리 생성합니다.
	InitializeColliderPool(5);
}

// [핵심] UAttackComponent::SetForm로직을 이곳으로 가져옵니다.
void UWeaponComponent::SetWeaponDefinition(const UWeaponData* Def)
{
	//CurrentWeaponDef = Def;


	////====================================trail
	//// 캐릭터의 메쉬 캐싱 (이미 되어 있다고 가정)
	//ACharacter* Char = Cast<ACharacter>(GetOwner());
	//if (Char) CachedMesh = Char->GetMesh();

	//// 1) 기존 트레일 이펙트가 있다면 제거
	//if (CurrentTrailVFX)
	//{
	//	CurrentTrailVFX->DestroyComponent();
	//	CurrentTrailVFX = nullptr;
	//}

	//// 2) 데이터에 트레일 설정이 있고, 메쉬가 유효하다면 생성
	//if (Def && Def->TrailEffect && CachedMesh.IsValid())
	//{
	//	CurrentTrailVFX = UNiagaraFunctionLibrary::SpawnSystemAttached(
	//		Def->TrailEffect,
	//		CachedMesh.Get(),
	//		Def->TrailSocketStart, // 시작 소켓에 부착 (혹은 무기 루트)
	//		FVector::ZeroVector,
	//		FRotator::ZeroRotator,
	//		EAttachLocation::SnapToTarget,
	//		false // bAutoDestroy
	//	);

	//	// 중요: 생성하자마자 켜지지 않게 끔 (공격할 때만 켜야 함)
	//	if (CurrentTrailVFX)
	//	{
	//		CurrentTrailVFX->SetAutoActivate(false);
	//		CurrentTrailVFX->Deactivate();
	//	}
	//}
	////====================================trail


	//// 기존 폼의 콜리전을 비활성화하고 풀로 반환합니다.
	//DeactivateAllColliders();

	//if (!CurrentWeaponDef) return;
	//if (!CachedMesh.IsValid()) return;

	//USkeletalMeshComponent* Mesh = CachedMesh.Get();

	//// 1. 새 WeaponData의 Hitboxes 배열을 순회
	//for (const FHitboxConfig& Config : CurrentWeaponDef->Hitboxes)
	//{
	//	UShapeComponent* NewCollider = nullptr;

	//	// 2. 설정에 따라 풀에서 콜리전 가져오기
	//	if (Config.Shape == EHitboxShape::Sphere)
	//	{
	//		if (USphereComponent* Sphere = GetPooledSphereCollider())
	//		{
	//			Sphere->SetSphereRadius(Config.Size.X); // Size.X를 Radius로 사용
	//			NewCollider = Sphere;
	//		}
	//	}
	//	else // EHitboxShape::Box
	//	{
	//		if (UBoxComponent* Box = GetPooledBoxCollider())
	//		{
	//			Box->SetBoxExtent(Config.Size); // Size를 BoxExtent로 사용
	//			NewCollider = Box;
	//		}
	//	}

	//	// 3. 콜리전을 소켓에 부착하고 활성 목록에 추가
	//	if (NewCollider)
	//	{
	//		NewCollider->SetRelativeLocationAndRotation(Config.RelativeLocation, Config.RelativeRotation);
	//		NewCollider->AttachToComponent(Mesh, FAttachmentTransformRules::KeepRelativeTransform, Config.SocketName);
	//		ActiveColliders.Add(NewCollider);
	//	}
	//}

	CurrentWeaponDef = Def;

	ACharacter* Char = Cast<ACharacter>(GetOwner());
	if (Char) CachedMesh = Char->GetMesh();

	// 1. 기존에 만들어진 트레일 이펙트가 있다면 모두 삭제 및 초기화
	for (UNiagaraComponent* VFX : CurrentTrailVFXs)
	{
		if (VFX) VFX->DestroyComponent();
	}
	CurrentTrailVFXs.Reset();

	// 2. 데이터에 정의된 트레일 설정 개수만큼 생성 (양손이면 2번 돔)
	if (Def && CachedMesh.IsValid())
	{
		for (const FWeaponTrailConfig& TrailCfg : Def->TrailConfigs)
		{
			if (TrailCfg.TrailEffect)
			{
				// 나이아가라 컴포넌트 생성 및 소켓 부착
				UNiagaraComponent* NewVFX = UNiagaraFunctionLibrary::SpawnSystemAttached(
					TrailCfg.TrailEffect,
					CachedMesh.Get(),
					TrailCfg.SocketStart, // 설정한 소켓 이름 (예: FX_Trail_L_Start)
					FVector::ZeroVector,
					FRotator::ZeroRotator,
					EAttachLocation::SnapToTarget,
					false // 자동 파괴 끄기
				);

				if (NewVFX)
				{
					// 생성 직후엔 꺼둠 (공격할 때만 켜야 하므로)
					NewVFX->SetAutoActivate(false);
					NewVFX->Deactivate();

					// 관리 배열에 추가
					CurrentTrailVFXs.Add(NewVFX);
				}
			}
		}
	}
}

// [핵심] UAttackComponent::OnNotifyBeginReceived("StartAttack")로직
void UWeaponComponent::EnableCollision()
{

	//// 이 스윙에서 맞은 액터 목록을 초기화
	//HitActorsThisSwing.Empty();

	//// 현재 폼이 가진 모든 콜리전 볼륨(히트박스)을 활성화
	//for (UShapeComponent* Collider : ActiveColliders)
	//{
	//	UE_LOG(LogTemp, Log, TEXT("[WeaponComponent] Enabled collision for %s"), *Collider->GetName());
	//	if (Collider)
	//	{
	//		Collider->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	//		// 디버깅 메시지 각 콜리전 활성화 로그 UE_LOG로
	//		UE_LOG(LogTemp, Log, TEXT("[WeaponComponent] Enabled collision for %s"), *Collider->GetName());

	//	}
	//}

	// 1. 기존 활성 목록 초기화 및 스윙 피격 기록 초기화
	// (기존에 켜져있던 게 있다면 끄고 목록에서 비움)
	DeactivateAllColliders();
	HitActorsThisSwing.Empty();

	// 2. 현재 무기 데이터가 유효한지 확인
	if (CurrentWeaponDef && CachedMesh.IsValid())
	{
		// 데이터에 정의된 히트박스(Hitboxes) 개수만큼 반복
		for (const FHitboxConfig& BoxCfg : CurrentWeaponDef->Hitboxes)
		{
			UShapeComponent* ShapeComp = nullptr;

			// 모양에 따라 박스/구체 풀에서 하나 꺼내오기
			if (BoxCfg.Shape == EHitboxShape::Box)
			{
				UBoxComponent* Box = GetPooledBoxCollider();
				Box->SetBoxExtent(BoxCfg.Size);
				ShapeComp = Box;
			}
			else // Sphere
			{
				USphereComponent* Sphere = GetPooledSphereCollider();
				Sphere->SetSphereRadius(BoxCfg.Size.X); // X값을 반지름으로 사용
				ShapeComp = Sphere;
			}

			// 꺼내온 콜리전이 유효하면 설정 시작
			if (ShapeComp)
			{
				// 소켓에 부착 (위치/회전 오프셋 적용)
				ShapeComp->AttachToComponent(CachedMesh.Get(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, BoxCfg.SocketName);
				ShapeComp->SetRelativeLocation(BoxCfg.RelativeLocation);
				ShapeComp->SetRelativeRotation(BoxCfg.RelativeRotation);

				// 콜리전 켜기 (QueryOnly = 물리충돌X, 오버랩O)
				ShapeComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

				// 활성 목록에 등록 (나중에 끄기 위해)
				ActiveColliders.Add(ShapeComp);

				UE_LOG(LogTemp, Verbose, TEXT("[WeaponComponent] Enabled collision for %s on socket %s"), *ShapeComp->GetName(), *BoxCfg.SocketName.ToString());
			}
		}
	}

	// 3. [추가] 트레일 이펙트 켜기
	BeginTrail();


}

// [핵심] UAttackComponent::OnNotifyBeginReceived("EndAttack") 로직
void UWeaponComponent::DisableCollision()
{

	//// 모든 콜리전 볼륨(히트박스)을 비활성화
	//for (UShapeComponent* Collider : ActiveColliders)
	//{
	//	if (Collider)
	//	{
	//		Collider->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	//	}
	//}
	// 1. 활성화된 모든 콜리전 끄기 (풀로 반환)
	DeactivateAllColliders();

	// 2. [추가] 트레일 이펙트 끄기
	EndTrail();
}

/** [복사] AttackComponent.cpp에서 가져옴 */
void UWeaponComponent::OnAttackOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	UE_LOG(LogTemp, Log,
		TEXT("[WeaponComponent] OnAttackOverlap triggered with %s"),
		*GetNameSafe(OtherActor));

	AActor* Owner = GetOwner();
	if (!Owner || !OtherActor || OtherActor == Owner) return;

	if (Owner->IsA(AEnemy_Base::StaticClass()) &&
		OtherActor->IsA(AEnemy_Base::StaticClass()))
	{
		UE_LOG(LogTemp, Warning, TEXT("[WeaponComponent] Enemy hit Enemy, ignoring"));
		return;
	}

	if (HitActorsThisSwing.Contains(OtherActor)) return;

	// 1) 우선 피격 대상이 '데미지를 받을 수 있는' 대상인지 확인 (HealthComponent 존재 여부)
	UHealthComponent* Health = OtherActor->FindComponentByClass<UHealthComponent>();
	if (!Health)
	{
		// HealthComponent가 없는 대상이면 실제 체력 감소는 없음
		return;
	}

	// 2) 팀/타입 필터
	//    - 플레이어 무기면 Enemy_Base만
	//    - Enemy_Base 무기면 플레이어만
	if (AKHU_GEBCharacter* OwnerChar = Cast<AKHU_GEBCharacter>(Owner))
	{
		// 플레이어 입장에서 적이 아니면 무시
		if (!OwnerChar->IsEnemyFor(OtherActor))
		{
			UE_LOG(LogTemp, Verbose,
				TEXT("[WeaponComponent] Player weapon hit non-enemy: %s -> ignored"),
				*GetNameSafe(OtherActor));
			return;
		}
	}
	else if (AEnemy_Base* OwnerEnemy = Cast<AEnemy_Base>(Owner))
	{
		// 적 입장에서 적이 아니면 무시 (즉, 주로 플레이어만 타격)
		if (!OwnerEnemy->IsEnemyFor(OtherActor))
		{
			UE_LOG(LogTemp, Verbose,
				TEXT("[WeaponComponent] Enemy weapon hit non-enemy: %s -> ignored"),
				*GetNameSafe(OtherActor));
			return;
		}
	}
	// 그 외(트랩 등) 오너는 별도 팀 로직 없이 HealthComp 존재만으로 데미지 허용

	// 3) 데미지 양 결정 (우선은 임시 값 10 유지)
	float DamageToApply = 10.f;


	if (AKHU_GEBCharacter* Player = Cast<AKHU_GEBCharacter>(GetOwner()))
	{
		if (Player->StatManager && Player->FormManager)
		{
			if (const FFormRuntimeStats* Stats =
				Player->StatManager->GetStats(Player->FormManager->CurrentForm))
			{
				DamageToApply = Stats->Attack;
			}
		}
	}

	if (AEnemy_Base* Enemy = Cast<AEnemy_Base>(GetOwner()))
	{
		DamageToApply = Enemy->GetAttackStat();
	}

	if (DamageToApply <= 0.f) return;

	// 4) AActor::ApplyDamage 파이프라인을 통해 데미지를 넣는다.
	APawn* OwnerPawn = Cast<APawn>(Owner);
	AController* InstigatorController = OwnerPawn ? OwnerPawn->GetController() : nullptr;

	UGameplayStatics::ApplyDamage(
		OtherActor,
		DamageToApply,
		InstigatorController,
		Owner,
		UDamageType::StaticClass());

	// ===== [흡혈 로직] Enemy_Minion_Special이 공격 성공 시 체력 회복 =====
	if (AEnemy_Minion_Special* SpecialMinion = Cast<AEnemy_Minion_Special>(Owner))
	{
		// 공격력만큼 자신의 체력 회복
		if (UHealthComponent* OwnerHealth = SpecialMinion->FindComponentByClass<UHealthComponent>())
		{
			OwnerHealth->AddHealth(DamageToApply);
			
			UE_LOG(LogTemp, Log, 
				TEXT("[WeaponComponent] Enemy_Minion_Special healed %.1f HP after attack. Current HP: %.1f"),
				DamageToApply, OwnerHealth->Health);
		}
	}
	// ===== [흡혈 로직 끝] =====

	// ---------------- [사운드 로직 시작] ----------------

	USoundBase* SoundToPlay = nullptr;

	// 1. 때린 주체가 '플레이어'인지 확인
	AKHU_GEBCharacter* PlayerOwner = Cast<AKHU_GEBCharacter>(GetOwner());

	// 2. 맞은 대상이 '적(Enemy)'인지 확인
	AEnemy_Base* HitEnemy = Cast<AEnemy_Base>(OtherActor);

	// [조건] 플레이어가 때렸고 && 적이 맞았을 때만 실행
	if (PlayerOwner && HitEnemy && PlayerOwner->FormManager)
	{
		EFormType CurrentForm = PlayerOwner->FormManager->GetCurrentFormType();

		switch (CurrentForm)
		{
			// [그룹 1] 근접 타격감
		case EFormType::Base:
		case EFormType::Guard:
		case EFormType::Swift:
			SoundToPlay = MeleeHitSound;
			break;

			// [그룹 2] 원거리/마법 타격감
		case EFormType::Range:
		case EFormType::Special:
			SoundToPlay = HeavyHitSound; // 혹은 MagicHitSound
			break;
		}

		// 소리 재생 (위 조건이 맞을 때만 SoundToPlay가 설정됨)
		if (SoundToPlay)
		{
			UGameplayStatics::PlaySoundAtLocation(this, SoundToPlay, HitEnemy->GetActorLocation());
		}
	}
	// ---------------- [사운드 로직 끝] ----------------


	// === Swift 궁극기 은신 공격 처리 ===
	if (APawn* InstigatorPawn = Cast<APawn>(Owner))
	{
		// 소유 Actor에 붙어있는 Swift 궁극기 컴포넌트 찾기
		if (USkill_Ultimate* UltimateSkill = InstigatorPawn->FindComponentByClass<USkill_Ultimate>())
		{
			if (UltimateSkill->IsSwiftStealthActive())
			{
				// "은신 중 성공적으로 공격이 적중한 순간" → 스턴 + 은신 해제
				UltimateSkill->OnAttackFromStealth(OtherActor);
			}
		}
	}

	UE_LOG(LogTemp, Log,
		TEXT("[WeaponComponent] %s hit %s for %.1f"),
		*Owner->GetName(),
		*OtherActor->GetName(),
		DamageToApply);

	// 한 번 맞은 액터는 이 스윙 동안 다시 안 맞도록 기록
	HitActorsThisSwing.Add(OtherActor);

}

/** [복사] AttackComponent.cpp에서 가져옴 */
void UWeaponComponent::InitializeColliderPool(int32 PoolSize)
{
	if (!GetOwner()) return;
	for (int32 i = 0; i < PoolSize; ++i)
	{
		if (UBoxComponent* Box = CreateNewBoxCollider())
		{
			BoxColliderPool.Add(Box);
		}
		if (USphereComponent* Sphere = CreateNewSphereCollider())
		{
			SphereColliderPool.Add(Sphere);
		}
	}
}

/** [복사] AttackComponent.cpp에서 가져옴 */
UBoxComponent* UWeaponComponent::GetPooledBoxCollider()
{
	for (UBoxComponent* Box : BoxColliderPool)
	{
		if (Box && !ActiveColliders.Contains(Box))
		{
			return Box;
		}
	}
	UBoxComponent* NewBox = CreateNewBoxCollider();
	BoxColliderPool.Add(NewBox);
	return NewBox;
}

/** [복사] AttackComponent.cpp에서 가져옴 */
USphereComponent* UWeaponComponent::GetPooledSphereCollider()
{
	for (USphereComponent* Sphere : SphereColliderPool)
	{
		if (Sphere && !ActiveColliders.Contains(Sphere))
		{
			return Sphere;
		}
	}
	USphereComponent* NewSphere = CreateNewSphereCollider();
	SphereColliderPool.Add(NewSphere);
	return NewSphere;
}

/** [복사] AttackComponent.cpp에서 가져옴 */
void UWeaponComponent::DeactivateAllColliders()
{
	for (UShapeComponent* Collider : ActiveColliders)
	{
		if (!Collider) continue;
		Collider->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Collider->DetachFromComponent(FDetachmentTransformRules::KeepRelativeTransform);
		Collider->SetRelativeLocation(FVector::ZeroVector);
		Collider->SetRelativeRotation(FRotator::ZeroRotator);

		if (UBoxComponent* Box = Cast<UBoxComponent>(Collider))
		{
			Box->SetBoxExtent(FVector::ZeroVector);
		}
		else if (USphereComponent* Sphere = Cast<USphereComponent>(Collider))
		{
			Sphere->SetSphereRadius(0.0f);
		}
	}
	ActiveColliders.Empty();
}

/** [복사] AttackComponent.cpp에서 가져옴 (OnAttackOverlap 바인딩 확인) */
UBoxComponent* UWeaponComponent::CreateNewBoxCollider()
{
	AActor* Owner = GetOwner();
	if (!Owner) return nullptr;
	UBoxComponent* NewBox = NewObject<UBoxComponent>(Owner);
	if (!NewBox) return nullptr;
	NewBox->RegisterComponent();
	NewBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	// 바인딩 함수를 이 클래스(UWeaponComponent)의 것으로 변경
	NewBox->OnComponentBeginOverlap.AddDynamic(this, &UWeaponComponent::OnAttackOverlap);
	NewBox->SetCollisionObjectType(ECC_WorldDynamic);
	NewBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	NewBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	NewBox->SetHiddenInGame(true);
	return NewBox;
}

/** [복사] AttackComponent.cpp에서 가져옴 (OnAttackOverlap 바인딩 확인) */
USphereComponent* UWeaponComponent::CreateNewSphereCollider()
{
	AActor* Owner = GetOwner();
	if (!Owner) return nullptr;
	USphereComponent* NewSphere = NewObject<USphereComponent>(Owner);
	if (!NewSphere) return nullptr;
	NewSphere->RegisterComponent();
	NewSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	// 바인딩 함수를 이 클래스(UWeaponComponent)의 것으로 변경
	NewSphere->OnComponentBeginOverlap.AddDynamic(this, &UWeaponComponent::OnAttackOverlap);
	NewSphere->SetCollisionObjectType(ECC_WorldDynamic);
	NewSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	NewSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	NewSphere->SetHiddenInGame(true);
	return NewSphere;
}

// Called every frame
void UWeaponComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}



// ---------------------------------------------------------------
// 4. 트레일 제어 내부 함수
// ---------------------------------------------------------------
void UWeaponComponent::BeginTrail()
{
	for (UNiagaraComponent* VFX : CurrentTrailVFXs)
	{
		if (VFX)
		{
			VFX->Activate(true); // 활성화 (Reset)
		}
	}
}

void UWeaponComponent::EndTrail()
{
	for (UNiagaraComponent* VFX : CurrentTrailVFXs)
	{
		if (VFX)
		{
			VFX->Deactivate(); // 비활성화 (자연스럽게 사라짐)
		}
	}
}