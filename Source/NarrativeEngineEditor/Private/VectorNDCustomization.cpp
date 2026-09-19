#include "VectorNDCustomization.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "IPropertyUtilities.h"
#include "NarrativeCoreData.h"
#include "PropertyCustomizationHelpers.h"
#include "PropertyHandle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "VectorNDCustomization"

TSharedRef<IPropertyTypeCustomization> FVectorNDCustomization::MakeInstance()
{
	return MakeShared<FVectorNDCustomization>();
}

void FVectorNDCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> Handle, FDetailWidgetRow& Row,
	IPropertyTypeCustomizationUtils& Utils)
{
	VectorHandle = Handle;
	PropertyUtilities = Utils.GetPropertyUtilities();
	// Replacing the struct (including paste/reset) can change its array lengths or axis
	// order. A tree refresh reuses our custom rows and their now-stale element handles.
	// Defer a full layout rebuild until after the active property/button callback returns.
	Handle->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(
		PropertyUtilities.ToSharedRef(), &IPropertyUtilities::RequestForceRefresh));
	Row.NameContent()[Handle->CreatePropertyNameWidget()];
	Row.ValueContent().MinDesiredWidth(200.f)
	[
		SNew(STextBlock).Text_Lambda([Handle]()
		{
			TArray<void*> Values;
			Handle->AccessRawData(Values);
			if (Values.Num() != 1 || !Values[0]) { return LOCTEXT("Multiple", "Multiple vectors"); }
			const FVectorND& Vector = *static_cast<const FVectorND*>(Values[0]);
			return FText::Format(LOCTEXT("Summary", "{0} axes | magnitude {1}"),
				FText::AsNumber(Vector.Num()), FText::AsNumber(Vector.Length()));
		})
	];
}

void FVectorNDCustomization::EditVectors(TFunctionRef<void(FVectorND&)> Edit)
{
	TArray<void*> Values;
	VectorHandle->AccessRawData(Values);
	TArray<FString> TextValues;
	for (void* Value : Values)
	{
		if (!Value) { return; }
		FVectorND Copy = *static_cast<FVectorND*>(Value);
		Edit(Copy);
		FString Text;
		// Export a complete replacement, including empty arrays. Null defaults omit empty
		// fields, producing "()" when removing the last axis; importing that leaves old data.
		FVectorND::StaticStruct()->ExportText(Text, &Copy, &Copy, nullptr, PPF_None, nullptr);
		TextValues.Add(MoveTemp(Text));
	}
	// The property system owns transactions, dirtying, notifications and per-object values.
	if (VectorHandle->SetPerObjectValues(TextValues) == FPropertyAccess::Success)
	{
		PropertyUtilities->RequestForceRefresh();
	}
}

void FVectorNDCustomization::PostUndo(bool bSuccess)
{
	// Undo restores array storage without invoking the property-handle change delegate.
	if (bSuccess && PropertyUtilities) { PropertyUtilities->RequestForceRefresh(); }
}

void FVectorNDCustomization::PostRedo(bool bSuccess)
{
	PostUndo(bSuccess);
}

void FVectorNDCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> Handle, IDetailChildrenBuilder& Builder,
	IPropertyTypeCustomizationUtils& Utils)
{
	TArray<void*> Values;
	Handle->AccessRawData(Values);
	if (Values.IsEmpty() || !Values[0]) { return; }
	const FVectorND& First = *static_cast<const FVectorND*>(Values[0]);
	for (void* Value : Values)
	{
		FString Error;
		if (!Value || static_cast<const FVectorND*>(Value)->GetBasis().Num() != static_cast<const FVectorND*>(Value)->Num() ||
			!First.HasSameBasis(*static_cast<const FVectorND*>(Value)))
		{
			Builder.AddCustomRow(LOCTEXT("DifferentAxes", "Vector layout"))
			.WholeRowContent()[SNew(STextBlock).AutoWrapText(true).Text(FText::FromString(
				TEXT("Select one vector to edit differing layouts. Basis and coordinate counts must match.")))];
			return;
		}
		if (!static_cast<const FVectorND*>(Value)->IsValid(&Error))
		{
			Builder.AddCustomRow(LOCTEXT("InvalidVector", "Invalid vector"))
			.WholeRowContent()[SNew(STextBlock).AutoWrapText(true).Text(FText::FromString(Error))];
		}
	}

	const TSharedPtr<IPropertyHandleArray> Coordinates = Handle->GetChildHandle(FVectorND::CoordinatesPropertyName())->AsArray();
	for (int32 Index = 0; Index < First.Num(); ++Index)
	{
		const auto Axis = First.GetBasis()[Index];
		const UNarrativeBasisVector* LoadedAxis = Axis.Get();
		const FText Label = LoadedAxis && !LoadedAxis->ArchetypeTitle.IsEmpty()
			? LoadedAxis->ArchetypeTitle : FText::FromString(Axis.ToSoftObjectPath().GetAssetName());
		Builder.AddCustomRow(Label)
		.NameContent()[SNew(STextBlock).Text(Label).ToolTipText(FText::FromString(Axis.ToString()))]
		.ValueContent().MinDesiredWidth(250.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f)[Coordinates->GetElement(Index)->CreatePropertyValueWidget()]
			+ SHorizontalBox::Slot().AutoWidth().Padding(4.f, 0.f)
			[
				SNew(SButton).Text(LOCTEXT("Remove", "Remove")).OnClicked_Lambda([this, Axis]()
				{
					EditVectors([Axis](FVectorND& Vector)
					{
						const int32 AxisIndex = Vector.GetBasis().IndexOfByKey(Axis);
						if (AxisIndex != INDEX_NONE) { Vector.RemoveAxis(AxisIndex); }
					});
					return FReply::Handled();
				})
			]
		];
	}

	Builder.AddCustomRow(LOCTEXT("AddAxis", "Add axis"))
	.NameContent()[SNew(STextBlock).Text(LOCTEXT("AddAxis", "Add axis"))]
	.ValueContent().MinDesiredWidth(250.f)
	[
		SNew(SObjectPropertyEntryBox).AllowedClass(UNarrativeBasisVector::StaticClass()).AllowClear(false)
		.OnObjectChanged_Lambda([this](const FAssetData& Asset)
		{
			if (!Asset.IsValid()) { return; }
			const TSoftObjectPtr<UNarrativeBasisVector> Axis(Asset.GetSoftObjectPath());
			EditVectors([Axis](FVectorND& Vector) { Vector.SetCoordinate(Axis, Vector.GetCoordinate(Axis)); });
		})
	];
	Builder.AddCustomRow(LOCTEXT("AllAxes", "Add all basis axes"))
	.WholeRowContent()
	[
		SNew(SButton).Text(LOCTEXT("AllAxes", "Add all basis axes"))
		.ToolTipText(LOCTEXT("AllAxesTip", "Add missing basis assets at zero without changing existing coordinates."))
		.OnClicked_Lambda([this]()
		{
			TArray<FAssetData> Assets;
			FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get().GetAssetsByClass(
				UNarrativeBasisVector::StaticClass()->GetClassPathName(), Assets, true);
			Assets.Sort([](const FAssetData& A, const FAssetData& B) { return A.GetSoftObjectPath().ToString() < B.GetSoftObjectPath().ToString(); });
			EditVectors([&Assets](FVectorND& Vector)
			{
				for (const FAssetData& Asset : Assets)
				{
					const TSoftObjectPtr<UNarrativeBasisVector> Axis(Asset.GetSoftObjectPath());
					Vector.SetCoordinate(Axis, Vector.GetCoordinate(Axis));
				}
			});
			return FReply::Handled();
		})
	];
}

#undef LOCTEXT_NAMESPACE
