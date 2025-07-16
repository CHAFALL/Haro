// Fill out your copyright notice in the Description page of Project Settings.


#include "HaroProjectile.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DamageEvents.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayEffect.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(HaroProjectile)

// Sets default values
AHaroProjectile::AHaroProjectile()
{
    PrimaryActorTick.bCanEverTick = false;

    // 충돌 컴포넌트 설정
    CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComponent"));
    CollisionComponent->SetCollisionProfileName("Projectile");
    CollisionComponent->SetGenerateOverlapEvents(false);
    CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    CollisionComponent->SetCollisionResponseToAllChannels(ECR_Block);
    RootComponent = CollisionComponent;

    // 메시 컴포넌트
    ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileMesh"));
    ProjectileMesh->SetupAttachment(RootComponent);
    ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    // 투사체 무브먼트
    ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
    ProjectileMovement->SetUpdatedComponent(CollisionComponent);
    ProjectileMovement->bRotationFollowsVelocity = true;

    // 네트워크 설정
    bReplicates = true;
    SetReplicateMovement(true);
}

// Called when the game starts or when spawned
void AHaroProjectile::BeginPlay()
{
	Super::BeginPlay();

    // 블루프린트 설정값들을 컴포넌트에 적용
    if (ProjectileMovement)
    {
        ProjectileMovement->InitialSpeed = InitialSpeed;
        ProjectileMovement->MaxSpeed = InitialSpeed;
        ProjectileMovement->ProjectileGravityScale = GravityScale;
    }

    if (USphereComponent* SphereComp = Cast<USphereComponent>(CollisionComponent))
    {
        SphereComp->SetSphereRadius(CollisionRadius);
    }

    SetLifeSpan(LifeSpan);

    // 현재 데미지 초기화 (관통 시스템용)
    CurrentDamageAmount = DamageAmount;

    // 충돌 이벤트 바인딩
    CollisionComponent->OnComponentHit.AddDynamic(this, &AHaroProjectile::OnHit);

    // 발사자 무시 설정
    if (APawn* InstigatorPawn = GetInstigator())
    {
        CollisionComponent->IgnoreActorWhenMoving(InstigatorPawn, true);
    }
	
}

void AHaroProjectile::Destroyed()
{
    Super::Destroyed();
}

void AHaroProjectile::FireInDirection(const FVector& ShootDirection)
{
    if (ProjectileMovement)
    {
        // 런타임 설정값 동기화
        ProjectileMovement->InitialSpeed = InitialSpeed;
        ProjectileMovement->MaxSpeed = InitialSpeed;
        ProjectileMovement->ProjectileGravityScale = GravityScale;

        ProjectileMovement->Velocity = ShootDirection.GetSafeNormal() * InitialSpeed;
    }
}

void AHaroProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
    // 기본 체크
    if (OtherActor == this || OtherActor == GetOwner() || OtherActor == GetInstigator())
    {
        return;
    }

    // 이미 어태치된 상태면 추가 충돌 무시
    if (bIsAttached)
    {
        return;
    }

    // 중복 데미지 방지: 이미 맞은 액터는 스킵
    if (HitActors.Contains(OtherActor))
    {
        return;
    }

    // 히트한 액터 기록
    HitActors.Add(OtherActor);

    // 데미지 적용
    ApplyDamageToTarget(OtherActor, Hit);

    // 관통 처리
    if (bCanPenetrate && HandlePenetration(OtherActor, Hit))
    {
        // 관통 성공 - 계속 날아감
        return;
    }

    // 어태치먼트 처리
    if (bAttachToHitTarget)
    {
        HandleAttachment(OtherActor, OtherComp, Hit);
        return;
    }

    // 일반적인 경우: 투사체 파괴
    Destroy();
}

