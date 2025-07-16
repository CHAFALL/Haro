// Fill out your copyright notice in the Description page of Project Settings.


#include "HaroGameplayAbility_ProjectileWeapon.h"
#include "HaroProjectileBase.h"
#include "LyraLogChannels.h"
#include "Character/LyraCharacter.h"
#include "Player/LyraPlayerController.h"
#include "HaroProjectileWeaponInstance.h"
#include "AbilitySystemComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(HaroGameplayAbility_ProjectileWeapon)

UHaroGameplayAbility_ProjectileWeapon::UHaroGameplayAbility_ProjectileWeapon(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Base에서 함.
	//SourceBlockedTags.AddTag(FGameplayTag::RequestGameplayTag("Ability.Weapon.NoFiring"));
}

UHaroProjectileWeaponInstance* UHaroGameplayAbility_ProjectileWeapon::GetWeaponInstance() const
{
	return Cast<UHaroProjectileWeaponInstance>(GetAssociatedEquipment());
}

bool UHaroGameplayAbility_ProjectileWeapon::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, OUT FGameplayTagContainer* OptionalRelevantTags) const
{
	bool bResult = Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);

	if (bResult)
	{
		if (GetWeaponInstance() == nullptr)
		{
			UE_LOG(LogLyraAbilitySystem, Error, TEXT("Weapon ability %s cannot be activated because there is no associated ranged weapon (equipment instance=%s but needs to be derived from %s)"),
				*GetPathName(),
				*GetPathNameSafe(GetAssociatedEquipment()),
				*UHaroProjectileWeaponInstance::StaticClass()->GetName());
			bResult = false;
		}
	}

	return bResult;
}

void UHaroGameplayAbility_ProjectileWeapon::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	// 1. 타겟 데이터 콜백 등록
	UAbilitySystemComponent* MyAbilityComponent = CurrentActorInfo->AbilitySystemComponent.Get();
	check(MyAbilityComponent); // MyAbilityComponent 없으면 크래시.

	OnTargetDataReadyCallbackDelegateHandle = MyAbilityComponent->AbilityTargetDataSetDelegate(
		CurrentSpecHandle, // 이 특정 어빌리티 인스턴스
		CurrentActivationInfo.GetActivationPredictionKey() // 어떤 어빌리티의 어떤 실행인지 구분이 필요하므로
	).AddUObject(this, &ThisClass::OnTargetDataReadyCallback);

	// 2. 무기 발사 시간 업데이트
	if (ULyraWeaponInstance* WeaponData = GetWeaponInstance())
	{
		WeaponData->UpdateFiringTime(); // 마지막 발사 시간을 업데이트해 연사 속도 제어
	}

	// 3. Super 호출로 블루프린트 로직 실행 
	// (블루프린트의 K2_ActivateAbility 실행) 
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// 4. 어빌리티는 아직 종료되지 않음! 
	//    블루프린트에서 StartProjectileTargeting() 호출을 기다림

	// (참고) 왜 즉시 투사체를 생성하지 않고 이렇게 복잡하게 해? (아직 투사체 안쏨.)
	// -> 네트워크 예측 때문.
}

void UHaroGameplayAbility_ProjectileWeapon::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (IsEndAbilityValid(Handle, ActorInfo))
	{
		UAbilitySystemComponent* MyAbilityComponent = CurrentActorInfo->AbilitySystemComponent.Get();
		check(MyAbilityComponent);

		// 타겟 데이터 콜백 제거 및 정리 (메모리 누수 방지)
		MyAbilityComponent->AbilityTargetDataSetDelegate(CurrentSpecHandle, CurrentActivationInfo.GetActivationPredictionKey()).Remove(OnTargetDataReadyCallbackDelegateHandle);
		MyAbilityComponent->ConsumeClientReplicatedTargetData(CurrentSpecHandle, CurrentActivationInfo.GetActivationPredictionKey());

		Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
	}
}

