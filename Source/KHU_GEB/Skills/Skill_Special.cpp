// Fill out your copyright notice in the Description page of Project Settings.


#include "Skills/Skill_Special.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Engine/OverlapResult.h"
#include "Kismet/GameplayStatics.h"
#include "KHU_GEBCharacter.h"
#include "HealthComponent.h"
#include "SkillManagerComponent.h"

void USkill_Special::InitializeFromDefinition(const USkillDefinition* Def)
{
    Super::InitializeFromDefinition(Def);

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
    // 이것 때문에 Enemy가 마나 부족으로 스킬을 못써서 해제 했어요
    /*if (!CanActivate())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[Skill_Special] ActivateSkill blocked (CanActivate=false)"));
        return;
    }*/

    //애니메이션 몽타주 재생
    PlayFormSkillMontage();
    // 공통 비용 처리 (쿨타임 시작 + 마나 소모)
    Super::ActivateSkill();

    bIsActive = true;

    if (USkillManagerComponent* Manager = GetSkillManager())
    {
        Manager->OnSpecialSkillStarted(this);
    }

    // 1) 플레이어 이동속도 배율 적용
    if (AKHU_GEBCharacter* OwnerChar = Cast<AKHU_GEBCharacter>(Owner))
    {
        CachedOwnerChar = OwnerChar;
        OwnerChar->SetSkillSpeedMultiplier(SelfMoveSpeedMultiplier);
    }

    // 2) 어디에 붙일지 결정

    if (DarkFogNS)
    {
        // 플레이어를 따라다니는 원형 흑안개 스폰 & 부착
        SpawnedNS = UNiagaraFunctionLibrary::SpawnSystemAttached(
            DarkFogNS,
            GetOwner()->GetRootComponent(),
            NAME_None,
            FVector::ZeroVector,
            FRotator::ZeroRotator,
            EAttachLocation::KeepRelativeOffset,
            true  // AutoDestroy: true → 소유자와 함께 정리
        );

        if (SpawnedNS)
        {
            const float RadiusScale = FogRadius / SkillReferenceRadius;

            // XY로만 크게, Z는 아주 얇게
            SpawnedNS->SetWorldScale3D(
                FVector(RadiusScale, RadiusScale, 0.05f)
            );
        }
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
    else { Center = Owner->GetActorLocation() + RelativeOffset; }

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    // 흑안개 범위 디버그 표시 (구 → 원)
    // XY 평면에 반지름 FogRadius인 보라색 원을 그림
    DrawDebugCircle(
        World,
        Center,
        FogRadius,
        64,                // 세그먼트 수
        FColor::Emerald,
        false,
        SlowTickInterval,  // 다음 틱 전에 사라지도록
        0,
        2.f,
        FVector(1, 0, 0),  // X축 방향
        FVector(0, 1, 0),  // Y축 방향
        false              // 두께 있는 라인 (필요하면 true 로 변경)
    );
#endif

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

    if (USkillManagerComponent* Manager = GetSkillManager())
    {
        Manager->OnSpecialSkillEnded(this);
    }

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

    const float HealAmount = GetDamageForCurrentLevel();    // 자기 힐
    const float DotAmount = GetDamageForCurrentLevel() * 2; // 적 도트 (예시)

    // 1) 시전자 힐
    if (HealAmount > 0.f)
    {
        if (UHealthComponent* Health = Owner->FindComponentByClass<UHealthComponent>())
        {
            Health->AddHealth(HealAmount);
        }
    }

    // 2) 흑안개 안 적들에게 도트 대미지
    if (FogRadius <= 0.f || DotAmount <= 0.f) return;

    // DoT 영역 중심: 흑안개 NS가 살아있으면 그 위치, 아니면 시전자 + 오프셋
    FVector Center;
    if (SpawnedNS) { Center = SpawnedNS->GetComponentLocation(); }
    else { Center = Owner->GetActorLocation() + RelativeOffset; }

    TArray<FOverlapResult> Overlaps;
    FCollisionObjectQueryParams ObjParams;
    ObjParams.AddObjectTypesToQuery(ECC_Pawn);

    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(SkillSpecialDot), false, Owner);

    const bool bAnyOverlap = World->OverlapMultiByObjectType(
        Overlaps,
        Center,
        FQuat::Identity,
        ObjParams,
        FCollisionShape::MakeSphere(FogRadius),
        QueryParams);

    if (!bAnyOverlap) return;

    TSet<AActor*> UniqueTargets;
    AKHU_GEBCharacter* OwnerChar = Cast<AKHU_GEBCharacter>(Owner);
    AController* InstigatorController = OwnerChar ? OwnerChar->GetController() : nullptr;

    const float RadiusSq = FogRadius * FogRadius;
    bool bAnyDotHit = false;  // ★ 이 틱에 도트가 한 번이라도 들어갔는지

    for (const FOverlapResult& Result : Overlaps)
    {
        AActor* OtherActor = Result.GetActor();
        if (!OtherActor || OtherActor == Owner) continue;
        if (UniqueTargets.Contains(OtherActor)) continue;

        if (!OwnerChar || !OwnerChar->IsEnemyFor(OtherActor)) continue;

        const FVector Delta = OtherActor->GetActorLocation() - Center;
        const float DistSq2D = Delta.X * Delta.X + Delta.Y * Delta.Y;
        if (DistSq2D > RadiusSq) continue;

        UniqueTargets.Add(OtherActor);

        UGameplayStatics::ApplyDamage(
            OtherActor,
            DotAmount,
            InstigatorController,
            Owner,
            UDamageType::StaticClass());

        bAnyDotHit = true;
    }

    // 3) DotDamage가 한 번이라도 들어갔다면 → 시전자 발밑에서 NS 한 번 재생
    if (bAnyDotHit && DotHitNS)
    {
        float Scale = 1.f;
        if (SkillReferenceRadius > 0.f && FogRadius > 0.f)
        {
            Scale = FogRadius / SkillReferenceRadius;
        }

        if (UNiagaraComponent* DotFX = UNiagaraFunctionLibrary::SpawnSystemAttached(
            DotHitNS,
            Owner->GetRootComponent(),
            NAME_None,
            RelativeOffset,          // 발 쪽으로 조금 내리기
            FRotator::ZeroRotator,
            EAttachLocation::SnapToTarget,
            true))                  // AutoDestroy
        {
            DotFX->SetWorldScale3D(FVector(Scale, Scale, 1.f));
        }
    }
}
