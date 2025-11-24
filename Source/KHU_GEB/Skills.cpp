// Fill out your copyright notice in the Description page of Project Settings.


#include "Skills.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/MovementComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"
#include "KHU_GEBCharacter.h"
#include "HealthComponent.h" 
#include "ManaComponent.h"
#include "FireballProjectile.h"
#include "TimerManager.h"
#include "DrawDebugHelpers.h"

static void ApplyFixedDotDamage(
    USkillBase* SourceSkill,
    ACharacter* Target,
    float DamagePerTick,
    int32 HitCount = 1)
{
    if (!Target || DamagePerTick <= 0.f || HitCount <= 0) return;

    UHealthComponent* Health = Target->FindComponentByClass<UHealthComponent>();
    if (!Health) return;

    FDamageSpec Spec;
    Spec.RawDamage = DamagePerTick;
    Spec.bIgnoreDefense = true;     // 방어력 무시
    Spec.bPeriodic = true;     // 주기적(DoT) 플래그
    Spec.bFixedDot = true;     // 고정 도트 모드 ON
    Spec.HitCount = HitCount;
    Spec.Instigator = SourceSkill ? SourceSkill->GetOwner() : nullptr;
    Spec.SourceSkill = SourceSkill;

    Health->ApplyDamageSpec(Spec);
}

/*=============================Base=============================*/


/*=============================Range=============================*/

USkill_Range::USkill_Range()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
}

void USkill_Range::InitializeFromDefinition(const USkillDefinition* Def)
{
    Super::InitializeFromDefinition(Def);

    if (Params.Range > 0.f) { TargetRadius = Params.Range; }
}

void USkill_Range::SpawnOrUpdateIndicator()
{
    UWorld* World = GetWorld();
    if (!World || !TargetAreaNS) return;

    const float Radius = GetCurrentTargetRadius();
    const float UniformScale = (Radius > 0.f) ? (Radius / 100.f) : 1.f; // 이펙트 사이즈에 맞게 조정

    if (!TargetAreaComp)
    {
        TargetAreaComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
            World,
            TargetAreaNS,
            CurrentTargetLocation,
            FRotator::ZeroRotator,
            FVector(UniformScale)
        );
    }
    else
    {
        TargetAreaComp->SetWorldLocation(CurrentTargetLocation);
        TargetAreaComp->SetWorldScale3D(FVector(UniformScale));
    }
}

void USkill_Range::CleanupIndicator()
{
    if (TargetAreaComp)
    {
        TargetAreaComp->Deactivate();
        TargetAreaComp = nullptr;
    }
}

void USkill_Range::ActivateSkill()
{
    UWorld* World = GetWorld();
    AActor* Owner = GetOwner();
    if (!World || !Owner) return;

    if (!FireballClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Skill_Range] FireballClass is not set."));
        return;
    }

    bIsAiming = true;
    bHasValidTarget = false;
    AimMoveInput = FVector2D::ZeroVector;
    SetComponentTickEnabled(true);

    if (ACharacter* OwnerChar = Cast<ACharacter>(Owner))
    {
        // 초기 조준 위치: 앞쪽으로 Range의 절반 지점
        const float Radius = GetCurrentTargetRadius();
        FVector Forward = OwnerChar->GetActorForwardVector();
        Forward.Z = 0.f;
        if (!Forward.Normalize()) { Forward = FVector::ForwardVector; }

        const FVector StartLoc = OwnerChar->GetActorLocation();
        CurrentTargetLocation = StartLoc + Forward * (Radius * 0.5f);
        
        // 여기서 지면으로 스냅
        const FVector TraceStart = CurrentTargetLocation + FVector(0.f, 0.f, GroundTraceHalfHeight);
        const FVector TraceEnd = CurrentTargetLocation - FVector(0.f, 0.f, GroundTraceHalfHeight);

        FHitResult Hit;
        FCollisionQueryParams CQParams(SCENE_QUERY_STAT(SkillRangeGroundInit), false, OwnerChar);

        if (World->LineTraceSingleByChannel(
            Hit,
            TraceStart,
            TraceEnd,
            ECC_Visibility,
            CQParams))
        {
            CurrentTargetLocation = Hit.ImpactPoint + FVector(0.f, 0.f, GroundOffsetZ);
        }
        // 실패하면 기존처럼 발 위치 높이 사용
        else { CurrentTargetLocation.Z = StartLoc.Z; }

        // 캐릭터 이동 멈추기
        if (UCharacterMovementComponent* MoveComp = OwnerChar->GetCharacterMovement())
        {
            MoveComp->StopMovementImmediately();
        }

        // 캐릭터에게 "지금 Range 조준 중" 알리기
        if (AKHU_GEBCharacter* PlayerChar = Cast<AKHU_GEBCharacter>(OwnerChar))
        {
            PlayerChar->OnRangeAimingStarted(this);
        }
    }
    else { CurrentTargetLocation = Owner->GetActorLocation(); }

    // 범위 표시 이펙트 스폰/갱신
    SpawnOrUpdateIndicator();

    // 입에서 나가는 캐스팅 이펙트 (원하시면 그대로 유지)
    if (ACharacter* OwnerChar = Cast<ACharacter>(Owner))
    {
        if (USkeletalMeshComponent* Mesh = OwnerChar->GetMesh())
        {
            if (Mesh->DoesSocketExist(MuzzleSocketName) && CastNS)
            {
                UNiagaraFunctionLibrary::SpawnSystemAttached(
                    CastNS,
                    Mesh,
                    MuzzleSocketName,
                    FVector::ZeroVector,
                    FRotator::ZeroRotator,
                    EAttachLocation::SnapToTarget,
                    true
                );
            }
        }
    }

    UE_LOG(LogTemp, Log, TEXT("[Skill_Range] Start Aiming (keyboard)."));
}

