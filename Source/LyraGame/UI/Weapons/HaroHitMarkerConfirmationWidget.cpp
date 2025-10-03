// Fill out your copyright notice in the Description page of Project Settings.


#include "HaroHitMarkerConfirmationWidget.h"

#include "Blueprint/UserWidget.h"
#include "SHaroHitMarkerConfirmationWidget.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(HaroHitMarkerConfirmationWidget)

class SWidget;

UHaroHitMarkerConfirmationWidget::UHaroHitMarkerConfirmationWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetVisibility(ESlateVisibility::HitTestInvisible);
	bIsVolatile = true;
	AnyHitsMarkerImage.DrawAs = ESlateBrushDrawType::NoDrawType;
}

void UHaroHitMarkerConfirmationWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);

	MyMarkerWidget.Reset();
}

TSharedRef<SWidget> UHaroHitMarkerConfirmationWidget::RebuildWidget()
{
	UUserWidget* OuterUserWidget = GetTypedOuter<UUserWidget>();
	FLocalPlayerContext DummyContext;
	const FLocalPlayerContext& PlayerContextRef = (OuterUserWidget != nullptr) ? OuterUserWidget->GetPlayerContext() : DummyContext;

	MyMarkerWidget = SNew(SHaroHitMarkerConfirmationWidget, PlayerContextRef, PerHitMarkerZoneOverrideImages)
		.PerHitMarkerImage(&(this->PerHitMarkerImage))
		.AnyHitsMarkerImage(&(this->AnyHitsMarkerImage))
		.HitNotifyDuration(this->HitNotifyDuration);

	return MyMarkerWidget.ToSharedRef();
}