void UHaroGameplayAbility_ProjectileWeapon::SpawnProjectile()
{
	// 서버에서만 투사체 스폰
	if (HasAuthority(&CurrentActivationInfo) == false)
		return;

	UHaroProjectileWeaponInstance* ProjectileWeaponInstance = Cast<UHaroProjectileWeaponInstance>(GetWeaponInstance());

	ALyraCharacter* LyraCharacter = GetLyraCharacterFromActorInfo();
	ALyraPlayerController* LyraPlayerController = GetLyraPlayerControllerFromActorInfo();

	if (LyraCharacter && LyraPlayerController)
	{

		// 무기의 소켓(머즐) 위치 가져오기 
		FTransform MuzzleTransform = ProjectileWeaponInstance->GetMuzzleTransform();

		// 타겟팅 트랜스폼 가져오기 (Base 함수)
		FTransform TargetingTransform = GetTargetingTransform(LyraCharacter, TargetingSource);

		// 스폰 위치 설정
		FTransform SpawnTransform;
		SpawnTransform.SetLocation(MuzzleTransform.GetLocation());
		SpawnTransform.SetRotation(TargetingTransform.GetRotation());
		
		// 무기 인스턴스에서 직접 무기 액터 가져오기
		AActor* WeaponActor = ProjectileWeaponInstance->GetPrimaryActor();
		
		// 스폰 파라미터 설정
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnParams.Owner = WeaponActor ? WeaponActor : LyraCharacter;
		SpawnParams.Instigator = LyraCharacter;

		// 투사체 스폰
		GetWorld()->SpawnActor<AHaroProjectileBase>(ProjectileClass, SpawnTransform, SpawnParams);

		// Todo -> 스폰한 투사체에 데미지 적용. 투사체는 투사체 스폰 부분에서 반환 받을 수 있음.
	}
}

void UHaroGameplayAbility_ProjectileWeapon::SpawnProjectileFromTargetData(const FGameplayAbilityTargetDataHandle& TargetData)
{
	// 서버에서만 투사체 스폰
	if (!HasAuthority(&CurrentActivationInfo))
		return;

	UHaroProjectileWeaponInstance* ProjectileWeaponInstance = Cast<UHaroProjectileWeaponInstance>(GetWeaponInstance());
	ALyraCharacter* LyraCharacter = GetLyraCharacterFromActorInfo();

	if (!LyraCharacter || !ProjectileWeaponInstance || !ProjectileClass)
		return;

	// 타겟 데이터에서 발사 정보 추출
	if (TargetData.Num() > 0)
	{
		if (const FGameplayAbilityTargetData_LocationInfo* LocationData =
			static_cast<const FGameplayAbilityTargetData_LocationInfo*>(TargetData.Get(0)))
		{
			// 클라이언트에서 계산된 발사 Transform 가져오기
			FTransform LaunchTransform = LocationData->SourceLocation.LiteralTransform;

			// 서버 검증: 클라이언트 데이터가 유효한가? (치팅 방지)
			if (IsValidLaunchTransform(LaunchTransform, LyraCharacter))
			{
				// 무기 액터 가져오기
				AActor* WeaponActor = ProjectileWeaponInstance->GetPrimaryActor();

				// 스폰 파라미터 설정
				FActorSpawnParameters SpawnParams;
				SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				SpawnParams.Owner = WeaponActor ? WeaponActor : LyraCharacter;
				SpawnParams.Instigator = LyraCharacter;

				// 실제 투사체 스폰 (클라이언트가 보낸 정보 사용!)
				if (AHaroProjectileBase* SpawnedProjectile = GetWorld()->SpawnActor<AHaroProjectileBase>(ProjectileClass, LaunchTransform, SpawnParams))
				{
					// 투사체 초기화
					OnProjectileSpawned(SpawnedProjectile);

					UE_LOG(LogTemp, Log, TEXT("Projectile spawned at: %s"), *LaunchTransform.GetLocation().ToString());
				}
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("Server rejected invalid launch transform from client"));
				// 치팅 방지: 클라이언트가 이상한 데이터를 보냈음
			}
		}
	}
}