void USkill_Range::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!bIsAiming) return;

    ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
    if (!OwnerChar) return;

    AController* Controller = OwnerChar->GetController();
    if (!Controller) return;

    const float Radius = GetCurrentTargetRadius();   // 조준 원 크기
    const float MaxDist = GetMaxAimDistance();        // 이동 가능한 최대 거리
    const FVector OwnerLoc = OwnerChar->GetActorLocation();

    // --- XY 평면에서 이동 (지형지물 무시) ---
    if (!AimMoveInput.IsNearlyZero())
    {
        const FRotator Rotation = Controller->GetControlRotation();
        const FRotator YawRotation(0.f, Rotation.Yaw, 0.f);

        const FVector ForwardDir = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
        const FVector RightDir = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

        FVector MoveDir = ForwardDir * AimMoveInput.Y + RightDir * AimMoveInput.X;
        MoveDir.Z = 0.f;

        if (!MoveDir.IsNearlyZero())
        {
            const FVector Delta = MoveDir.GetSafeNormal() * AimMoveSpeed * DeltaTime;
            CurrentTargetLocation += Delta;

            // 캐릭터 기준 최대 거리 제한 (XY만)
            FVector FlatOwner(OwnerLoc.X, OwnerLoc.Y, 0.f);
            FVector FlatTarget(CurrentTargetLocation.X, CurrentTargetLocation.Y, 0.f);
            FVector FlatDir = FlatTarget - FlatOwner;
            const float Dist = FlatDir.Size();

            if (MaxDist > 0.f && Dist > MaxDist)
            {
                FlatDir = FlatDir.GetSafeNormal() * MaxDist;
                FlatTarget = FlatOwner + FlatDir;
                CurrentTargetLocation.X = FlatTarget.X;
                CurrentTargetLocation.Y = FlatTarget.Y;
            }
        }
    }

    // --- Z를 지형에 맞춰 보정 (중앙 포인트를 땅에 붙이기) ---
    if (UWorld* World = GetWorld())
    {
        const FVector TraceStart = CurrentTargetLocation + FVector(0.f, 0.f, GroundTraceHalfHeight);
        const FVector TraceEnd = CurrentTargetLocation - FVector(0.f, 0.f, GroundTraceHalfHeight);

        FHitResult Hit;
        FCollisionQueryParams CQParams(SCENE_QUERY_STAT(SkillRangeGroundTick), false, OwnerChar);

        if (World->LineTraceSingleByChannel(
            Hit,
            TraceStart,
            TraceEnd,
            ECC_Visibility,
            CQParams))
        {
            CurrentTargetLocation = Hit.ImpactPoint + FVector(0.f, 0.f, GroundOffsetZ);
        }
        // 지면을 못 찾으면 최소한 캐릭터 발 높이 근처에 유지
        else { CurrentTargetLocation.Z = OwnerLoc.Z; }
    }

    bHasValidTarget = true;

    SpawnOrUpdateIndicator();

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    DrawDebugCircle(
        GetWorld(),
        CurrentTargetLocation,
        Radius,
        32,
        FColor::Red,
        false,
        -1.f,
        0,
        5.f,
        FVector(1, 0, 0),
        FVector(0, 1, 0),
        false
    );
#endif
}

void USkill_Range::StopSkill()
{
    AActor* Owner = GetOwner();
    UWorld* World = GetWorld();

    if (!bIsAiming)
    {
        Super::StopSkill();
        return;
    }

    auto CleanupState = [&]()
        {
            bIsAiming = false;
            bHasValidTarget = false;
            AimMoveInput = FVector2D::ZeroVector;
            CleanupIndicator();
            SetComponentTickEnabled(false);

            if (AKHU_GEBCharacter* PlayerChar = Cast<AKHU_GEBCharacter>(Owner))
            {
                PlayerChar->OnRangeAimingEnded(this);
            }
        };

    if (!World || !Owner)
    {
        CleanupState();
        Super::StopSkill();
        return;
    }

    // "조준 원 중심"이 곧 최종 타겟
    FVector FinalTarget = CurrentTargetLocation;

    // === 여기서 실제 비용 처리 (쿨타임 + 마나) ===
    Super::ActivateSkill();

    // === 발사 ===
    if (bHasValidTarget) { SpawnProjectileTowards(FinalTarget); }
    else { SpawnDefaultProjectile(); }

    UE_LOG(LogTemp, Log, TEXT("[Skill_Range] Stop Aiming & Fire."));

    CleanupState();
    Super::StopSkill();
}

