// Fill out your copyright notice in the Description page of Project Settings.


#include "HaroRangedWeaponInstance.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/LyraCameraComponent.h"
#include "Physics/PhysicalMaterialWithTags.h"
#include "Weapons/HaroProjectileBase.h"
#include "Weapons/LyraWeaponInstance.h" // 이건 라이라의 실수일까???

#include UE_INLINE_GENERATED_CPP_BY_NAME(HaroRangedWeaponInstance)

UHaroRangedWeaponInstance::UHaroRangedWeaponInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 기본 열 설정 (확산 시스템용)
	HeatToHeatPerShotCurve.EditorCurveData.AddKey(0.0f, 1.0f);
	HeatToCoolDownPerSecondCurve.EditorCurveData.AddKey(0.0f, 2.0f);
}

void UHaroRangedWeaponInstance::PostLoad()
{
	Super::PostLoad();

#if WITH_EDITOR
	UpdateDebugVisualization();
#endif
}

#if WITH_EDITOR
void UHaroRangedWeaponInstance::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	UpdateDebugVisualization();
}

void UHaroRangedWeaponInstance::UpdateDebugVisualization()
{
	ComputeHeatRange(/*out*/ Debug_MinHeat, /*out*/ Debug_MaxHeat);
	ComputeSpreadRange(/*out*/ Debug_MinSpreadAngle, /*out*/ Debug_MaxSpreadAngle);
	Debug_CurrentHeat = CurrentHeat;
	Debug_CurrentSpreadAngle = CurrentSpreadAngle;
	Debug_CurrentSpreadAngleMultiplier = CurrentSpreadAngleMultiplier;
}
#endif

void UHaroRangedWeaponInstance::ConfigureProjectileForInput(AHaroProjectileBase* Projectile, EHaroFireInputType InputType) const
{
	if (const FHaroFireModeConfig* Mode = GetFireModeForInput(InputType))
	{
		if (!Projectile || Mode->FireType != EHaroWeaponFireType::Projectile)
		{
			return;
		}

		// 성능 개선을 위해 Getter 함수를 따로 사용하지 않음.
		const FHaroProjectileFireConfig& Config = Mode->ProjectileConfig;
		const bool bHasCharging = Config.MaxChargingTime > 0.0f;

		const float BaseSpeed = Config.ProjectileSpeed;
		const float FinalSpeed = (bHasCharging && Config.TimeToSpeedCurve.GetRichCurveConst()->HasAnyData())
			? BaseSpeed * Config.TimeToSpeedCurve.GetRichCurveConst()->Eval(ChargingTime)
			: BaseSpeed;

		const float FinalSizeMultiplier = (bHasCharging && Config.TimeToSizeCurve.GetRichCurveConst()->HasAnyData())
			? Config.TimeToSizeCurve.GetRichCurveConst()->Eval(ChargingTime)
			: Config.SizeMultiplier;

		// 투사체에 설정 적용
		Projectile->SetSpeed(FinalSpeed);
		Projectile->SetLifeSpan(Config.ProjectileLifespan);
		Projectile->SetGravityScale(Config.ProjectileGravityScale);
		Projectile->SetActorScale3D(FVector(FinalSizeMultiplier));

		// 데미지 처리는 나중에 GameplayEffect를 통해 처리
		UE_LOG(LogTemp, Log, TEXT("Configured projectile - Speed: %f, Size: %f"),
			FinalSpeed, FinalSizeMultiplier);
	}
}

// TODO : 나중에 히트스캔도 차징이 생기면 수정 필요.
float UHaroRangedWeaponInstance::GetChargedDamageMultiplier(EHaroFireInputType InputType) const
{
	if (const FHaroFireModeConfig* Mode = GetFireModeForInput(InputType))
	{
		if (Mode->FireType == EHaroWeaponFireType::Projectile && Mode->ProjectileConfig.MaxChargingTime > 0.0f)
		{
			if (Mode->ProjectileConfig.TimeToDamageCurve.GetRichCurveConst()->HasAnyData())
			{
				return Mode->ProjectileConfig.TimeToDamageCurve.GetRichCurveConst()->Eval(ChargingTime);
			}
		}
		return Mode->ProjectileConfig.DamageMultiplier; // 기본값
	}
	return 1.0f;
}

