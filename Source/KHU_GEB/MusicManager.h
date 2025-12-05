// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/AudioComponent.h"
#include "MusicManager.generated.h"

UCLASS()
class KHU_GEB_API AMusicManager : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AMusicManager();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Audio")
	UAudioComponent* AudioComp;

	// 마지막으로 재생한 음악 기억 (부활용)
	UPROPERTY(BlueprintReadOnly, Category = "Audio")
	USoundBase* LastPlayedBGM;

	// 음악 재생 (트리거가 호출)
	UFUNCTION(BlueprintCallable, Category = "Audio")
	void PlayMusic(USoundBase* NewMusic);

	// 음악 정지 (보스 사망 시 호출)
	UFUNCTION(BlueprintCallable, Category = "Audio")
	void StopMusic();

	// 부활 시 음악 다시 재생 (UI가 호출)
	UFUNCTION(BlueprintCallable, Category = "Audio")
	void RestartMusic();

};
