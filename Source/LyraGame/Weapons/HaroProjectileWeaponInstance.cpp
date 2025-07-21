// Fill out your copyright notice in the Description page of Project Settings.


#include "HaroProjectileWeaponInstance.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/LyraCameraComponent.h"
#include "Physics/PhysicalMaterialWithTags.h"
#include "Weapons/LyraWeaponInstance.h"
#include "Weapons/HaroProjectileBase.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(HaroProjectileWeaponInstance)

UHaroProjectileWeaponInstance::UHaroProjectileWeaponInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 기본 열 설정
	HeatToHeatPerShotCurve.EditorCurveData.AddKey(0.0f, 1.0f);
	HeatToCoolDownPerSecondCurve.EditorCurveData.AddKey(0.0f, 2.0f);
}

void UHaroProjectileWeaponInstance::PostLoad()
{
	Super::PostLoad();

#if WITH_EDITOR
	UpdateDebugVisualization();
#endif

}

#if WITH_EDITOR
void UHaroProjectileWeaponInstance::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	UpdateDebugVisualization();
}

void UHaroProjectileWeaponInstance::UpdateDebugVisualization()
{
	ComputeHeatRange(/*out*/ Debug_MinHeat, /*out*/ Debug_MaxHeat);
	ComputeSpreadRange(/*out*/ Debug_MinSpreadAngle, /*out*/ Debug_MaxSpreadAngle);
	Debug_CurrentHeat = CurrentHeat;
	Debug_CurrentSpreadAngle = CurrentSpreadAngle;
	Debug_CurrentSpreadAngleMultiplier = CurrentSpreadAngleMultiplier;
}
#endif

void UHaroProjectileWeaponInstance::ConfigureProjectile(AHaroProjectileBase* Projectile) const
{
	if (!Projectile) return;

	Projectile->SetSpeed(ProjectileSpeed);
	Projectile->SetLifeSpan(ProjectileLifespan);
	Projectile->SetGravityScale(ProjectileGravityScale);
}

void UHaroProjectileWeaponInstance::OnEquipped()
{
	Super::OnEquipped();

	// 열을 중간값에서 시작
	float MinHeatRange;
	float MaxHeatRange;
	ComputeHeatRange(/*out*/ MinHeatRange, /*out*/ MaxHeatRange);
	CurrentHeat = (MinHeatRange + MaxHeatRange) * 0.5f;

	// 확산 도출
	CurrentSpreadAngle = HeatToSpreadCurve.GetRichCurveConst()->Eval(CurrentHeat);

	// 배율들을 기본값 1x로 설정
	CurrentSpreadAngleMultiplier = 1.0f;
	StandingStillMultiplier = 1.0f;
	JumpFallMultiplier = 1.0f;
	CrouchingMultiplier = 1.0f;
}

void UHaroProjectileWeaponInstance::OnUnequipped()
{
	Super::OnUnequipped();
}

void UHaroProjectileWeaponInstance::Tick(float DeltaSeconds)
{
	APawn* Pawn = GetPawn();
	check(Pawn != nullptr);

	const bool bMinSpread = UpdateSpread(DeltaSeconds);
	const bool bMinMultipliers = UpdateMultipliers(DeltaSeconds);

	bHasFirstShotAccuracy = bAllowFirstShotAccuracy && bMinMultipliers && bMinSpread;

#if WITH_EDITOR
	UpdateDebugVisualization();
#endif
}

void UHaroProjectileWeaponInstance::ComputeHeatRange(float& MinHeat, float& MaxHeat)
{
	float Min1, Max1;
	HeatToHeatPerShotCurve.GetRichCurveConst()->GetTimeRange(/*out*/ Min1, /*out*/ Max1);

	float Min2, Max2;
	HeatToCoolDownPerSecondCurve.GetRichCurveConst()->GetTimeRange(/*out*/ Min2, /*out*/ Max2);

	float Min3, Max3;
	HeatToSpreadCurve.GetRichCurveConst()->GetTimeRange(/*out*/ Min3, /*out*/ Max3);

	MinHeat = FMath::Min(FMath::Min(Min1, Min2), Min3);
	MaxHeat = FMath::Max(FMath::Max(Max1, Max2), Max3);
}

