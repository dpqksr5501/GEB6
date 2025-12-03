// Fill out your copyright notice in the Description page of Project Settings.


#include "Skills/Skill_Ultimate.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h" 
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/GameplayStatics.h" 
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"
#include "TimerManager.h"
#include "DrawDebugHelpers.h"
#include "HealthComponent.h"
#include "FormDefinition.h"

void ApplyFixedDotDamage(USkillBase* SourceSkill, ACharacter* Target, float DamagePerTick, int32 HitCount);

void USkill_Ultimate::BeginPlay()
{
    Super::BeginPlay();

    // 소유자 HealthComponent의 OnDamageApplied에 바인딩 → 피격 시 은신 해제
    if (AActor* Owner = GetOwner())
    {
        if (UHealthComponent* Health = Owner->FindComponentByClass<UHealthComponent>())
        {
            Health->OnDamageApplied.AddDynamic(
                this,
                &USkill_Ultimate::HandleOwnerDamaged);
        }
    }
}

USkill_Ultimate::USkill_Ultimate()
{
    PrimaryComponentTick.bCanEverTick = false;
    PrimaryComponentTick.bStartWithTickEnabled = false;
}

void USkill_Ultimate::InitializeFromDefinition(const USkillDefinition* Def)
{
    Super::InitializeFromDefinition(Def);
}

bool USkill_Ultimate::CanActivate() const
{
    // 이미 켜져 있으면 불가
    if (bIsActive)
    {
        UE_LOG(LogTemp, Verbose, TEXT("[Skill_Ultimate] CanActivate? false (already active)"));
        return false;
    }

    // 나머지(쿨타임/마나)는 공통 처리
    return Super::CanActivate();
}

void USkill_Ultimate::ActivateSkill()
{
	UE_LOG(LogTemp, Log, TEXT("[Skill_Ultimate] ActivateSkill called"));
    UWorld* World = GetWorld();
    AActor* Owner = GetOwner();
    if (!World || !Owner) return;

    if (!CanActivate())
    {
        UE_LOG(LogTemp, Warning, TEXT("[Skill_Ultimate] ActivateSkill blocked (CanActivate=false)"));
        return;
    }

    // 궁극기 애니메이션 재생 (FormDefinition에 궁극기 몽타주가 연결되어 있다는 가정)
    PlayFormSkillMontage();

    // 공통 비용 처리 (쿨타임 + 마나)
    Super::ActivateSkill();

    // === 폼별로 분기 ===
    const EFormType Form = GetCurrentFormType();

    switch (Form)
    {
    case EFormType::Range:
        UE_LOG(LogTemp, Log, TEXT("[Skill_Ultimate] Activate Range Ultimate"));
        ActivateRangeUltimate();
        break;

    case EFormType::Swift:
        UE_LOG(LogTemp, Log, TEXT("[Skill_Ultimate] Activate Swift Ultimate"));
        ActivateSwiftUltimate();
        break;

    case EFormType::Guard:
        UE_LOG(LogTemp, Log, TEXT("[Skill_Ultimate] Activate Guard Ultimate"));
        ActivateGuardUltimate();
        break;

    case EFormType::Special:
        UE_LOG(LogTemp, Log, TEXT("[Skill_Ultimate] Activate Special Ultimate"));
        ActivateSpecialUltimate();
        break;

    default:
        UE_LOG(LogTemp, Warning,
            TEXT("[Skill_Ultimate] No ultimate logic implemented for form %d"),
            static_cast<int32>(Form));
        break;
    }
}

void USkill_Ultimate::StopSkill()
{
    // Range 궁극기는 "지속시간 기반"이라 입력으로 들어오는 Stop은 무시
    UE_LOG(LogTemp, Verbose,
        TEXT("[Skill_Ultimate] StopSkill called (probably from input). Ignored; duration-based skill."));
}

/*============================= Range =============================*/

