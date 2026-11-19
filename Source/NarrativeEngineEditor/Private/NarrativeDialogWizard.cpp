#include "NarrativeDialogWizard.h"

#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "Framework/Application/SlateApplication.h"
#include "IDetailCustomization.h"
#include "IDetailsView.h"
#include "Input/Reply.h"
#include "InputCoreTypes.h"
#include "NarrativeDialogEditing.h"
#include "NarrativeLints.h"
#include "NarrativeStaticData.h"
#include "PropertyEditorModule.h"
#include "SNarrativeReachField.h"
#include "Styling/AppStyle.h"
#include "UObject/StrongObjectPtr.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Input/SSegmentedControl.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SWindow.h"
#include "Widgets/Text/STextBlock.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NarrativeDialogWizard)

#define LOCTEXT_NAMESPACE "NarrativeDialogWizard"

FString UNarrativeDialogWizardInput::AssetName() const
{
	return NarrativeDialogEditing::CardAssetName(Phrase.ToString(), Family);
}

void UNarrativeDialogWizardInput::Apply(UNarrativeDialogDef& Card) const
{
	Card.Phrase = Phrase;
	Card.Family = Family;
	Card.Label = FText::FromString(NarrativeDialogEditing::LabelFromFamily(Family));
	Card.SpaceSigma = SpaceSigma;
	const TOptional<float> Cap = NarrativeDialogEditing::StrengthCap(SpaceSigma);
	Card.Strength = FMath::Max(0.f, Cap.IsSet() ? FMath::Min(Strength, Cap.GetValue()) : Strength);
	Card.Stance = Stance < 0 ? -1 : 1;
	Card.Cooldown = FMath::Max(0.f, Cooldown);
}

namespace
{
	/**
	 * The wizard's one category, in the card details' own vocabulary: reach as the rarity bar, strength held to
	 * its cap, stance as friend or foe. Phrase, family and cooldown keep their stock rows (the family row is the
	 * usual tag picker). The input is transient and outside the undo buffer, so widgets write it directly.
	 */
	class FWizardDetails : public IDetailCustomization
	{
	public:
		FWizardDetails(FSimpleDelegate InOnAccept, TSharedRef<TSharedPtr<SWidget>> InFirstFocus)
			: OnAccept(MoveTemp(InOnAccept)), FirstFocus(MoveTemp(InFirstFocus)) {}

		virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override
		{
			TArray<TWeakObjectPtr<UObject>> Objects;
			DetailBuilder.GetObjectsBeingCustomized(Objects);
			const TWeakObjectPtr<UNarrativeDialogWizardInput> Input = Objects.Num() == 1 ? Cast<UNarrativeDialogWizardInput>(Objects[0].Get()) : nullptr;
			if (!Input.IsValid()) { return; }

			IDetailCategoryBuilder& Category = DetailBuilder.EditCategory("New Card", LOCTEXT("NewCard", "New Card"), ECategoryPriority::Important);
			const TSharedRef<IPropertyHandle> SigmaHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UNarrativeDialogWizardInput, SpaceSigma));
			const TSharedRef<IPropertyHandle> StrengthHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UNarrativeDialogWizardInput, Strength));
			const TSharedRef<IPropertyHandle> StanceHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UNarrativeDialogWizardInput, Stance));
			const TSharedRef<IPropertyHandle> CooldownHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UNarrativeDialogWizardInput, Cooldown));
			DetailBuilder.HideProperty(SigmaHandle);
			DetailBuilder.HideProperty(StrengthHandle);
			DetailBuilder.HideProperty(StanceHandle);
			DetailBuilder.HideProperty(CooldownHandle);

			// Phrase writes as it is typed, so the name preview and Create follow every keystroke; Enter creates.
			const TSharedRef<IPropertyHandle> PhraseHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UNarrativeDialogWizardInput, Phrase));
			DetailBuilder.HideProperty(PhraseHandle);
			const FSimpleDelegate Accept = OnAccept;
			TSharedRef<SEditableTextBox> PhraseBox = SNew(SEditableTextBox)
				.Font(IDetailLayoutBuilder::GetDetailFont())
				.HintText(LOCTEXT("PhraseHint", "What is said, in full"))
				.Text_Lambda([Input]() { return Input.IsValid() ? Input->Phrase : FText::GetEmpty(); })
				.OnTextChanged_Lambda([Input](const FText& Text) { if (Input.IsValid()) { Input->Phrase = Text; } })
				.OnTextCommitted_Lambda([Input, Accept](const FText& Text, ETextCommit::Type Commit)
				{
					if (Input.IsValid()) { Input->Phrase = Text; }
					if (Commit == ETextCommit::OnEnter) { Accept.ExecuteIfBound(); }
				});
			*FirstFocus = PhraseBox;
			Category.AddCustomRow(LOCTEXT("Phrase", "Phrase"))
			.NameContent()
			[
				SNew(STextBlock)
				.Font(IDetailLayoutBuilder::GetDetailFont())
				.Text(LOCTEXT("PhraseName", "Phrase"))
				.ToolTipText(PhraseHandle->GetToolTipText())
			]
			.ValueContent()
			.MinDesiredWidth(280.f)
			[
				PhraseBox
			];
			Category.AddProperty(DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UNarrativeDialogWizardInput, Family)));

			auto Sigma = [Input]() { return Input.IsValid() ? Input->SpaceSigma : 5.f; };
			auto Cap = [Sigma]() { return NarrativeDialogEditing::StrengthCap(Sigma()); };
			auto ClampStrength = [Input, Cap]()
			{
				if (Input.IsValid() && Cap().IsSet()) { Input->Strength = FMath::Min(Input->Strength, Cap().GetValue()); }
			};

			Category.AddCustomRow(LOCTEXT("Reach", "Reach"))
			.NameContent()
			[
				SNew(STextBlock)
				.Font(IDetailLayoutBuilder::GetDetailFont())
				.Text(LOCTEXT("ReachName", "Reach (\u03C3)"))
				.ToolTipText(SigmaHandle->GetToolTipText())
			]
			.ValueContent()
			.MinDesiredWidth(280.f)
			[
				SNew(SNarrativeReachField)
				.Sigma_Lambda(Sigma)
				.OnReachChanged_Lambda([Input, ClampStrength](float Value, bool)
				{
					if (!Input.IsValid()) { return; }
					Input->SpaceSigma = FMath::Max(0.1f, Value);
					// A wider reach lowers the cap; keep J legal rather than hand the author an L6 on Create.
					ClampStrength();
				})
			];

			Category.AddCustomRow(LOCTEXT("Strength", "Strength"))
			.NameContent()
			[
				SNew(STextBlock)
				.Font(IDetailLayoutBuilder::GetDetailFont())
				.Text(LOCTEXT("StrengthName", "Strength (J)"))
				.ToolTipText(StrengthHandle->GetToolTipText())
			]
			.ValueContent()
			.MinDesiredWidth(280.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SBox).WidthOverride(72.f)
					[
						SNew(SNumericEntryBox<float>)
						.Font(IDetailLayoutBuilder::GetDetailFont())
						.AllowSpin(true)
						.MinValue(TOptional<float>(0.f))
						.MaxValue_Lambda(Cap)
						.MinSliderValue(TOptional<float>(0.f))
						.MaxSliderValue_Lambda([Cap]() { return TOptional<float>(Cap().Get(2.f)); })
						.Value_Lambda([Input]() { return TOptional<float>(Input.IsValid() ? Input->Strength : 0.f); })
						.OnValueChanged_Lambda([Input](float Value) { if (Input.IsValid()) { Input->Strength = FMath::Max(0.f, Value); } })
						.OnValueCommitted_Lambda([Input](float Value, ETextCommit::Type) { if (Input.IsValid()) { Input->Strength = FMath::Max(0.f, Value); } })
					]
				]
				+ SHorizontalBox::Slot().FillWidth(1.f).Padding(6.f, 0.f).VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Font(IDetailLayoutBuilder::GetDetailFont())
					.ColorAndOpacity(FSlateColor::UseSubduedForeground())
					.Text_Lambda([Input, Sigma, Cap]()
					{
						if (!Input.IsValid() || Input->Strength <= 0.f) { return LOCTEXT("NotADeed", "0: saying it is not a deed."); }
						const FText Tier = NarrativeDialogEditing::TierName(NarrativeLints::TierForReach(Sigma()));
						return Cap().IsSet()
							? FText::Format(LOCTEXT("WithinCap", "of {0} at {1}"), SNarrativeReachField::Number(Cap().GetValue()), Tier)
							: LOCTEXT("Uncapped", "Legendary: uncapped");
					})
				]
			];

			Category.AddCustomRow(LOCTEXT("Stance", "Stance"))
			.NameContent()
			[
				SNew(STextBlock)
				.Font(IDetailLayoutBuilder::GetDetailFont())
				.Text(LOCTEXT("StanceName", "Stance (h)"))
				.ToolTipText(StanceHandle->GetToolTipText())
			]
			.ValueContent()
			[
				SNew(SSegmentedControl<int32>)
				.Value_Lambda([Input]() { return Input.IsValid() && Input->Stance < 0 ? -1 : 1; })
				.OnValueChanged_Lambda([Input](int32 Value) { if (Input.IsValid()) { Input->Stance = Value; } })
				+ SSegmentedControl<int32>::Slot(1)
					.Text(LOCTEXT("Friend", "Friend (+1)"))
					.ToolTip(LOCTEXT("FriendTip", "What is done to friends."))
				+ SSegmentedControl<int32>::Slot(-1)
					.Text(LOCTEXT("Foe", "Foe (\u22121)"))
					.ToolTip(LOCTEXT("FoeTip", "What is done to foes."))
			];

			Category.AddProperty(CooldownHandle);
		}

	private:
		FSimpleDelegate OnAccept;
		TSharedRef<TSharedPtr<SWidget>> FirstFocus;
	};

	/** Answers carried from one card to the next, so a run of cards in one family takes only phrases. */
	struct FCarryOver
	{
		FGameplayTag Family;
		float SpaceSigma = 5.f;
		float Strength = 0.f;
		int32 Stance = 1;
		float Cooldown = 0.f;
	};
	FCarryOver& CarryOver()
	{
		static FCarryOver Value;
		return Value;
	}

	/** Enter creates and Escape cancels from anywhere in the window not already using the key. */
	class SWizardKeys : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SWizardKeys) {}
			SLATE_DEFAULT_SLOT(FArguments, Content)
			SLATE_EVENT(FSimpleDelegate, OnAccept)
			SLATE_EVENT(FSimpleDelegate, OnCancel)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			OnAccept = InArgs._OnAccept;
			OnCancel = InArgs._OnCancel;
			ChildSlot[InArgs._Content.Widget];
		}
		virtual bool SupportsKeyboardFocus() const override { return true; }
		virtual FReply OnKeyDown(const FGeometry&, const FKeyEvent& Event) override
		{
			if (Event.GetKey() == EKeys::Escape) { OnCancel.ExecuteIfBound(); return FReply::Handled(); }
			if (Event.GetKey() == EKeys::Enter) { OnAccept.ExecuteIfBound(); return FReply::Handled(); }
			return FReply::Unhandled();
		}

	private:
		FSimpleDelegate OnAccept;
		FSimpleDelegate OnCancel;
	};
}