void UHaroProjectileWeaponInstance::ComputeSpreadRange(float& MinSpread, float& MaxSpread)
{
	HeatToSpreadCurve.GetRichCurveConst()->GetValueRange(/*out*/ MinSpread, /*out*/ MaxSpread);
}


void UHaroProjectileWeaponInstance::AddSpread()
{
	// 열 증가 커브 샘플링
	const float HeatPerShot = HeatToHeatPerShotCurve.GetRichCurveConst()->Eval(CurrentHeat);
	CurrentHeat = ClampHeat(CurrentHeat + HeatPerShot);

	// 열을 확산각도로 매핑
	CurrentSpreadAngle = HeatToSpreadCurve.GetRichCurveConst()->Eval(CurrentHeat);

	// 마지막 발사 시간 업데이트
	LastFireTime = GetWorld()->GetTimeSeconds();

#if WITH_EDITOR
	UpdateDebugVisualization();
#endif
}

float UHaroProjectileWeaponInstance::GetDistanceAttenuation(float Distance, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags) const
{
	const FRichCurve* Curve = DistanceDamageFalloff.GetRichCurveConst();
	return Curve->HasAnyData() ? Curve->Eval(Distance) : 1.0f;
}

float UHaroProjectileWeaponInstance::GetPhysicalMaterialAttenuation(const UPhysicalMaterial* PhysicalMaterial, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags) const
{
	float CombinedMultiplier = 1.0f;
	if (const UPhysicalMaterialWithTags* PhysMatWithTags = Cast<const UPhysicalMaterialWithTags>(PhysicalMaterial))
	{
		for (const FGameplayTag MaterialTag : PhysMatWithTags->Tags)
		{
			if (const float* pTagMultiplier = MaterialDamageMultiplier.Find(MaterialTag))
			{
				CombinedMultiplier *= *pTagMultiplier;
			}
		}
	}

	return CombinedMultiplier;
}



bool UHaroProjectileWeaponInstance::UpdateSpread(float DeltaSeconds)
{
	const float TimeSinceFired = GetWorld()->TimeSince(LastFireTime);

	if (TimeSinceFired > SpreadRecoveryCooldownDelay)
	{
		const float CooldownRate = HeatToCoolDownPerSecondCurve.GetRichCurveConst()->Eval(CurrentHeat);
		CurrentHeat = ClampHeat(CurrentHeat - (CooldownRate * DeltaSeconds));
		CurrentSpreadAngle = HeatToSpreadCurve.GetRichCurveConst()->Eval(CurrentHeat);
	}

	float MinSpread;
	float MaxSpread;
	ComputeSpreadRange(/*out*/ MinSpread, /*out*/ MaxSpread);

	return FMath::IsNearlyEqual(CurrentSpreadAngle, MinSpread, KINDA_SMALL_NUMBER);
}

