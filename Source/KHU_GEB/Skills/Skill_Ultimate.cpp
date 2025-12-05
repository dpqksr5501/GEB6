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
#include "KHU_GEBCharacter.h"
#include "LockOnComponent.h"
#include "Enemy_AI/EnemyState.h"
#include "Enemy_AI/Enemy_Base.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "CrowdControlComponent.h"

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

    // 궁극기 애니메이션 재생
    PlayFormUltimateMontage();

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

    // === 0) 소유 캐릭터 / 소켓 트랜스폼 ===
    ACharacter* OwnerChar = Cast<ACharacter>(Owner);
    USkeletalMeshComponent* Mesh = OwnerChar ? OwnerChar->GetMesh() : nullptr;

    FTransform SocketTM;
    if (Mesh)
    {
        if (Mesh->DoesSocketExist(MuzzleSocketName))
        {
            SocketTM = Mesh->GetSocketTransform(MuzzleSocketName);
        }
        else
        {
            SocketTM = OwnerChar->GetActorTransform();
        }
    }
    else
    {
        SocketTM = Owner->GetActorTransform();
    }

    const FVector StartLoc = SocketTM.GetLocation();
    FVector Forward = SocketTM.GetUnitAxis(EAxis::X);   // 기본 전방
    const FVector Up = SocketTM.GetUnitAxis(EAxis::Z);

    // === 1) '주시 중인 타겟' 좌표 구하기 ===
    bool bHasFocus = false;
    FVector FocusLocation = FVector::ZeroVector;

    // 플레이어면 LockOn 타겟 우선
    if (AKHU_GEBCharacter* PlayerOwner = Cast<AKHU_GEBCharacter>(Owner))
    {
        if (AActor* LockTarget = PlayerOwner->GetLockOnTarget())
        {
            FocusLocation = LockTarget->GetActorLocation();
            bHasFocus = true;
        }
    }
    // (옵션) Enemy면 0번 플레이어를 기본 타겟으로 본다
    else if (AEnemy_Base* EnemyOwner = Cast<AEnemy_Base>(Owner))
    {
        if (ACharacter* PlayerChar = Cast<ACharacter>(
            UGameplayStatics::GetPlayerCharacter(World, 0)))
        {
            FocusLocation = PlayerChar->GetActorLocation();
            bHasFocus = true;
        }
    }

    // 타겟을 알고 있으면, 그 방향으로 Forward 갱신
    if (bHasFocus)
    {
        FVector DirToTarget = FocusLocation - StartLoc;
        if (!DirToTarget.IsNearlyZero())
        {
            Forward = DirToTarget.GetSafeNormal();
        }
    }

    // === 3) 브레스 FX 소환 (입 소켓에 붙여서 위치/회전 따라가게) ===
    if (BreathNS && OwnerChar && Mesh)
    {
        const float LengthScale = BreathLength / 100.f;
        const float RadiusScale = BreathRadius / 100.f;

        SpawnedBreathNS = UNiagaraFunctionLibrary::SpawnSystemAttached(
            BreathNS,
            Mesh,
            MuzzleSocketName,                               // 입 소켓 이름
            FVector::ZeroVector,                            // 소켓 기준 오프셋
            FRotator::ZeroRotator,                          // 소켓 회전 그대로 사용
            EAttachLocation::SnapToTargetIncludingScale,    // 위치/회전/스케일 스냅
            true,                                           // bAutoDestroy
            true                                            // bAutoActivate
        );

        if (SpawnedBreathNS)
        {
            SpawnedBreathNS->SetWorldScale3D(
                FVector(LengthScale, RadiusScale, RadiusScale));
        }
    }

    // === 4) 지속시간 타이머 ===
    if (BreathDuration > 0.f)
    {
        World->GetTimerManager().SetTimer(
            DurationTimerHandle,
            this,
            &USkill_Ultimate::OnBreathDurationEnded,
            BreathDuration,
            false);
    }

    // === 5) 데미지 틱 타이머 ===
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

    // Range 브레스 박스 캐시 초기화
    bHasBreathBoxCache = false;
    CachedBreathCenter = FVector::ZeroVector;
    CachedBreathRotation = FQuat::Identity;
    CachedBreathHalfExtent = FVector::ZeroVector;

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
    // 항상 현재 입 소켓 기준으로 계산
    AActor* Owner = GetOwner();
    ACharacter* OwnerChar = Cast<ACharacter>(Owner);
    if (!OwnerChar) return false;

    USkeletalMeshComponent* Mesh = OwnerChar->GetMesh();
    if (!Mesh) return false;

    FTransform SocketTM;
    if (Mesh->DoesSocketExist(MuzzleSocketName)) { SocketTM = Mesh->GetSocketTransform(MuzzleSocketName); }
    else { SocketTM = OwnerChar->GetActorTransform(); }

    const FVector StartLoc = SocketTM.GetLocation();

    // 소켓 X축 = 브레스 방향, Z축 = 위
    const FVector SocketForward = SocketTM.GetUnitAxis(EAxis::X);
    const FVector Up = SocketTM.GetUnitAxis(EAxis::Z);

    // 1) 주시 중인 타겟 위치 구하기
    bool bHasFocus = false;
    FVector FocusLocation = StartLoc;

    if (AKHU_GEBCharacter* PlayerOwner = Cast<AKHU_GEBCharacter>(Owner))
    {
        if (AActor* LockTarget = PlayerOwner->GetLockOnTarget())
        {
            FocusLocation = LockTarget->GetActorLocation();
            bHasFocus = true;
        }
    }
    else if (AEnemy_Base* EnemyOwner = Cast<AEnemy_Base>(Owner))
    {
        if (ACharacter* PlayerChar = Cast<ACharacter>(
            UGameplayStatics::GetPlayerCharacter(GetWorld(), 0)))
        {
            FocusLocation = PlayerChar->GetActorLocation();
            bHasFocus = true;
        }
    }

    // 2) 끝점 Z가 타겟 Z를 따라가도록, "시작점 → 끝점" 구간 벡터 계산
    FVector Segment;

    if (bHasFocus)
    {
        // 가로 방향은 입의 전방(Yaw) 기준
        FVector ForwardXY = SocketForward;
        ForwardXY.Z = 0.f;

        if (ForwardXY.IsNearlyZero())
        {
            // 정면이 너무 위/아래면 그냥 소켓 전방 사용
            ForwardXY = SocketForward;
        }
        ForwardXY = ForwardXY.GetSafeNormal();

        // 브레스 기본 끝점 (가로 방향으로 BreathLength 만큼)
        const FVector BaseEnd = StartLoc + ForwardXY * BreathLength;

        // 끝점 Z를 타겟 Z로 덮어쓰기
        FVector EndLoc = BaseEnd;
        EndLoc.Z = FocusLocation.Z;

        Segment = EndLoc - StartLoc;
    }
    else
    {
        // 타겟이 없으면 기존처럼 소켓 전방으로 고정
        Segment = SocketForward * BreathLength;
    }

    if (Segment.IsNearlyZero())
    {
        Segment = SocketForward * BreathLength;
    }

    const float HalfLength = Segment.Size() * 0.5f;

    // 중심 = 시작점과 끝점의 중간
    OutCenter = StartLoc + Segment * 0.5f;

    // 박스의 X축(길이 방향)은 시작→끝 방향
    const FVector ForwardDir = Segment.GetSafeNormal();
    OutRotation = FRotationMatrix::MakeFromXZ(ForwardDir, Up).ToQuat();

    // 두께는 그대로, 길이는 세그먼트 길이 기준
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

    // 한 틱에 같은 액터 중복 타격 방지
    TSet<AActor*> UniqueTargets;

    const bool bOwnerIsPlayer = Owner->IsA<AKHU_GEBCharacter>();
    const bool bOwnerIsEnemy = Owner->IsA<AEnemy_Base>();

    // InstigatorController (ApplyDamage용)
    AController* InstigatorController = nullptr;
    if (APawn* PawnOwner = Cast<APawn>(Owner))
    {
        InstigatorController = PawnOwner->GetController();
    }

    for (const FOverlapResult& O : Overlaps)
    {
        AActor* Other = O.GetActor();
        if (!Other || Other == Owner) continue;

        if (UniqueTargets.Contains(Other)) continue;

        // === 팀/타입 필터: 플레이어면 Enemy만, Enemy면 플레이어만 ===
        if (bOwnerIsPlayer)
        {
            if (!Other->IsA<AEnemy_Base>()) continue;
        }
        else if (bOwnerIsEnemy)
        {
            if (!Other->IsA<AKHU_GEBCharacter>()) continue;
        }

        UniqueTargets.Add(Other);

        // 틱당 일반 데미지 1회
        UGameplayStatics::ApplyDamage(
            Other,
            DamagePerTick,
            InstigatorController,
            Owner,
            UDamageType::StaticClass());

        UE_LOG(LogTemp, Verbose,
            TEXT("[Skill_Ultimate] Range breath tick: ApplyDamage %.1f to %s"),
            DamagePerTick,
            *GetNameSafe(Other));
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

    // 적이 은신을 쓸 때 플레이어 락온 해제
    if (OwnerChar->IsA<AEnemy_Base>())
    {
        if (AKHU_GEBCharacter* Player = Cast<AKHU_GEBCharacter>(
            UGameplayStatics::GetPlayerCharacter(World, 0)))
        {
            // 플레이어가 현재 이 적을 락온하고 있다면 락온 해제
            Player->ClearLockOnIfTarget(OwnerChar);
        }
    }
}

