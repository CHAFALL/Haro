// Copyright Epic Games, Inc. All Rights Reserved.

#include "LyraWeaponInstance.h"

#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "Math/UnrealMathUtility.h"
#include "Misc/AssertionMacros.h"
#include "GameFramework/InputDeviceSubsystem.h"
#include "GameFramework/InputDeviceProperties.h"
#include "Character/LyraHealthComponent.h"
#include "Character/LyraCharacter.h"
#include "LyraLogChannels.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LyraWeaponInstance)

class UAnimInstance;
struct FGameplayTagContainer;

ULyraWeaponInstance::ULyraWeaponInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Listen for death of the owning pawn so that any device properties can be removed if we
	// die and can't unequip
	if (APawn* Pawn = GetPawn())
	{
		// We only need to do this for player controlled pawns, since AI and others won't have input devices on the client
		if (Pawn->IsPlayerControlled())
		{
			if (ULyraHealthComponent* HealthComponent = ULyraHealthComponent::FindHealthComponent(GetPawn()))
			{
				HealthComponent->OnDeathStarted.AddDynamic(this, &ThisClass::OnDeathStarted);
			}
		}
	}
}

void ULyraWeaponInstance::OnEquipped()
{
	Super::OnEquipped();

	UWorld* World = GetWorld();
	check(World);
	TimeLastEquipped = World->GetTimeSeconds();

	ApplyDeviceProperties();
}

void ULyraWeaponInstance::OnUnequipped()
{
	Super::OnUnequipped();

	RemoveDeviceProperties();
}

void ULyraWeaponInstance::UpdateFiringTime()
{
	UWorld* World = GetWorld();
	check(World);
	TimeLastFired = World->GetTimeSeconds();
}

float ULyraWeaponInstance::GetTimeSinceLastInteractedWith() const
{
	UWorld* World = GetWorld();
	check(World);
	const double WorldTime = World->GetTimeSeconds();

	double Result = WorldTime - TimeLastEquipped;

	if (TimeLastFired > 0.0)
	{
		const double TimeSinceFired = WorldTime - TimeLastFired;
		Result = FMath::Min(Result, TimeSinceFired);
	}

	return Result;
}

TSubclassOf<UAnimInstance> ULyraWeaponInstance::PickBestAnimLayer(bool bEquipped, const FGameplayTagContainer& CosmeticTags) const
{
	const FLyraAnimLayerSelectionSet& SetToQuery = (bEquipped ? EquippedAnimSet_3P : UnequippedAnimSet_3P);
	return SetToQuery.SelectBestLayer(CosmeticTags);
}

UPARAM(DisplayName = "Anim Layer 3P")TSubclassOf<UAnimInstance> ULyraWeaponInstance::PickBestAnimLayerWithFirstPerson(bool bEquipped, const FGameplayTagContainer& CosmeticTags, TSubclassOf<UAnimInstance>& AnimLayer_1P) const
{
	const FLyraAnimLayerSelectionSet& SetToQuery_3P = (bEquipped ? EquippedAnimSet_3P : UnequippedAnimSet_3P);
	const FLyraAnimLayerSelectionSet& SetToQuery_1P = (bEquipped ? EquippedAnimSet_1P : UnequippedAnimSet_1P);
	AnimLayer_1P = SetToQuery_1P.SelectBestLayer(CosmeticTags);
	return SetToQuery_3P.SelectBestLayer(CosmeticTags);
}

const FPlatformUserId ULyraWeaponInstance::GetOwningUserId() const
{
	if (const APawn* Pawn = GetPawn())
	{
		return Pawn->GetPlatformUserId();
	}
	return PLATFORMUSERID_NONE;
}

