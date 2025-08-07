// Fill out your copyright notice in the Description page of Project Settings.


#include "HaroAOEBase.h"
#include "Components/SphereComponent.h"
#include "Character/LyraCharacter.h"
#include "NiagaraComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(HaroAOEBase)

AHaroAOEBase::AHaroAOEBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	// Sphere Collision Component
	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("SphereCollisionComponent"));
	CollisionComponent->SetCollisionProfileName(TEXT("NoCollision"));
	SetRootComponent(CollisionComponent);

	// Niagara Component
	NiagaraComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("NiagaraComponent"));
	NiagaraComponent->SetupAttachment(CollisionComponent);
}

void AHaroAOEBase::SetDamageEffectSpec(const FGameplayEffectSpecHandle& InDamageEffectSpec)
{
	AOEDamageEffectSpecHandle = InDamageEffectSpec;
}

void AHaroAOEBase::BeginPlay()
{
	Super::BeginPlay();

	switch (AOEType)
	{
	case EHaroAOEType::Explosion:
		ExecuteExplosion();
		break;

	case EHaroAOEType::DOTField:
		ExecuteDOTField();
		break;
	}
}

void AHaroAOEBase::ExecuteExplosion()
{

	OnExplosionStarted(); // 시각적 효과 (블루프린트에서)

	if (!HasAuthority()) return;

	// 1. 범위 내 모든 타겟 찾기
	TArray<AActor*> AllTargets = FindTargetsInRange();
	if (AllTargets.Num() == 0)
	{
		OnExplosionCompleted();
		return;
	}

	FVector ExplosionCenter = GetActorLocation();

	// 2. 각 타겟에 대해 개별적으로 처리
	for (AActor* CurrentTarget : AllTargets)
	{
		if (!CurrentTarget || !IsValidTarget(CurrentTarget))
			continue;

		// 3. 시야 확인 (Lyra LineTrace 방식)
		if (bCheckLineOfSight)
		{
			if (!CheckLineOfSight(ExplosionCenter, CurrentTarget, AllTargets))
			{
				continue; // 벽에 막혔으면 데미지 없음
			}
		}

		// 4. 거리별 데미지 계산 및 적용
		float Distance = FVector::Dist(ExplosionCenter, CurrentTarget->GetActorLocation());
		float DamageLevel = CalculateDistanceDamageLevel(Distance);

		if (DamageLevel > 0.0f)
		{
			ApplyDamageToTarget(CurrentTarget, DamageLevel, Distance, ExplosionCenter);
		}
	}

	// 5. 폭발 완료 처리
	OnExplosionCompleted();
}

void AHaroAOEBase::ExecuteDOTField()
{
}

TArray<AActor*> AHaroAOEBase::FindTargetsInRange()
{
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_Pawn));

	TArray<AActor*> IgnoredActors = { this, GetOwner() };
	TArray<AActor*> OverlappingActors;

	UKismetSystemLibrary::SphereOverlapActors(
		GetWorld(),
		GetActorLocation(),
		CollisionComponent->GetScaledSphereRadius(),
		ObjectTypes,
		APawn::StaticClass(), 
		IgnoredActors,
		OverlappingActors
	);

	// 유효한 타겟만 필터링
	TArray<AActor*> ValidTargets;
	for (AActor* Actor : OverlappingActors)
	{
		if (Actor && IsValidTarget(Actor))
		{
			ValidTargets.Add(Actor);
		}
	}

	return ValidTargets;
}



bool AHaroAOEBase::CheckLineOfSight(FVector ExplosionCenter, AActor* TargetActor, const TArray<AActor*>& AllTargets)
{
	if (!TargetActor) return false;

	// 다른 모든 타겟들을 무시 리스트에 추가
	TArray<AActor*> IgnoredActors = AllTargets;
	IgnoredActors.Add(this);
	IgnoredActors.Add(GetOwner());
	IgnoredActors.Remove(TargetActor); // 현재 타겟만 제외

	FHitResult HitResult;
	FVector TargetLocation = TargetActor->GetActorLocation();

	// LineTrace로 시야 확인
	bool bHit = UKismetSystemLibrary::LineTraceSingle(
		GetWorld(),
		ExplosionCenter,
		TargetLocation,
		UEngineTypes::ConvertToTraceType(ECollisionChannel::ECC_Visibility),
		false, // bTraceComplex
		IgnoredActors,
		EDrawDebugTrace::None,
		HitResult,
		true // bIgnoreSelf
	);

	// 충돌이 없거나 충돌한 것이 타겟 자신이면 시야 확보
	return !bHit || (HitResult.GetActor() == TargetActor);
}

float AHaroAOEBase::CalculateDistanceDamageLevel(float Distance)
{
	float MaxRadius = CollisionComponent->GetScaledSphereRadius();

	if (Distance >= MaxRadius) return 0.0f;

	// Distance / Radius -> FClamp(0.1, 1.0)
	float DistanceRatio = Distance / MaxRadius;

	// 최소 10% 데미지 보장
	// 가까우면 0.1, 멀면 1.0
	return FMath::Clamp(DistanceRatio, 0.1f, 1.0f);

}


// 이렇게 많은 매개 변수가 필요할지도 고민이 됨.
void AHaroAOEBase::ApplyDamageToTarget(AActor* Target, float DamageLevel, float Distance, FVector ExplosionCenter)
{
	if (!Target || !AOEDamageEffectSpecHandle.IsValid())
		return;

	UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target);
	if (!TargetASC)
		return;

	// 기존 스펙을 복사하고 레벨만 수정
	FGameplayEffectSpec* OriginalSpec = AOEDamageEffectSpecHandle.Data.Get();
	FGameplayEffectSpec ModifiedSpec(*OriginalSpec); // 스냅샷 복사!
	ModifiedSpec.SetLevel(DamageLevel); // 레벨만 스케일링

	// 수정된 스펙 적용
	TargetASC->ApplyGameplayEffectSpecToSelf(ModifiedSpec);

}

bool AHaroAOEBase::IsValidTarget(AActor* Target) const
{
	if (!Target) return false;
	if (Target == GetOwner()) return false;
	if (!Target->IsA<ALyraCharacter>()) return false;

	return true;
}

void AHaroAOEBase::OnExplosionCompleted()
{
	// 블루프린트
	OnExplosionFinished();
}