/** Range 폼 궁극기: MouthSocket 앞에 막대를 만들고, n초 간격으로 도트 데미지 */
void USkill_Ultimate::ActivateRangeUltimate()
{
    UWorld* World = GetWorld();
    AActor* Owner = GetOwner();
    if (!World || !Owner) return;

    bIsActive = true;

    // 1) 브레스 FX 스폰
    if (BreathNS)
    {
        if (ACharacter* OwnerChar = Cast<ACharacter>(Owner))
        {
            if (USkeletalMeshComponent* Mesh = OwnerChar->GetMesh())
            {
                SpawnedBreathNS = UNiagaraFunctionLibrary::SpawnSystemAttached(
                    BreathNS,
                    Mesh,
                    MuzzleSocketName,
                    FVector::ZeroVector,
                    FRotator::ZeroRotator,
                    EAttachLocation::SnapToTarget,
                    true);

                // 간단히: 길이/두께 비율에 맞춰 스케일
                if (SpawnedBreathNS)
                {
                    const float LengthScale = BreathLength / 100.f;
                    const float RadiusScale = BreathRadius / 100.f;
                    SpawnedBreathNS->SetWorldScale3D(
                        FVector(LengthScale, RadiusScale, RadiusScale));
                }
            }
        }
    }

    // 2) 지속시간 타이머
    if (BreathDuration > 0.f)
    {
        World->GetTimerManager().SetTimer(
            DurationTimerHandle,
            this,
            &USkill_Ultimate::OnBreathDurationEnded,
            BreathDuration,
            false);
    }

    // 3) 데미지 틱 타이머
    if (TickInterval > 0.f)
    {
        World->GetTimerManager().SetTimer(
            TickTimerHandle,
            this,
            &USkill_Ultimate::OnBreathTick,
            TickInterval,
            true);
    }

    UE_LOG(LogTemp, Log,
        TEXT("[Skill_Ultimate] Range Breath activated (Duration=%.2f, Len=%.1f, R=%.1f, Tick=%.2f, Dmg/Tick=%.1f)"),
        BreathDuration, BreathLength, BreathRadius, TickInterval, DamagePerTick);
}

/** BreathDuration 이 끝났을 때 실제 종료 처리 */
void USkill_Ultimate::OnBreathDurationEnded()
{
    UWorld* World = GetWorld();
    if (World)
    {
        World->GetTimerManager().ClearTimer(DurationTimerHandle);
        World->GetTimerManager().ClearTimer(TickTimerHandle);
    }

    bIsActive = false;

    if (SpawnedBreathNS)
    {
        SpawnedBreathNS->Deactivate();
        SpawnedBreathNS = nullptr;
    }

    UE_LOG(LogTemp, Log, TEXT("[Skill_Ultimate] Range Breath finished."));

    // 공통 Stop 로그 등
    Super::StopSkill();
}

/**
 * 현재 MouthSocket 기준으로 "긴 박스" 정보를 계산:
 * - 중심: MouthSocket에서 전방으로 BreathLength/2 만큼 이동
 * - 회전: Socket Forward 를 X축으로 사용하는 회전
 * - HalfExtent: (BreathLength/2, BreathRadius, BreathRadius)
 */
bool USkill_Ultimate::GetBreathBox(
    FVector& OutCenter,
    FQuat& OutRotation,
    FVector& OutHalfExtent) const
{
    AActor* Owner = GetOwner();
    ACharacter* OwnerChar = Cast<ACharacter>(Owner);
    if (!OwnerChar) return false;

    USkeletalMeshComponent* Mesh = OwnerChar->GetMesh();
    if (!Mesh) return false;

    FTransform SocketTM;

    if (Mesh->DoesSocketExist(MuzzleSocketName))
    {
        SocketTM = Mesh->GetSocketTransform(MuzzleSocketName);
    }
    else
    {
        SocketTM = OwnerChar->GetActorTransform();
    }

    const FVector StartLoc = SocketTM.GetLocation();

    // 소켓의 X축을 "브레스가 나가는 방향"으로 사용
    const FVector Forward = SocketTM.GetUnitAxis(EAxis::X);
    const FVector Up = SocketTM.GetUnitAxis(EAxis::Z);

    const float HalfLength = BreathLength * 0.5f;

    OutCenter = StartLoc + Forward * HalfLength;
    OutRotation = FRotationMatrix::MakeFromXZ(Forward, Up).ToQuat();
    OutHalfExtent = FVector(HalfLength, BreathRadius, BreathRadius);

    return true;
}

