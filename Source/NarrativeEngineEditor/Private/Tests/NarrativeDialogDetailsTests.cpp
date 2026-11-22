#include "NarrativeDialogEditing.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Editor.h"
#include "Editor/Transactor.h"
#include "IDetailsView.h"
#include "IDetailTreeNode.h"
#include "Misc/AutomationTest.h"
#include "NarrativeCoreData.h"
#include "NarrativeLints.h"
#include "NarrativeDialogWizard.h"
#include "NarrativeStaticData.h"
#include "PropertyEditorModule.h"
#include "SNarrativeReachBar.h"
#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Misc/EngineVersionComparison.h"

namespace NarrativeDialogDetailsTests
{
	using namespace NarrativeDialogEditing;
	using FAxis = TSoftObjectPtr<UNarrativeBasisVector>;

	struct FAxes
	{
		UPackage* Package = CreatePackage(*(TEXT("/Temp/DialogDetails_") + FGuid::NewGuid().ToString(EGuidFormats::Digits)));
		TStrongObjectPtr<UNarrativeBasisVector> A{NewObject<UNarrativeBasisVector>(Package, TEXT("AxisA"), RF_Transient)};
		TStrongObjectPtr<UNarrativeBasisVector> B{NewObject<UNarrativeBasisVector>(Package, TEXT("AxisB"), RF_Transient)};
		TStrongObjectPtr<UNarrativeBasisVector> C{NewObject<UNarrativeBasisVector>(Package, TEXT("AxisC"), RF_Transient)};
		TStrongObjectPtr<UNarrativeBasisVector> D{NewObject<UNarrativeBasisVector>(Package, TEXT("AxisD"), RF_Transient)};
	};

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

