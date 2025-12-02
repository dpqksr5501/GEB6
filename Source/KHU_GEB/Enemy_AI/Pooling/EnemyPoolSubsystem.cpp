#include "EnemyPoolSubsystem.h"
#include "Enemy_AI/Enemy_Base.h"
#include "Engine/World.h"

void UEnemyPoolSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    // 서브시스템 초기화
}

void UEnemyPoolSubsystem::Deinitialize()
{
    // 모든 풀 정리 (필요시)
    EnemyPools.Empty();
    Super::Deinitialize();
}

void UEnemyPoolSubsystem::InitializePool(TSubclassOf<AEnemy_Base> EnemyClass, int32 PoolSize)
{   
    UWorld* World = GetWorld();
    if (!World || !EnemyClass || PoolSize <= 0) {
		UE_LOG(LogTemp, Warning, TEXT("EnemyPoolSubsystem: Invalid parameters for InitializePool."));
        return;
    }

	// EnemyPools에 EnemyClass로 생성된 풀이 없으면 새로운 풀 생성
    FEnemyPool& EnemyPoolStruct = EnemyPools.FindOrAdd(EnemyClass);

    // 풀용 안전한 위치 (맵 밖 또는 숨겨진 위치)
    FVector PoolStorageLocation = FVector(0, 0, -10000.0f);

    for (int32 i = 0; i < PoolSize; ++i)
    {
		// 모두 같은 위치에 생성하므로, 충돌 문제를 피하기 위해 충돌 처리 방식을 AlwaysSpawn으로 설정
        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        
        // Pool 위치에 기본 회전, 충돌 무시로 적 생성
        AEnemy_Base* NewEnemy = World->SpawnActor<AEnemy_Base>(
            EnemyClass,
            PoolStorageLocation,  // 풀 저장 위치
            FRotator::ZeroRotator, // 기본 회전
            SpawnParams
        );
        // 여기에 소환한 Actor에 AIController가 할당되어있는지 확인하는 코드
		if (NewEnemy && !NewEnemy->GetController())
        {
            UE_LOG(LogTemp, Warning, TEXT("EnemyPoolSubsystem: Newly spawned %s has no controller!"), 
                   *NewEnemy->GetName());
        }

		// 생성된 적을 풀 상태로 설정하고 Pool Struct에 추가
        if (NewEnemy)
        {
            SetEnemyActive(NewEnemy, false); // 완전 비활성화가 아님. Enemy의 초기 설정을 위해 충돌 등 필요한 것만 비활성화.
            EnemyPoolStruct.Pool.Add(NewEnemy);

            UE_LOG(LogTemp, Log, TEXT("EnemyPoolSubsystem: Created %s for pool at location %s"), 
                   *NewEnemy->GetName(), *PoolStorageLocation.ToString());
        }
    }
}

AEnemy_Base* UEnemyPoolSubsystem::GetEnemyFromPool(TSubclassOf<AEnemy_Base> EnemyClass,
    const FVector& SpawnLocation, const FRotator& SpawnRotation) // 헤더파일에 기본인자 지정되어 있음
{
    if (!EnemyClass) {
        UE_LOG(LogTemp, Warning, TEXT("EnemyPoolSubsystem: Invalid EnemyClass in GetEnemyFromPool."));
        return nullptr;    }

    FEnemyPool& EnemyPoolStruct = EnemyPools.FindOrAdd(EnemyClass);

    // 1. 풀에서 재활용할 적을 찾음 (충돌이 비활성화된 Enemy를 찾음)
    for (AEnemy_Base* Enemy : EnemyPoolStruct.Pool)
    {
        if (Enemy && !Enemy->GetActorEnableCollision()) // 충돌이 비활성화된 Enemy = 풀에서 대기중
        {
            SetEnemyActive(Enemy, true); // 게임에서 사용 가능한 상태로 활성화
            
            UE_LOG(LogTemp, Log, TEXT("EnemyPoolSubsystem: Reused %s from pool at location %s"), 
                   *Enemy->GetName(), *SpawnLocation.ToString());
            return Enemy;
        }
    }

    // 2. 재활용할 적이 없음. 새로 생성
    UWorld* World = GetWorld();
    if (!World) return nullptr;

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    AEnemy_Base* NewEnemy = World->SpawnActor<AEnemy_Base>(
        EnemyClass,
		SpawnLocation,    // 풀의 위치 (0, 0, -10000)
        SpawnRotation,    // 0, 0, 0
        SpawnParams
    );

    if (NewEnemy)
    {
        EnemyPoolStruct.Pool.Add(NewEnemy); // 새로 생성한 경우도 풀에 추가

        SetEnemyActive(NewEnemy, true); // 풀 전용 상태로 활성화
        
        UE_LOG(LogTemp, Log, TEXT("EnemyPoolSubsystem: Created new %s at location %s"), 
               *NewEnemy->GetName(), *SpawnLocation.ToString());
        return NewEnemy;
    }

    return nullptr;
}

void UEnemyPoolSubsystem::ReturnEnemyToPool(AEnemy_Base* Enemy)
{
    if (Enemy)
    {
        UE_LOG(LogTemp, Log, TEXT("EnemyPoolSubsystem: Returning %s to pool"), *Enemy->GetName());
        SetEnemyActive(Enemy, false); // 풀 상태로 되돌리기
    }
}

void UEnemyPoolSubsystem::SetEnemyActive(AEnemy_Base* Enemy, bool bIsActive)
{
    if (!Enemy) {
		UE_LOG(LogTemp, Warning, TEXT("EnemyPoolSubsystem: SetEnemyActive called with null Enemy."));
        return;
    }

    if (bIsActive)
    {
        // [활성화 - 게임에서 사용 가능한 상태]
        Enemy->SetActorEnableCollision(true);   // 충돌 활성화
        Enemy->SetActorTickEnabled(true);       // 틱 활성화
        Enemy->SetActorHiddenInGame(false);     // 보이게 설정
        
        // AIController가 정상적으로 작동하도록 보장
        if (!Enemy->GetController())
        {
            UE_LOG(LogTemp, Warning, TEXT("EnemyPoolSubsystem: %s has no controller after activation!"), 
                   *Enemy->GetName());
            // 여기에 AIController 생성 코드를 추가해줘
			Enemy->SpawnDefaultController();
        }
    }
    else
    {
        // [비활성화 - 풀에서 대기 상태]
        Enemy->SetActorEnableCollision(false);  // 충돌만 비활성화 (겹쳐져도 문제없음)
        Enemy->SetActorTickEnabled(false);      // 틱 비활성화 (성능 절약)
        Enemy->SetActorHiddenInGame(true);      // 보이지 않게 설정
        
        // 풀 저장소로 이동 (맵 밖 안전한 위치)
        FVector PoolStorageLocation = FVector(0, 0, -10000.0f);
        Enemy->SetActorLocation(PoolStorageLocation);
    }
}