int32 UHaroRangedWeaponInstance::GetBulletsPerCartridge(EHaroFireInputType InputType) const
{
	if (const FHaroFireModeConfig* Mode = GetFireModeForInput(InputType))
	{
		if (Mode->FireType == EHaroWeaponFireType::Hitscan)
		{
			return Mode->HitscanConfig.BulletsPerCartridge;
		}
	}
	return 1; // 기본값
}

float UHaroRangedWeaponInstance::GetMaxDamageRange(EHaroFireInputType InputType) const
{
	if (const FHaroFireModeConfig* Mode = GetFireModeForInput(InputType))
	{
		if (Mode->FireType == EHaroWeaponFireType::Hitscan)
		{
			return Mode->HitscanConfig.MaxDamageRange;
		}
	}
	return 25000.0f; // 기본값
}

float UHaroRangedWeaponInstance::GetBulletTraceSweepRadius(EHaroFireInputType InputType) const
{
	if (const FHaroFireModeConfig* Mode = GetFireModeForInput(InputType))
	{
		if (Mode->FireType == EHaroWeaponFireType::Hitscan)
		{
			return Mode->HitscanConfig.BulletTraceSweepRadius;
		}
	}
	return 0.0f; // 기본값
}

int32 UHaroRangedWeaponInstance::GetProjectilesPerCartridge(EHaroFireInputType InputType) const
{
	if (const FHaroFireModeConfig* Mode = GetFireModeForInput(InputType))
	{
		if (Mode->FireType == EHaroWeaponFireType::Projectile)
		{
			return Mode->ProjectileConfig.ProjectilesPerCartridge;
		}
	}
	return 1; // 기본값
}

float UHaroRangedWeaponInstance::GetProjectileSpeed(EHaroFireInputType InputType) const
{
	if (const FHaroFireModeConfig* Mode = GetFireModeForInput(InputType))
	{
		if (Mode->FireType == EHaroWeaponFireType::Projectile)
		{
			const FHaroProjectileFireConfig& Config = Mode->ProjectileConfig;

			// 기본 속도에서 시작
			float BaseSpeed = Config.ProjectileSpeed;

			// 차징 기능이 있고 차징 시간이 설정되어 있으면 배수 적용
			if (Config.MaxChargingTime > 0.0f && Config.TimeToSpeedCurve.GetRichCurveConst()->HasAnyData())
			{
				float SpeedMultiplier = Config.TimeToSpeedCurve.GetRichCurveConst()->Eval(ChargingTime);
				return BaseSpeed * SpeedMultiplier;
			}

			return BaseSpeed;
		}
	}
	return 1000.0f; // 기본값
}

float UHaroRangedWeaponInstance::GetProjectileSizeMultiplier(EHaroFireInputType InputType) const
{
	if (const FHaroFireModeConfig* Mode = GetFireModeForInput(InputType))
	{
		if (Mode->FireType == EHaroWeaponFireType::Projectile)
		{
			const FHaroProjectileFireConfig& Config = Mode->ProjectileConfig;
			// 차징 기능이 있고 차징 시간이 설정되어 있으면 차징된 크기 배율 반환
			if (Config.MaxChargingTime > 0.0f && Config.TimeToSizeCurve.GetRichCurveConst()->HasAnyData())
			{
				return Config.TimeToSizeCurve.GetRichCurveConst()->Eval(ChargingTime);
			}
			// 일반 크기 배율 반환
			return Config.SizeMultiplier;
		}
	}
	return 1.0f; // 기본값
}

float UHaroRangedWeaponInstance::GetProjectileGravityScale(EHaroFireInputType InputType) const
{
	if (const FHaroFireModeConfig* Mode = GetFireModeForInput(InputType))
	{
		if (Mode->FireType == EHaroWeaponFireType::Projectile)
		{
			return Mode->ProjectileConfig.ProjectileGravityScale;
		}
	}
	return 1.0f; // 기본값
}

float UHaroRangedWeaponInstance::GetProjectileLifespan(EHaroFireInputType InputType) const
{
	if (const FHaroFireModeConfig* Mode = GetFireModeForInput(InputType))
	{
		if (Mode->FireType == EHaroWeaponFireType::Projectile)
		{
			return Mode->ProjectileConfig.ProjectileLifespan;
		}
	}
	return 10.0f; // 기본값
}

