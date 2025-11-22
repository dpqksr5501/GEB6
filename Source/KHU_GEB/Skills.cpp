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
#include "DrawDebugHelpers.h"

/*=============================Base=============================*/


/*=============================Range=============================*/

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

    // 기본 마나/쿨타임 처리 (SkillBase::CanActivate는 SkillManager에서 이미 한 번 체크됨)
    Super::ActivateSkill();

    FVector  SpawnLocation = Owner->GetActorLocation();
    FRotator SpawnRotation = Owner->GetActorRotation();

    // 캐릭터의 입 소켓 기준으로 위치/방향 결정
    if (ACharacter* OwnerChar = Cast<ACharacter>(Owner))
    {
        if (USkeletalMeshComponent* Mesh = OwnerChar->GetMesh())
        {
            if (Mesh->DoesSocketExist(MuzzleSocketName))
            {
                SpawnLocation = Mesh->GetSocketLocation(MuzzleSocketName);
                SpawnRotation = Mesh->GetSocketRotation(MuzzleSocketName);

                // 입 소켓에서 나가는 나이아가라
                if (CastNS)
                {
                    UNiagaraFunctionLibrary::SpawnSystemAttached(
                        CastNS,
                        Mesh,
                        MuzzleSocketName,
                        FVector::ZeroVector,
                        FRotator::ZeroRotator,
                        EAttachLocation::SnapToTarget,
                        true  // AutoDestroy
                    );
                }
            }
            else
            {
                // 소켓이 없으면 대충 머리 근처에서 전방으로
                const FVector Forward = OwnerChar->GetActorForwardVector();
                SpawnLocation = OwnerChar->GetActorLocation()
                    + Forward * 50.f
                    + FVector(0.f, 0.f, 50.f);
                SpawnRotation = Forward.Rotation();

                // 소켓이 없더라도 위치 기준으로 이펙트만은 재생
                if (CastNS)
                {
                    UNiagaraFunctionLibrary::SpawnSystemAtLocation(
                        World,
                        CastNS,
                        SpawnLocation,
                        SpawnRotation
                    );
                }
            }
        }
    }

    // 위/아래 발사 각도 조정 (기본 -10도 = 위로 약간)
    SpawnRotation.Pitch += LaunchPitchOffsetDegrees;

    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = Owner;
    SpawnParams.Instigator = Cast<APawn>(Owner);

    // AFireballProjectile로 스폰
    if (AFireballProjectile* Fireball = World->SpawnActor<AFireballProjectile>(
        FireballClass,
        SpawnLocation,
        SpawnRotation,
        SpawnParams))
    {
        UE_LOG(LogTemp, Log, TEXT("[Skill_Range] Fireball spawned: %s"), *GetNameSafe(Fireball));

        // 1) Params.Damage 기반으로 Direct/Explosion 데미지 나누기
        const float BaseDamage = Params.Damage;             // SkillDefinition의 Damage

        Fireball->DirectDamage = BaseDamage * 1.0f;      // ★ 1배
        Fireball->ExplosionDamage = BaseDamage * 0.5f;      // ★ 0.5배

        // 2) 폭발 반경은 Range를 그대로 쓰도록 (원하면 에디터에서 덮어쓰면 됨)
        if (Params.Range > 0.f)
        {
            Fireball->ExplosionRadius = Params.Range;
        }

        // 3) 발사 방향/속도 설정 (ProjectileMovement의 InitialSpeed 사용)
        if (Fireball->ProjectileMovement)
        {
            const FVector Dir = SpawnRotation.Vector();
            Fireball->ProjectileMovement->Velocity = Dir * Fireball->ProjectileMovement->InitialSpeed;
        }

        // 4) 화염구 비행 중 나이아가라 (꼬리/바디)
        if (ProjectileNS)
        {
            UNiagaraFunctionLibrary::SpawnSystemAttached(
                ProjectileNS,
                Fireball->GetRootComponent(),
                NAME_None,
                FVector::ZeroVector,
                FRotator::ZeroRotator,
                EAttachLocation::KeepRelativeOffset,
                true  // AutoDestroy
            );
        }
    }
}

/*=============================Swift=============================*/

