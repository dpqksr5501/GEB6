// Fill out your copyright notice in the Description page of Project Settings.


#include "BossMusicTrigger.h"
#include "MusicManager.h" // 매니저 기능을 써야 하니 인클루드
#include "Kismet/GameplayStatics.h" // 액터 찾는 기능(GetActorOfClass)용
#include "KHU_GEBCharacter.h"

// Sets default values
ABossMusicTrigger::ABossMusicTrigger()
{
	PrimaryActorTick.bCanEverTick = false;

	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	RootComponent = TriggerBox;
	TriggerBox->SetBoxExtent(FVector(200.f, 200.f, 100.f));

	// 충돌 설정: 겹침 이벤트만 발생하도록 설정
	TriggerBox->SetCollisionProfileName(TEXT("Trigger"));
	TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &ABossMusicTrigger::OnOverlapBegin);

}

// Called when the game starts or when spawned
void ABossMusicTrigger::BeginPlay()
{
	Super::BeginPlay();
	
	// 월드에 있는 뮤직 매니저를 미리 찾아둡니다.
	AActor* FoundActor = UGameplayStatics::GetActorOfClass(GetWorld(), AMusicManager::StaticClass());
	MusicManagerRef = Cast<AMusicManager>(FoundActor);
}

// Called every frame
void ABossMusicTrigger::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}


void ABossMusicTrigger::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// 1. 닿은 액터가 플레이어 캐릭터인지 확인
	AKHU_GEBCharacter* PlayerChar = Cast<AKHU_GEBCharacter>(OtherActor);

	// 2. 플레이어가 맞고 + 매니저가 존재하며 + 재생할 음악이 설정되어 있을 때만 실행
	if (PlayerChar && MusicManagerRef && LevelBGM)
	{
		MusicManagerRef->PlayMusic(LevelBGM);
	}
}