void USkill_Range::SpawnProjectileTowards(const FVector& TargetLocation)
{
    UWorld* World = GetWorld();
    AActor* Owner = GetOwner();
    if (!World || !Owner || !FireballClass) return;

    FVector SpawnLocation = Owner->GetActorLocation();
    FRotator SpawnRotation = Owner->GetActorRotation();

    // 캐릭터의 입 소켓 기준
    if (ACharacter* OwnerChar = Cast<ACharacter>(Owner))
    {
        if (USkeletalMeshComponent* Mesh = OwnerChar->GetMesh())
        {
            if (Mesh->DoesSocketExist(MuzzleSocketName))
            {
                SpawnLocation = Mesh->GetSocketLocation(MuzzleSocketName);
                SpawnRotation = (TargetLocation - SpawnLocation).Rotation();

                // 입에서 나가는 이펙트(이미 조준 시작 시 재생했다면 생략 가능)
                // 여기서는 생략
            }
        }
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = Owner;
    SpawnParams.Instigator = Cast<APawn>(Owner);

    AFireballProjectile* Fireball = World->SpawnActor<AFireballProjectile>(
        FireballClass,
        SpawnLocation,
        SpawnRotation,
        SpawnParams);

    if (!Fireball)
    {
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("[Skill_Range] Fireball spawned toward target."));

    // 데미지/범위 세팅 (기존 로직 유지)
    const float BaseDamage = Params.Damage;
    Fireball->DirectDamage = BaseDamage * 1.0f;
    Fireball->ExplosionDamage = BaseDamage * 0.5f;
    if (Params.Range > 0.f) { Fireball->ExplosionRadius = Params.Range; }

    // 비행 중 꼬리 이펙트
    if (ProjectileNS)
    {
        UNiagaraFunctionLibrary::SpawnSystemAttached(
            ProjectileNS,
            Fireball->GetRootComponent(),
            NAME_None,
            FVector::ZeroVector,
            FRotator::ZeroRotator,
            EAttachLocation::KeepRelativeOffset,
            true
        );
    }

    // ==== 포물선 초기 속도 계산 ====
    if (UProjectileMovementComponent* Proj = Fireball->ProjectileMovement)
    {
        const float Speed = Proj->InitialSpeed;
        const FVector Start = SpawnLocation;
        const FVector End = TargetLocation;

        FVector ToTarget = End - Start;

        FVector Flat = FVector(ToTarget.X, ToTarget.Y, 0.f);
        const float DistXY = Flat.Size();
        const float DeltaZ = End.Z - Start.Z;

        const float GravityZ = World->GetGravityZ() * Proj->ProjectileGravityScale; // 음수
        const float g = -GravityZ;

        FVector InitialVelocity;

        if (Speed > 0.f && DistXY > KINDA_SMALL_NUMBER && g > KINDA_SMALL_NUMBER)
        {
            const float Speed2 = Speed * Speed;
            const float Speed4 = Speed2 * Speed2;

            const float Disc = Speed4 - g * (g * DistXY * DistXY + 2.f * DeltaZ * Speed2);

            if (Disc >= 0.f)
            {
                // 두 개의 해 중 하나 선택 (여기서는 낮은 탄도)
                const float RootDisc = FMath::Sqrt(Disc);
                const float TanTheta = (Speed2 - RootDisc) / (g * DistXY);

                const float CosTheta = 1.f / FMath::Sqrt(1.f + TanTheta * TanTheta);
                const float SinTheta = TanTheta * CosTheta;

                const FVector DirXY = Flat.GetSafeNormal();

                InitialVelocity =
                    DirXY * (Speed * CosTheta) +
                    FVector::UpVector * (Speed * SinTheta);
            }
            // 해당 속도로는 도달 불가 → 그냥 직선 발사
            else { InitialVelocity = ToTarget.GetSafeNormal() * Speed; }
        }
        // 이상값 → 그냥 직선 발사
        else { InitialVelocity = ToTarget.GetSafeNormal() * (Speed > 0.f ? Speed : 1000.f); }

        Proj->Velocity = InitialVelocity;
    }
}

void USkill_Range::SpawnDefaultProjectile()
{
    // 기존 ActivateSkill에서 하던 "그냥 앞쪽으로 발사" 로직을 여기로 옮김
    UWorld* World = GetWorld();
    AActor* Owner = GetOwner();
    if (!World || !Owner || !FireballClass) return;

    FVector  SpawnLocation = Owner->GetActorLocation();
    FRotator SpawnRotation = Owner->GetActorRotation();

    if (ACharacter* OwnerChar = Cast<ACharacter>(Owner))
    {
        if (USkeletalMeshComponent* Mesh = OwnerChar->GetMesh())
        {
            if (Mesh->DoesSocketExist(MuzzleSocketName))
            {
                SpawnLocation = Mesh->GetSocketLocation(MuzzleSocketName);
                SpawnRotation = Mesh->GetSocketRotation(MuzzleSocketName);
            }
            else
            {
                const FVector Forward = OwnerChar->GetActorForwardVector();
                SpawnLocation = OwnerChar->GetActorLocation()
                    + Forward * 50.f
                    + FVector(0.f, 0.f, 50.f);
                SpawnRotation = Forward.Rotation();
            }
        }
    }

    SpawnRotation.Pitch += LaunchPitchOffsetDegrees;

    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = Owner;
    SpawnParams.Instigator = Cast<APawn>(Owner);

    if (AFireballProjectile* Fireball = World->SpawnActor<AFireballProjectile>(
        FireballClass,
        SpawnLocation,
        SpawnRotation,
        SpawnParams))
    {
        const float BaseDamage = Params.Damage;
        Fireball->DirectDamage = BaseDamage * 1.0f;
        Fireball->ExplosionDamage = BaseDamage * 0.5f;

        if (Params.Range > 0.f) { Fireball->ExplosionRadius = Params.Range; }

        if (Fireball->ProjectileMovement)
        {
            const FVector Dir = SpawnRotation.Vector();
            Fireball->ProjectileMovement->Velocity =
                Dir * Fireball->ProjectileMovement->InitialSpeed;
        }

        if (ProjectileNS)
        {
            UNiagaraFunctionLibrary::SpawnSystemAttached(
                ProjectileNS,
                Fireball->GetRootComponent(),
                NAME_None,
                FVector::ZeroVector,
                FRotator::ZeroRotator,
                EAttachLocation::KeepRelativeOffset,
                true
            );
        }
    }
}

float USkill_Range::GetCurrentTargetRadius() const
{
    // 이제 SkillDefinition.Params.Range 는 "폭발 반경"만 쓰고
    // 조준 원 크기는 이 값만 사용합니다.
    return TargetRadius;
}

float USkill_Range::GetMaxAimDistance() const
{
    return MaxAimDistance;
}

void USkill_Range::HandleAimMoveInput(const FVector2D& Input)
{
    AimMoveInput = Input;
}

/*=============================Swift=============================*/

void USkill_Swift::InitializeFromDefinition(const USkillDefinition* Def)
{
    Super::InitializeFromDefinition(Def);

    if (Params.Damage > 0.f) { DamagePerSample = Params.Damage; }
    if (Params.Range > 0.f) { DashDistance = Params.Range; }
}

bool USkill_Swift::CanActivate() const
{
    // 지금은 별도 제약 없이 기본 조건만 사용
    return Super::CanActivate();
}

void USkill_Swift::ActivateSkill()
{
    UWorld* World = GetWorld();
    AActor* Owner = GetOwner();
    if (!World || !Owner) return;

    if (!CanActivate())
    {
        UE_LOG(LogTemp, Warning, TEXT("[Skill_Swift] ActivateSkill blocked (CanActivate=false)"));
        return;
    }

    // 쿨타임 시작 + 마나 1회 소모.
    Super::ActivateSkill();

    // ----- 시작/끝 위치 계산 -----
    const FVector StartLocation = Owner->GetActorLocation();

    FVector Forward = Owner->GetActorForwardVector();
    Forward.Z = 0.f;
    if (!Forward.Normalize()) { Forward = FVector::ForwardVector; }

    const FVector EndLocation = StartLocation + Forward * DashDistance;

    // ----- 큰 직육면체(Oriented Box) 안의 적에게 데미지 -----
   // ----- 경로 상의 적(ACharacter) 수집용 박스 계산 -----
    const FVector Segment = EndLocation - StartLocation;
    const float Distance = Segment.Size();

    SwiftTargets.Reset();
    CurrentHitIndex = 0;

    if (Distance > KINDA_SMALL_NUMBER)
    {
        const FVector Direction = Segment / Distance;
        const FVector BoxCenter = StartLocation + 0.5f * Segment;
        const FQuat   BoxRotation = FRotationMatrix::MakeFromX(Direction).ToQuat();

        // X: 진행 방향, Y: 좌우, Z: 상하
        const float ExtraForwardPadding = 50.f; // 살짝 여유
        const FVector HalfExtent(
            Distance * 0.5f + ExtraForwardPadding,
            BoxHalfWidth,
            BoxHalfHeight);

        TArray<FOverlapResult> Overlaps;
        FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(SkillSwift), false, Owner);
        FCollisionObjectQueryParams ObjParams;
        ObjParams.AddObjectTypesToQuery(ECC_Pawn); // Pawn (적들)만 확인

        const bool bAnyHit = World->OverlapMultiByObjectType(
            Overlaps,
            BoxCenter,
            BoxRotation,
            ObjParams,
            FCollisionShape::MakeBox(HalfExtent),
            QueryParams);

        if (bAnyHit)
        {
            for (const FOverlapResult& O : Overlaps)
            {
                AActor* Other = O.GetActor();
                if (!Other || Other == Owner) continue;

                if (ACharacter* OtherChar = Cast<ACharacter>(Other))
                {
                    SwiftTargets.AddUnique(OtherChar);
                }
            }
        }

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
        DrawDebugBox(World, BoxCenter, HalfExtent, BoxRotation, FColor::Cyan, false, 0.2f);
#endif
    }

    // ----- 실제 점멸 이동 (기존 코드 그대로) -----
    if (ACharacter* OwnerChar = Cast<ACharacter>(Owner))
    {
        if (UCapsuleComponent* Capsule = OwnerChar->GetCapsuleComponent())
        {
            const ECollisionResponse OriginalPawnResponse =
                Capsule->GetCollisionResponseToChannel(ECC_Pawn);

            Capsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);

            FHitResult Hit;
            Owner->SetActorLocation(EndLocation, true, &Hit, ETeleportType::TeleportPhysics);

            Capsule->SetCollisionResponseToChannel(ECC_Pawn, OriginalPawnResponse);
        }
        else { Owner->SetActorLocation(EndLocation); }
    }
    else { Owner->SetActorLocation(EndLocation); }

    // ----- 수집된 타겟들을 DamageSamples번에 걸쳐 때리기 -----
    if (SwiftTargets.Num() > 0 && DamageSamples > 0)
    {
        const float Interval = FMath::Max(HitInterval, KINDA_SMALL_NUMBER);
        World->GetTimerManager().SetTimer(
            SwiftDamageTimerHandle,
            this,
            &USkill_Swift::HandleSwiftDamageTick,
            Interval,
            true);

        UE_LOG(LogTemp, Log,
            TEXT("[Skill_Swift] Start multi hit: Targets=%d, Hits=%d, Interval=%.2f"),
            SwiftTargets.Num(), DamageSamples, Interval);
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("[Skill_Swift] No targets found for multi hit."));
    }
}

