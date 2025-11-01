// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapons/HaroWeaponBase.h"
#include "Character/LyraCharacter.h"

AHaroWeaponBase::AHaroWeaponBase()
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;

	RootComponent = WeaponMesh_3P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh_3P"));

	WeaponMesh_3P->SetOwnerNoSee(true);
	WeaponMesh_3P->CastShadow = true;
	WeaponMesh_3P->bCastHiddenShadow = true;

	WeaponMesh_1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh_1P"));
	WeaponMesh_1P->SetupAttachment(RootComponent);
	WeaponMesh_1P->CastShadow = false;
	WeaponMesh_1P->bCastHiddenShadow = false;
	WeaponMesh_1P->SetOnlyOwnerSee(true);

	bReplicates = true;	
}

// 1인칭 메시를 가져와서 이를 세팅하고 완료된 것을 캐릭터에게 알려주는 작업이 필요하다!
void AHaroWeaponBase::BeginPlay()
{
	Super::BeginPlay();
	
	APawn* PawnOwner = Cast<APawn>(GetOwner());

	if (GetNetMode() != NM_DedicatedServer)
	{
		if (ALyraCharacter* Character = Cast<ALyraCharacter>(PawnOwner))
		{
			WeaponMesh_1P->AttachToComponent(Character->GetFirstPersonMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, AttachSocket);
			WeaponMesh_1P->SetRelativeRotation(FRotator(WeaponRotPitch, WeaponRotYaw, WeaponRotRoll));
			WeaponMesh_1P->SetRelativeLocation(FVector(WeaponPosX, WeaponPosY, WeaponPosZ));

			Character->SetCurrentWeapon(this);
			Character->OnPlayerEquippedNewWeapon(WeaponCameraOffset); // Good
		}
	}
}

// 활성/ 비활성화 방식이라 이를 추가.
void AHaroWeaponBase::OnWeaponActivated()
{
	APawn* PawnOwner = Cast<APawn>(GetOwner());
	if (GetNetMode() != NM_DedicatedServer)
	{
		if (ALyraCharacter* Character = Cast<ALyraCharacter>(PawnOwner))
		{
			// 무기 상태만 캐릭터에게 알려줌 (부착 작업은 생략) -> 만약에 3인칭 <-> 1인칭을 왔다갔다하게 되고 이에 위치가 건드려진다면 위의 3줄도 가져오는 것이 좋음.
			Character->SetCurrentWeapon(this);
			Character->OnPlayerEquippedNewWeapon(WeaponCameraOffset);
		}
	}
}



USkeletalMeshComponent* AHaroWeaponBase::GetProperWeaponMesh() const
{
	const APawn* PawnOwner = Cast<APawn>(GetOwner());

	return PawnOwner->IsLocallyControlled() ? WeaponMesh_1P : WeaponMesh_3P;
}