void USkill_Swift::InitializeFromDefinition(const USkillDefinition* Def)
{
    Params = Def ? Def->Params : FSkillParams{};

    // SkillDefinition.Params.Range가 있으면 대쉬 거리로 사용
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
    if (!Forward.Normalize())
    {
        Forward = FVector::ForwardVector;
    }

    const FVector EndLocation = StartLocation + Forward * DashDistance;

    // ----- 큰 직육면체(Oriented Box) 안의 적에게 데미지 -----
    const FVector Segment = EndLocation - StartLocation;
    const float Distance = Segment.Size();

    if (Distance > KINDA_SMALL_NUMBER)
    {
        const FVector Direction = Segment / Distance;
        const FVector BoxCenter = StartLocation + 0.5f * Segment;
        const FQuat   BoxRotation = FRotationMatrix::MakeFromX(Direction).ToQuat();

        // X: 진행 방향, Y: 좌우, Z: 상하
        const float ExtraForwardPadding = 50.f; // 살짝 여유
        const FVector HalfExtent(Distance * 0.5f + ExtraForwardPadding,
            BoxHalfWidth,
            BoxHalfHeight);

        const int32 NumSamples = FMath::Max(DamageSamples, 1);

        for (int32 i = 0; i < NumSamples; ++i)
        {
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

                    // ACharacter를 상속받는 적에게 데미지
                    if (ACharacter* OtherChar = Cast<ACharacter>(Other))
                    {
                        // 고정 피해, 방어력 무시, Swift 스킬에서 직접 넣음
                        DealSkillDamage(
                            OtherChar,
                            Params.Damage,
                            /*bIgnoreDefense=*/true,
                            /*bPeriodic=*/false,
                            /*HitCount=*/DamageSamples);

                        // Hit Niagara 재생
                        if (HitNS)
                        {
                            UNiagaraFunctionLibrary::SpawnSystemAtLocation(
                                World,
                                HitNS,
                                OtherChar->GetActorLocation(),
                                Owner->GetActorRotation()
                            );
                        }
                    }

                }
            }

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
            // 디버그: 만들어지는 직육면체 확인
            DrawDebugBox(World, BoxCenter, HalfExtent, BoxRotation, FColor::Cyan, false, 0.2f);
#endif
        }
    }

    // ----- 실제 점멸 이동: 적(ACharacter)을 통과하도록 Pawn 충돌 무시 -----
    if (ACharacter* OwnerChar = Cast<ACharacter>(Owner))
    {
        if (UCapsuleComponent* Capsule = OwnerChar->GetCapsuleComponent())
        {
            const ECollisionResponse OriginalPawnResponse =
                Capsule->GetCollisionResponseToChannel(ECC_Pawn);

            // 적(=Pawn)과의 충돌은 무시 (통과)
            Capsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);

            FHitResult Hit;
            Owner->SetActorLocation(EndLocation, true, &Hit, ETeleportType::TeleportPhysics);

            // 원래 설정 복원
            Capsule->SetCollisionResponseToChannel(ECC_Pawn, OriginalPawnResponse);
        }
        else
        {
            // 캡슐이 없으면 그냥 스윕 없이 텔레포트
            Owner->SetActorLocation(EndLocation, false, nullptr, ETeleportType::TeleportPhysics);
        }
    }
    else
    {
        // 캐릭터가 아니면 간단히 이동
        Owner->SetActorLocation(EndLocation, false, nullptr, ETeleportType::TeleportPhysics);
    }

    // 한 번에 끝나는 스킬이라 바로 종료
    StopSkill();
}

void USkill_Swift::StopSkill()
{
    Super::StopSkill();
}

/*=============================Guard=============================*/

void USkill_Guard::InitializeFromDefinition(const USkillDefinition* Def)
{
    // 공통 파라미터( Damage, Range, ManaCost, Cooldown ) 로드
    Super::InitializeFromDefinition(Def);

    // Range를 기본 폭발 반경으로 사용 (원하면 에디터에서 덮어쓰기)
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

    // 여기서는 배리어가 0이 되어도 스킬을 자동 종료하지 않습니다.
    // (요구사항: 스킬 종료는 "우클릭 해제" 시점만 담당)

    // true를 반환하면 Character::HandleAnyDamage 쪽에서 체력 감소를 하지 않음 :contentReference[oaicite:4]{index=4}
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
    }

    // --- 3-2. 배리어가 소모된 만큼 광역 대미지 ---
    if (World && Owner && ConsumedShields > 0 && Params.Damage > 0.f)
    {
        const float TotalDamage = Params.Damage * ConsumedShields;

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
    if (!World || !Owner)
    {
        return;
    }

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
    // ★ 입력(우클릭 해제)으로 들어오는 Stop은 무시하고,
    //    오직 지속시간 타이머가 끝날 때만 실제 종료되도록 합니다.
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
    if (!World || !Owner || !bIsActive)
    {
        return;
    }

    // 흑안개 중심 위치
    FVector Center;
    if (SpawnedNS)
    {
        Center = SpawnedNS->GetComponentLocation();
    }
    else
    {
        Center = Owner->GetActorLocation() + Owner->GetActorRotation().RotateVector(RelativeOffset);
    }

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
            if (!OtherChar || OtherChar == Owner)
            {
                continue;
            }

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
    if (!bIsActive)
    {
        return;
    }

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
    if (!World || !Owner || !bIsActive)
    {
        return;
    }

    // 1) 플레이어 힐
    if (SelfHealPerTick > 0.f)
    {
        if (UHealthComponent* Health = Owner->FindComponentByClass<UHealthComponent>())
        {
            Health->AddHealth(SelfHealPerTick);
        }
    }

    // 2) 흑안개 안 적들에게 방어무시 도트 데미지
    if (FogRadius <= 0.f || DotDamagePerTick <= 0.f)
    {
        return;
    }

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

    if (!bAnyHit)
    {
        return;
    }

    for (const FOverlapResult& O : Overlaps)
    {
        AActor* Other = O.GetActor();
        ACharacter* OtherChar = Cast<ACharacter>(Other);
        if (!OtherChar || OtherChar == Owner)
        {
            continue;
        }

        // 고정 피해, 방어력 무시, 주기적 데미지 플래그
        DealSkillDamage(
            OtherChar,
            DotDamagePerTick,
            /*bIgnoreDefense=*/true,
            /*bPeriodic=*/true,
            /*HitCount=*/1);
    }
}