void USkill_Swift::StopSkill()
{
    Super::StopSkill();
}

void USkill_Swift::HandleSwiftDamageTick()
{
    UWorld* World = GetWorld();
    AActor* Owner = GetOwner();
    if (!World || !Owner)
    {
        World->GetTimerManager().ClearTimer(SwiftDamageTimerHandle);
        return;
    }

    ++CurrentHitIndex;

    // Hit 인덱스가 DamageSamples를 넘으면 종료
    if (CurrentHitIndex > DamageSamples)
    {
        World->GetTimerManager().ClearTimer(SwiftDamageTimerHandle);
        UE_LOG(LogTemp, Log, TEXT("[Skill_Swift] Multi hit finished."));
        return;
    }

    // 타겟 목록을 돌면서 한 번씩 데미지
    for (int32 i = SwiftTargets.Num() - 1; i >= 0; --i)
    {
        ACharacter* TargetChar = SwiftTargets[i].Get();
        if (!TargetChar)
        {
            SwiftTargets.RemoveAtSwap(i);
            continue;
        }

        // 고정 피해, 방어력 무시, 1타씩 직접 넣기
        DealSkillDamage(
            TargetChar,
            DamagePerSample,
            /*bIgnoreDefense=*/true,
            /*bPeriodic=*/false,
            /*HitCount=*/1); // 우리 쪽에서 10번 반복 호출하므로 HitCount는 1로

        // Hit 이펙트
        if (HitNS)
        {
            UNiagaraFunctionLibrary::SpawnSystemAtLocation(
                World,
                HitNS,
                TargetChar->GetActorLocation(),
                TargetChar->GetActorRotation());
        }
    }

    // 더 이상 유효 타겟이 없으면 타이머 정리
    if (SwiftTargets.Num() == 0)
    {
        World->GetTimerManager().ClearTimer(SwiftDamageTimerHandle);
        UE_LOG(LogTemp, Log, TEXT("[Skill_Swift] Multi hit stopped (no more targets)."));
    }
}