void USkill_Ultimate::EndSwiftUltimate()
{
    if (!bSwiftStealthActive) return;

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
        TEXT("[Skill_Ultimate] Swift stealth duration ended. Triggering end explosion."));

    // 혹시 도중에 이미 깨져서 비활성화된 경우 방어
    if (!bSwiftStealthActive) return;

    // 1) 은신 시간 만료로 인한 폭발 적용
    ApplySwiftEndExplosion();

    // 2) 공통 종료 처리 (투명/이속 복구)
    EndSwiftUltimate();
}

void USkill_Ultimate::OnAttackFromStealth(AActor* HitActor)
{
    if (!bSwiftStealthActive) return;

    if (SwiftStunDuration > 0.f && HitActor)
    {
        if (ACharacter* HitChar = Cast<ACharacter>(HitActor))
        {
            if (UCrowdControlComponent* CC =
                HitChar->FindComponentByClass<UCrowdControlComponent>())
            {
                CC->ApplyStun(SwiftStunDuration);
            }
        }
    }

    UE_LOG(LogTemp, Log,
        TEXT("[Skill_Ultimate] Swift stealth broken by attacking. Hit: %s"),
        *GetNameSafe(HitActor));

    EndSwiftUltimate();
}

void USkill_Ultimate::HandleOwnerDamaged(
    float NewHealth,
    float RawDamage,
    float FinalDamage,
    AActor* InstigatorActor,
    AActor* DamageCauser)
{
    if (!bSwiftStealthActive) return;

    UE_LOG(LogTemp, Log,
        TEXT("[Skill_Ultimate] Swift stealth broken by taking damage."));
    EndSwiftUltimate();
}

