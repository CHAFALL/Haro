// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "AbilitySystem/Abilities/LyraGameplayAbility.h"
#include "HaroSkillDataTypes.generated.h"

USTRUCT(BlueprintType)
struct LYRAGAME_API FHaroSkillDataRow : public FTableRowBase
{
    GENERATED_USTRUCT_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString Name;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString Description;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSoftObjectPtr<UTexture2D> IconTexture;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGameplayTag GameplayTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSoftClassPtr<ULyraGameplayAbility> AbilityClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FName> RequiredSkillIDs;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FName> ConflictSkillIDs;
};