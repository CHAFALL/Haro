// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HaroWeaponBase.generated.h"

class USkeletalMeshComponent;

UCLASS()
class LYRAGAME_API AHaroWeaponBase : public AActor
{
	GENERATED_BODY()
	
public:	
	AHaroWeaponBase();

protected:
	virtual void BeginPlay() override;

public:
	UFUNCTION(BlueprintPure, Category = "Weapon")
	FORCEINLINE USkeletalMeshComponent* GetWeaponMesh1P() const { return WeaponMesh_1P; }

	UFUNCTION(BlueprintPure, Category = "Weapon")
	FORCEINLINE USkeletalMeshComponent* GetWeaponMesh3P() const { return WeaponMesh_3P; }

	UFUNCTION(BlueprintPure, Category = "Weapon")
	FORCEINLINE bool IsADS() const { return bIsADS; }

	void OnWeaponActivated(); // 무기가 활성화될 때 호출될 함수

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	USkeletalMeshComponent* WeaponMesh_1P;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	USkeletalMeshComponent* WeaponMesh_3P;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Weapon")
	FName AttachSocket = "VB weapon_r";

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Weapon")
	float WeaponCameraOffset = 30.0f;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Weapon")
	bool bIsADS = false; // 줌 기능 여부.

	UFUNCTION(BlueprintPure, Category = "Weapon")
	USkeletalMeshComponent* GetProperWeaponMesh() const;

	// (추가) - 방향, 이거 순서 달라서 헷갈림. 그래서 따로 손 보기!
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WeaponOffset", meta = (DisplayName = "Weapon Mesh 1P Rotation Pitch"))
	float WeaponRotPitch = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WeaponOffset", meta = (DisplayName = "Weapon Mesh 1P Y Rotation Yaw"))
	float WeaponRotYaw = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WeaponOffset", meta = (DisplayName = "Weapon Mesh 1P Z Rotation Roll"))
	float WeaponRotRoll = 0.0f;

	// (추가) - 위치
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WeaponOffset", meta = (DisplayName = "Weapon Mesh 1P X Position"))
	float WeaponPosX = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WeaponOffset", meta = (DisplayName = "Weapon Mesh 1P Y Position"))
	float WeaponPosY = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WeaponOffset", meta = (DisplayName = "Weapon Mesh 1P Z Position"))
	float WeaponPosZ = 0.0f;
};