/** TickInterval 마다 호출되어, 막대 안에 겹쳐있는 적에게 도트 데미지 */
void USkill_Ultimate::OnBreathTick()
{
    UWorld* World = GetWorld();
    AActor* Owner = GetOwner();
    if (!World || !Owner || !bIsActive) return;

    if (DamagePerTick <= 0.f) return;

    FVector Center;
    FQuat   Rotation;
    FVector HalfExtent;

    if (!GetBreathBox(Center, Rotation, HalfExtent)) return;

    // Overlap 대상: 기본 Pawn
    FCollisionObjectQueryParams ObjParams;
    ObjParams.AddObjectTypesToQuery(BreathCollisionChannel);

    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(SkillUltimateBreath), false, Owner);

    TArray<FOverlapResult> Overlaps;

    const bool bAnyHit = World->OverlapMultiByObjectType(
        Overlaps,
        Center,
        Rotation,
        ObjParams,
        FCollisionShape::MakeBox(HalfExtent),
        QueryParams);

    // 디버그용 박스
    if (bDrawDebugBreath)
    {
        DrawDebugBox(
            World,
            Center,
            HalfExtent,
            Rotation,
            DebugBreathColor,
            false,
            TickInterval, // 다음 틱까지 유지
            0,
            2.f);
    }

    if (!bAnyHit) return;

    // 한 틱에 같은 캐릭터 중복 타격 방지
    TSet<ACharacter*> UniqueTargets;

    for (const FOverlapResult& O : Overlaps)
    {
        AActor* Other = O.GetActor();
        ACharacter* OtherChar = Cast<ACharacter>(Other);
        if (!OtherChar || OtherChar == Owner) continue;

        if (UniqueTargets.Contains(OtherChar)) continue;
        UniqueTargets.Add(OtherChar);

        // 고정 도트 데미지 1회
        ApplyFixedDotDamage(
            this,
            OtherChar,
            DamagePerTick,
            1);
    }
}
/*============================= Swift =============================*/

void USkill_Ultimate::ActivateSwiftUltimate()
{
    UWorld* World = GetWorld();
    AActor* Owner = GetOwner();
    if (!World || !Owner) return;

    ACharacter* OwnerChar = Cast<ACharacter>(Owner);
    if (!OwnerChar)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[Skill_Ultimate] Swift ultimate requires Character owner."));
        return;
    }

    if (bSwiftStealthActive)
    {
        UE_LOG(LogTemp, Verbose,
            TEXT("[Skill_Ultimate] Swift ultimate already active."));
        return;
    }

    bIsActive = true;
    bSwiftStealthActive = true;
    SwiftOwnerChar = OwnerChar;

    // 1) 투명화 (Mesh만 숨김 → 충돌은 유지)
    if (USkeletalMeshComponent* Mesh = OwnerChar->GetMesh())
    {
        Mesh->SetVisibility(false, true);
    }

    // 2) 이동 속도 배수 적용
    if (UCharacterMovementComponent* MoveComp = OwnerChar->GetCharacterMovement())
    {
        SwiftOriginalMaxWalkSpeed = MoveComp->MaxWalkSpeed;
        MoveComp->MaxWalkSpeed = SwiftOriginalMaxWalkSpeed * SwiftMoveSpeedMultiplier;
    }

    // 3) 지속시간 타이머
    if (SwiftDuration > 0.f)
    {
        World->GetTimerManager().SetTimer(
            SwiftDurationTimerHandle,
            this,
            &USkill_Ultimate::OnSwiftDurationEnded,
            SwiftDuration,
            false);
    }

    UE_LOG(LogTemp, Log,
        TEXT("[Skill_Ultimate] Swift stealth activated (Duration=%.2f, MoveMult=%.2f, AtkMult=%.2f)"),
        SwiftDuration, SwiftMoveSpeedMultiplier, SwiftAttackMultiplier);
}