bool NarrativeDialogWizard::Run(const TSharedPtr<SWidget>& Parent, UNarrativeDialogWizardInput& Input)
{
	const FCarryOver& Previous = CarryOver();
	Input.Family = Previous.Family;
	Input.SpaceSigma = Previous.SpaceSigma;
	Input.Strength = Previous.Strength;
	Input.Stance = Previous.Stance;
	Input.Cooldown = Previous.Cooldown;

	const TWeakObjectPtr<UNarrativeDialogWizardInput> WeakInput = &Input;
	auto CanCreate = [WeakInput]() { return WeakInput.IsValid() && !WeakInput->Phrase.IsEmptyOrWhitespace(); };

	const TSharedRef<bool> bCreate = MakeShared<bool>(false);
	TSharedRef<SWindow> Window = SNew(SWindow)
		.Title(LOCTEXT("Title", "New Dialog Card"))
		.SizingRule(ESizingRule::UserSized)
		.ClientSize(FVector2D(560.f, 330.f))
		.SupportsMinimize(false)
		.SupportsMaximize(false);
	const TWeakPtr<SWindow> WeakWindow = Window;
	auto Accept = [WeakWindow, CanCreate, bCreate]()
	{
		if (!CanCreate()) { return; }
		*bCreate = true;
		if (const TSharedPtr<SWindow> Pinned = WeakWindow.Pin()) { Pinned->RequestDestroyWindow(); }
	};
	auto Cancel = [WeakWindow]()
	{
		if (const TSharedPtr<SWindow> Pinned = WeakWindow.Pin()) { Pinned->RequestDestroyWindow(); }
	};

	FPropertyEditorModule& PropertyEditor = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs Args;
	Args.bAllowSearch = false;
	Args.bHideSelectionTip = true;
	Args.bShowOptions = false;
	Args.bShowPropertyMatrixButton = false;
	Args.bAllowFavoriteSystem = false;
	Args.bShowScrollBar = false;
	Args.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	const TSharedRef<IDetailsView> Details = PropertyEditor.CreateDetailView(Args);
	const FSimpleDelegate AcceptDelegate = FSimpleDelegate::CreateLambda(Accept);
	const TSharedRef<TSharedPtr<SWidget>> FirstFocus = MakeShared<TSharedPtr<SWidget>>();
	Details->RegisterInstancedCustomPropertyLayout(UNarrativeDialogWizardInput::StaticClass(),
		FOnGetDetailCustomizationInstance::CreateLambda([AcceptDelegate, FirstFocus] { return MakeShared<FWizardDetails>(AcceptDelegate, FirstFocus); }));
	Details->SetObject(&Input);

	Window->SetContent(
		SNew(SWizardKeys)
		.OnAccept_Lambda(Accept)
		.OnCancel_Lambda(Cancel)
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("Brushes.Panel"))
			.Padding(8.f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().FillHeight(1.f)
				[
					Details
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 8.f, 0.f, 0.f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.ColorAndOpacity(FSlateColor::UseSubduedForeground())
						.ToolTipText(LOCTEXT("NameTip", "The asset name: DA_, the family's leaf as the label, then the phrase collapsed. A taken name gets a number. It is filed in the folder Make New was opened on."))
						.Text_Lambda([WeakInput, CanCreate]()
						{
							if (!CanCreate()) { return LOCTEXT("NeedPhrase", "Type the phrase to name the card."); }
							return FText::Format(LOCTEXT("WillCreate", "Creates {0}"), FText::FromString(WeakInput->AssetName()));
						})
					]
					+ SHorizontalBox::Slot().AutoWidth().Padding(4.f, 0.f)
					[
						SNew(SButton)
						.ButtonStyle(FAppStyle::Get(), "PrimaryButton")
						.Text(LOCTEXT("Create", "Create"))
						.IsEnabled_Lambda(CanCreate)
						.OnClicked_Lambda([Accept]() { Accept(); return FReply::Handled(); })
					]
					+ SHorizontalBox::Slot().AutoWidth()
					[
						SNew(SButton)
						.Text(LOCTEXT("Cancel", "Cancel"))
						.OnClicked_Lambda([Cancel]() { Cancel(); return FReply::Handled(); })
					]
				]
			]
		]);

	if (FirstFocus->IsValid()) { Window->SetWidgetToFocusOnActivate(*FirstFocus); }
	FSlateApplication::Get().AddModalWindow(Window, Parent);

	if (*bCreate)
	{
		FCarryOver& Next = CarryOver();
		Next.Family = Input.Family;
		Next.SpaceSigma = Input.SpaceSigma;
		Next.Strength = Input.Strength;
		Next.Stance = Input.Stance;
		Next.Cooldown = Input.Cooldown;
	}
	return *bCreate;
}

#undef LOCTEXT_NAMESPACE
