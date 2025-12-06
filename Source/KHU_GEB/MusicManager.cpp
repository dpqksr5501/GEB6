// Fill out your copyright notice in the Description page of Project Settings.


#include "MusicManager.h"

// Sets default values
AMusicManager::AMusicManager()
{
	PrimaryActorTick.bCanEverTick = false;

	AudioComp = CreateDefaultSubobject<UAudioComponent>(TEXT("AudioComp"));
	RootComponent = AudioComp;
	AudioComp->bAutoActivate = false; // 시작하자마자 켜지지 않게 설정

}

// Called when the game starts or when spawned
void AMusicManager::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AMusicManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AMusicManager::PlayMusic(USoundBase* NewMusic)
{
	if (!AudioComp || !NewMusic) return;

	// 1. 기억하기 (부활을 위해)
	LastPlayedBGM = NewMusic;

	// 2. 이미 같은 노래가 나오고 있다면 무시 (중복 재생 방지)
	if (AudioComp->IsPlaying() && AudioComp->Sound == NewMusic) return;

	// 3. 재생
	AudioComp->SetSound(NewMusic);
	AudioComp->FadeIn(1.0f, 1.0f); // 1초간 페이드 인
}

void AMusicManager::StopMusic()
{
	if (AudioComp)
	{
		AudioComp->FadeOut(2.0f, 0.0f); // 2초간 페이드 아웃
	}
}

void AMusicManager::RestartMusic()
{
	// 기억해둔 노래가 있다면 다시 틈
	if (LastPlayedBGM)
	{
		PlayMusic(LastPlayedBGM);
	}
}

