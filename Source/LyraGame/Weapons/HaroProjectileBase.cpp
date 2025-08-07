// Fill out your copyright notice in the Description page of Project Settings.


#include "HaroProjectileBase.h"
#include "Components/SphereComponent.h"
#include "HaroAOEBase.h"
#include "Physics/LyraCollisionChannels.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "System/LyraAssetManager.h"
#include "System/LyraGameData.h"
#include "LyraGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"



#include UE_INLINE_GENERATED_CPP_BY_NAME(HaroProjectileBase)

AHaroProjectileBase::AHaroProjectileBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;
	AActor::SetReplicateMovement(true);	// 움직임 네트워크로 복제
	bNetLoadOnClient = false;
	bReplicates = true;	// 엑터 자체를 네트워크로 복제

	SphereCollisionComponent = CreateDefaultSubobject<USphereComponent>("SphereCollisionComponent");
	SetRootComponent(SphereCollisionComponent);
	SphereCollisionComponent->SetCollisionProfileName("Projectile");
	SphereCollisionComponent->bReturnMaterialOnMove = true;	// 이동 시 물리 재질 정보 반환
	SphereCollisionComponent->SetCanEverAffectNavigation(false); // 네비게이션 영향 x

	ProjectileMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>("ProjectileMeshComponent");
	ProjectileMeshComponent->SetCollisionProfileName("NoCollision"); // 충돌 없음 (시각적 용도만)
	ProjectileMeshComponent->SetupAttachment(SphereCollisionComponent);
	ProjectileMeshComponent->SetCanEverAffectNavigation(false); 

	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>("ProjectileMovementComponent");
	ProjectileMovementComponent->bRotationFollowsVelocity = true; // 속도 방향으로 회전
	ProjectileMovementComponent->bInterpMovement = true; // 부드러운 이동 보간
	ProjectileMovementComponent->bInterpRotation = true; // 부드러운 회전 보간
}

void AHaroProjectileBase::BeginPlay()
{
	Super::BeginPlay();
	
	SphereCollisionComponent->IgnoreActorWhenMoving(GetInstigator(), true); 

	switch (CollisionDetectionType)
	{
	case ECollisionDetectionType::Hit:
		SphereCollisionComponent->SetGenerateOverlapEvents(false);
		SphereCollisionComponent->OnComponentHit.AddUniqueDynamic(this, &ThisClass::HandleComponentHit);
		break;
	case ECollisionDetectionType::Overlap:
		SphereCollisionComponent->SetGenerateOverlapEvents(true);
		SphereCollisionComponent->OnComponentBeginOverlap.AddUniqueDynamic(this, &ThisClass::HandleComponentOverlap);
		break;
	}
}

void AHaroProjectileBase::Destroyed()
{
	if (HasAuthority() && bAttachToHitComponent && AttachingComponent.IsValid())
	{
		// 적이 죽을 시 박힌 투사체도 같이 사라지도록.
		AttachingComponent->OnComponentDeactivated.RemoveDynamic(this, &ThisClass::HandleOtherComponentDeactivated);
		AttachingComponent = nullptr; // 약한 참조 초기화
	}

	Super::Destroyed();
}

void AHaroProjectileBase::SetSpeed(float Speed)
{
	ProjectileMovementComponent->Velocity = ProjectileMovementComponent->Velocity.GetSafeNormal() * Speed;
}

void AHaroProjectileBase::SetGravityScale(float NewGravityScale)
{
	ProjectileMovementComponent->ProjectileGravityScale = NewGravityScale;
}

void AHaroProjectileBase::SetDamageEffectSpec(const FGameplayEffectSpecHandle& InDamageSpec)
{
	DamageEffectSpecHandle = InDamageSpec;
}

void AHaroProjectileBase::SetAOEClass(TSubclassOf<AHaroAOEBase> InAOEClass)
{
	AOEClass = InAOEClass;
}

void AHaroProjectileBase::SetAOEDamageSpec(const FGameplayEffectSpecHandle& InDamageSpec)
{
	AOEDamageEffectSpecHandle = InDamageSpec;
}

void AHaroProjectileBase::HandleComponentHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& HitResult)
{

	// 자기 자신과의 충돌은 무시
	if (OtherActor == GetInstigator())
	{
		return; 
	}

	if (HitActors.Num() > 0)
		return;

	HitActors.Add(OtherActor);

	SphereCollisionComponent->Deactivate();
	ProjectileMovementComponent->Deactivate();

	if (HasAuthority())
	{
		// 박히는 경우
		if (bAttachToHitComponent)
		{
			AttachingComponent = OtherComponent;
			OtherComponent->OnComponentDeactivated.AddUniqueDynamic(this, &ThisClass::HandleOtherComponentDeactivated);
			AttachToComponent(OtherComponent, FAttachmentTransformRules::KeepWorldTransform, HitResult.BoneName);
		}
		else
		{
			SetLifeSpan(2.f);
		}
	}

	HandleCollisionDetection(OtherActor, OtherComponent, HitResult);
}

void AHaroProjectileBase::HandleComponentOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{

	// 자기 자신과의 충돌은 무시
	if (OtherActor == GetInstigator())
	{
		return;
	}

	// 왜 얘는 Contains로? (ex. 적의 머리, 몸통, 다리 다 맞았다고 하면 폭딜 들어감.)
	if (HitActors.Contains(OtherActor))
		return;

	HitActors.Add(OtherActor);

	FHitResult HitResult = SweepResult; // 진짜 충돌했는지, 그냥 겹쳤는지를 파악하기 위함.
	HitResult.bBlockingHit = bFromSweep; 
	HandleCollisionDetection(OtherActor, OtherComponent, HitResult);
}

