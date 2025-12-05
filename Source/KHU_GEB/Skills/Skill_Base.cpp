// Fill out your copyright notice in the Description page of Project Settings.


#include "Skills/Skill_Base.h"
#include "Engine/World.h"
#include "Engine/EngineTypes.h"
#include "Engine/OverlapResult.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"

USkill_Base::USkill_Base()
{
    PrimaryComponentTick.bCanEverTick = false;
    PrimaryComponentTick.bStartWithTickEnabled = false;
}

void USkill_Base::ActivateSkill()
{
    UWorld* World = GetWorld();
    AActor* Owner = GetOwner();
    if (!World || !Owner) return;

    // 1) 공통 비용 처리 (쿨타임 시작 + 마나 소모 + 로그)
    Super::ActivateSkill();

    // 2) 현재 폼의 스킬 몽타주 재생
    PlayFormSkillMontage();

    // 3) 반경이 0 이하면 할 게 없음
    const float Radius = Params.Range;
    if (Radius <= 0.f)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[Skill_Base] Params.Range <= 0. Skip AoE."));
        return;
    }

    const FVector Center = Owner->GetActorLocation();

    // 4) Pawn 채널 대상으로 구형 Overlap
    FCollisionObjectQueryParams ObjParams;
    ObjParams.AddObjectTypesToQuery(ECC_Pawn);

    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(SkillBaseAoe), false, Owner);

    TArray<FOverlapResult> Overlaps;
    const bool bAnyHit = World->OverlapMultiByObjectType(
        Overlaps,
        Center,
        FQuat::Identity,
        ObjParams,
        FCollisionShape::MakeSphere(Radius),
        QueryParams);

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    DrawDebugCircle(
        World,
        Center,
        Radius,
        32,
        FColor::White,
        false,                  // 영구 아님
        1.5f,                   // Duration
        0,                      // Depth Priority
        3.f,                    // 선 두께
        FVector(1.f, 0.f, 0.f), // X축
        FVector(0.f, 1.f, 0.f), // Y축 → XY 평면에 원 생성
        false);                 // 축 표시 X
#endif

    if (!bAnyHit)
    {
        UE_LOG(LogTemp, Verbose,
            TEXT("[Skill_Base] AoE: no targets in range."));
        return;
    }

    UE_LOG(LogTemp, Log,
        TEXT("[Skill_Base] AoE: %d overlaps found. Radius=%.1f"),
        Overlaps.Num(), Radius);

    // 5) 한 번의 시전에서 같은 액터를 두 번 치지 않도록 Set 사용
    TSet<AActor*> UniqueTargets;

    const float RadiusSq = Radius * Radius;

    // ApplyDamage에 쓸 InstigatorController 미리 계산
    APawn* PawnOwner = Cast<APawn>(Owner);
    AController* InstigatorController = PawnOwner ? PawnOwner->GetController() : nullptr;

    const float DamageAmount = GetDamageForCurrentLevel();

    for (const FOverlapResult& O : Overlaps)
    {
        AActor* Other = O.GetActor();
        if (!Other || Other == Owner) continue;

        if (UniqueTargets.Contains(Other)) continue;

        // 시전자와 타겟의 XY 평면 거리만 사용 (원형 판 느낌)
        const FVector Delta = Other->GetActorLocation() - Center;
        const float   DistSq2D = Delta.X * Delta.X + Delta.Y * Delta.Y;

        if (DistSq2D > RadiusSq)
        {
            // 수평 거리가 반경보다 크면 범위 밖 → 스킵
            continue;
        }

        UniqueTargets.Add(Other);

        if (DamageAmount <= 0.f) continue;

        // 5-2) 스킬 데미지 적용 – AActor::ApplyDamage 파이프라인 사용
        UGameplayStatics::ApplyDamage(
            Other,
            DamageAmount,
            InstigatorController,
            Owner,
            UDamageType::StaticClass());

        UE_LOG(LogTemp, Log,
            TEXT("[Skill_Base] ApplyDamage %.1f to %s"),
            DamageAmount,
            *GetNameSafe(Other));
    }
}