	TSharedPtr<SButton> FindButton(const TArray<TSharedRef<IDetailTreeNode>>& Nodes, const FString& Text)
	{
		for (const TSharedRef<IDetailTreeNode>& Node : Nodes)
		{
			const FNodeWidgets Widgets = Node->CreateNodeWidgets();
			for (const TSharedPtr<SWidget>& Widget : {Widgets.NameWidget, Widgets.ValueWidget, Widgets.WholeRowWidget})
			{
				if (TSharedPtr<SButton> Found = FindButton(Widget, Text)) { return Found; }
			}
			TArray<TSharedRef<IDetailTreeNode>> Children;
			Node->GetChildren(Children, true);
			if (TSharedPtr<SButton> Found = FindButton(Children, Text)) { return Found; }
		}
		return nullptr;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNarrativeDialogMeaningRulesTest, "Narrative.DialogDetails.MeaningRules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNarrativeDialogMeaningRulesTest::RunTest(const FString& Parameters)
{
	using namespace NarrativeDialogDetailsTests;
	const FAxes Axes;
	const FAxis A(Axes.A.Get()), B(Axes.B.Get()), C(Axes.C.Get()), D(Axes.D.Get());
	TStrongObjectPtr<UNarrativeDialogDef> Card(NewObject<UNarrativeDialogDef>(GetTransientPackage()));

	SetCoordinate(*Card, A, 5.f);
	TestEqual(TEXT("Typing a coordinate makes the card about the axis"), GetAxisState(*Card, A), EAxisState::About);
	TestEqual(TEXT("with the typed value"), GetCoordinate(*Card, A), TOptional<float>(5.f));

	SetAbout(*Card, B, true);
	TestEqual(TEXT("Ticking an axis types it at 0, deliberately neutral"), GetCoordinate(*Card, B), TOptional<float>(0.f));
	TestEqual(TEXT("and masks it"), GetAxisState(*Card, B), EAxisState::About);

	SetAbout(*Card, A, false);
	TestEqual(TEXT("Unticking removes the axis from both fields"), GetAxisState(*Card, A), EAxisState::Absent);
	TestFalse(TEXT("its coordinate goes too"), GetCoordinate(*Card, A).IsSet());

	// Old cards: a coordinate outside the mask, and a masked axis with no coordinate.
	Card->Location.SetCoordinate(C, 3.f);
	Card->Mask.Add(D);
	TestEqual(TEXT("A coordinate outside the mask is ignored"), GetAxisState(*Card, C), EAxisState::Ignored);
	TestEqual(TEXT("A masked axis without a coordinate is blank"), GetAxisState(*Card, D), EAxisState::Blank);
	TestTrue(TEXT("so the card needs tidying"), NeedsTidy(*Card));

	SetAbout(*Card, C, true);
	TestEqual(TEXT("Ticking an ignored axis keeps its coordinate"), GetCoordinate(*Card, C), TOptional<float>(3.f));
	SetAbout(*Card, C, false);
	Card->Location.SetCoordinate(C, 3.f);

	Tidy(*Card);
	TestEqual(TEXT("Tidy drops the ignored coordinate"), GetAxisState(*Card, C), EAxisState::Absent);
	TestEqual(TEXT("and types the blank axis at 0"), GetCoordinate(*Card, D), TOptional<float>(0.f));
	TestFalse(TEXT("leaving Location and Mask on the same axes"), NeedsTidy(*Card));

	const TArray<FAxis> Gathered = GatherAxes(*Card);
	TestTrue(TEXT("Every axis the card names gets a row, registered or not"), Gathered.Contains(B) && Gathered.Contains(D));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNarrativeDialogReachTiersTest, "Narrative.DialogDetails.ReachTiers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNarrativeDialogReachTiersTest::RunTest(const FString& Parameters)
{
	using namespace NarrativeDialogEditing;
	TestEqual(TEXT("Epic at sigma 2 caps J at 0.9"), StrengthCap(2.f), TOptional<float>(0.9f));
	TestEqual(TEXT("Rare at sigma 3 caps J at 0.6"), StrengthCap(3.f), TOptional<float>(0.6f));
	TestTrue(TEXT("Rare at sigma 4 caps J at 1.8/4"), FMath::IsNearlyEqual(StrengthCap(4.f).Get(0.f), 0.45f));
	TestEqual(TEXT("Uncommon at sigma 5 caps J at 0.35"), StrengthCap(5.f), TOptional<float>(0.35f));
	TestEqual(TEXT("Common at sigma 8 caps J at 0.2"), StrengthCap(8.f), TOptional<float>(0.2f));
	TestFalse(TEXT("Legendary is uncapped"), StrengthCap(1.5f).IsSet());

	double Previous = 0.0;
	for (const ENarrativeTier Tier : TiersByReach())
	{
		const double Floor = NarrativeLints::TierReachFloor(Tier);
		TestEqual(*FString::Printf(TEXT("%s's floor is in %s"), NarrativeLints::LexToString(Tier), NarrativeLints::LexToString(Tier)), NarrativeLints::TierForReach(Floor), Tier);
		TestTrue(TEXT("Tiers run from narrowest to widest reach"), Floor > Previous);
		Previous = Floor;
	}

	const TSharedRef<SNarrativeReachBar> Bar = SNew(SNarrativeReachBar);
	TestEqual(TEXT("The bar's left edge is its narrowest reach"), Bar->SigmaAt(0.f, 100.f), SNarrativeReachBar::MinSigma);
	TestEqual(TEXT("its right edge the widest"), Bar->SigmaAt(100.f, 100.f), SNarrativeReachBar::MaxSigma);
	TestEqual(TEXT("and it rounds to 0.1"), Bar->SigmaAt(50.f, 100.f), 5.5f);
	return true;
}

// Walks the details panel with IDetailsView::GetRootTreeNodes(), which only exists on UE6; on 5.8 this needs
// porting to IPropertyRowGenerator, so it is compiled out there.
#if !UE_VERSION_OLDER_THAN(6, 0, 0)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNarrativeDialogDetailsViewTest, "Narrative.DialogDetails.View",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNarrativeDialogDetailsViewTest::RunTest(const FString& Parameters)
{
	using namespace NarrativeDialogDetailsTests;
	const FAxes Axes;
	TStrongObjectPtr<UNarrativeDialogDef> Card(NewObject<UNarrativeDialogDef>(GetTransientPackage(), NAME_None, RF_Transactional));
	Card->SpaceSigma = 3.f;
	Card->Strength = 4.f;
	Card->Location.SetCoordinate(FAxis(Axes.C.Get()), 3.f);

	FDetailsViewArgs Args;
	Args.bUpdatesFromSelection = false;
	const TSharedRef<IDetailsView> Details = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor").CreateDetailView(Args);
	Details->SetObject(Card.Get());

	const TSharedPtr<SButton> Clamp = FindButton(Details->GetRootTreeNodes(), TEXT("Clamp"));
	if (!TestTrue(TEXT("An over-cap card offers Clamp"), Clamp.IsValid())) { return false; }
	Clamp->SimulateClick();
	TestTrue(TEXT("Clamp lowers J to the cap at its reach"), FMath::IsNearlyEqual(Card->Strength, 0.6f));
	const int32 Steps = GEditor->Trans->GetQueueLength();

	// A field losing focus while the panel rebuilds commits again: a commit that changes nothing records nothing.
	Clamp->SimulateClick();
	TestEqual(TEXT("A commit that changes nothing adds no undo step"), GEditor->Trans->GetQueueLength(), Steps);

	// A row from before a refresh can still commit (the crash: its property handles were already destroyed).
	// Edits go to the card itself, so the commit lands safely and undoes like any other.
	Details->ForceRefresh();
	Card->Strength = 5.f;
	Clamp->SimulateClick();
	TestTrue(TEXT("A row from before a refresh edits the card safely"), FMath::IsNearlyEqual(Card->Strength, 0.6f));
	GEditor->UndoTransaction();
	TestTrue(TEXT("and its edit undoes"), FMath::IsNearlyEqual(Card->Strength, 5.f));
	Card->Strength = 0.6f;

	GEditor->UndoTransaction();
	TestTrue(TEXT("Clamp undoes as one step"), FMath::IsNearlyEqual(Card->Strength, 4.f));

	const TSharedPtr<SButton> TidyButton = FindButton(Details->GetRootTreeNodes(), TEXT("Tidy"));
	if (!TestTrue(TEXT("A card with an ignored coordinate offers Tidy"), TidyButton.IsValid())) { return false; }
	TidyButton->SimulateClick();
	TestEqual(TEXT("Tidy edits the card"), Card->Location.Num(), 0);
	GEditor->UndoTransaction();
	TestEqual(TEXT("Tidy undoes as one step"), Card->Location.Num(), 1);
	return true;
}
#endif // !UE_VERSION_OLDER_THAN(6, 0, 0)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNarrativeDialogWizardNamingTest, "Narrative.DialogDetails.WizardNaming",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNarrativeDialogWizardNamingTest::RunTest(const FString& Parameters)
{
	using namespace NarrativeDialogEditing;
	TestEqual(TEXT("Apostrophes join a word"), CollapsePhrase(TEXT("I've prepared you!")), FString(TEXT("IvePreparedYou")));
	TestEqual(TEXT("Hyphens and spaces split words"), CollapsePhrase(TEXT("good-for-nothing  wretch")), FString(TEXT("GoodForNothingWretch")));
	TestEqual(TEXT("Wild card braces split words"), CollapsePhrase(TEXT("Take this {Item.Name}.")), FString(TEXT("TakeThisItemName")));
	TestEqual(TEXT("Typed capitals are kept"), CollapsePhrase(TEXT("the RICK way")), FString(TEXT("TheRICKWay")));
	TestEqual(TEXT("Non-ASCII is dropped"), CollapsePhrase(TEXT("caf\u00E9 ol\u00E9")), FString(TEXT("CafOl")));
	TestEqual(TEXT("Collapsed phrases are capped"), CollapsePhrase(TEXT("a b c d e f"), 3), FString(TEXT("ABC")));
	TestEqual(TEXT("No family, no label"), LabelFromFamily(FGameplayTag()), FString());
	TestEqual(TEXT("Nothing usable still names the card"), CardAssetName(TEXT("?!"), FGameplayTag()), FString(TEXT("DA_NewDialog")));
	TestEqual(TEXT("No family drops the label"), CardAssetName(TEXT("Hello there"), FGameplayTag()), FString(TEXT("DA_HelloThere")));

	const FGameplayTag Give = FGameplayTag::RequestGameplayTag(TEXT("Dialog.Family.Item.Give"), false);
	if (!Give.IsValid())
	{
		AddWarning(TEXT("Dialog.Family.Item.Give is not registered; the family half of the naming test is skipped."));
		return true;
	}
	TestEqual(TEXT("Label is the family's leaf"), LabelFromFamily(Give), FString(TEXT("Give")));
	TestEqual(TEXT("Asset name is DA_Label_Phrase"), CardAssetName(TEXT("I've prepared you"), Give), FString(TEXT("DA_Give_IvePreparedYou")));

	const TStrongObjectPtr<UNarrativeDialogWizardInput> Input(NewObject<UNarrativeDialogWizardInput>());
	Input->Phrase = FText::FromString(TEXT("I've prepared you"));
	Input->Family = Give;
	Input->SpaceSigma = 9.f;
	Input->Strength = 1000.f;
	Input->Stance = -1;
	Input->Cooldown = 12.f;
	const TStrongObjectPtr<UNarrativeDialogDef> Card(NewObject<UNarrativeDialogDef>());
	Input->Apply(*Card);
	TestEqual(TEXT("Apply sets the phrase"), Card->Phrase.ToString(), Input->Phrase.ToString());
	TestEqual(TEXT("Apply infers the label"), Card->Label.ToString(), FString(TEXT("Give")));
	TestEqual(TEXT("Apply sets the family"), Card->Family, Give);
	TestEqual(TEXT("Apply sets the reach"), Card->SpaceSigma, 9.f);
	const TOptional<float> Cap = StrengthCap(9.f);
	TestEqual(TEXT("Apply holds J to the reach's cap"), Card->Strength, Cap.IsSet() ? Cap.GetValue() : 1000.f);
	TestEqual(TEXT("Apply sets the stance"), Card->Stance, -1);
	TestEqual(TEXT("Apply sets the cooldown"), Card->Cooldown, 12.f);
	return true;
}
#endif