void USkill_Ultimate::EndSwiftUltimate()
{
    if (!bSwiftStealthActive)
    {
        return;
    }

    UWorld* World = GetWorld();
    if (World)
    {
        World->GetTimerManager().ClearTimer(SwiftDurationTimerHandle);
    }

    bSwiftStealthActive = false;
    bIsActive = false;

    ACharacter* OwnerChar = SwiftOwnerChar.Get();
    if (!OwnerChar)
    {
        OwnerChar = Cast<ACharacter>(GetOwner());
    }

    if (OwnerChar)
    {
        // 1) 다시 보이게
        if (USkeletalMeshComponent* Mesh = OwnerChar->GetMesh())
        {
            Mesh->SetVisibility(true, true);
        }

        // 2) 이동속도 원복
        if (UCharacterMovementComponent* MoveComp = OwnerChar->GetCharacterMovement())
        {
            if (SwiftOriginalMaxWalkSpeed > 0.f)
            {
                MoveComp->MaxWalkSpeed = SwiftOriginalMaxWalkSpeed;
            }
        }
    }

    SwiftOriginalMaxWalkSpeed = 0.f;
    SwiftOwnerChar = nullptr;

    UE_LOG(LogTemp, Log, TEXT("[Skill_Ultimate] Swift stealth ended."));

    // 공통 Stop 처리 (로그 등)
    Super::StopSkill();
}

void USkill_Ultimate::OnSwiftDurationEnded()
{
    UE_LOG(LogTemp, Log,
        TEXT("[Skill_Ultimate] Swift stealth duration ended."));
    EndSwiftUltimate();
}

void USkill_Ultimate::OnAttackFromStealth()
{
    if (!bSwiftStealthActive)
    {
        return;
    }

    UE_LOG(LogTemp, Log,
        TEXT("[Skill_Ultimate] Swift stealth broken by attacking."));
    EndSwiftUltimate();
}

void USkill_Ultimate::HandleOwnerDamaged(
    float NewHealth,
    float FinalDamage,
    float RawDamage,
    AActor* InstigatorActor,
    USkillBase* SourceSkill)
{
    if (!bSwiftStealthActive)
    {
        return;
    }

    UE_LOG(LogTemp, Log,
        TEXT("[Skill_Ultimate] Swift stealth broken by taking damage."));
    EndSwiftUltimate();
}

/*============================= Guard =============================*/

