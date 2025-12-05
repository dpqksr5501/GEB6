// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemy_AI/Enemy_Minion_Range.h"
#include "Enemy_AI/Bullet.h"
#include "Components/SphereComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Kismet/GameplayStatics.h"

void AEnemy_Minion_Range::Shot()
{
	UWorld* World = GetWorld();
	if (!World || !BulletClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Enemy_Minion_Range] Shot failed: Invalid World or BulletClass"));
		return;
	}

	if (!BlackboardComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Enemy_Minion_Range] Shot failed: BlackboardComp is null"));
		return;
	}

	AActor* Target = Cast<AActor>(BlackboardComp->GetValueAsObject("Target"));
	if (!Target)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Enemy_Minion_Range] Shot failed: No Target in Blackboard"));
		return;
	}

	// 소켓 위치에서 총알 생성
	USkeletalMeshComponent* MeshComp = GetMesh();
	FVector SpawnLocation;
	FRotator SpawnRotation;

	if (MeshComp && MeshComp->DoesSocketExist(MuzzleSocketName))
	{
		// 소켓이 존재하면 소켓 위치 사용
		SpawnLocation = MeshComp->GetSocketLocation(MuzzleSocketName);
		
		// 타겟 방향으로 회전 설정
		FVector TargetLocation = Target->GetActorLocation();
		SpawnRotation = (TargetLocation - SpawnLocation).Rotation();
		
		UE_LOG(LogTemp, Log, TEXT("[Enemy_Minion_Range] Spawning bullet from socket: %s"), *MuzzleSocketName.ToString());
	}
	else
	{
		// 소켓이 없으면 기본 위치 사용 (앞쪽 50유닛,위쪽 50유닛)
		SpawnLocation = GetActorLocation() + GetActorForwardVector() * 50.0f + FVector(0.0f, 0.0f, 50.0f);
		FVector TargetLocation = Target->GetActorLocation();
		SpawnRotation = (TargetLocation - SpawnLocation).Rotation();
		
		UE_LOG(LogTemp, Warning, TEXT("[Enemy_Minion_Range] Socket '%s' not found, using default spawn location"), *MuzzleSocketName.ToString());
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// Bullet 생성
	ABullet* Bullet = World->SpawnActor<ABullet>(BulletClass, SpawnLocation, SpawnRotation, SpawnParams);
	
	if (!Bullet)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Enemy_Minion_Range] Failed to spawn bullet"));
		return;
	}

	// 총알에 데미지 값 전달
	Bullet->Damage = BulletDamage;

	// 발사체 이동 컴포넌트 설정
	if (UProjectileMovementComponent* ProjectileMovement = Bullet->ProjectileMovement)
	{
		ProjectileMovement->InitialSpeed = BulletSpeed;
		ProjectileMovement->MaxSpeed = BulletSpeed;
		
		FVector Direction = (Target->GetActorLocation() - SpawnLocation).GetSafeNormal();
		ProjectileMovement->Velocity = Direction * BulletSpeed;
	}

	UE_LOG(LogTemp, Log, TEXT("[Enemy_Minion_Range] Shot projectile toward %s with damage %.1f"), *Target->GetName(), BulletDamage);
}