void UHaroRangedWeaponInstance::OnEquipped()
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

void UHaroRangedWeaponInstance::OnUnequipped()
{
	Super::OnUnequipped();
}

void UHaroRangedWeaponInstance::Tick(float DeltaSeconds)
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

void UHaroRangedWeaponInstance::ComputeHeatRange(float& MinHeat, float& MaxHeat)
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

void UHaroRangedWeaponInstance::ComputeSpreadRange(float& MinSpread, float& MaxSpread)
{
	HeatToSpreadCurve.GetRichCurveConst()->GetValueRange(/*out*/ MinSpread, /*out*/ MaxSpread);
}

void UHaroRangedWeaponInstance::AddSpread()
{
	// 열 증가 커브 샘플링
	const float HeatPerShot = HeatToHeatPerShotCurve.GetRichCurveConst()->Eval(CurrentHeat);
	CurrentHeat = ClampHeat(CurrentHeat + HeatPerShot);

	// 열을 확산각도로 매핑
	CurrentSpreadAngle = HeatToSpreadCurve.GetRichCurveConst()->Eval(CurrentHeat);

#if WITH_EDITOR
	UpdateDebugVisualization();
#endif
}

float UHaroRangedWeaponInstance::GetDistanceAttenuation(float Distance, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags) const
{

	// SourceTags에서 설정 값을 찾는 식으로 해서 개선 가능.

	// Primary 모드의 설정을 사용 (향후 개선 가능)
	if (const FHaroFireModeConfig* PrimaryMode = GetFireModeForInput(EHaroFireInputType::Primary))
	{
		const FRuntimeFloatCurve* Curve = nullptr;
		if (PrimaryMode->FireType == EHaroWeaponFireType::Hitscan)
		{
			Curve = &PrimaryMode->HitscanConfig.DistanceDamageFalloff;
		}
		else if (PrimaryMode->FireType == EHaroWeaponFireType::Projectile)
		{
			Curve = &PrimaryMode->ProjectileConfig.DistanceDamageFalloff;
		}

		if (Curve)
		{
			const FRichCurve* RichCurve = Curve->GetRichCurveConst();
			return RichCurve->HasAnyData() ? RichCurve->Eval(Distance) : 1.0f;
		}
	}
	return 1.0f;
}

float UHaroRangedWeaponInstance::GetPhysicalMaterialAttenuation(const UPhysicalMaterial* PhysicalMaterial, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags) const
{
	float CombinedMultiplier = 1.0f;

	if (const UPhysicalMaterialWithTags* PhysMatWithTags = Cast<const UPhysicalMaterialWithTags>(PhysicalMaterial))
	{
		// Primary 모드의 설정을 사용 (향후 개선 가능)
		if (const FHaroFireModeConfig* PrimaryMode = GetFireModeForInput(EHaroFireInputType::Primary))
		{
			const TMap<FGameplayTag, float>* MaterialDamageMultiplier = nullptr;
			if (PrimaryMode->FireType == EHaroWeaponFireType::Hitscan)
			{
				MaterialDamageMultiplier = &PrimaryMode->HitscanConfig.MaterialDamageMultiplier;
			}
			else if (PrimaryMode->FireType == EHaroWeaponFireType::Projectile)
			{
				MaterialDamageMultiplier = &PrimaryMode->ProjectileConfig.MaterialDamageMultiplier;
			}

			if (MaterialDamageMultiplier)
			{
				for (const FGameplayTag MaterialTag : PhysMatWithTags->Tags)
				{
					if (const float* pTagMultiplier = MaterialDamageMultiplier->Find(MaterialTag))
					{
						CombinedMultiplier *= *pTagMultiplier;
					}
				}
			}
		}
	}

	return CombinedMultiplier;
}

bool UHaroRangedWeaponInstance::UpdateSpread(float DeltaSeconds)
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

bool UHaroRangedWeaponInstance::UpdateMultipliers(float DeltaSeconds)
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

	return bStandingStillMultiplierAtMin && bCrouchingMultiplierAtTarget && bJumpFallMultiplerIs1 && bAimingMultiplierAtTarget;
}
