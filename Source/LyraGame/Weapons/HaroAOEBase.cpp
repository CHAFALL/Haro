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

void AHaroAOEBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 모든 남은 DOT 효과들 제거
	if (!ActiveEffectHandles.IsEmpty())
	{
		TArray<FActiveGameplayEffectHandle> HandlesToRemove;
		for (auto HandlePair : ActiveEffectHandles)
		{
			if (UAbilitySystemComponent* ASC = HandlePair.Value)
			{
				if (IsValid(ASC))
				{
					ASC->RemoveActiveGameplayEffect(HandlePair.Key, 1);
				}
			}
			HandlesToRemove.Add(HandlePair.Key);
		}

		for (auto& Handle : HandlesToRemove)
		{
			ActiveEffectHandles.FindAndRemoveChecked(Handle);
		}
	}

	Super::EndPlay(EndPlayReason);
}

void AHaroAOEBase::ExecuteExplosion()
{

	OnAOEStarted(); // 시각적 효과 (블루프린트에서)

	// 폭발 범위 시각화
	//{
	//	float ExplosionRadius = CollisionComponent->GetScaledSphereRadius();
	//	UKismetSystemLibrary::DrawDebugSphere(
	//		GetWorld(),
	//		GetActorLocation(),
	//		ExplosionRadius,
	//		12, // 세그먼트 수
	//		FLinearColor::Red,
	//		5.0f, // 지속 시간 (초)
	//		2.0f  // 두께
	//	);
	//}

	if (!HasAuthority()) return;

	// 1. 범위 내 모든 타겟 찾기
	TArray<AActor*> AllTargets = FindTargetsInRange();
	if (AllTargets.Num() == 0)
	{
		OnAOECompleted();
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
		if (bUseDistanceBasedDamage)
		{
			float Distance = FVector::Dist(ExplosionCenter, CurrentTarget->GetActorLocation());
			float DamageLevel = CalculateDistanceDamageLevel(Distance);

			if (DamageLevel > 0.0f)
			{
				ApplyDistanceBasedDamageToTarget(CurrentTarget, DamageLevel);
			}
		}
		else
		{
			// 거리 상관없이 고정 데미지
			ApplyDamageToTarget(CurrentTarget);
		}
	}

	// 5. 완료 처리
	OnAOECompleted();
}

void AHaroAOEBase::ExecuteDOTField()
{
	OnAOEStarted(); // 시각적 효과

	if (!HasAuthority()) return;

	SetLifeSpan(DOTFieldLifeSpan);

	// 컬리전을 트리거로 설정하여 블루프린트에서 오버랩 감지 가능하게
	CollisionComponent->SetCollisionProfileName(TEXT("Trigger"));
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

	return OverlappingActors;
}

void AHaroAOEBase::OnAOECompleted()
{
	// 블루프린트
	OnAOEFinished();
}

void AHaroAOEBase::OnOverlap(AActor* TargetActor)
{
	if (!HasAuthority()) return;

	if (IsValidTarget(TargetActor))
	{
		// 시야 체크 (필요한 경우)
		if (bCheckLineOfSight)
		{
			TArray<AActor*> AllTargets = { TargetActor }; // 단일 타겟용
			if (!CheckLineOfSight(GetActorLocation(), TargetActor, AllTargets))
			{
				return; // 시야 막힘
			}
		}

		ApplyDOTEffectToTarget(TargetActor);
	}
}

void AHaroAOEBase::OnEndOverlap(AActor* TargetActor)
{
	if (!HasAuthority()) return;

	RemoveDOTEffectFromTarget(TargetActor);

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

bool AHaroAOEBase::IsValidTarget(AActor* Target) const
{
	if (!Target) return false;
	if (Target == GetOwner()) return false;
	if (!Target->IsA<ALyraCharacter>()) return false;

	return true;
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
void AHaroAOEBase::ApplyDistanceBasedDamageToTarget(AActor* Target, float DamageLevel)
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


void AHaroAOEBase::ApplyDamageToTarget(AActor* Target)
{
	if (!Target || !AOEDamageEffectSpecHandle.IsValid())
		return;

	// 타겟의 AbilitySystemComponent 찾기
	UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target);
	if (!TargetASC) return;

	// GameplayEffect 적용
	TargetASC->ApplyGameplayEffectSpecToSelf(*AOEDamageEffectSpecHandle.Data.Get());
}

void AHaroAOEBase::ApplyDOTEffectToTarget(AActor* Target)
{
	if (!Target || !AOEDamageEffectSpecHandle.IsValid())
		return;

	UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target);
	if (!TargetASC) return;

	
	const FActiveGameplayEffectHandle ActiveEffectHandle = TargetASC->ApplyGameplayEffectSpecToSelf(
		*AOEDamageEffectSpecHandle.Data.Get()
	);

	// Infinite로 설정되어있으면 따로 처리 필요함.
	const bool bIsInfinite = AOEDamageEffectSpecHandle.Data.Get()->Def.Get()->DurationPolicy == EGameplayEffectDurationType::Infinite;
	if (bIsInfinite && ActiveEffectHandle.IsValid())
	{
		ActiveEffectHandles.Add(ActiveEffectHandle, TargetASC);
	}

}

void AHaroAOEBase::RemoveDOTEffectFromTarget(AActor* Target)
{
	if (!Target) return;

	if (ActiveEffectHandles.IsEmpty()) return;

	UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target);
	if (!IsValid(TargetASC)) return;

	TArray<FActiveGameplayEffectHandle> HandlesToRemove;
	for (auto HandlePair : ActiveEffectHandles)
	{
		if (TargetASC == HandlePair.Value)
		{
			TargetASC->RemoveActiveGameplayEffect(HandlePair.Key, 1);
			HandlesToRemove.Add(HandlePair.Key);
		}
	}

	// 별도 루프에서 맵에서 제거
	for (auto& Handle : HandlesToRemove)
	{
		ActiveEffectHandles.FindAndRemoveChecked(Handle);
	}
}