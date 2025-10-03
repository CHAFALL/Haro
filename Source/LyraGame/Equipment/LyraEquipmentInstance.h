// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/World.h"

#include "LyraEquipmentInstance.generated.h"

class AActor;
class APawn;
struct FFrame;
struct FLyraEquipmentActorToSpawn;

/**
 * ULyraEquipmentInstance
 *
 * A piece of equipment spawned and applied to a pawn
 */
UCLASS(BlueprintType, Blueprintable)
class ULyraEquipmentInstance : public UObject
{
	GENERATED_BODY()

public:
	ULyraEquipmentInstance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~UObject interface
	virtual bool IsSupportedForNetworking() const override { return true; }
	virtual UWorld* GetWorld() const override final;
	//~End of UObject interface

	UFUNCTION(BlueprintPure, Category=Equipment)
	UObject* GetInstigator() const { return Instigator; }

	void SetInstigator(UObject* InInstigator) { Instigator = InInstigator; }

	UFUNCTION(BlueprintPure, Category=Equipment)
	APawn* GetPawn() const;

	UFUNCTION(BlueprintPure, Category=Equipment, meta=(DeterminesOutputType=PawnType))
	APawn* GetTypedPawn(TSubclassOf<APawn> PawnType) const;

	UFUNCTION(BlueprintPure, Category=Equipment)
	TArray<AActor*> GetSpawnedActors() const { return SpawnedActors; }

	virtual void SpawnEquipmentActors(const TArray<FLyraEquipmentActorToSpawn>& ActorsToSpawn);
	virtual void DestroyEquipmentActors();

	virtual void OnEquipped();
	virtual void OnUnequipped();

protected:
#if UE_WITH_IRIS
	/** Register all replication fragments */
	virtual void RegisterReplicationFragments(UE::Net::FFragmentRegistrationContext& Context, UE::Net::EFragmentRegistrationFlags RegistrationFlags) override;
#endif // UE_WITH_IRIS

	UFUNCTION(BlueprintImplementableEvent, Category=Equipment, meta=(DisplayName="OnEquipped"))
	void K2_OnEquipped();

	UFUNCTION(BlueprintImplementableEvent, Category=Equipment, meta=(DisplayName="OnUnequipped"))
	void K2_OnUnequipped();

private:
	UFUNCTION()
	void OnRep_Instigator();

private:
	UPROPERTY(ReplicatedUsing=OnRep_Instigator)
	TObjectPtr<UObject> Instigator;

	UPROPERTY(Replicated)
	TArray<TObjectPtr<AActor>> SpawnedActors;


//----------------(추가)-------------------
public:


	/**
	* 주 장비 액터 (보통 0번)를 가져오는 함수 (헬퍼 함수)
	* 이를 통해 무기 액터를 가져오는 곳에 쓰일 것이다.
	* 무기 액터임이 보장되는 이유 :
	* - ULyraEquipmentDefinition의 ActorsToSpawn 배열에서
	*   일관되게 0번을 주 장비 액터(무기 메시)로 정의함
	* - SpawnEquipmentActors()에서 ActorsToSpawn 배열을 순서대로 순회하여
	*   SpawnedActors 배열에 동일한 순서로 추가하므로 순서가 보장됨
	* - 따라서 Definition의 0번 = Runtime의 SpawnedActors[0]로 1:1 매칭됨
	*/
	
	UFUNCTION(BlueprintPure, Category = Equipment)
	AActor* GetPrimaryActor() const
	{
		return (GetSpawnedActors().Num() > 0) ? GetSpawnedActors()[0] : nullptr;
	}

	// 추가 (활성화 여부) - Temp
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Equipment")
	bool bIsActive = false;
};