void AHaroProjectile::ApplyDamageToTarget(AActor* Target, const FHitResult& Hit)
{
    if (!Target || !DamageEffect || DamageAmount <= 0.0f)
    {
        return;
    }

    // Lyra 어빌리티 시스템을 통한 데미지 적용
    UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
    UAbilitySystemComponent* SourceASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetInstigator());

    if (TargetASC && SourceASC)
    {
        // GameplayEffect Context 생성
        FGameplayEffectContextHandle EffectContext = SourceASC->MakeEffectContext();
        EffectContext.AddHitResult(Hit);
        EffectContext.AddInstigator(GetInstigator(), this);

        // GameplayEffect Spec 생성
        FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(DamageEffect, 1.0f, EffectContext);

        if (SpecHandle.IsValid())
        {
            // 관통으로 감소된 데미지 적용
            SpecHandle.Data.Get()->SetSetByCallerMagnitude(
                FGameplayTag::RequestGameplayTag("Damage.Basic"),
                CurrentDamageAmount
            );

            SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);

            UE_LOG(LogTemp, Log, TEXT("Applied %f damage to %s (Penetration: %d/%d)"),
                CurrentDamageAmount, *Target->GetName(), CurrentPenetrationCount, MaxPenetrationCount);
        }
    }
    else
    {
        // 폴백: 기본 데미지 시스템
        FPointDamageEvent DamageEvent;
        DamageEvent.DamageTypeClass = UDamageType::StaticClass();
        DamageEvent.Damage = CurrentDamageAmount;
        DamageEvent.HitInfo = Hit;

        Target->TakeDamage(CurrentDamageAmount, DamageEvent,
            GetInstigator() ? GetInstigator()->GetController() : nullptr, this);
    }
}


bool AHaroProjectile::HandlePenetration(AActor* HitActor, const FHitResult& Hit)
{
    // 관통 가능 횟수 체크
    if (CurrentPenetrationCount >= MaxPenetrationCount)
    {
        UE_LOG(LogTemp, Log, TEXT("Max penetration reached (%d/%d)"), CurrentPenetrationCount, MaxPenetrationCount);
        return false; // 더 이상 관통 불가
    }

    // 특정 액터는 관통 불가 (예: 두꺼운 벽, 보스 등)
    // 이 부분은 게임 디자인에 따라 구현
    if (HitActor->ActorHasTag("Impenetrable"))
    {
        UE_LOG(LogTemp, Log, TEXT("Hit impenetrable object: %s"), *HitActor->GetName());
        return false;
    }

    // 관통 성공
    CurrentPenetrationCount++;

    // 관통할 때마다 데미지 감소
    CurrentDamageAmount *= PenetrationDamageReduction;

    UE_LOG(LogTemp, Log, TEXT("Penetrated %s (%d/%d), damage reduced to %f"),
        *HitActor->GetName(), CurrentPenetrationCount, MaxPenetrationCount, CurrentDamageAmount);

    return true; // 관통 성공, 계속 날아감
}

void AHaroProjectile::HandleAttachment(AActor* HitActor, UPrimitiveComponent* HitComponent, const FHitResult& Hit)
{
    if (!HitComponent)
    {
        // 컴포넌트가 없으면 그냥 파괴
        Destroy();
        return;
    }

    // 투사체 이동 중지
    if (ProjectileMovement)
    {
        ProjectileMovement->SetActive(false);
    }

    // 어태치 수행
    FAttachmentTransformRules AttachRules(EAttachmentRule::KeepWorld, EAttachmentRule::KeepWorld, EAttachmentRule::KeepWorld, false);

    if (bAttachToBone && Hit.BoneName.IsValid())
    {
        // 본에 어태치
        AttachToComponent(HitComponent, AttachRules, Hit.BoneName);
        UE_LOG(LogTemp, Log, TEXT("Attached to bone %s on %s"), *Hit.BoneName.ToString(), *HitActor->GetName());
    }
    else
    {
        // 컴포넌트에 어태치
        AttachToComponent(HitComponent, AttachRules);
        UE_LOG(LogTemp, Log, TEXT("Attached to component on %s"), *HitActor->GetName());
    }

    // 어태치 상태 설정
    bIsAttached = true;

    // 어태치 수명 설정
    SetLifeSpan(AttachmentLifeTime);

    UE_LOG(LogTemp, Log, TEXT("Projectile attached, will be destroyed in %f seconds"), AttachmentLifeTime);
}