void USkill_Ultimate::ApplySwiftEndExplosion()
{
    UWorld* World = GetWorld();
    AActor* Owner = GetOwner();
    if (!World || !Owner) return;

    if (SwiftEndRadius <= 0.f)
    {
        UE_LOG(LogTemp, Verbose,
            TEXT("[Skill_Ultimate] SwiftEndExplosion skipped: radius <= 0"));
        return;
    }

    const bool bOwnerIsPlayer = Owner->IsA<AKHU_GEBCharacter>();
    const bool bOwnerIsEnemy = Owner->IsA<AEnemy_Base>();

    // InstigatorController 설정 (데미지 가해자용)
    AController* InstigatorController = nullptr;
    if (APawn* PawnOwner = Cast<APawn>(Owner))
    {
        InstigatorController = PawnOwner->GetController();
    }

    const FVector Origin = Owner->GetActorLocation();

    // Overlap 설정
    FCollisionObjectQueryParams ObjParams;
    ObjParams.AddObjectTypesToQuery(SwiftEndCollisionChannel);

    FCollisionQueryParams QueryParams(
        SCENE_QUERY_STAT(SkillUltimateSwiftEnd),
        false,
        Owner);

    TArray<FOverlapResult> Overlaps;

    const bool bAnyHit = World->OverlapMultiByObjectType(
        Overlaps,
        Origin,
        FQuat::Identity,
        ObjParams,
        FCollisionShape::MakeSphere(SwiftEndRadius),
        QueryParams);

    // 디버그용 구체
    if (bDrawDebugSwiftEnd)
    {
        DrawDebugSphere(
            World,
            Origin,
            SwiftEndRadius,
            24,
            SwiftEndDebugColor,
            false,
            1.5f,
            0,
            2.0f);
    }

    if (!bAnyHit)
    {
        UE_LOG(LogTemp, Log,
            TEXT("[Skill_Ultimate] SwiftEndExplosion: no targets in range."));
        return;
    }

    TSet<ACharacter*> UniqueTargets;

    for (const FOverlapResult& O : Overlaps)
    {
        AActor* Other = O.GetActor();
        ACharacter* Target = Cast<ACharacter>(Other);
        if (!Target || Target == Owner) continue;

        if (UniqueTargets.Contains(Target)) continue;
        UniqueTargets.Add(Target);

        // 팀 필터: 플레이어면 Enemy만, Enemy면 Player만
        if (bOwnerIsPlayer && !Target->IsA<AEnemy_Base>()) continue;
        if (bOwnerIsEnemy && !Target->IsA<AKHU_GEBCharacter>()) continue;

        // 1) 대미지
        if (SwiftEndDamage > 0.f)
        {
            UGameplayStatics::ApplyDamage(
                Target,
                SwiftEndDamage,
                InstigatorController,
                Owner,
                UDamageType::StaticClass());
        }

        // 2) 스턴
        if (SwiftEndStunDuration > 0.f)
        {
            if (UCrowdControlComponent* CC =
                Target->FindComponentByClass<UCrowdControlComponent>())
            {
                CC->ApplyStun(SwiftEndStunDuration);
            }
        }
    }

    UE_LOG(LogTemp, Log,
        TEXT("[Skill_Ultimate] SwiftEndExplosion executed (Radius=%.1f, Damage=%.1f, Stun=%.2f)"),
        SwiftEndRadius, SwiftEndDamage, SwiftEndStunDuration);
}