void USkill_Ultimate::ActivateGuardUltimate()
{
    UWorld* World = GetWorld();
    AActor* Owner = GetOwner();
    if (!World || !Owner)
    {
        return;
    }

    const FVector Origin = Owner->GetActorLocation();
    const FVector Forward = Owner->GetActorForwardVector();

    // 1) 사정거리 내 Pawn들 Overlap (구체)
    FCollisionObjectQueryParams ObjParams;
    ObjParams.AddObjectTypesToQuery(GuardCollisionChannel);

    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(SkillUltimateGuard), false, Owner);

    TArray<FOverlapResult> Overlaps;
    const bool bAnyHit = World->OverlapMultiByObjectType(
        Overlaps,
        Origin,
        FQuat::Identity,
        ObjParams,
        FCollisionShape::MakeSphere(GuardRange),
        QueryParams);

    // 디버그: 부채꼴 표시 (원 + 경계선 2개 정도)
    if (bDrawDebugGuard)
    {
        // 범위 원
        DrawDebugCircle(
            World,
            Origin,
            GuardRange,
            32,
            GuardDebugColor,
            false,
            1.f,
            0,
            2.f,
            FVector(1.f, 0.f, 0.f),
            FVector(0.f, 1.f, 0.f),
            false);

        const float HalfRad = FMath::DegreesToRadians(GuardConeHalfAngleDeg);
        const FVector RightDir = Forward.RotateAngleAxis(+GuardConeHalfAngleDeg, FVector::UpVector);
        const FVector LeftDir = Forward.RotateAngleAxis(-GuardConeHalfAngleDeg, FVector::UpVector);

        DrawDebugLine(World, Origin, Origin + RightDir * GuardRange, GuardDebugColor, false, 1.f, 0, 2.f);
        DrawDebugLine(World, Origin, Origin + LeftDir * GuardRange, GuardDebugColor, false, 1.f, 0, 2.f);
    }

    if (!bAnyHit)
    {
        UE_LOG(LogTemp, Log,
            TEXT("[Skill_Ultimate] Guard ultimate: no targets in range."));
        return;
    }

    const float ConeHalfRad = FMath::DegreesToRadians(GuardConeHalfAngleDeg);
    const float CosThreshold = FMath::Cos(ConeHalfRad);
    const float RangeSq = GuardRange * GuardRange;

    for (const FOverlapResult& O : Overlaps)
    {
        AActor* Other = O.GetActor();
        ACharacter* TargetChar = Cast<ACharacter>(Other);
        if (!TargetChar || TargetChar == Owner)
        {
            continue;
        }

        // 거리 체크
        const FVector ToTarget = TargetChar->GetActorLocation() - Origin;
        const float DistSq = ToTarget.SizeSquared();
        if (DistSq > RangeSq || DistSq <= KINDA_SMALL_NUMBER)
        {
            continue;
        }

        // 각도 체크 (부채꼴 안에 있는지)
        const FVector DirToTarget = ToTarget.GetSafeNormal();
        const float   CosValue = FVector::DotProduct(Forward, DirToTarget);
        if (CosValue < CosThreshold)
        {
            continue; // 부채꼴 밖
        }

        // === 여기까지 온 것은: 사정거리 + 부채꼴 안의 "적" ===

        // 2) 데미지 부여
        if (GuardDamage > 0.f)
        {
            UHealthComponent* Health = TargetChar->FindComponentByClass<UHealthComponent>();
            if (Health)
            {
                FDamageSpec Spec;
                Spec.RawDamage = GuardDamage;
                Spec.bIgnoreDefense = false;              // 기절용이라 방어력 적용
                Spec.bPeriodic = false;
                Spec.HitCount = 1;
                Spec.bFixedDot = false;
                Spec.Instigator = Owner;
                Spec.SourceSkill = this;

                Health->ApplyDamageSpec(Spec);           // 기존 시스템 사용 :contentReference[oaicite:5]{index=5}
            }
        }

        // 3) 추락 + 기절(속박)
        if (UCharacterMovementComponent* MoveComp = TargetChar->GetCharacterMovement())
        {
            // 추락: 아래 방향으로 강하게 날려서 땅으로 떨어뜨림
            const FVector DownImpulse(0.f, 0.f, -GuardFallStrength);
            TargetChar->LaunchCharacter(DownImpulse, true, true);

            // 바로 움직임 금지
            MoveComp->DisableMovement();

            // GuardStunDuration 후 기절 해제
            if (GuardStunDuration > 0.f)
            {
                FTimerDelegate StunEndDelegate;
                StunEndDelegate.BindUObject(this, &USkill_Ultimate::EndGuardStun, TargetChar);

                FTimerHandle TmpHandle;
                World->GetTimerManager().SetTimer(
                    TmpHandle,
                    StunEndDelegate,
                    GuardStunDuration,
                    false);
            }
            else
            {
                // 지속시간 0 이하라면 바로 해제
                EndGuardStun(TargetChar);
            }
        }
    }

    UE_LOG(LogTemp, Log,
        TEXT("[Skill_Ultimate] Guard ultimate executed (Range=%.1f, Damage=%.1f, Stun=%.2f)"),
        GuardRange, GuardDamage, GuardStunDuration);
}

