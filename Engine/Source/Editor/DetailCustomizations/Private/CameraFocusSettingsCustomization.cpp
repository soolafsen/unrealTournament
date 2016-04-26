// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "DetailCustomizationsPrivatePCH.h"
#include "CameraFocusSettingsCustomization.h"
#include "CineCameraComponent.h"
#include "ScopedTransaction.h"

#define LOCTEXT_NAMESPACE "CameraFocusSettingsCustomization"

static FName NAME_Category(TEXT("Category"));
static FString ManualFocusSettingsString(TEXT("Manual Focus Settings"));
static FString SpotFocusSettingsString(TEXT("Spot Focus Settings"));
static FString TrackingFocusSettingsString(TEXT("Tracking Focus Settings"));
static FString GeneralFocusSettingsString(TEXT("Focus Settings"));

TSharedRef<IPropertyTypeCustomization> FCameraFocusSettingsCustomization::MakeInstance()
{
	return MakeShareable(new FCameraFocusSettingsCustomization);
}

void FCameraFocusSettingsCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	HeaderRow.
		NameContent()
		[
			StructPropertyHandle->CreatePropertyNameWidget()
		]
		.ValueContent()
		[
			StructPropertyHandle->CreatePropertyValueWidget()
		];
}

void FCameraFocusSettingsCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	// Retrieve structure's child properties
	uint32 NumChildren;
	StructPropertyHandle->GetNumChildren(NumChildren);
	TMap<FName, TSharedPtr< IPropertyHandle > > PropertyHandles;
	for (uint32 ChildIndex = 0; ChildIndex < NumChildren; ++ChildIndex)
	{
		TSharedRef<IPropertyHandle> ChildHandle = StructPropertyHandle->GetChildHandle(ChildIndex).ToSharedRef();
		const FName PropertyName = ChildHandle->GetProperty()->GetFName();

		PropertyHandles.Add(PropertyName, ChildHandle);
	}

	// Retrieve special case properties
	FocusMethodHandle = PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FCameraFocusSettings, FocusMethod));
	ManualFocusDistanceHandle = PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FCameraFocusSettings, ManualFocusDistance));

	for (auto Iter(PropertyHandles.CreateConstIterator()); Iter; ++Iter)
	{
		// make the widget
		IDetailPropertyRow& PropertyRow = ChildBuilder.AddChildProperty(Iter.Value().ToSharedRef());

		// set up delegate to know if we need to hide it
		FString const& Category = Iter.Value()->GetMetaData(NAME_Category);
		if (Category == ManualFocusSettingsString)
		{
			PropertyRow.Visibility(TAttribute<EVisibility>(this, &FCameraFocusSettingsCustomization::IsManualSettingGroupVisible));
		}
		else if (Category == SpotFocusSettingsString)
		{
			PropertyRow.Visibility(TAttribute<EVisibility>(this, &FCameraFocusSettingsCustomization::IsSpotSettingGroupVisible));
		}
		else if (Category == TrackingFocusSettingsString)
		{
			PropertyRow.Visibility(TAttribute<EVisibility>(this, &FCameraFocusSettingsCustomization::IsTrackingSettingGroupVisible));
		}
		else if (Category == GeneralFocusSettingsString)
		{
			PropertyRow.Visibility(TAttribute<EVisibility>(this, &FCameraFocusSettingsCustomization::IsGeneralSettingGroupVisible));
		}
	}
}

EVisibility FCameraFocusSettingsCustomization::IsManualSettingGroupVisible() const
{
	uint8 FocusMethodNumber;
	FocusMethodHandle->GetValue(FocusMethodNumber);
	ECameraFocusMethod const FocusMethod = static_cast<ECameraFocusMethod>(FocusMethodNumber);
	if (FocusMethod == ECameraFocusMethod::Manual)
	{
		// if focus method is set to none, all non-none setting groups are collapsed
		return EVisibility::Visible;
	}

	return EVisibility::Collapsed;
}

EVisibility FCameraFocusSettingsCustomization::IsSpotSettingGroupVisible() const
{
	uint8 FocusMethodNumber;
	FocusMethodHandle->GetValue(FocusMethodNumber);
	ECameraFocusMethod const FocusMethod = static_cast<ECameraFocusMethod>(FocusMethodNumber);
	if (FocusMethod == ECameraFocusMethod::Spot)
	{
		// if focus method is set to none, all non-none setting groups are collapsed
		return EVisibility::Visible;
	}

	return EVisibility::Collapsed;
}

EVisibility FCameraFocusSettingsCustomization::IsTrackingSettingGroupVisible() const
{
	uint8 FocusMethodNumber;
	FocusMethodHandle->GetValue(FocusMethodNumber);
	ECameraFocusMethod const FocusMethod = static_cast<ECameraFocusMethod>(FocusMethodNumber);
	if (FocusMethod == ECameraFocusMethod::Tracking)
	{
		// if focus method is set to none, all non-none setting groups are collapsed
		return EVisibility::Visible;
	}

	return EVisibility::Collapsed;
}

EVisibility FCameraFocusSettingsCustomization::IsGeneralSettingGroupVisible() const
{
	uint8 FocusMethodNumber;
	FocusMethodHandle->GetValue(FocusMethodNumber);
	ECameraFocusMethod const FocusMethod = static_cast<ECameraFocusMethod>(FocusMethodNumber);
	if (FocusMethod != ECameraFocusMethod::None)
	{
		// if focus method is set to none, all non-none setting groups are collapsed
		return EVisibility::Visible;
	}

	return EVisibility::Collapsed;
}

#undef LOCTEXT_NAMESPACE // CameraFocusSettingsCustomization