// Dragon.h😄

#pragma once

#include "CoreMinimal.h"
#include "MonsterBase.h"
#include "Dragon.generated.h"

UCLASS()
class KHU_GEB_API ADragon : public AMonsterBase // 부모가 MonsterBase 입니다.
{
	GENERATED_BODY()

public:
	// 생성자
	ADragon();

protected:
	// --- 드래곤 전용 상태 변수 ---
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Flight", meta = (AllowPrivateAccess = "true"))
	bool bIsFlying = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Flight", meta = (AllowPrivateAccess = "true"))
	bool bIsDescending = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Flight", meta = (AllowPrivateAccess = "true"))
	bool bIsAscending = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Flight", meta = (AllowPrivateAccess = "true"))
	bool bDescentMovementActive = false;

	

	// 엔진 기본 함수 재정의
	virtual void Landed(const FHitResult& Hit) override;
	virtual void Tick(float DeltaTime) override;
	virtual void BeginPlay() override;

	// --- 비행 설정 변수 (블루프린트에서 조절 가능) ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight", meta = (AllowPrivateAccess = "true"))
	float FlyingBrakingDeceleration = 500.f; // 비행 시 제동 감속

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight", meta = (AllowPrivateAccess = "true"))
	float MaxFlySpeed = 900.f; // 최대 비행 속도

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight", meta = (AllowPrivateAccess = "true"))
	float LandingTraceDistance = 150.f; // 착륙 감지 거리

private:
	// CharacterMovement 컴포넌트를 저장할 포인터 (private으로 변경)
	UPROPERTY()
	TObjectPtr<class UCharacterMovementComponent> DragonMovementComponent; // TObjectPtr 사용 권장

};