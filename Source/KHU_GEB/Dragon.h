// Dragon.h

#pragma once

#include "CoreMinimal.h"
#include "MonsterBase.h"
#include "Dragon.generated.h"

UCLASS()
class KHU_GEB_API ADragon : public AMonsterBase // �θ� MonsterBase �Դϴ�.
{
	GENERATED_BODY()

public:
	// ������
	ADragon();

protected:
	// --- �巡�� ���� ���� ���� ---
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Flight", meta = (AllowPrivateAccess = "true"))
	bool bIsFlying = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Flight", meta = (AllowPrivateAccess = "true"))
	bool bIsDescending = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Flight", meta = (AllowPrivateAccess = "true"))
	bool bIsAscending = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Flight", meta = (AllowPrivateAccess = "true"))
	bool bDescentMovementActive = false;

	

	// ���� �⺻ �Լ� ������
	virtual void Landed(const FHitResult& Hit) override;
	virtual void Tick(float DeltaTime) override;
	virtual void BeginPlay() override;

	// --- ���� ���� ���� (�������Ʈ���� ���� ����) ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight", meta = (AllowPrivateAccess = "true"))
	float FlyingBrakingDeceleration = 500.f; // ���� �� ���� ����

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight", meta = (AllowPrivateAccess = "true"))
	float MaxFlySpeed = 900.f; // �ִ� ���� �ӵ�

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight", meta = (AllowPrivateAccess = "true"))
	float LandingTraceDistance = 150.f; // ���� ���� �Ÿ�

private:
	// CharacterMovement ������Ʈ�� ������ ������ (private���� ����)
	UPROPERTY()
	TObjectPtr<class UCharacterMovementComponent> DragonMovementComponent; // TObjectPtr ��� ����

};