void USkill_Ultimate::EndGuardStun(ACharacter* Target)
{
    if (!Target)
    {
        return;
    }

    if (UCharacterMovementComponent* MoveComp = Target->GetCharacterMovement())
    {
        // 다시 걷기 모드로
        MoveComp->SetMovementMode(MOVE_Walking);
    }

    UE_LOG(LogTemp, Verbose,
        TEXT("[Skill_Ultimate] Guard stun ended for %s"),
        *Target->GetName());
}

/*============================= Special =============================*/

void USkill_Ultimate::ActivateSpecialUltimate()
{
    UWorld* World = GetWorld();
    AActor* Owner = GetOwner();
    if (!World || !Owner)
    {
        return;
    }

    if (!SpecialOrbClass)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[Skill_Ultimate] SpecialOrbClass is not set."));
        return;
    }

    SpecialOrbs.Empty();

    const FVector Center = Owner->GetActorLocation();
    const FVector Forward = Owner->GetActorForwardVector();
    const FVector Right = Owner->GetActorRightVector();

    // 오브 높이 약간 띄우기
    const float HeightOffset = 50.f;

    // 5개의 구체를 오망성 모양(원 위 5점)으로 배치
    for (int32 i = 0; i < 5; ++i)
    {
        const float AngleDeg = 72.f * i; // 360 / 5
        const float AngleRad = FMath::DegreesToRadians(AngleDeg);

        const float X = FMath::Cos(AngleRad);
        const float Y = FMath::Sin(AngleRad);

        // Right / Forward 기준으로 원형 배치
        FVector Offset = Right * (X * SpecialRadius)
            + Forward * (Y * SpecialRadius)
            + FVector(0.f, 0.f, HeightOffset);

        const FVector SpawnLocation = Center + Offset;
        const FRotator SpawnRotation = (Center - SpawnLocation).Rotation(); // 시전자 쪽을 보도록

        FActorSpawnParameters ActorParams;
        ActorParams.Owner = Owner;

        AActor* Orb = World->SpawnActor<AActor>(
            SpecialOrbClass,
            SpawnLocation,
            SpawnRotation,
            ActorParams);

        if (Orb)
        {
            // 시전자에 붙여서 같이 이동하도록
            Orb->AttachToActor(Owner,
                FAttachmentTransformRules::KeepWorldTransform);

            SpecialOrbs.Add(Orb);

            // 파괴될 때 콜백
            Orb->OnDestroyed.AddDynamic(
                this,
                &USkill_Ultimate::HandleSpecialOrbDestroyed);
        }
    }

    // Special 지속시간 타이머
    if (SpecialDuration > 0.f)
    {
        World->GetTimerManager().SetTimer(
            SpecialDurationTimerHandle,
            this,
            &USkill_Ultimate::OnSpecialDurationEnded,
            SpecialDuration,
            false);
    }

    UE_LOG(LogTemp, Log,
        TEXT("[Skill_Ultimate] Special ultimate activated (Duration=%.2f, Radius=%.1f, SelfDot=%.1f)"),
        SpecialDuration, SpecialRadius, SpecialSelfDotDamage);
}

void USkill_Ultimate::OnSpecialDurationEnded()
{
    UWorld* World = GetWorld();
    if (World)
    {
        World->GetTimerManager().ClearTimer(SpecialDurationTimerHandle);
    }

    // 아직 살아있는 구체 수 세기
    int32 AliveCount = 0;
    for (TWeakObjectPtr<AActor>& WeakOrb : SpecialOrbs)
    {
        if (AActor* Orb = WeakOrb.Get())
        {
            ++AliveCount;
        }
    }

    // 사용 끝났으니 구체들은 제거
    // 복사본으로 Destroy를 호출해서, Destroy 중에 SpecialOrbs가 바뀌어도 안전하게
    TArray<TWeakObjectPtr<AActor>> OrbsCopy = SpecialOrbs;
    for (const TWeakObjectPtr<AActor>& WeakOrb : OrbsCopy)
    {
        if (AActor* Orb = WeakOrb.Get())
        {
            Orb->Destroy();   // 여기서 HandleSpecialOrbDestroyed가 SpecialOrbs를 수정해도 OK
        }
    }
    SpecialOrbs.Empty();

    if (AliveCount <= 0)
    {
        UE_LOG(LogTemp, Log,
            TEXT("[Skill_Ultimate] Special: all orbs destroyed -> no penalty to player."));
        return;
    }

    UE_LOG(LogTemp, Log,
        TEXT("[Skill_Ultimate] Special: %d orbs left -> applying self DoT & root to player."),
        AliveCount);

    // 플레이어 속박 + 도트 데미지 3틱
    ApplyRootToPlayer(SpecialRootDuration);

    if (SpecialSelfDotDamage > 0.f && SpecialSelfDotInterval > 0.f)
    {
        SpecialSelfDotTicksRemaining = 3;

        World->GetTimerManager().SetTimer(
            SpecialSelfDotTimerHandle,
            this,
            &USkill_Ultimate::OnSpecialSelfDotTick,
            SpecialSelfDotInterval,
            true);
    }
}