/*=============================Guard=============================*/

void USkill_Guard::InitializeFromDefinition(const USkillDefinition* Def)
{
    Super::InitializeFromDefinition(Def);

    if (Params.ManaCost > 0.f) { ManaPerShield = Params.ManaCost; }
    if (Params.Damage > 0.f) { DamagePerSheild = Params.Damage; }
    if (Params.Range > 0.f) { ExplosionRadius = Params.Range; }
}

bool USkill_Guard::CanActivate() const
{
    if (bIsActive)
    {
        UE_LOG(LogTemp, Verbose, TEXT("[Skill_Guard] CanActivate? false (already active)"));
        return false;
    }

    UManaComponent* Mana = GetManaComponent();
    if (!Mana)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Skill_Guard] CanActivate? false (no ManaComponent)"));
        return false;
    }

    const float Current = Mana->GetCurrentMana();
    const float Target = 100.f;
    const float Tolerance = 0.1f; // float 오차 방지용

    if (FMath::Abs(Current - Target) > Tolerance)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[Skill_Guard] CanActivate? false (Mana must be 100. Current=%.1f)"),
            Current);
        return false;
    }

    // 쿨타임은 지금은 사용하지 않음 (필요하면 여기서 Now < NextAvailableTime 체크 추가)
    return true;
}