// 투사체 스폰 후 호출되는 함수 (추가 설정용)
void UHaroGameplayAbility_ProjectileWeapon::OnProjectileSpawned(AHaroProjectileBase* SpawnedProjectile)
{
	// 여기서 투사체에 추가 설정 적용 가능
	// 예: 속도 조정 등

	// 데미지는 적중 시점에 계산이 되어야 한다고 함
	// 거리 감쇠, 재질 배율 등

	// 블루프린트에서 오버라이드할 수 있도록 이벤트 호출
	OnProjectileSpawnedBP(SpawnedProjectile);
}

bool UHaroGameplayAbility_ProjectileWeapon::IsValidLaunchTransform(const FTransform& LaunchTransform, APawn* SourcePawn)
{
	// 1. 거리 검증: 플레이어 근처에서 발사되는가?
	float DistanceFromPlayer = FVector::Dist(LaunchTransform.GetLocation(), SourcePawn->GetActorLocation());
	if (DistanceFromPlayer > 300.0f) // 3미터 이상 떨어진 곳에서 발사? 의심스러움
	{
		UE_LOG(LogTemp, Warning, TEXT("Launch position too far from player: %f"), DistanceFromPlayer);
		return false;
	}

	// 2. 방향 검증: 플레이어가 실제로 볼 수 있는 방향인가?
	FVector PlayerForward = SourcePawn->GetActorForwardVector();
	FVector LaunchForward = LaunchTransform.GetUnitAxis(EAxis::X);
	float DotProduct = FVector::DotProduct(PlayerForward, LaunchForward);

	if (DotProduct < -0.7f) // 거의 뒤쪽으로 쏘려고 함? 의심스러움
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid launch direction: %f"), DotProduct);
		return false;
	}

	return true;
}

void UHaroGameplayAbility_ProjectileWeapon::OnTargetDataReadyCallback(const FGameplayAbilityTargetDataHandle& InData, FGameplayTag ApplicationTag)
{
	UAbilitySystemComponent* MyAbilityComponent = CurrentActorInfo->AbilitySystemComponent.Get();
	check(MyAbilityComponent);

	if (const FGameplayAbilitySpec* AbilitySpec = MyAbilityComponent->FindAbilitySpecFromHandle(CurrentSpecHandle))
	{
		FScopedPredictionWindow ScopedPrediction(MyAbilityComponent);

		// 타겟 데이터 소유권 가져오기 (네트워크 안전성을 위해)
		// 이동을 위해 임시로 const 해제
		FGameplayAbilityTargetDataHandle LocalTargetDataHandle(MoveTemp(const_cast<FGameplayAbilityTargetDataHandle&>(InData)));

		// 클라이언트면 서버에 타겟 데이터 전송
		const bool bShouldNotifyServer = CurrentActorInfo->IsLocallyControlled() && !CurrentActorInfo->IsNetAuthority();
		if (bShouldNotifyServer)
		{
			MyAbilityComponent->CallServerSetReplicatedTargetData(
				CurrentSpecHandle,
				CurrentActivationInfo.GetActivationPredictionKey(),
				LocalTargetDataHandle,
				ApplicationTag,
				MyAbilityComponent->ScopedPredictionKey
			);
		}

		// 기본 검증
		const bool bIsTargetDataValid = true; // 그냥 바로 true 해버림.

		// 투사체는 히트마커 처리 안함 (히트스캔과 다른 점) - ProjectileBase에서 함.

		// 탄약 소모 및 쿨다운 적용 (커밋)
		if (bIsTargetDataValid && CommitAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo))
		{
			// 실제 투사체 스폰 (서버에서만)
			SpawnProjectileFromTargetData(LocalTargetDataHandle);

			// 블루프린트에서 추가 로직 처리 가능 (발사 이펙트, 사운드 등)
			OnProjectileTargetDataReady(LocalTargetDataHandle);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Projectile weapon ability failed to commit"));
			K2_EndAbility();
		}
	}

	// 타겟 데이터 소비 (네트워크 정리)
	// 네트워크 버퍼에서 처리된 타겟 데이터 제거 (메모리 누수 방지)
	MyAbilityComponent->ConsumeClientReplicatedTargetData(CurrentSpecHandle, CurrentActivationInfo.GetActivationPredictionKey());
}

