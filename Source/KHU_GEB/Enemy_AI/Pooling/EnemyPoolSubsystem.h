#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Templates/SubclassOf.h"
#include "EnemyPoolSubsystem.generated.h"

class AEnemy_Base; // 모든 Enemy의 공통 부모

/** TMap가 TArray를 요소로 가질 수 없어서 TArray를 구조체로 래핑 */
USTRUCT()
struct FEnemyPool
{
    GENERATED_BODY()

    UPROPERTY()
    TArray<TObjectPtr<AEnemy_Base>> Pool;
};

/**
 * 게임 내 모든 적(Pawn) 오브젝트 풀을 관리하는 서브시스템입니다.
 * TSubclassOf<AEnemy_Base>을 키로 사용하여 여러 종류의 적 풀을 가질 수 있습니다.
 */
UCLASS()
class KHU_GEB_API UEnemyPoolSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    /**
     * 지정된 클래스의 적을 특정 수만큼 미리 생성하여 풀을 초기화합니다.
     * @param EnemyClass 풀링할 적 클래스
     * @param PoolSize 초기 풀 크기
     */
    UFUNCTION(BlueprintCallable, Category = "Enemy Pool")
    void InitializePool(TSubclassOf<AEnemy_Base> EnemyClass, int32 PoolSize);

    /**
     * 풀에서 사용 가능한(비활성화된) 적을 가져옵니다.
     * 사용 가능한 적이 없다면 새로 생성하여 풀에 추가하고 반환합니다.
     * @param EnemyClass 가져올 적 클래스
     * @return 활성화 준비가 된 AEnemy_Base* (위치/회전 설정 및 활성화 필요)
     */
    UFUNCTION(BlueprintCallable, Category = "Enemy Pool")
    AEnemy_Base* GetEnemyFromPool(TSubclassOf<AEnemy_Base> EnemyClass,
        const FVector& SpawnLocation = FVector(0, 0, -10000.0f),
        const FRotator& SpawnRotation = FRotator::ZeroRotator);

    /**
     * 사용이 끝난 적을 풀에 반환합니다 (비활성화).
     * @param Enemy 반환할 적 액터
     */
    UFUNCTION(BlueprintCallable, Category = "Enemy Pool")
    void ReturnEnemyToPool(AEnemy_Base* Enemy);

    // GameInstanceSubsystem 오버라이드
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

private:
    /** * 적 클래스별로 오브젝트 풀을 관리합니다.
     * TArray를 직접 값으로 사용할 수 없으므로 FEnemyPool 구조체로 래핑합니다.
     */
    UPROPERTY()
    TMap<TSubclassOf<AEnemy_Base>, FEnemyPool> EnemyPools;

    /** Enemy를 활성화/비활성화 하는 함수 */
    void SetEnemyActive(AEnemy_Base* Enemy, bool bIsActive);
};