void USkill_Guard::ActivateSkill()
{
    // 이미 켜져 있으면 다시 초기화하지 않고 무시
    if (bIsActive)
    {
        UE_LOG(LogTemp, Verbose,
            TEXT("[Skill_Guard] ActivateSkill called while already active. Ignoring."));
        return;
    }

    UWorld* World = GetWorld();
    AActor* Owner = GetOwner();
    if (!World || !Owner) return;

    bIsActive = true;
    bEndedByDepletion = false;

    RemainingShields = FMath::Max(MaxShields, 0);
    ConsumedShields = 0;

    if (UManaComponent* Mana = GetManaComponent()) { Mana->AddRegenBlock(); }

    // 보호막 이펙트 켜기
    if (SkillNS && !SpawnedNS)
    {
        SpawnedNS = UNiagaraFunctionLibrary::SpawnSystemAttached(
            SkillNS,
            Owner->GetRootComponent(),
            NAME_None,
            FVector::ZeroVector,
            FRotator::ZeroRotator,
            EAttachLocation::SnapToTarget,
            true);
    }

    UE_LOG(LogTemp, Log, TEXT("[Skill_Guard] Activated: MaxShields=%d"), RemainingShields);
}

bool USkill_Guard::HandleIncomingDamage(
    float Damage,
    const UDamageType* DamageType,
    AController* InstigatedBy,
    AActor* DamageCauser)
{
    // 스킬이 꺼져 있거나, 남은 배리어가 없으면 그냥 맞게 둔다.
    if (!bIsActive || RemainingShields <= 0)
    {
        UE_LOG(LogTemp, Verbose,
            TEXT("[Skill_Guard] HandleIncomingDamage: no shield (Active=%d, Remaining=%d)"),
            bIsActive ? 1 : 0, RemainingShields);
        return false;
    }

    // 배리어 1개 소모
    RemainingShields = FMath::Max(RemainingShields - 1, 0);
    ConsumedShields++;

    UE_LOG(LogTemp, Log,
        TEXT("[Skill_Guard] Damage absorbed. RemainingShields=%d, Consumed=%d"),
        RemainingShields, ConsumedShields);

    if (RemainingShields <= 0)
    {
        bEndedByDepletion = true;
        UE_LOG(LogTemp, Log,
            TEXT("[Skill_Guard] Shields depleted. No explosion will occur on stop."));
    }

    if (UManaComponent* Mana = GetManaComponent())
    {
        if (ManaPerShield > 0.f)
        {
            const float Before = Mana->GetCurrentMana();
            Mana->ConsumeMana(ManaPerShield);
            const float After = Mana->GetCurrentMana();

            UE_LOG(LogTemp, Log,
                TEXT("[Skill_Guard] Consumed mana per shield: %.1f -> %.1f (Delta=%.1f)"),
                Before, After, Before - After);
        }
    }

    return true;
}

void USkill_Guard::StopSkill()
{
    // 이미 꺼져 있으면 무시
    if (!bIsActive)
    {
        Super::StopSkill();
        return;
    }

    UWorld* World = GetWorld();
    AActor* Owner = GetOwner();

    // 이펙트 끄기
    if (SpawnedNS)
    {
        SpawnedNS->Deactivate();
        SpawnedNS = nullptr;
    }

    // --- 3-1. 마나를 0으로 설정 ---
    if (UManaComponent* Mana = GetManaComponent())
    {
        const float Current = Mana->GetCurrentMana();
        if (Current > 0.f)
        {
            // 현재 마나만큼 한 번에 소모 → 0이 됨
            Mana->ConsumeMana(Current);
        }
        UE_LOG(LogTemp, Log,
            TEXT("[Skill_Guard] Mana set to 0 on skill end (was %.1f)"), Current);

        Mana->RemoveRegenBlock();
    }

    // --- 3-2. 배리어가 소모된 만큼 광역 대미지 ---
    if (World && Owner && ConsumedShields > 0 && DamagePerSheild > 0.f && !bEndedByDepletion)
    {
        const float TotalDamage = DamagePerSheild * ConsumedShields;

        TArray<AActor*> IgnoreActors;
        IgnoreActors.Add(Owner);

        UE_LOG(LogTemp, Log,
            TEXT("[Skill_Guard] Stop: ConsumedShields=%d, TotalDamage=%.1f"),
            ConsumedShields, TotalDamage);

        UGameplayStatics::ApplyRadialDamage(
            World,
            TotalDamage,
            Owner->GetActorLocation(),
            ExplosionRadius,
            nullptr,            // Default DamageType
            IgnoreActors,
            Owner,
            Owner->GetInstigatorController(),
            true);              // Do full damage
    }
    else
    {
        UE_LOG(LogTemp, Log,
            TEXT("[Skill_Guard] Stop: No shields consumed or no damage value. No explosion."));
    }

    // 상태 리셋
    bIsActive = false;
    RemainingShields = 0;
    ConsumedShields = 0;
    bEndedByDepletion = false;

    Super::StopSkill();
}

/*=============================Special=============================*/