bool UHaroProjectileWeaponInstance::UpdateMultipliers(float DeltaSeconds)
{
	const float MultiplierNearlyEqualThreshold = 0.05f;

	APawn* Pawn = GetPawn();
	check(Pawn != nullptr);
	UCharacterMovementComponent* CharMovementComp = Cast<UCharacterMovementComponent>(Pawn->GetMovementComponent());

	// 정지 상태인지 확인하고, 그렇다면 부드럽게 보너스 적용
	const float PawnSpeed = Pawn->GetVelocity().Size();
	const float MovementTargetValue = FMath::GetMappedRangeValueClamped(
		/*InputRange=*/ FVector2D(StandingStillSpeedThreshold, StandingStillSpeedThreshold + StandingStillToMovingSpeedRange),
		/*OutputRange=*/ FVector2D(SpreadAngleMultiplier_StandingStill, 1.0f),
		/*Alpha=*/ PawnSpeed);
	StandingStillMultiplier = FMath::FInterpTo(StandingStillMultiplier, MovementTargetValue, DeltaSeconds, TransitionRate_StandingStill);
	const bool bStandingStillMultiplierAtMin = FMath::IsNearlyEqual(StandingStillMultiplier, SpreadAngleMultiplier_StandingStill, SpreadAngleMultiplier_StandingStill * 0.1f);

	// 웅크리고 있는지 확인하고, 그렇다면 부드럽게 보너스 적용
	const bool bIsCrouching = (CharMovementComp != nullptr) && CharMovementComp->IsCrouching();
	const float CrouchingTargetValue = bIsCrouching ? SpreadAngleMultiplier_Crouching : 1.0f;
	CrouchingMultiplier = FMath::FInterpTo(CrouchingMultiplier, CrouchingTargetValue, DeltaSeconds, TransitionRate_Crouching);
	const bool bCrouchingMultiplierAtTarget = FMath::IsNearlyEqual(CrouchingMultiplier, CrouchingTargetValue, MultiplierNearlyEqualThreshold);

	// 공중에 있는지 (점프/낙하) 확인하고, 그렇다면 부드럽게 페널티 적용
	const bool bIsJumpingOrFalling = (CharMovementComp != nullptr) && CharMovementComp->IsFalling();
	const float JumpFallTargetValue = bIsJumpingOrFalling ? SpreadAngleMultiplier_JumpingOrFalling : 1.0f;
	JumpFallMultiplier = FMath::FInterpTo(JumpFallMultiplier, JumpFallTargetValue, DeltaSeconds, TransitionRate_JumpingOrFalling);
	const bool bJumpFallMultiplerIs1 = FMath::IsNearlyEqual(JumpFallMultiplier, 1.0f, MultiplierNearlyEqualThreshold);

	// 조준 상태인지 확인하고, 카메라 전환 정도에 따라 보너스 적용
	float AimingAlpha = 0.0f;
	if (const ULyraCameraComponent* CameraComponent = ULyraCameraComponent::FindCameraComponent(Pawn))
	{
		float TopCameraWeight;
		FGameplayTag TopCameraTag;
		CameraComponent->GetBlendInfo(/*out*/ TopCameraWeight, /*out*/ TopCameraTag);

		// 태그를 직접 요청하여 사용 (중복 정의 방지)
		static const FGameplayTag SteadyAimingCameraTag = FGameplayTag::RequestGameplayTag("Lyra.Weapon.SteadyAimingCamera");
		AimingAlpha = (TopCameraTag == SteadyAimingCameraTag) ? TopCameraWeight : 0.0f;
	}
	const float AimingMultiplier = FMath::GetMappedRangeValueClamped(
		/*InputRange=*/ FVector2D(0.0f, 1.0f),
		/*OutputRange=*/ FVector2D(1.0f, SpreadAngleMultiplier_Aiming),
		/*Alpha=*/ AimingAlpha);
	const bool bAimingMultiplierAtTarget = FMath::IsNearlyEqual(AimingMultiplier, SpreadAngleMultiplier_Aiming, KINDA_SMALL_NUMBER);

	// 모든 배율 결합
	const float CombinedMultiplier = AimingMultiplier * StandingStillMultiplier * CrouchingMultiplier * JumpFallMultiplier;
	CurrentSpreadAngleMultiplier = CombinedMultiplier;

	// 이 확산 배율들이 최소 확산이 아님을 나타내는 경우 처리 필요
	return bStandingStillMultiplierAtMin && bCrouchingMultiplierAtTarget && bJumpFallMultiplerIs1 && bAimingMultiplierAtTarget;
}
