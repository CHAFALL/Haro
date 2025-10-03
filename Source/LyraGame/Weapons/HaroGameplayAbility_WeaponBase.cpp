// Fill out your copyright notice in the Description page of Project Settings.


#include "HaroGameplayAbility_WeaponBase.h"
#include "Weapons/HaroRangedWeaponInstance.h"
#include "AIController.h"
//#include "NativeGameplayTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(HaroGameplayAbility_WeaponBase) 

// Weapon fire will be blocked/canceled if the player has this tag (기존 Lyra 방식 사용)
//UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_WeaponFireBlocked, "Ability.Weapon.NoFiring");

UHaroGameplayAbility_WeaponBase::UHaroGameplayAbility_WeaponBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	//SourceBlockedTags.AddTag(TAG_WeaponFireBlocked);
	
	// 간단한 방식 - 태그를 직접 생성
    // 해당 방식은 런타임에 검색해서 주석처리한 위의 방식보다 느림.
	SourceBlockedTags.AddTag(FGameplayTag::RequestGameplayTag("Ability.Weapon.NoFiring"));
}

UHaroRangedWeaponInstance* UHaroGameplayAbility_WeaponBase::GetWeaponInstance() const
{
    return Cast<UHaroRangedWeaponInstance>(GetAssociatedEquipment());
}

FVector UHaroGameplayAbility_WeaponBase::GetWeaponTargetingSourceLocation() const
{
	APawn* const AvatarPawn = Cast<APawn>(GetAvatarActorFromActorInfo());
	check(AvatarPawn);

	const FVector SourceLoc = AvatarPawn->GetActorLocation();
	FVector TargetingSourceLocation = SourceLoc;

	//@TODO: Add an offset from the weapon instance and adjust based on pawn crouch/aiming/etc...

	return TargetingSourceLocation;
}

FTransform UHaroGameplayAbility_WeaponBase::GetTargetingTransform(APawn* SourcePawn, EHaroAbilityTargetingSource Source) const
{
    check(SourcePawn);
    AController* SourcePawnController = SourcePawn->GetController();

    // Custom은 블루프린트에서 처리해야 함
    check(Source != EHaroAbilityTargetingSource::Custom);

    const FVector ActorLoc = SourcePawn->GetActorLocation();
    FQuat AimQuat = SourcePawn->GetActorQuat();
    AController* Controller = SourcePawn->Controller;
    FVector SourceLoc;

    double FocalDistance = 1024.0f;
    FVector FocalLoc;
    FVector CamLoc;
    FRotator CamRot;
    bool bFoundFocus = false;

    // 카메라 기반 타겟팅 처리
    if ((Controller != nullptr) &&
        ((Source == EHaroAbilityTargetingSource::CameraTowardsFocus) ||
            (Source == EHaroAbilityTargetingSource::PawnTowardsFocus) ||
            (Source == EHaroAbilityTargetingSource::WeaponTowardsFocus)))
    {
        bFoundFocus = true;

        APlayerController* PC = Cast<APlayerController>(Controller);
        if (PC != nullptr)
        {
            PC->GetPlayerViewPoint(/*out*/ CamLoc, /*out*/ CamRot);
        }
        else
        {
            SourceLoc = GetWeaponTargetingSourceLocation();
            CamLoc = SourceLoc;
            CamRot = Controller->GetControlRotation();
        }

        // 조준 방향 계산
        FVector AimDir = CamRot.Vector().GetSafeNormal();
        FocalLoc = CamLoc + (AimDir * FocalDistance);

        // 플레이어 컨트롤러 특별 처리
        if (PC)
        {
            const FVector WeaponLoc = GetWeaponTargetingSourceLocation();
            CamLoc = FocalLoc + (((WeaponLoc - FocalLoc) | AimDir) * AimDir);
            FocalLoc = CamLoc + (AimDir * FocalDistance);
        }
        // AI 컨트롤러 처리
        else if (AAIController* AIController = Cast<AAIController>(Controller))
        {
            CamLoc = SourcePawn->GetActorLocation() + FVector(0, 0, SourcePawn->BaseEyeHeight);
        }

        // 카메라에서 조준점으로
        if (Source == EHaroAbilityTargetingSource::CameraTowardsFocus)
        {
            return FTransform(CamRot, CamLoc);
        }
    }

    // 발사 위치 결정
    if ((Source == EHaroAbilityTargetingSource::WeaponForward) ||
        (Source == EHaroAbilityTargetingSource::WeaponTowardsFocus))
    {
        SourceLoc = GetWeaponTargetingSourceLocation();
    }
    else
    {
        // PawnForward 또는 PawnTowardsFocus
        SourceLoc = ActorLoc;
    }

    // 조준점을 향한 방향 계산
    if (bFoundFocus &&
        ((Source == EHaroAbilityTargetingSource::PawnTowardsFocus) ||
            (Source == EHaroAbilityTargetingSource::WeaponTowardsFocus)))
    {
        return FTransform((FocalLoc - SourceLoc).Rotation(), SourceLoc);
    }

    // 기본: 정면 방향
    return FTransform(AimQuat, SourceLoc);
}

FVector UHaroGameplayAbility_WeaponBase::VRandConeNormalDistribution_Haro(const FVector& Dir, const float ConeHalfAngleRad, const float Exponent)
{
    if (ConeHalfAngleRad > 0.f)
    {
        const float ConeHalfAngleDegrees = FMath::RadiansToDegrees(ConeHalfAngleRad);

        // consider the cone a concatenation of two rotations. one "away" from the center line, and another "around" the circle
        // apply the exponent to the away-from-center rotation. a larger exponent will cluster points more tightly around the center
        const float FromCenter = FMath::Pow(FMath::FRand(), Exponent);
        const float AngleFromCenter = FromCenter * ConeHalfAngleDegrees;
        const float AngleAround = FMath::FRand() * 360.0f;

        FRotator Rot = Dir.Rotation();
        FQuat DirQuat(Rot);
        FQuat FromCenterQuat(FRotator(0.0f, AngleFromCenter, 0.0f));
        FQuat AroundQuat(FRotator(0.0f, 0.0, AngleAround));
        FQuat FinalDirectionQuat = DirQuat * AroundQuat * FromCenterQuat;
        FinalDirectionQuat.Normalize();

        return FinalDirectionQuat.RotateVector(FVector::ForwardVector);
    }
    else
    {
        return Dir.GetSafeNormal();
    }
}


