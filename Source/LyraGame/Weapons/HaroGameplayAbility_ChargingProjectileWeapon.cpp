// Fill out your copyright notice in the Description page of Project Settings.


#include "HaroGameplayAbility_ChargingProjectileWeapon.h"
#include "HaroRangedWeaponInstance.h"
#include "LyraLogChannels.h"
#include "Character/LyraCharacter.h"
#include "HaroProjectileBase.h"
#include "LyraGameplayTags.h"
#include "Abilities/Tasks/AbilityTask_WaitInputRelease.h"
#include "AbilitySystemComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(HaroGameplayAbility_ChargingProjectileWeapon)

UHaroGameplayAbility_ChargingProjectileWeapon::UHaroGameplayAbility_ChargingProjectileWeapon(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 태그 넣는 곳.
}

void UHaroGameplayAbility_ChargingProjectileWeapon::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Blue, TEXT("어빌리티 실행!"));
	}

	// 부모 클래스의 ActivateAbility는 호출하지 않음 (즉시 발사를 방지)

	// 1. 타겟 데이터 콜백 등록
	UAbilitySystemComponent* MyAbilityComponent = CurrentActorInfo->AbilitySystemComponent.Get();
	check(MyAbilityComponent);

	// 타겟 데이터가 준비되었을 때 호출되는 멀티캐스트 델리게이트
	// 이게 맞나 싶다. (일단 보류)
	OnTargetDataReadyCallbackDelegateHandle = MyAbilityComponent->AbilityTargetDataSetDelegate(
		CurrentSpecHandle,
		CurrentActivationInfo.GetActivationPredictionKey()
	).AddUObject(this, &ThisClass::OnChargingTargetDataReadyCallback);

	// 2. 무기 발사 시간 업데이트
	if (UHaroRangedWeaponInstance* WeaponData = GetWeaponInstance())
	{
		WeaponData->UpdateFiringTime();
	}

	// 3. 입력 해제 대기 태스크 설정
	InputReleaseTask = UAbilityTask_WaitInputRelease::WaitInputRelease(this, true); // true : 이미 입력이 해제된 상태라면 즉시 콜백 호출
	if (InputReleaseTask)
	{
		InputReleaseTask->OnRelease.AddDynamic(this, &ThisClass::OnInputRelease);
		InputReleaseTask->ReadyForActivation(); // 태스크 활성화
	}

	// 4. 차징 시작 이벤트 (상태 관리 없이 단순 이벤트만) - 애니메이션 같은 처리 (블루프린트에서)
	OnChargingStarted(); // 사실 이것도 없어도 되긴 함. 아래 코드를 보면 거기서 처리해도 되니깐.

	// 5. 블루프린트 로직 실행을 위해 부모의 부모 클래스 호출
	UGameplayAbility::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UHaroGameplayAbility_ChargingProjectileWeapon::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("어빌리티 종료!"));
	}

	// 태스크 정리
	CleanupTasks();

	// 취소된 경우 이벤트 호출 - 어떤 특이한 상황에 끊어진 경우, (없어도 될듯.)
	if (bWasCancelled)
	{
		OnChargingCancelled();
	}

	// WeaponInstance 차징 시간 초기화
	if (UHaroRangedWeaponInstance* WeaponInstance = GetWeaponInstance())
	{
		WeaponInstance->UpdateChargingTime(0.0f);
	}

	// 부모 클래스 EndAbility 호출
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UHaroGameplayAbility_ChargingProjectileWeapon::OnChargingTargetDataReadyCallback(const FGameplayAbilityTargetDataHandle& InData, FGameplayTag ApplicationTag)
{
	OnTargetDataReadyCallback(InData, ApplicationTag);
}

// 좀 더 보완이 가능할 것 같음.
void UHaroGameplayAbility_ChargingProjectileWeapon::ConfigureProjectileDamageEffect(AHaroProjectileBase* Projectile, UHaroRangedWeaponInstance* WeaponInstance, ALyraCharacter* SourceCharacter)
{
	// Super 호출하지 않고 직접 처리
	UAbilitySystemComponent* SourceASC = SourceCharacter->GetAbilitySystemComponent();
	if (SourceASC && DamageEffectClass)
	{
		// 컨텍스트 생성
		FGameplayEffectContextHandle EffectContext = SourceASC->MakeEffectContext();
		EffectContext.SetAbility(this);
		EffectContext.AddSourceObject(WeaponInstance);

		// 새로운 스펙 생성 -> 이때 스냅샷
		FGameplayEffectSpecHandle DamageSpec = SourceASC->MakeOutgoingSpec(
			DamageEffectClass,
			GetAbilityLevel(),
			EffectContext
		);

		if (DamageSpec.IsValid())
		{
			// 차징 배율을 바로 설정
			if (UHaroRangedWeaponInstance* WeaponData = GetWeaponInstance())
			{
				const EHaroFireInputType InputType = GetCurrentFireInputType();

				float ChargeMultiplier = WeaponData->GetChargedDamageMultiplier(InputType);
				DamageSpec.Data->SetSetByCallerMagnitude(
					LyraGameplayTags::SetByCaller_ChargeMultiplier,
					ChargeMultiplier
				);
			}

			// 투사체에 설정
			Projectile->SetDamageEffectSpec(DamageSpec);
		}
	}
}

void UHaroGameplayAbility_ChargingProjectileWeapon::OnInputRelease(float TimeHeld)
{
	// TimeHeld 파라미터가 입력을 얼마나 오래 눌렀는지 나타냄

	UHaroRangedWeaponInstance* WeaponInstance = GetWeaponInstance();
	if (!WeaponInstance)
	{
		K2_EndAbility();
		return;
	}

	// WeaponInstance에 차징 시간 설정
	WeaponInstance->UpdateChargingTime(TimeHeld);

	// (테스트 후 지울 예정)
	// 차징 레벨 가져오기 (WeaponInstance에서 계산)
	//float ChargeLevel = WeaponInstance->GetChargeLevel();


	// 블루프린트 이벤트 호출 (차징 끝날때에 대한.)
	// StartProjectileTargeting();도 블루프린트에서 처리
	OnChargingCompleted();

	/*if (GEngine)
	{
		FString DebugMessage = FString::Printf(TEXT("Charged projectile fired - TimeHeld: %f, ChargeLevel: %f"), TimeHeld, ChargeLevel);
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, DebugMessage);
	}*/
}

void UHaroGameplayAbility_ChargingProjectileWeapon::CleanupTasks()
{
	if (InputReleaseTask)
	{
		InputReleaseTask->EndTask();
		InputReleaseTask = nullptr;
	}
}