void USkill_Special::InitializeFromDefinition(const USkillDefinition* Def)
{
    Super::InitializeFromDefinition(Def);

    if (Params.Damage > 0.f) { SelfHealPerTick = Params.Damage * 2.f; DotDamagePerTick = Params.Damage; }
    if (Params.Range > 0.f) { FogRadius = Params.Range; }
}

bool USkill_Special::CanActivate() const
{
    // 이미 켜져 있으면 다시는 못 켬
    if (bIsActive)
    {
        UE_LOG(LogTemp, Verbose, TEXT("[Skill_Special] CanActivate? false (already active)"));
        return false;
    }

    // 나머지(쿨타임, 마나)는 공통 로직 사용
    return Super::CanActivate();
}

void USkill_Special::ActivateSkill()
{
    UWorld* World = GetWorld();
    AActor* Owner = GetOwner();
    if (!World || !Owner) return;

    // SkillManager에서 한 번 CanActivate를 호출하지만,
    // 혹시 모를 중복 호출 방지를 위해 한 번 더 체크
    if (!CanActivate())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[Skill_Special] ActivateSkill blocked (CanActivate=false)"));
        return;
    }

    // 공통 비용 처리 (쿨타임 시작 + 마나 소모)
    Super::ActivateSkill();

    bIsActive = true;

    // 1) 플레이어 이동속도 배율 적용
    if (AKHU_GEBCharacter* OwnerChar = Cast<AKHU_GEBCharacter>(Owner))
    {
        CachedOwnerChar = OwnerChar;
        OwnerChar->SetSkillSpeedMultiplier(SelfMoveSpeedMultiplier);
    }

    // 2) 어디에 붙일지 결정 (가능하면 Mesh, 아니면 루트)
    USceneComponent* AttachComp = Owner->GetRootComponent();

    if (ACharacter* OwnerCharacter = Cast<ACharacter>(Owner))
    {
        if (USkeletalMeshComponent* Mesh = OwnerCharacter->GetMesh())
        {
            AttachComp = Mesh;
        }
    }

    if (DarkFogNS && AttachComp)
    {
        // 플레이어를 따라다니는 원형 흑안개 스폰 & 부착
        SpawnedNS = UNiagaraFunctionLibrary::SpawnSystemAttached(
            DarkFogNS,
            AttachComp,
            AttachSocketName.IsNone() ? NAME_None : AttachSocketName,
            RelativeOffset,
            FRotator::ZeroRotator,
            EAttachLocation::KeepRelativeOffset,
            true  // AutoDestroy: true → 소유자와 함께 정리
        );
    }

    // 3) 지속시간 타이머 시작
    if (Duration > 0.f)
    {
        World->GetTimerManager().SetTimer(
            DurationTimerHandle,
            this,
            &USkill_Special::OnDurationEnded,
            Duration,
            false);
    }

    // 4) 흑안개 안의 적 슬로우를 주기적으로 갱신
    if (FogRadius > 0.f && SlowTickInterval > 0.f)
    {
        World->GetTimerManager().SetTimer(
            SlowTickTimerHandle,
            this,
            &USkill_Special::UpdateFogEffects,
            SlowTickInterval,
            true);
    }

    // 5) 힐/도트 효과 타이머
    if (EffectTickInterval > 0.f)
    {
        World->GetTimerManager().SetTimer(
            EffectTickTimerHandle,
            this,
            &USkill_Special::OnEffectTick,
            EffectTickInterval,
            true);
    }

    UE_LOG(LogTemp, Log, TEXT("[Skill_Special] Activated (DarkFog spawned, duration=%.1f)"), Duration);
}

void USkill_Special::StopSkill()
{
    // 입력(우클릭 해제)으로 들어오는 Stop은 무시하고,
    // 오직 지속시간 타이머가 끝날 때만 실제 종료되도록 합니다.
    UE_LOG(LogTemp, Log,
        TEXT("[Skill_Special] StopSkill called (probably from input). Ignoring; duration-based skill."));
}

/** 지속시간 종료 콜백 */
void USkill_Special::OnDurationEnded()
{
    UE_LOG(LogTemp, Log, TEXT("[Skill_Special] Duration ended."));
    EndSpecial();
}

