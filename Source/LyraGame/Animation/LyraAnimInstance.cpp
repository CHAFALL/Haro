// Copyright Epic Games, Inc. All Rights Reserved.

#include "LyraAnimInstance.h"
#include "AbilitySystemGlobals.h"
#include "Character/LyraCharacter.h"
#include "Character/LyraCharacterMovementComponent.h"

#include "Camera/LyraCameraComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Weapons/HaroWeaponBase.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(LyraAnimInstance)


ULyraAnimInstance::ULyraAnimInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void ULyraAnimInstance::InitializeWithAbilitySystem(UAbilitySystemComponent* ASC)
{
	check(ASC);

	GameplayTagPropertyMap.Initialize(this, ASC);
}

#if WITH_EDITOR
EDataValidationResult ULyraAnimInstance::IsDataValid(FDataValidationContext& Context) const
{
	Super::IsDataValid(Context);

	GameplayTagPropertyMap.IsDataValid(this, Context);

	return ((Context.GetNumErrors() > 0) ? EDataValidationResult::Invalid : EDataValidationResult::Valid);
}
#endif // WITH_EDITOR

void ULyraAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	if (AActor* OwningActor = GetOwningActor())
	{
		if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OwningActor))
		{
			InitializeWithAbilitySystem(ASC);
		}
	}
}

void ULyraAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (LyraCharacter == nullptr)
	{
		LyraCharacter = Cast<ALyraCharacter>(GetOwningActor());
	}

	if (LyraCharacter == nullptr) return;

	ULyraCharacterMovementComponent* CharMoveComp = CastChecked<ULyraCharacterMovementComponent>(LyraCharacter->GetCharacterMovement());
	const FLyraCharacterGroundInfo& GroundInfo = CharMoveComp->GetGroundInfo();
	GroundDistance = GroundInfo.GroundDistance;

	if (bInterpAiming)
	{
		InterpAiming(DeltaSeconds);
	}
}

void ULyraAnimInstance::OnNewWeaponEquipped(float NewWeaponOffset)
{
	SightOffset = NewWeaponOffset;

	FTimerHandle SyncWeapon_TimerHandle;

	// 0.5초 후에 해당 함수 실행
	// 이렇게 지연을 두는 이유는 무기 교체 애니메이션이나 다른 시각적 효과가 적용될 시간을 주기 위함
	GetWorld()->GetTimerManager().SetTimer(SyncWeapon_TimerHandle, this, &ThisClass::FinalizeWeaponSync, 0.2f, false);
}

void ULyraAnimInstance::SetIsAiming(bool NewAiming)
{
	if (bIsAiming != NewAiming)
	{
		bIsAiming = NewAiming;
		bInterpAiming = true;
	}
}

void ULyraAnimInstance::SetSightTransform()
{
	if (!IsValid(LyraCharacter)) return;
	if (!LyraCharacter->IsLocallyControlled()) return;

	if (LyraCharacter->GetNetMode() != NM_DedicatedServer)
	{
		const FTransform CameraTransform = LyraCharacter->GetCameraComponent()->GetComponentTransform();
		const FTransform MeshTransform = LyraCharacter->GetFirstPersonMesh()->GetComponentTransform();

		SightTransform = UKismetMathLibrary::MakeRelativeTransform(CameraTransform, MeshTransform);

		SightTransform.SetLocation(SightTransform.GetLocation() + SightTransform.GetRotation().Vector() * SightOffset);

	}
}

void ULyraAnimInstance::SetRelativeHandTransform()
{
	if (!IsValid(LyraCharacter)) return;
	if (!LyraCharacter->IsLocallyControlled()) return;

	if (LyraCharacter->GetNetMode() != NM_DedicatedServer)
	{
		if (LyraCharacter->GetCurrentWeapon() == nullptr || !IsValid(LyraCharacter->GetCurrentWeapon()->GetWeaponMesh1P())) return;

		const FTransform OpticTransform = LyraCharacter->GetCurrentWeapon()->GetWeaponMesh1P()->GetSocketTransform(FName("S_IronSights"));

		const FTransform MeshTransform = LyraCharacter->GetFirstPersonMesh()->GetSocketTransform(FName("hand_r"));

		// 오른손을 기준으로 한 조준경의 상댁적 위치/회전 계산
		RelativeHandTransform = UKismetMathLibrary::MakeRelativeTransform(OpticTransform, MeshTransform);
	}
}

void ULyraAnimInstance::InterpAiming(float DeltaTime)
{
	//AimAlpha = FMath::FInterpTo(AimAlpha, bIsAiming ? 1.0f : 0.0f, DeltaTime, 15.0f);
	AimAlpha = FMath::FInterpTo(AimAlpha, bIsAiming, DeltaTime, 15.0f); // 이렇게도 가능.

	if (AimAlpha >= 1.0f || AimAlpha <= 0.0f)
	{
		bInterpAiming = false;
	}
}

void ULyraAnimInstance::FinalizeWeaponSync()
{
	SetSightTransform();
	SetRelativeHandTransform();
}