/*============================= Guard =============================*/

void USkill_Ultimate::ActivateGuardUltimate()
{
    UWorld* World = GetWorld();
    AActor* Owner = GetOwner();
    if (!World || !Owner) return;

    const bool bOwnerIsPlayer = Owner->IsA<AKHU_GEBCharacter>();
    const bool bOwnerIsEnemy = Owner->IsA<AEnemy_Base>();

    AController* InstigatorController = nullptr;
    if (APawn* PawnOwner = Cast<APawn>(Owner))
    {
        InstigatorController = PawnOwner->GetController();
    }

    const FVector Origin = Owner->GetActorLocation();
    const FVector Forward = Owner->GetActorForwardVector();

    // 1) 구체 오버랩
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

    // 디버그 선/원 그리기
    if (bDrawDebugGuard)
    {
        const float LifeTime = 1.5f;   // 얼마 동안 보일지 (원하는 값으로)
        const float Thickness = 2.0f;

        // Z축 기준 회전 (Yaw만 사용)
        const FVector UpVector = FVector::UpVector;

        // 왼/오른쪽 경계 벡터
        const FVector LeftDir = Forward.RotateAngleAxis(-GuardConeHalfAngleDeg, UpVector);
        const FVector RightDir = Forward.RotateAngleAxis(+GuardConeHalfAngleDeg, UpVector);

        // 부채꼴 경계선 2개
        DrawDebugLine(
            World,
            Origin,
            Origin + LeftDir * GuardRange,
            GuardDebugColor,
            false,
            LifeTime,
            0,
            Thickness);

        DrawDebugLine(
            World,
            Origin,
            Origin + RightDir * GuardRange,
            GuardDebugColor,
            false,
            LifeTime,
            0,
            Thickness);

        // 반지름 원(위에서 내려다본) 대략적인 표시
        const int32 NumSegments = 24;
        const float StepAngle = (GuardConeHalfAngleDeg * 2.f) / NumSegments;

        FVector PrevPoint = Origin + Forward * GuardRange;
        for (int32 i = 1; i <= NumSegments; ++i)
        {
            const float Angle = -GuardConeHalfAngleDeg + StepAngle * i;
            const FVector Dir = Forward.RotateAngleAxis(Angle, UpVector);
            const FVector CurPoint = Origin + Dir * GuardRange;

            DrawDebugLine(
                World,
                PrevPoint,
                CurPoint,
                GuardDebugColor,
                false,
                LifeTime,
                0,
                1.0f);

            PrevPoint = CurPoint;
        }
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

    // 한 궁극기 발동당 한 번만 처리할 대상들
    TSet<ACharacter*> UniqueTargets;

    for (const FOverlapResult& O : Overlaps)
    {
        AActor* Other = O.GetActor();
        ACharacter* TargetChar = Cast<ACharacter>(Other);
        if (!TargetChar || TargetChar == Owner) continue;

        // 이미 처리한 캐릭터면 스킵
        if (UniqueTargets.Contains(TargetChar)) continue;

        // 거리 체크
        const FVector ToTarget = TargetChar->GetActorLocation() - Origin;
        const float   DistSq = ToTarget.SizeSquared();
        if (DistSq > RangeSq || DistSq <= KINDA_SMALL_NUMBER) continue;

        // 각도 체크 (부채꼴)
        const FVector DirToTarget = ToTarget.GetSafeNormal();
        const float   CosValue = FVector::DotProduct(Forward, DirToTarget);
        if (CosValue < CosThreshold) continue;

        // 여기까지 온 대상만 "이번 궁극기의 타깃"으로 확정
        UniqueTargets.Add(TargetChar);

        // Enemy 상태 강제 Damaged (기존 코드 그대로)
        if (AEnemy_Base* EnemyTarget = Cast<AEnemy_Base>(TargetChar))
        {
            if (UBlackboardComponent* BB = EnemyTarget->BlackboardComp)
            {
                const EEnemyState CurrentState = static_cast<EEnemyState>(
                    BB->GetValueAsEnum("EnemyState"));

                BB->SetValueAsEnum("EnemyState",
                    static_cast<uint8>(EEnemyState::EES_Damaged));

                UE_LOG(LogTemp, Log,
                    TEXT("[GuardUltimate] Enemy %s state FORCED: %d -> Damaged (EES_Attacking bypass)"),
                    *EnemyTarget->GetName(),
                    static_cast<int32>(CurrentState));
            }
        }

        // 2) 데미지
        if (GuardDamage > 0.f)
        {
            if (bOwnerIsPlayer && !TargetChar->IsA<AEnemy_Base>()) { /*continue;*/ }
            if (bOwnerIsEnemy && !TargetChar->IsA<AKHU_GEBCharacter>()) { /*continue;*/ }

            UGameplayStatics::ApplyDamage(
                TargetChar,
                GuardDamage,
                InstigatorController,
                Owner,
                UDamageType::StaticClass());
        }

        // 3) 추락 + 스턴
        if (UCharacterMovementComponent* MoveComp = TargetChar->GetCharacterMovement())
        {
            // 1) 기존 속도는 모두 정지 (수평 미끄러짐 방지)
            MoveComp->StopMovementImmediately();

            // 2) "떨어지는 상태"로 전환
            MoveComp->SetMovementMode(MOVE_Falling);

            // 3) 아래로 강하게 밀어주는 임펄스 (낙하 연출)
            const FVector DownImpulse(0.f, 0.f, -GuardFallStrength);
            TargetChar->LaunchCharacter(DownImpulse, true, true);

            // 4) 스턴
            if (UCrowdControlComponent* CC =
                TargetChar->FindComponentByClass<UCrowdControlComponent>())
            {
                CC->ApplyStun(GuardStunDuration);
            }
        }
    }

    UE_LOG(LogTemp, Log,
        TEXT("[Skill_Ultimate] Guard ultimate executed (Range=%.1f, Damage=%.1f, Stun=%.2f)"),
        GuardRange, GuardDamage, GuardStunDuration);
}

/*============================= Special =============================*/

void USkill_Ultimate::ActivateSpecialUltimate()
{
    UWorld* World = GetWorld();
    AActor* Owner = GetOwner();
    if (!World || !Owner) return;

    if (!SpecialOrbClass)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[Skill_Ultimate] SpecialOrbClass is not set."));
        return;
    }

    // [추가] Special 궁극기 시작
    bSpecialUltimateActive = true;

    SpecialOrbs.Empty();

    const FVector Center = Owner->GetActorLocation();
    const FVector Forward = Owner->GetActorForwardVector();
    const FVector Right = Owner->GetActorRightVector();

    const float HeightOffset = 50.f;

    // 오브의 스폰 위치를 저장할 배열
    TArray<FVector> OrbLocations;
    OrbLocations.Reserve(5);

    // 5개의 구체를 오망성 모양(원 위 5점)으로 배치
    for (int32 i = 0; i < 5; ++i)
    {
        const float AngleDeg = 72.f * i; // 360 / 5
        const float AngleRad = FMath::DegreesToRadians(AngleDeg);

        const float X = FMath::Cos(AngleRad);
        const float Y = FMath::Sin(AngleRad);

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
            Orb->AttachToActor(
                Owner,
                FAttachmentTransformRules::KeepWorldTransform);

            SpecialOrbs.Add(Orb);

            Orb->OnDestroyed.AddDynamic(
                this,
                &USkill_Ultimate::HandleSpecialOrbDestroyed);

            // 스폰된 위치 저장
            OrbLocations.Add(SpawnLocation);
        }
    }

    // Orb들이 모두 생성된 뒤, 바닥에 오망성 그리기
    if (bDrawSpecialPentagram && OrbLocations.Num() == 5)
    {
        DrawPentagramOnGround(OrbLocations);
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
    if (World) { World->GetTimerManager().ClearTimer(SpecialDurationTimerHandle); }

    // 아직 살아있는 구체 수 세기
    int32 AliveCount = 0;
    for (TWeakObjectPtr<AActor>& WeakOrb : SpecialOrbs)
    {
        if (AActor* Orb = WeakOrb.Get()) { ++AliveCount; }
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
        
        // [수정] 스킬 완료 처리
        CompleteSpecialUltimate();
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
    else {
        // 도트 데미지가 0이면 바로 완료 처리
		CompleteSpecialUltimate();
    }
}

void USkill_Ultimate::OnSpecialSelfDotTick()
{
    UWorld* World = GetWorld();
    if (!World) return;

    // 0번 플레이어 캐릭터 (주인공)
    ACharacter* PlayerChar = Cast<ACharacter>(
        UGameplayStatics::GetPlayerCharacter(World, 0));

    if (!PlayerChar)
    {
        World->GetTimerManager().ClearTimer(SpecialSelfDotTimerHandle);
        SpecialSelfDotTicksRemaining = 0;
        CompleteSpecialUltimate(); // [추가] 완료 처리
        return;
    }

    if (SpecialSelfDotDamage > 0.f)
    {
        // 도트도 결국 "일반 데미지"를 주기적으로 넣는 형태
        // InstigatorController는 이 스킬의 소유자 기준
        AActor* Owner = GetOwner();
        AController* InstigatorController = nullptr;
        if (APawn* PawnOwner = Cast<APawn>(Owner))
        {
            InstigatorController = PawnOwner->GetController();
        }

        UGameplayStatics::ApplyDamage(
            PlayerChar,
            SpecialSelfDotDamage,
            InstigatorController,
            Owner,
            UDamageType::StaticClass());
    }

    --SpecialSelfDotTicksRemaining;
    if (SpecialSelfDotTicksRemaining <= 0)
    {
        World->GetTimerManager().ClearTimer(SpecialSelfDotTimerHandle);
        // [추가] 도트 완료 후 스킬 종료
		CompleteSpecialUltimate();
    }
}

void USkill_Ultimate::ApplyRootToPlayer(float Duration)
{
    if (Duration <= 0.f) return;

    UWorld* World = GetWorld();
    if (!World) return;

    ACharacter* PlayerChar = Cast<ACharacter>(
        UGameplayStatics::GetPlayerCharacter(World, 0));
    if (!PlayerChar) return;

    SpecialRootedPlayer = PlayerChar;

    if (UCrowdControlComponent* CC =
        PlayerChar->FindComponentByClass<UCrowdControlComponent>())
    {
        CC->ApplyStun(Duration); // 또는 ApplyRoot(Duration) 만 쓰고 싶으면 Root
    }

    // 굳이 MovementMode를 직접 건드릴 필요 없음
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
    if (World) { World->GetTimerManager().ClearTimer(SpecialRootTimerHandle); }

    if (ACharacter* PlayerChar = Cast<ACharacter>(SpecialRootedPlayer.Get()))
    {
        if (UCrowdControlComponent* CC =
            PlayerChar->FindComponentByClass<UCrowdControlComponent>())
        {
            // 여기서 ClearCC()를 직접 호출할 수도 있지만,
            // 어차피 타이머로 Duration이 끝나면 자동 해제되므로
            // 별도 처리 안 해도 됨 (원한다면 명시적으로 ClearCC 호출)
        }
    }

    SpecialRootedPlayer = nullptr;

    UE_LOG(LogTemp, Log,
        TEXT("[Skill_Ultimate] Special: player root ended."));
}

void USkill_Ultimate::DrawPentagramOnGround(const TArray<FVector>& OrbWorldLocations)
{
    UWorld* World = GetWorld();
    if (!World) return;
    if (OrbWorldLocations.Num() < 5) return;

    // 1) 각 오브 위치를 바닥으로 투영 (라인트레이스)
    TArray<FVector> GroundPoints;
    GroundPoints.SetNum(5);

    for (int32 i = 0; i < 5; ++i)
    {
        const FVector& P = OrbWorldLocations[i];

        FVector Start = P + FVector(0.f, 0.f, 500.f);
        FVector End = P - FVector(0.f, 0.f, 5000.f);

        FHitResult Hit;
        FCollisionQueryParams QueryParams(
            SCENE_QUERY_STAT(SpecialPentagramTrace),
            false,
            nullptr);

        if (World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, QueryParams))
        {
            GroundPoints[i] = Hit.ImpactPoint;      // 실제 바닥
        }
        else
        {
            // 바닥이 안 잡히면 그냥 위치의 Z를 약간 내린 값 사용
            GroundPoints[i] = FVector(P.X, P.Y, P.Z - 50.f);
        }
    }

    // 2) 오망성 연결 순서: 0 → 2 → 4 → 1 → 3 → 0
    static const int32 StarOrder[6] = { 0, 2, 4, 1, 3, 0 };

    const float LifeTime = SpecialDuration;  // Special 유지시간 동안 유지
    const float Thickness = 6.f;

    for (int32 i = 0; i < 5; ++i)
    {
        const FVector& A = GroundPoints[StarOrder[i]];
        const FVector& B = GroundPoints[StarOrder[i + 1]];

        DrawDebugLine(
            World,
            A,
            B,
            SpecialPentagramColor,
            false,
            LifeTime,
            0,
            Thickness);
    }
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

// [추가] Special 궁극기 완료 처리 함수
void USkill_Ultimate::CompleteSpecialUltimate()
{
    if (!bSpecialUltimateActive) return;

    bSpecialUltimateActive = false;

    UE_LOG(LogTemp, Log,
        TEXT("[Skill_Ultimate] Special ultimate COMPLETED - Broadcasting delegate"));

    // 델리게이트 브로드캐스트 (TUltimate가 구독 중)
    OnSpecialUltimateCompleted.Broadcast();
}