/** 흑안개 영역 안의 적들에게 슬로우 적용 / 영역 밖으로 나간 적은 복구 */
void USkill_Special::UpdateFogEffects()
{
    UWorld* World = GetWorld();
    AActor* Owner = GetOwner();
    if (!World || !Owner || !bIsActive) return;

    // 흑안개 중심 위치
    FVector Center;
    if (SpawnedNS) { Center = SpawnedNS->GetComponentLocation(); }
    else { Center = Owner->GetActorLocation() + Owner->GetActorRotation().RotateVector(RelativeOffset); }

    // 현재 영역 안에 있는 적들
    TArray<FOverlapResult> Overlaps;
    FCollisionObjectQueryParams ObjParams;
    ObjParams.AddObjectTypesToQuery(ECC_Pawn);

    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(SkillSpecialFog), false, Owner);

    const bool bAnyHit = World->OverlapMultiByObjectType(
        Overlaps,
        Center,
        FQuat::Identity,
        ObjParams,
        FCollisionShape::MakeSphere(FogRadius),
        QueryParams);

    TSet<TWeakObjectPtr<ACharacter>> CurrentlyInside;

    if (bAnyHit)
    {
        for (const FOverlapResult& O : Overlaps)
        {
            AActor* Other = O.GetActor();
            ACharacter* OtherChar = Cast<ACharacter>(Other);
            if (!OtherChar || OtherChar == Owner) continue;

            CurrentlyInside.Add(OtherChar);

            if (UCharacterMovementComponent* MoveComp = OtherChar->GetCharacterMovement())
            {
                // 처음 슬로우 되는 적이면 원래 속도 저장
                if (!OriginalEnemySpeeds.Contains(OtherChar))
                {
                    OriginalEnemySpeeds.Add(OtherChar, MoveComp->MaxWalkSpeed);
                }

                const float OriginalSpeed = OriginalEnemySpeeds[OtherChar];
                MoveComp->MaxWalkSpeed = OriginalSpeed * EnemyMoveSpeedMultiplier;
            }
        }
    }

    // 더 이상 영역 안에 없어진 적들 → 속도 복구
    for (auto It = OriginalEnemySpeeds.CreateIterator(); It; ++It)
    {
        ACharacter* EnemyChar = It.Key().Get();
        const float OriginalSpeed = It.Value();

        if (!EnemyChar ||
            !CurrentlyInside.Contains(EnemyChar) ||
            !EnemyChar->GetCharacterMovement())
        {
            if (EnemyChar && EnemyChar->GetCharacterMovement())
            {
                EnemyChar->GetCharacterMovement()->MaxWalkSpeed = OriginalSpeed;
            }
            It.RemoveCurrent();
        }
    }
}

/** Special 종료: 버프/슬로우/이펙트/타이머 정리 */
void USkill_Special::EndSpecial()
{
    if (!bIsActive) return;

    bIsActive = false;

    UWorld* World = GetWorld();
    AActor* Owner = GetOwner();

    // 1) 타이머 정리
    if (World)
    {
        World->GetTimerManager().ClearTimer(DurationTimerHandle);
        World->GetTimerManager().ClearTimer(SlowTickTimerHandle);
        World->GetTimerManager().ClearTimer(EffectTickTimerHandle);
    }

    // 2) 플레이어 이동속도 배율 복구
    if (CachedOwnerChar.IsValid())
    {
        CachedOwnerChar->SetSkillSpeedMultiplier(1.0f);
        CachedOwnerChar = nullptr;
    }

    // 3) 적들 이동속도 복구
    for (auto& Pair : OriginalEnemySpeeds)
    {
        ACharacter* EnemyChar = Pair.Key.Get();
        const float OriginalSpeed = Pair.Value;

        if (EnemyChar && EnemyChar->GetCharacterMovement())
        {
            EnemyChar->GetCharacterMovement()->MaxWalkSpeed = OriginalSpeed;
        }
    }
    OriginalEnemySpeeds.Empty();

    // 4) 나이아가라 이펙트 끄기
    if (SpawnedNS)
    {
        SpawnedNS->Deactivate();
        SpawnedNS = nullptr;
    }

    UE_LOG(LogTemp, Log, TEXT("[Skill_Special] EndSpecial: All buffs/debuffs cleared."));

    // 5) 공통 Stop 로그
    Super::StopSkill();
}

void USkill_Special::OnEffectTick()
{
    UWorld* World = GetWorld();
    AActor* Owner = GetOwner();
    if (!World || !Owner || !bIsActive) return;

    // 1) 플레이어 힐
    if (SelfHealPerTick > 0.f)
    {
        if (UHealthComponent* Health = Owner->FindComponentByClass<UHealthComponent>())
        {
            Health->AddHealth(SelfHealPerTick);
        }
    }

    // 2) 흑안개 안 적들에게 방어무시 도트 데미지
    if (FogRadius <= 0.f || DotDamagePerTick <= 0.f) return;

    TArray<FOverlapResult> Overlaps;
    FCollisionObjectQueryParams ObjParams;
    ObjParams.AddObjectTypesToQuery(ECC_Pawn);

    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(SkillSpecialDot), false, Owner);

    const bool bAnyHit = World->OverlapMultiByObjectType(
        Overlaps,
        Owner->GetActorLocation(),
        FQuat::Identity,
        ObjParams,
        FCollisionShape::MakeSphere(FogRadius),
        QueryParams);

    if (!bAnyHit) return;

    // 이 틱에 이미 맞춘 캐릭터를 기록
    TSet<ACharacter*> UniqueTargets;

    for (const FOverlapResult& O : Overlaps)
    {
        AActor* Other = O.GetActor();
        ACharacter* OtherChar = Cast<ACharacter>(Other);
        if (!OtherChar || OtherChar == Owner) continue;

        if (UniqueTargets.Contains(OtherChar)) continue;
        UniqueTargets.Add(OtherChar);

        // 도트용 고정 피해 모드 사용 (한 틱당 한 번)
        ApplyFixedDotDamage(
            this,
            OtherChar,
            DotDamagePerTick,  // 예: 5
            1);
    }
}
