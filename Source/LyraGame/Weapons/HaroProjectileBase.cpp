// Fill out your copyright notice in the Description page of Project Settings.


#include "HaroProjectileBase.h"
#include "Components/SphereComponent.h"
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

	//AActor::SetLifeSpan(5.f);

	SphereCollisionComponent = CreateDefaultSubobject<USphereComponent>("SphereCollisionComponent");
	SetRootComponent(SphereCollisionComponent);
	SphereCollisionComponent->SetCollisionProfileName("Projectile");
	SphereCollisionComponent->SetCollisionObjectType(Haro_ObjectChannel_Projectile);
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

void AHaroProjectileBase::HandleComponentHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& HitResult)
{
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


		// 시각적 효과 실행
		if (SourceASC && HitGameplayCueTag.IsValid() && HitResult.bBlockingHit)
		{
			FGameplayCueParameters SourceCueParams;
			SourceCueParams.Location = HitResult.ImpactPoint;
			SourceCueParams.Normal = HitResult.ImpactNormal;
			SourceCueParams.PhysicalMaterial = HitResult.PhysMaterial;
			SourceASC->ExecuteGameplayCue(HitGameplayCueTag, SourceCueParams);
		}

		// 데미지 적용 (아직 Ability와 연동 안됨.)
		if (TargetASC && GetOwner() != OtherActor && GetOwner() != OtherComponent->GetOwner() && GetInstigator() != OtherActor)
		{
			// Lyra의 데미지 GameplayEffect 가져오기
			const TSubclassOf<UGameplayEffect> DamageGEClass = ULyraAssetManager::GetSubclass(ULyraGameData::Get().DamageGameplayEffect_SetByCaller);

			// GameplayEffect Context 생성
			FGameplayEffectContextHandle EffectContextHandle = SourceASC->MakeEffectContext();
			EffectContextHandle.AddHitResult(HitResult);
			EffectContextHandle.AddInstigator(SourceASC->AbilityActorInfo->OwnerActor.Get(), this);

			// GameplayEffect Spec 생성
			FGameplayEffectSpecHandle EffectSpecHandle = SourceASC->MakeOutgoingSpec(DamageGEClass, 1.f, EffectContextHandle);
			
			// 데미지 양 설정
			EffectSpecHandle.Data->SetSetByCallerMagnitude(LyraGameplayTags::SetByCaller_Damage, Damage);
			TargetASC->ApplyGameplayEffectSpecToSelf(*EffectSpecHandle.Data.Get());
		}
	}
}



