#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FlightComponent.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class KHU_GEB_API UFlightComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UFlightComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight")
	float MaxFlySpeed = 900.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight")
	float FlyingBrakingDeceleration = 500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight")
	float LandingTraceDistance = 150.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Flight")
	bool bIsFlying = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Flight")
	bool bIsDescending = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Flight")
	bool bDescentMovementActive = false;

	UFUNCTION(BlueprintCallable, Category = "Flight")
	void StartFlight();

	UFUNCTION(BlueprintCallable, Category = "Flight")
	void StopFlight();

	UFUNCTION(BlueprintCallable, Category = "Flight")
	void OnLanded();

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	class UCharacterMovementComponent* CachedMovementComp;
};
