// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Skills/SkillBase.h"
#include "Skill_Range.generated.h"

class UNiagaraSystem;
class UNiagaraComponent;
class AFireballProjectile;

UCLASS(ClassGroup = (Skills), meta = (BlueprintSpawnableComponent))
class KHU_GEB_API USkill_Range : public USkillBase
{
    GENERATED_BODY()

public:
    USkill_Range();

    /** 발사할 화염구 액터 (보통 Projectile 블루프린트) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Range")
    TSubclassOf<AFireballProjectile> FireballClass;

    /** 화염구를 발사할 소켓 이름 (캐릭터 Mesh에 이 이름의 소켓 필요) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Range")
    FName MuzzleSocketName = TEXT("MouthSocket");

    /** (예전) 초기 발사 각도 오프셋 – 이제는 “기본 발사” fallback에만 사용 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Range")
    float LaunchPitchOffsetDegrees = -10.f;

    /** 스킬 시전 시, 입 소켓에서 나갈 발사 이펙트 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Range|FX")
    TObjectPtr<UNiagaraSystem> CastNS;

    /** 화염구 비행 중에 붙을 나이아가라(꼬리/코어 등) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Range|FX")
    TObjectPtr<UNiagaraSystem> ProjectileNS;

    /** 조준 중에 바닥에 보여줄 범위 표시 나이아가라(원형 영역 같은 이펙트) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Range|Target")
    TObjectPtr<UNiagaraSystem> TargetAreaNS;

    /** 조준 원(빨간 서클)의 실제 반경 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Range|Target")
    float TargetRadius = 0.f;

    /** 캐릭터를 중심으로 조준 원의 "중심"이 이동할 수 있는 최대 거리 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Range|Target")
    float MaxAimDistance = 1200.f;

    /** 조준 중 이동키로 원을 움직일 때 사용할 속도 (유닛/초) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Range|Target")
    float AimMoveSpeed = 600.f;

    /** 조준 원의 중앙을 땅에 붙일 때 쓸 라인트레이스 높이 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Range|Target")
    float GroundTraceHalfHeight = 5000.f;

    /** 지면에서 살짝 띄우기 위한 오프셋 (이펙트가 묻히지 않게) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Range|Target")
    float GroundOffsetZ = 5.f;

    virtual void InitializeFromDefinition(const USkillDefinition* Def) override;
    virtual void ActivateSkill() override;
    virtual void StopSkill() override;
    virtual void TickComponent(
        float DeltaTime,
        ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

    /** 조준 중 이동 입력을 받아 저장 (캐릭터 Move에서 호출) */
    void HandleAimMoveInput(const FVector2D& Input);

private:
    bool bIsAiming = false;
    bool bHasValidTarget = false;
    FVector CurrentTargetLocation = FVector::ZeroVector;

    UPROPERTY() TObjectPtr<UNiagaraComponent> TargetAreaComp = nullptr;

    FVector2D AimMoveInput = FVector2D::ZeroVector;

    float GetCurrentTargetRadius() const;
    float GetMaxAimDistance() const;

    void SpawnOrUpdateIndicator();
    void CleanupIndicator();
    void SpawnProjectileTowards(const FVector& TargetLocation);
    void SpawnDefaultProjectile();
};