// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Enemy_AI/Enemy_Minion.h"
#include "Enemy_Minion_Range.generated.h"

class USphereComponent;
class UProjectileMovementComponent;

/**
 * 원거리 미니언 클래스
 * Target을 향해 구체(총알)를 발사하는 기능을 가짐
 */
UCLASS()
class KHU_GEB_API AEnemy_Minion_Range : public AEnemy_Minion
{
	GENERATED_BODY()

public:
	/** BT Task에서 호출할 발사 함수 */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void Shot();

protected:
	/** 소환할 총알의 블루프린트 클래스 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	TSubclassOf<AActor> BulletClass;

	/** 총알의 발사 속도 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float BulletSpeed = 1000.0f;

	/** 총알의 공격력 (테스트용, 나중에 외부에서 주입 예정) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float BulletDamage = 10.0f;

	/** 총알이 생성될 소켓 이름 (스켈레톤 메시의 소켓) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	FName MuzzleSocketName = FName("Muzzle");
};