void AHaroProjectileBase::HandleOtherComponentDeactivated(UActorComponent* OtherComponent)
{
	if (HasAuthority())
	{
		Destroy();
	}
}

// 실제 충돌 처리
void AHaroProjectileBase::HandleCollisionDetection(AActor* OtherActor, UPrimitiveComponent* OtherComponent, const FHitResult& HitResult)
{
	if (OtherActor == nullptr || OtherComponent == nullptr)
		return;

	if (HasAuthority())
	{
		UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OtherActor);
		UAbilitySystemComponent* SourceASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetInstigator());


		// 시각적 효과 실행 - 여기도 나중에 블루프린트로 뺄 것임.
		if (SourceASC && HitGameplayCueTag.IsValid() && HitResult.bBlockingHit)
		{
			FGameplayCueParameters SourceCueParams;
			SourceCueParams.Location = HitResult.ImpactPoint;
			SourceCueParams.Normal = HitResult.ImpactNormal;
			SourceCueParams.PhysicalMaterial = HitResult.PhysMaterial;
			SourceASC->ExecuteGameplayCue(HitGameplayCueTag, SourceCueParams);
		}

		// 데미지 적용 (아직 Ability와 연동 안됨.)
		//if (TargetASC && GetOwner() != OtherActor && GetOwner() != OtherComponent->GetOwner() && GetInstigator() != OtherActor)
		//{
		//	// Lyra의 데미지 GameplayEffect 가져오기
		//	const TSubclassOf<UGameplayEffect> DamageGEClass = ULyraAssetManager::GetSubclass(ULyraGameData::Get().DamageGameplayEffect_SetByCaller);

		//	// GameplayEffect Context 생성
		//	FGameplayEffectContextHandle EffectContextHandle = SourceASC->MakeEffectContext();
		//	EffectContextHandle.AddHitResult(HitResult);
		//	EffectContextHandle.AddInstigator(SourceASC->AbilityActorInfo->OwnerActor.Get(), this);

		//	// GameplayEffect Spec 생성
		//	FGameplayEffectSpecHandle EffectSpecHandle = SourceASC->MakeOutgoingSpec(DamageGEClass, 1.f, EffectContextHandle);
		//	
		//	// 데미지 양 설정
		//	EffectSpecHandle.Data->SetSetByCallerMagnitude(LyraGameplayTags::SetByCaller_Damage, Damage);
		//	TargetASC->ApplyGameplayEffectSpecToSelf(*EffectSpecHandle.Data.Get());
		//}

		// 투사체 데미지 적용.
		ApplyDamageToTarget(OtherActor, HitResult);

		// AOE 스폰 (새로 추가)
		if (AOEClass)
		{
			SpawnAOEOnHit(HitResult);
		}
	}
}

// 스냅샷을 true로 해뒀기 때문에 스폰 시점 기준으로 스텟이 적용이 됨.
void AHaroProjectileBase::ApplyDamageToTarget(AActor* TargetActor, const FHitResult& HitResult)
{
	// DamageEffectSpecHandle이 유효한지 확인
	if (!DamageEffectSpecHandle.IsValid()) return;

	FGameplayEffectContext* Context = DamageEffectSpecHandle.Data->GetContext().Get();
	if (Context)
	{
		Context->AddHitResult(HitResult);  // 충돌 정보 추가
		// AddInstigator도 해줘야 할까? // 내가 알기로는 MakeEffectContext할때 함수 안에 AddInstigator도 같이 있는 것으로 암.
		// AddInstigator(instigator, effectCauser) 여서 effectCauser를 this로 하면 투사체로 설정이 되어 데미지가 들어왔던 것임.
	}

	// 타겟의 AbilitySystemComponent 찾기
	UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(TargetActor);
	if (!TargetASC) return;

	// GameplayEffect 적용
	FActiveGameplayEffectHandle EffectHandle = TargetASC->ApplyGameplayEffectSpecToSelf(
		*DamageEffectSpecHandle.Data.Get()
	);
}

void AHaroProjectileBase::SpawnAOEOnHit(const FHitResult& HitResult)
{
	if (!AOEClass) return;

	// 서버에서만 AOE 스폰
	if (!HasAuthority()) return;

	// AOE 스폰 위치 결정
	FVector AOELocation = HitResult.ImpactPoint;

	// AOE 타입에 따른 위치 조정 (필요시)
	
	FTransform SpawnTransform(FRotator::ZeroRotator, AOELocation, FVector::OneVector);

	AHaroAOEBase* SpawnedAOE = GetWorld()->SpawnActorDeferred<AHaroAOEBase>(
		AOEClass,
		SpawnTransform,
		GetOwner(),           // 무기 액터
		GetInstigator(),      // 플레이어 캐릭터
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn
	);

	if (SpawnedAOE)
	{
		// BeginPlay 호출 전에 데미지 스펙 설정
		SpawnedAOE->SetDamageEffectSpec(AOEDamageEffectSpecHandle);

		// 실제 스폰 완료
		SpawnedAOE->FinishSpawning(SpawnTransform);

		UE_LOG(LogTemp, Log, TEXT("AOE spawned at: %s by projectile: %s"),
			*AOELocation.ToString(),
			*GetClass()->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to spawn deferred AOE class: %s"),
			AOEClass ? *AOEClass->GetName() : TEXT("NULL"));
	}
}