void UHaroGameplayAbility_ProjectileWeapon::StartProjectileTargeting()
{
	// 1. 기본 체크들
	check(CurrentActorInfo);
	AActor* AvatarActor = CurrentActorInfo->AvatarActor.Get(); // 실제 캐릭터 액터
	check(AvatarActor);

	UAbilitySystemComponent* MyAbilityComponent = CurrentActorInfo->AbilitySystemComponent.Get();
	check(MyAbilityComponent);

	AController* Controller = GetControllerFromActorInfo();
	check(Controller);

	// 2. 네트워크 예측 윈도우 
	// 클라이언트가 미리 예측해서 즉시 반응
	// 서버가 나중에 검증해서 맞으면 그대로, 틀리면 교정
	FScopedPredictionWindow ScopedPrediction(MyAbilityComponent, CurrentActivationInfo.GetActivationPredictionKey());

	// 3. 투사체 발사 정보 계산
	// /out/은 뭐야? -> 함수가 결과를 돌려주는 방식 (LaunchTransform에 계산된 발사 정보(결과값)가 들어감)
	FTransform LaunchTransform;
	if (!CalculateProjectileLaunchTransform(AvatarActor, Controller, /*out*/ LaunchTransform))
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to calculate projectile launch transform"));
		K2_EndAbility();
		return;
	}

	// 4. 타겟 데이터 생성 (히트스캔은 HitResult -> 투사체는 Transform!)
	FGameplayAbilityTargetDataHandle TargetData;

	// 5. 위치 기반 타겟 데이터 생성
	FGameplayAbilityTargetData_LocationInfo* NewTargetData = new FGameplayAbilityTargetData_LocationInfo();

	// 6. 발사 위치 설정
	NewTargetData->SourceLocation.LocationType = EGameplayAbilityTargetingLocationType::LiteralTransform;
	NewTargetData->SourceLocation.LiteralTransform = LaunchTransform;

	// 7. 목표 방향 설정 (발사 Transform과 동일) : 필요없을 듯 -> 나중에 필요시 살릴 것.
	/*NewTargetData->TargetLocation.LocationType = EGameplayAbilityTargetingLocationType::LiteralTransform;
	NewTargetData->TargetLocation.LiteralTransform = LaunchTransform;*/

	// 8. 타겟 데이터에 추가
	TargetData.Add(NewTargetData);

	// 9. 투사체는 히트 마커 여기서 설정 안함. 맞았는지 지금 알 수 없으므로
	// 한다면 AHaroProjectileBase에서 할 것!

	// 10. 타겟 데이터 즉시 처리 (Lyra와 동일!) -> 나중에 보완 필요.
	OnTargetDataReadyCallback(TargetData, FGameplayTag());

	// 11. 디버그 로그
	UE_LOG(LogTemp, Log, TEXT("Projectile targeting completed - Launch Location: %s, Forward: %s"),
		*LaunchTransform.GetLocation().ToString(),
		*LaunchTransform.GetRotation().GetForwardVector().ToString());
}

bool UHaroGameplayAbility_ProjectileWeapon::CalculateProjectileLaunchTransform(AActor* AvatarActor, AController* Controller, FTransform& OutLaunchTransform)
{
	if (!AvatarActor || !Controller)
		return false;

	APawn* SourcePawn = Cast<APawn>(AvatarActor);
	if (!SourcePawn)
		return false;

	UHaroProjectileWeaponInstance* ProjectileWeaponInstance = Cast<UHaroProjectileWeaponInstance>(GetWeaponInstance());
	if (!ProjectileWeaponInstance)
		return false;

	// GetTargetingTransform을 사용해서 타겟팅 트랜스폼 계산
	FTransform TargetingTransform = GetTargetingTransform(SourcePawn, TargetingSource);

	// 무기의 머즐 위치 가져오기
	FTransform MuzzleTransform = ProjectileWeaponInstance->GetMuzzleTransform();

	// 발사 트랜스폼 설정
	OutLaunchTransform.SetLocation(MuzzleTransform.GetLocation());
	OutLaunchTransform.SetRotation(TargetingTransform.GetRotation());
	OutLaunchTransform.SetScale3D(FVector::OneVector);

	return true;
}
