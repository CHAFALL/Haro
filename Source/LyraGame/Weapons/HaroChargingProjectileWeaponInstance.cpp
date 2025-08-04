// Fill out your copyright notice in the Description page of Project Settings.


#include "HaroChargingProjectileWeaponInstance.h"
#include "Weapons/HaroProjectileBase.h"

UHaroChargingProjectileWeaponInstance::UHaroChargingProjectileWeaponInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 기본 커브 데이터 설정
	TimeToSpeedCurve.EditorCurveData.AddKey(0.0f, ProjectileSpeed);
	TimeToSpeedCurve.EditorCurveData.AddKey(MaxChargingTime, ProjectileSpeed * 2.0f);

	TimeToDamageCurve.EditorCurveData.AddKey(0.0f, DamageMultiplier);
	TimeToDamageCurve.EditorCurveData.AddKey(MaxChargingTime, DamageMultiplier * 3.0f);

	TimeToSizeCurve.EditorCurveData.AddKey(0.0f, SizeMultiplier);
	TimeToSizeCurve.EditorCurveData.AddKey(MaxChargingTime, SizeMultiplier * 10.5f);
}

void UHaroChargingProjectileWeaponInstance::PostLoad()
{
	Super::PostLoad();

#if WITH_EDITOR
	UpdateChargingDebugVisualization();
#endif
}

void UHaroChargingProjectileWeaponInstance::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	UpdateChargingDebugVisualization();
}

void UHaroChargingProjectileWeaponInstance::UpdateChargingDebugVisualization()
{
	Debug_ChargingTime = ChargingTime;
	Debug_CurrentChargeLevel = GetChargeLevel();
}

void UHaroChargingProjectileWeaponInstance::UpdateChargingTime(float TimeHeld)
{
	ChargingTime = FMath::Clamp(TimeHeld, 0.0f, MaxChargingTime);

	UE_LOG(LogTemp, Log, TEXT("Charging time updated: %f"), ChargingTime);

#if WITH_EDITOR
	UpdateChargingDebugVisualization();
#endif
}

float UHaroChargingProjectileWeaponInstance::GetChargeLevel() const
{
	return FMath::Clamp(ChargingTime / MaxChargingTime, 0.0f, 1.0f);
}

float UHaroChargingProjectileWeaponInstance::GetChargedProjectileSpeed() const
{
	// 생성자에서 기본값 설정했으므로 바로 사용
	return TimeToSpeedCurve.GetRichCurveConst()->Eval(ChargingTime);
}

float UHaroChargingProjectileWeaponInstance::GetChargedDamageMultiplier() const
{
	return TimeToDamageCurve.GetRichCurveConst()->Eval(ChargingTime);
}

float UHaroChargingProjectileWeaponInstance::GetChargedSizeMultiplier() const
{
	return TimeToSizeCurve.GetRichCurveConst()->Eval(ChargingTime);
}

void UHaroChargingProjectileWeaponInstance::ConfigureProjectile(AHaroProjectileBase* Projectile) const
{
	if (!Projectile)
		return;

	// 기본 설정 먼저 적용 (부모 클래스)
	Super::ConfigureProjectile(Projectile);

	// 차징된 설정 계산 및 적용
	float ChargedSpeed = GetChargedProjectileSpeed();
	float ChargedSizeMultiplier = GetChargedSizeMultiplier();
	float ChargedDamageMultiplier = GetChargedDamageMultiplier();

	// 투사체에 적용
	Projectile->SetSpeed(ChargedSpeed);
	Projectile->SetActorScale3D(FVector(ChargedSizeMultiplier)); // 콜리전도 같이 커짐.

	// 데미지 처리는 현재 빠져있음.

	UE_LOG(LogTemp, Log, TEXT("Configured charged projectile - ChargingTime: %f, Speed: %f, Size: %f, DamageMultiplier: %f"),
		ChargingTime, ChargedSpeed, ChargedSizeMultiplier, ChargedDamageMultiplier);
}
