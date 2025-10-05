// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Templates/SubclassOf.h"

#include "LyraEquipmentDefinition.generated.h"

class AActor;
class ULyraAbilitySet;
class ULyraEquipmentInstance;
class ULyraGameplayAbility;

USTRUCT()
struct FLyraEquipmentActorToSpawn
{
	GENERATED_BODY()

	FLyraEquipmentActorToSpawn()
	{}

	UPROPERTY(EditAnywhere, Category=Equipment)
	TSubclassOf<AActor> ActorToSpawn;

	UPROPERTY(EditAnywhere, Category=Equipment)
	FName AttachSocket;

	UPROPERTY(EditAnywhere, Category=Equipment)
	FTransform AttachTransform;
};


/**
 * ULyraEquipmentDefinition
 *
 * Definition of a piece of equipment that can be applied to a pawn
 */
UCLASS(Blueprintable, Const, Abstract, BlueprintType)
class ULyraEquipmentDefinition : public UObject
{
	GENERATED_BODY()

public:
	ULyraEquipmentDefinition(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// Class to spawn
	UPROPERTY(EditDefaultsOnly, Category=Equipment)
	TSubclassOf<ULyraEquipmentInstance> InstanceType;

	// Gameplay ability sets to grant when this is equipped
	UPROPERTY(EditDefaultsOnly, Category=Equipment)
	TArray<TObjectPtr<const ULyraAbilitySet>> AbilitySetsToGrant;

	// Actors to spawn on the pawn when this is equipped
	UPROPERTY(EditDefaultsOnly, Category=Equipment)
	TArray<FLyraEquipmentActorToSpawn> ActorsToSpawn;

	// 스킬로 관리할 Ability들 (디폴트 스킬만)
	UPROPERTY(EditDefaultsOnly, meta = (AllowAbstract = "false"))
	TArray<TSubclassOf<ULyraGameplayAbility>> EquipmentSkillAbilities;
};


// EquipmentSkillAbilities에 대한 설명
// ID로 할까 어빌리티로 할까 했는데 어빌리티로 하기로 함.
// 1. 어빌리티도 고유하다고 볼 수 있음(내 게임에 한해서만)
// 2. ID로 하면 직접 또 ID 번호를 찾고 쳐야하는 번거로움이 있음
// 3. 어빌리티 클래스로 하면 오타날 일이 없음

// 대신 ID로 안하고 어빌리티로 하기 때문에 캐싱 부분이 조금 복잡해지긴 함. (SkillData 부분.)