// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/BoxComponent.h"
#include "MusicManager.h"
#include "BossMusicTrigger.generated.h"



UCLASS()
class KHU_GEB_API ABossMusicTrigger : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ABossMusicTrigger();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// 1. 트리거 영역 (박스)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Trigger")
	UBoxComponent* TriggerBox;

	// 2. 이 구역에서 틀어야 할 음악 (에디터에서 설정 가능하도록 EditAnywhere)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	USoundBase* LevelBGM;

	// 3. 겹침 이벤트 함수
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	
	// 매니저 참조 변수
	UPROPERTY(BlueprintReadOnly, Category = "Reference")
	AMusicManager* MusicManagerRef;
};