void ULyraWeaponInstance::ApplyDeviceProperties()
{
	const FPlatformUserId UserId = GetOwningUserId();

	if (UserId.IsValid())
	{
		if (UInputDeviceSubsystem* InputDeviceSubsystem = UInputDeviceSubsystem::Get())
		{
			for (TObjectPtr<UInputDeviceProperty>& DeviceProp : ApplicableDeviceProperties)
			{
				FActivateDevicePropertyParams Params = {};
				Params.UserId = UserId;

				// By default, the device property will be played on the Platform User's Primary Input Device.
				// If you want to override this and set a specific device, then you can set the DeviceId parameter.
				//Params.DeviceId = <some specific device id>;
				
				// Don't remove this property it was evaluated. We want the properties to be applied as long as we are holding the 
				// weapon, and will remove them manually in OnUnequipped
				Params.bLooping = true;
			
				DevicePropertyHandles.Emplace(InputDeviceSubsystem->ActivateDeviceProperty(DeviceProp, Params));
			}
		}	
	}
}

void ULyraWeaponInstance::RemoveDeviceProperties()
{
	const FPlatformUserId UserId = GetOwningUserId();
	
	if (UserId.IsValid() && !DevicePropertyHandles.IsEmpty())
	{
		// Remove any device properties that have been applied
		if (UInputDeviceSubsystem* InputDeviceSubsystem = UInputDeviceSubsystem::Get())
		{
			InputDeviceSubsystem->RemoveDevicePropertyHandles(DevicePropertyHandles);
			DevicePropertyHandles.Empty();
		}
	}
}


void ULyraWeaponInstance::OnDeathStarted(AActor* OwningActor)
{
	// Remove any possibly active device properties when we die to make sure that there aren't any lingering around
	RemoveDeviceProperties();
}


//--------------------(추가)------------------
FTransform ULyraWeaponInstance::GetMuzzleTransform() const
{
	// 1. 주 장비 액터에서 소켓 찾기
	if (AActor* PrimaryActor = GetPrimaryActor())
	{
		if (USkeletalMeshComponent* WeaponMesh = PrimaryActor->FindComponentByClass<USkeletalMeshComponent>())
		{
			// 커스텀 소켓 우선
			if (WeaponMesh->DoesSocketExist(MuzzleSocketName))
			{
				return WeaponMesh->GetSocketTransform(MuzzleSocketName, RTS_World);
			}
			// 기본 Muzzle 소켓
			if (WeaponMesh->DoesSocketExist(TEXT("Muzzle")))
			{
				return WeaponMesh->GetSocketTransform(TEXT("Muzzle"), RTS_World);
			}
		}
	}

	// 2. 캐릭터 소켓 사용 (fallback)
	if (APawn* OwnerPawn = GetPawn())
	{
		if (ALyraCharacter* LyraChar = Cast<ALyraCharacter>(OwnerPawn))
		{
			if (USkeletalMeshComponent* CharMesh = LyraChar->GetMesh())
			{
				if (CharMesh->DoesSocketExist(TEXT("weapon_r")))
				{
					return CharMesh->GetSocketTransform(TEXT("weapon_r"), RTS_World);
				}
				if (CharMesh->DoesSocketExist(MuzzleSocketName))
				{
					return CharMesh->GetSocketTransform(MuzzleSocketName, RTS_World);
				}
			}

			// 소켓이 없으면 캐릭터 앞쪽으로 오프셋
			FTransform CharTransform = LyraChar->GetActorTransform();
			FVector ForwardOffset = CharTransform.GetRotation().GetForwardVector() * 100.0f; // 1미터 앞
			FVector UpOffset = CharTransform.GetRotation().GetUpVector() * 150.0f; // 가슴 높이
			CharTransform.SetLocation(CharTransform.GetLocation() + ForwardOffset + UpOffset);
			return CharTransform;
		}
	}

	// 3. 정말 마지막 fallback (Identity는 위험!)
	UE_LOG(LogTemp, Error, TEXT("GetMuzzleTransform failed for weapon %s - using Identity!"), *GetPathNameSafe(this));
	return FTransform::Identity;
}