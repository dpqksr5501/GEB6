// Fill out your copyright notice in the Description page of Project Settings.


#include "Skills/Skill_Range.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "KHU_GEBCharacter.h"
#include "SkillManagerComponent.h"
#include "FireballProjectile.h"

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

    if (USkillManagerComponent* Manager = GetSkillManager())
    {
        Manager->OnRangeAimingStarted(this);
    }

    if (ACharacter* OwnerChar = Cast<ACharacter>(Owner))
    {
        const float Radius = GetCurrentTargetRadius();
        const float MaxDist = GetMaxAimDistance();
        const FVector StartLoc = OwnerChar->GetActorLocation();

        // --- 1) 기본 초기 위치 = 자기 앞쪽 Range의 절반 지점 ---
        FVector InitialLoc;

        FVector Forward = OwnerChar->GetActorForwardVector();
        Forward.Z = 0.f;
        if (!Forward.Normalize())
        {
            Forward = FVector::ForwardVector;
        }
        InitialLoc = StartLoc + Forward * (Radius * 0.5f);

        // --- 2) 만약 플레이어이고, 락온 타겟이 있다면 → 락온 타겟 위치를 우선 사용 ---
        if (AKHU_GEBCharacter* PlayerChar = Cast<AKHU_GEBCharacter>(OwnerChar))
        {
            if (AActor* LockOnTarget = PlayerChar->GetLockOnTarget())
            {
                InitialLoc = LockOnTarget->GetActorLocation();

                // 너무 멀리 있으면 MaxAimDistance 안으로 클램프
                if (MaxDist > 0.f)
                {
                    FVector FlatOwner(StartLoc.X, StartLoc.Y, 0.f);
                    FVector FlatTarget(InitialLoc.X, InitialLoc.Y, 0.f);
                    FVector FlatDir = FlatTarget - FlatOwner;
                    const float Dist = FlatDir.Size();

                    if (Dist > MaxDist && Dist > KINDA_SMALL_NUMBER)
                    {
                        FlatDir = FlatDir / Dist * MaxDist;
                        FlatTarget = FlatOwner + FlatDir;
                        InitialLoc.X = FlatTarget.X;
                        InitialLoc.Y = FlatTarget.Y;
                    }
                }
            }
        }

        CurrentTargetLocation = InitialLoc;

        // --- 3) 여기서 지면으로 스냅 ---
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
        else { CurrentTargetLocation.Z = StartLoc.Z; }

        // 캐릭터 이동 멈추기 (기존)
        if (UCharacterMovementComponent* MoveComp = OwnerChar->GetCharacterMovement())
        {
            MoveComp->StopMovementImmediately();
        }
    }
    else { CurrentTargetLocation = Owner->GetActorLocation(); }

    // 범위 표시 이펙트 스폰/갱신
    SpawnOrUpdateIndicator();

    // 입에서 나가는 캐스팅 이펙트
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

    // --- 카메라/캐릭터가 조준 원 중심을 바라보도록 Yaw 회전 ---
    if (Controller)
    {
        FVector ToTarget = CurrentTargetLocation - OwnerLoc;
        ToTarget.Z = 0.f;

        if (!ToTarget.IsNearlyZero())
        {
            const float InterpSpeed = 10.f; // 필요하면 UPROPERTY로 빼도 됨

            const FRotator DesiredYawRot = ToTarget.Rotation(); // Yaw만 의미 있음
            const FRotator CurrentRot = Controller->GetControlRotation();

            FRotator TargetRot = CurrentRot;
            TargetRot.Yaw = DesiredYawRot.Yaw;

            const FRotator NewRot = FMath::RInterpTo(
                CurrentRot,
                TargetRot,
                DeltaTime,
                InterpSpeed
            );

            Controller->SetControlRotation(NewRot);
        }
    }
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

            if (USkillManagerComponent* Manager = GetSkillManager())
            {
                Manager->OnRangeAimingEnded(this);
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

    //애니메이션 몽타주 재생
    PlayFormSkillMontage();
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

    // 데미지/범위 세팅
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

float USkill_Range::GetCurrentTargetRadius() const { return TargetRadius; }
float USkill_Range::GetMaxAimDistance() const { return MaxAimDistance; }
void USkill_Range::HandleAimMoveInput(const FVector2D& Input) { AimMoveInput = Input; }
