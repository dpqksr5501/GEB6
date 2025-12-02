#include "EnemySpawner.h"
#include "EnemyPoolSubsystem.h"
#include "EnemySpawnDirector.h"
#include "Enemy_AI/Enemy_Base.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "CollisionQueryParams.h"
#include "Engine/OverlapResult.h"

AEnemySpawner::AEnemySpawner()
{
    PrimaryActorTick.bCanEverTick = true;
    // 스포너 자체는 보일 필요가 없을 수 있습니다.
    // SetActorHiddenInGame(true); 
    // SetActorEnableCollision(false);
}

void AEnemySpawner::BeginPlay()
{
    Super::BeginPlay();

    // 서브시스템 캐시
    UGameInstance* GI = GetGameInstance();
    if (GI)
    {
        EnemyPool = GI->GetSubsystem<UEnemyPoolSubsystem>();
        SpawnDirector = GI->GetSubsystem<UEnemySpawnDirector>();
    }

    // 첫 스폰 타이머 설정. 첫 스폰은 SpawnInterval 후에 시도
    SpawnTimer = SpawnInterval;
}

void AEnemySpawner::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // 서브시스템이 없거나 스폰할 클래스가 지정되지 않으면 틱 중지
    if (!EnemyPool || !SpawnDirector || !EnemyClassToSpawn)
    {
        SetActorTickEnabled(false);
		UE_LOG(LogTemp, Warning, TEXT("EnemySpawner: Missing subsystem or EnemyClassToSpawn. Disabling tick."));
        return;
    }

    // 최대 스폰 상태인지 확인 (전체 예산과 타입별 예산 모두 확인)
    if (SpawnDirector->GetCurrentSpawnCount() >= SpawnDirector->GetMaxTotalSpawns())
    {
        // 전체 누적 예산이 모두 소진되었다면 이 스포너의 틱을 영구히 끔
        SetActorTickEnabled(false);
        UE_LOG(LogTemp, Warning, TEXT("EnemySpawner: Max Total Spawn reached! Disabling tick."));
        return;
    }

    // 해당 타입의 최대 스폰 수 확인 (설정된 경우)
    int32 MaxForThisType = SpawnDirector->GetMaxSpawnsForEnemyType(EnemyClassToSpawn);
    if (MaxForThisType >= 0) // -1이 아니면 설정된 것
    {
        int32 CurrentForThisType = SpawnDirector->GetCurrentSpawnCountForEnemyType(EnemyClassToSpawn);
        if (CurrentForThisType >= MaxForThisType)
        {
            // 이 타입의 예산이 모두 소진되었다면 이 스포너의 틱을 영구히 끔
            SetActorTickEnabled(false);
            UE_LOG(LogTemp, Warning, TEXT("EnemySpawner: Max Spawn for type %s reached! Disabling tick."), 
                   EnemyClassToSpawn ? *EnemyClassToSpawn->GetName() : TEXT("None"));
            return;
        }
    }

    SpawnTimer -= DeltaTime;
    if (SpawnTimer <= 0.0f)
    {
        TrySpawnEnemy();
        SpawnTimer = SpawnInterval; // 타이머 리셋
    }
}

void AEnemySpawner::TrySpawnEnemy()
{
    //위치 점유 확인
    if (IsOccupied())
    {
        // 점유됨, 다음 틱까지 대기
		UE_LOG(LogTemp, Warning, TEXT("EnemySpawner: Spawn point occupied. Waiting."));
        return;
    }

    //스폰 요청 (타입별 예산 관리 사용)
    if (SpawnDirector->RequestSpawn(EnemyClassToSpawn))
    {
        // 3. 예산 허가됨. 풀에서 적 가져오기
        AEnemy_Base* EnemyToSpawn = EnemyPool->GetEnemyFromPool(EnemyClassToSpawn);

        if (EnemyToSpawn)
        {
            // Spawnr의 위치와 회전으로 스폰
            // 활성화는 Pool에서 처리됨
            FVector Location = GetActorLocation();
            FRotator Rotation = GetActorRotation();
            EnemyToSpawn->SetActorLocationAndRotation(Location, Rotation);

            // 스폰된 Enemy가 할 일이 있다면 이벤트 호출하는 것을 권장
            UE_LOG(LogTemp, Log, TEXT("EnemySpawner: Successfully spawned %s at location %s"), 
                   EnemyClassToSpawn ? *EnemyClassToSpawn->GetName() : TEXT("None"),
                   *Location.ToString());
        }
    }
    // 스폰량이 최대치라면 다음 틱까지 대기
    // 어차피 Tick에서 최대치 체크되면 틱이 꺼지므로 별도 처리 X
}

bool AEnemySpawner::IsOccupied() const
{
    UWorld* World = GetWorld();
    if (!World) return false;

    TArray<FOverlapResult> Overlaps;
    FCollisionShape Shape = FCollisionShape::MakeSphere(SpawnPointCheckRadius);

    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(this); // 스포너 자신은 무시

    bool bHit = World->OverlapMultiByObjectType(
        Overlaps,        // 1. 결과를 저장할 배열
        GetActorLocation(), // 2. 검사할 중심 위치
        FQuat::Identity,    // 3. 회전값 (Identity = 회전없음)
        FCollisionObjectQueryParams(ECollisionChannel::ECC_Pawn), // 4. 검사할 객체 타입
        Shape,              // 5. 검사할 모양 (구체)
        QueryParams         // 6. 추가 검사 옵션
    );

    if (bHit)
    {
        for (const FOverlapResult& Result : Overlaps)
        {
            AEnemy_Base* HitPawn = Cast<AEnemy_Base>(Result.GetActor());
            // 부딪힌 Pawn이 있고, 그 Pawn이 비활성(Hidden) 상태가 아니라면
            // 점유된 것으로 판단
            if (HitPawn && !HitPawn->IsHidden())
            {
                return true;
            }
        }
    }

    return false; // 점유되지 않음
}