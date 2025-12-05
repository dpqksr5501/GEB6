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
#include "Skills/Skill_Ultimate.h"

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
	CurrentWeaponDef = Def;

	// 기존 폼의 콜리전을 비활성화하고 풀로 반환합니다.
	DeactivateAllColliders();

	if (!CurrentWeaponDef) return;
	if (!CachedMesh.IsValid()) return;

	USkeletalMeshComponent* Mesh = CachedMesh.Get();

	// 1. 새 WeaponData의 Hitboxes 배열을 순회
	for (const FHitboxConfig& Config : CurrentWeaponDef->Hitboxes)
	{
		UShapeComponent* NewCollider = nullptr;

		// 2. 설정에 따라 풀에서 콜리전 가져오기
		if (Config.Shape == EHitboxShape::Sphere)
		{
			if (USphereComponent* Sphere = GetPooledSphereCollider())
			{
				Sphere->SetSphereRadius(Config.Size.X); // Size.X를 Radius로 사용
				NewCollider = Sphere;
			}
		}
		else // EHitboxShape::Box
		{
			if (UBoxComponent* Box = GetPooledBoxCollider())
			{
				Box->SetBoxExtent(Config.Size); // Size를 BoxExtent로 사용
				NewCollider = Box;
			}
		}

		// 3. 콜리전을 소켓에 부착하고 활성 목록에 추가
		if (NewCollider)
		{
			NewCollider->SetRelativeLocationAndRotation(Config.RelativeLocation, Config.RelativeRotation);
			NewCollider->AttachToComponent(Mesh, FAttachmentTransformRules::KeepRelativeTransform, Config.SocketName);
			ActiveColliders.Add(NewCollider);
		}
	}
}

// [핵심] UAttackComponent::OnNotifyBeginReceived("StartAttack")로직
void UWeaponComponent::EnableCollision()
{

	//디버깅 메시지 콜리전 활성화 로그
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green, TEXT("[WeaponComponent] COLLISION ENABLED"));
	}
	// 이 스윙에서 맞은 액터 목록을 초기화
	HitActorsThisSwing.Empty();

	// 현재 폼이 가진 모든 콜리전 볼륨(히트박스)을 활성화
	for (UShapeComponent* Collider : ActiveColliders)
	{
		UE_LOG(LogTemp, Log, TEXT("[WeaponComponent] Enabled collision for %s"), *Collider->GetName());
		if (Collider)
		{
			Collider->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			// 디버깅 메시지 각 콜리전 활성화 로그 UE_LOG로
			UE_LOG(LogTemp, Log, TEXT("[WeaponComponent] Enabled collision for %s"), *Collider->GetName());

		}
	}
}

// [핵심] UAttackComponent::OnNotifyBeginReceived("EndAttack") 로직
void UWeaponComponent::DisableCollision()
{

	//디버깅 메시지 콜리전 비활성화 로그
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red, TEXT("[WeaponComponent] COLLISION DISABLED"));
	}
	// 모든 콜리전 볼륨(히트박스)을 비활성화
	for (UShapeComponent* Collider : ActiveColliders)
	{
		if (Collider)
		{
			Collider->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}
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

	// TODO: 나중에 StatManager/WeaponData에서 공격력을 가져오고 싶으면 여기서 계산
	// if (AKHU_GEBCharacter* OwnerChar = Cast<AKHU_GEBCharacter>(Owner))
	// {
	//     if (OwnerChar->StatManager)
	//     {
	//         if (const FFormRuntimeStats* Stats =
	//             OwnerChar->StatManager->GetStats(OwnerChar->FormManager->CurrentForm))
	//         {
	//             DamageToApply = Stats->Attack;
	//         }
	//     }
	// }

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
	NewBox->SetHiddenInGame(false);
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
	NewSphere->SetHiddenInGame(false);
	return NewSphere;
}

// Called every frame
void UWeaponComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