void USkill_Ultimate::OnSpecialSelfDotTick()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    // 0번 플레이어 캐릭터 (주인공)
    ACharacter* PlayerChar = Cast<ACharacter>(
        UGameplayStatics::GetPlayerCharacter(World, 0));

    if (!PlayerChar)
    {
        World->GetTimerManager().ClearTimer(SpecialSelfDotTimerHandle);
        SpecialSelfDotTicksRemaining = 0;
        return;
    }

    if (SpecialSelfDotDamage > 0.f)
    {
        // HealthComponent 쪽 static 헬퍼 (방어무시, 고정 도트 데미지)
        ApplyFixedDotDamage(
            this,
            PlayerChar,
            SpecialSelfDotDamage,
            1);
    }

    --SpecialSelfDotTicksRemaining;
    if (SpecialSelfDotTicksRemaining <= 0)
    {
        World->GetTimerManager().ClearTimer(SpecialSelfDotTimerHandle);
    }
}

void USkill_Ultimate::ApplyRootToPlayer(float Duration)
{
    if (Duration <= 0.f)
    {
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    ACharacter* PlayerChar = Cast<ACharacter>(
        UGameplayStatics::GetPlayerCharacter(World, 0));

    if (!PlayerChar)
    {
        return;
    }

    SpecialRootedPlayer = PlayerChar;

    if (UCharacterMovementComponent* MoveComp = PlayerChar->GetCharacterMovement())
    {
        // 완전 고정
        MoveComp->DisableMovement();
    }

    World->GetTimerManager().SetTimer(
        SpecialRootTimerHandle,
        this,
        &USkill_Ultimate::EndSpecialRoot,
        Duration,
        false);

    UE_LOG(LogTemp, Log,
        TEXT("[Skill_Ultimate] Special: player rooted for %.2f seconds."),
        Duration);
}

void USkill_Ultimate::EndSpecialRoot()
{
    UWorld* World = GetWorld();
    if (World)
    {
        World->GetTimerManager().ClearTimer(SpecialRootTimerHandle);
    }

    if (ACharacter* PlayerChar = Cast<ACharacter>(SpecialRootedPlayer.Get()))
    {
        if (UCharacterMovementComponent* MoveComp = PlayerChar->GetCharacterMovement())
        {
            // 다시 걷기 모드로
            MoveComp->SetMovementMode(MOVE_Walking);
        }
    }

    SpecialRootedPlayer = nullptr;

    UE_LOG(LogTemp, Log,
        TEXT("[Skill_Ultimate] Special: player root ended."));
}

void USkill_Ultimate::HandleSpecialOrbDestroyed(AActor* DestroyedActor)
{
    SpecialOrbs.RemoveAll(
        [DestroyedActor](const TWeakObjectPtr<AActor>& Elem)
        {
            return !Elem.IsValid() || Elem.Get() == DestroyedActor;
        });

    UE_LOG(LogTemp, Verbose,
        TEXT("[Skill_Ultimate] Special orb destroyed. Remaining: %d"),
        SpecialOrbs.Num());
}
