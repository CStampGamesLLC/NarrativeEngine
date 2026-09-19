#include "NarrativeStaticData.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "AssetRegistry/AssetRegistryModule.h"
#include "Editor.h"
#include "IDetailsView.h"
#include "IDetailTreeNode.h"
#include "Misc/AutomationTest.h"
#include "PropertyEditorModule.h"
#include "PropertyHandle.h"
#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"

namespace VectorNDDetailsTests
{
bool HasText(const TSharedRef<SWidget>& Widget, const FString& Text)
{
	if (Widget->GetType() == FName("STextBlock") && StaticCastSharedRef<STextBlock>(Widget)->GetText().ToString() == Text) { return true; }
	FChildren* Children = Widget->GetChildren();
	for (int32 Index = 0; Index < Children->Num(); ++Index)
	{
		if (HasText(Children->GetChildAt(Index), Text)) { return true; }
	}
	return false;
}

TSharedPtr<SButton> FindButton(const TSharedPtr<SWidget>& Widget, const FString& Text)
{
	if (!Widget) { return nullptr; }
	if (Widget->GetType() == FName("SButton") && HasText(Widget.ToSharedRef(), Text)) { return StaticCastSharedPtr<SButton>(Widget); }
	FChildren* Children = Widget->GetChildren();
	for (int32 Index = 0; Index < Children->Num(); ++Index)
	{
		if (TSharedPtr<SButton> Found = FindButton(Children->GetChildAt(Index), Text)) { return Found; }
	}
	return nullptr;
}

TSharedPtr<IDetailTreeNode> FindVector(const TArray<TSharedRef<IDetailTreeNode>>& Nodes)
{
	for (const auto& Node : Nodes)
	{
		const auto Handle = Node->CreatePropertyHandle();
		if (Handle && Handle->GetProperty() && Handle->GetProperty()->GetFName() == GET_MEMBER_NAME_CHECKED(UNarrativeEntityDef, StartingCoordinates)) { return Node; }
		TArray<TSharedRef<IDetailTreeNode>> Children;
		Node->GetChildren(Children, true);
		if (auto Found = FindVector(Children)) { return Found; }
	}
	return nullptr;
}

struct FState
{
	TStrongObjectPtr<UNarrativeEntityDef> Object{NewObject<UNarrativeEntityDef>(GetTransientPackage(), NAME_None, RF_Transactional)};
	TStrongObjectPtr<UNarrativeBasisVector> Axis;
	TSharedPtr<IDetailsView> Details;

	~FState()
	{
		Details.Reset();
		if (Axis.IsValid()) { FAssetRegistryModule::AssetDeleted(Axis.Get()); }
	}

	TArray<TSharedRef<IDetailTreeNode>> Rows() const
	{
		TArray<TSharedRef<IDetailTreeNode>> Result;
		if (auto Vector = FindVector(Details->GetRootTreeNodes())) { Vector->GetChildren(Result, true); }
		return Result;
	}

	TSharedPtr<SButton> Button(const FString& Text) const
	{
		for (const auto& Row : Rows())
		{
			const FNodeWidgets Widgets = Row->CreateNodeWidgets();
			if (auto Found = FindButton(Widgets.ValueWidget, Text)) { return Found; }
			if (auto Found = FindButton(Widgets.WholeRowWidget, Text)) { return Found; }
		}
		return nullptr;
	}

	int32 AxisRows() const
	{
		int32 Count = 0;
		for (const auto& Row : Rows())
		{
			if (FindButton(Row->CreateNodeWidgets().ValueWidget, TEXT("Remove"))) { ++Count; }
		}
		return Count;
	}
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVectorNDDetailsRefreshTest, "Narrative.VectorND.DetailsRefresh",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FVectorNDDetailsRefreshTest::RunTest(const FString& Parameters)
{
	using namespace VectorNDDetailsTests;
	auto State = MakeShared<FState>();
	// A temporary registry entry makes the Add All button test independent of project content.
	UPackage* Package = CreatePackage(*(TEXT("/Temp/VectorNDDetails_") + FGuid::NewGuid().ToString(EGuidFormats::Digits)));
	State->Axis.Reset(NewObject<UNarrativeBasisVector>(Package, TEXT("TestAxis"), RF_Public | RF_Standalone | RF_Transient));
	FAssetRegistryModule::AssetCreated(State->Axis.Get());
	State->Object->StartingCoordinates = FVectorND({TSoftObjectPtr<UNarrativeBasisVector>(State->Axis.Get())}, 7.f);
	FDetailsViewArgs Args;
	Args.bUpdatesFromSelection = false;
	State->Details = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor").CreateDetailView(Args);
	State->Details->SetObject(State->Object.Get());
	TestEqual(TEXT("Initial axis row"), State->AxisRows(), 1);
	const auto Remove = State->Button(TEXT("Remove"));
	if (!TestTrue(TEXT("Remove button exists"), Remove.IsValid())) { return false; }
	Remove->SimulateClick();
	if (!TestEqual(TEXT("Remove updates stored vector"), State->Object->StartingCoordinates.Num(), 0)) { return false; }

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, State]()
	{
		TestEqual(TEXT("Remove rebuilds the visible axis rows"), State->AxisRows(), 0);
		GEditor->UndoTransaction();
	}));
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, State]()
	{
		TestEqual(TEXT("Undo restores data"), State->Object->StartingCoordinates.Num(), 1);
		TestEqual(TEXT("Undo restores the visible row"), State->AxisRows(), 1);
		TestEqual(TEXT("Undo preserves magnitude"), State->Object->StartingCoordinates[0], 7.f);
		GEditor->RedoTransaction();
	}));
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, State]()
	{
		TestEqual(TEXT("Redo removes the visible row"), State->AxisRows(), 0);
		const auto Add = State->Button(TEXT("Add all basis axes"));
		if (TestTrue(TEXT("Add all button exists"), Add.IsValid())) { Add->SimulateClick(); }
		TestTrue(TEXT("Add all changes the stored vector"), State->Object->StartingCoordinates.Num() > 0);
	}));
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, State]()
	{
		TestEqual(TEXT("Add all creates a visible row for every stored axis"), State->AxisRows(), State->Object->StartingCoordinates.Num());
		const auto Vector = FindVector(State->Details->GetRootTreeNodes());
		if (!TestTrue(TEXT("Refreshed vector node exists"), Vector.IsValid())) { return; }
		const auto Coordinate = Vector->CreatePropertyHandle()->GetChildHandle(FVectorND::CoordinatesPropertyName())->AsArray()->GetElement(0);
		TestTrue(TEXT("New coordinate handle is writable"), Coordinate->SetValue(3.f) == FPropertyAccess::Success);
		TestEqual(TEXT("New row edits the current storage"), State->Object->StartingCoordinates[0], 3.f);
	}));
	return true;
}
#endif
