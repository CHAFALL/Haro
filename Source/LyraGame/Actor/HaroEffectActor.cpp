// Fill out your copyright notice in the Description page of Project Settings.


#include "HaroEffectActor.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"


AHaroEffectActor::AHaroEffectActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SetRootComponent(CreateDefaultSubobject<USceneComponent>("SceneRoot"));
}

void AHaroEffectActor::BeginPlay()
{
	Super::BeginPlay();

}

void AHaroEffectActor::ApplyEffectToTarget(AActor* TargetActor, TSubclassOf<UGameplayEffect> GameplayEffectClass)
{
	 UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
	 if (TargetASC == nullptr) return;

	 check(GameplayEffectClass);
	 
	 // 이펙트 적용에 필요한 컨텍스트 생성
	 // 컨텍스트는 "누가, 언제, 어떻게" 이펙트를 적용했는지 정보를 담음
	 FGameplayEffectContextHandle EffectContextHandle = TargetASC->MakeEffectContext();
	 EffectContextHandle.AddSourceObject(this);
	 
	 // MakeOutgoingSpec(): GameplayEffect 클래스를 실제 적용 가능한 Spec으로 변환
	 const FGameplayEffectSpecHandle EffectSpecHandle = TargetASC->MakeOutgoingSpec(GameplayEffectClass, ActorLevel, EffectContextHandle);
	 
	 // ApplyGameplayEffectSpecToSelf(): 실제로 이펙트를 Target에게 적용
	 //EffectSpecHandle.Data.Get() : SpecHandle에서 실제 Spec 데이터를 가져옴
	 // *를 달아서 역참조해줌
	 const FActiveGameplayEffectHandle ActiveEffectHandle = TargetASC->ApplyGameplayEffectSpecToSelf(*EffectSpecHandle.Data.Get());

	 // 무한 효과 추적 및 저장. (Infinite 효과는 수동으로 제거해야 함)
	 const bool bIsInfinite = EffectSpecHandle.Data.Get()->Def.Get()->DurationPolicy == EGameplayEffectDurationType::Infinite;
	 if (bIsInfinite && InfiniteEffectRemovalPolicy == EEffectRemovalPolicy::RemoveOnEndOverlap)
	 {
		 // 맵으로 관리하는 이유?
		 // 다중 타겟을 지원하기 위함, ex. 여러 플레이어가 동시에 버프 구역 안에 있음.
		 ActiveEffectHandles.Add(ActiveEffectHandle, TargetASC);
	 }
}

void AHaroEffectActor::OnOverlap(AActor* TargetActor)
{
	if (InstantEffectApplicationPolicy == EEffectApplicationPolicy::ApplyOnOverlap)
	{
		ApplyEffectToTarget(TargetActor, InstantGameplayEffectClass);
	}
	if (DurationEffectApplicationPolicy == EEffectApplicationPolicy::ApplyOnOverlap)
	{
		ApplyEffectToTarget(TargetActor, DurationGameplayEffectClass);
	}
	if (InfiniteEffectApplicationPolicy == EEffectApplicationPolicy::ApplyOnOverlap)
	{
		ApplyEffectToTarget(TargetActor, InfiniteGameplayEffectClass);
	}
}

void AHaroEffectActor::OnEndOverlap(AActor* TargetActor)
{
	if (InstantEffectApplicationPolicy == EEffectApplicationPolicy::ApplyOnEndOverlap)
	{
		ApplyEffectToTarget(TargetActor, InstantGameplayEffectClass);
	}
	if (DurationEffectApplicationPolicy == EEffectApplicationPolicy::ApplyOnEndOverlap)
	{
		ApplyEffectToTarget(TargetActor, DurationGameplayEffectClass);
	}
	if (InfiniteEffectApplicationPolicy == EEffectApplicationPolicy::ApplyOnEndOverlap)
	{
		ApplyEffectToTarget(TargetActor, InfiniteGameplayEffectClass);
	}
	if (InfiniteEffectRemovalPolicy == EEffectRemovalPolicy::RemoveOnEndOverlap)
	{
		UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
		if (!IsValid(TargetASC)) return;

		TArray<FActiveGameplayEffectHandle> HandlesToRemove; // for문 중에 요소를 삭제하면 문제 생기므로 이를 이용.
		for (auto HandlePair : ActiveEffectHandles)
		{
			if (TargetASC == HandlePair.Value)
			{
				TargetASC->RemoveActiveGameplayEffect(HandlePair.Key, 1); // 스택을 한 개만 제거.
				HandlesToRemove.Add(HandlePair.Key);
			}
		}
		// 별도 루프에서 맵에서 제거 (반복 중 삭제 방지)
		for (auto& Handle : HandlesToRemove)
		{
			ActiveEffectHandles.FindAndRemoveChecked(Handle);
		}

	}
}
