// Fill out your copyright notice in the Description page of Project Settings.


#include "MonsterAnimInstanceBase.h"
#include "Animation/AnimInstance.h"
#include "MonsterBase.h"
#include "Kismet/KismetMathLibrary.h"

void UMonsterAnimInstanceBase::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	OwningMonster = Cast<AMonsterBase>(TryGetPawnOwner());
	if (OwningMonster) {
		LastYaw = OwningMonster->GetActorRotation().Yaw;
	}
	YawDeltaSpeed = 0.f;
}

void UMonsterAnimInstanceBase::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (OwningMonster)
	{
		Speed = OwningMonster->GetVelocity().Size();
		CharacterState = OwningMonster->GetCharacterState();

        if (DeltaSeconds > 0.f) // 0���� ������ ����
        {
            FRotator CurrentRotation = OwningMonster->GetActorRotation();
            // �� ���� ������ �ִ� �Ÿ� ���� ���̸� ���մϴ�.
            const float DeltaYaw = FMath::FindDeltaAngleDegrees(LastYaw, CurrentRotation.Yaw);

            // �ʴ� ȸ�� �ӵ� (DeltaYaw / DeltaSeconds)
            const float TargetYawDeltaSpeed = DeltaYaw / DeltaSeconds;

            // ���� �ε巴�� ����(Interp)�Ͽ� �ް��� ��ȭ�� �����ϴ�.
            YawDeltaSpeed = FMath::FInterpTo(YawDeltaSpeed, TargetYawDeltaSpeed, DeltaSeconds, 6.0f); // 6.0f�� ���� �ӵ�(���� �ʿ�)

            LastYaw = CurrentRotation.Yaw; // ���� �������� ���� ���� Yaw �� ����

            // 1. ī�޶�(��Ʈ�ѷ�)�� �ٶ󺸴� ȸ�� ��
            const FRotator ControlRotation = OwningMonster->GetControlRotation();
            // 2. ĳ���� �޽�(����)�� ���� ���ϰ� �ִ� ȸ�� ��
            const FRotator ActorRotation = OwningMonster->GetActorRotation();

            // 3. �� ȸ�� ���� ���̸� ��� (NormalizedDeltaRotator�� -180~180 ������ �ִ� ���� ���̸� ����)
            const FRotator DeltaRotation = UKismetMathLibrary::NormalizedDeltaRotator(ControlRotation, ActorRotation);

            // 4. ��ǥ �� (Pitch, Yaw)
            const float TargetAimPitch = DeltaRotation.Pitch;
            const float TargetAimYaw = DeltaRotation.Yaw;

            // 5. �ε巴�� ���� ����(Interp)�Ͽ� AimPitch, AimYaw�� ����
            // (6.0f�� ���� �ӵ��̸�, �׽�Ʈ�ϸ� �����ϼ���)
            AimPitch = FMath::FInterpTo(AimPitch, TargetAimPitch, DeltaSeconds, 6.0f);
            AimYaw = FMath::FInterpTo(AimYaw, TargetAimYaw, DeltaSeconds, 6.0f);
        }

        // ĳ���Ͱ� ���������� ����ӵ� ������ 0���� ����
        if (Speed < 10.f)
        {
            YawDeltaSpeed = FMath::FInterpTo(YawDeltaSpeed, 0.f, DeltaSeconds, 4.0f);
        }
	}
    else
    {
        // ... (���� OwningMonster�� ���� ���� ����) ...
        AimPitch = 0.f;
        AimYaw = 0.f;
    }

   
}