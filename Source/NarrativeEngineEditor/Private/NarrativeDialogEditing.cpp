#include "NarrativeDialogEditing.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "NarrativeCoreData.h"
#include "NarrativeStaticData.h"

#define LOCTEXT_NAMESPACE "NarrativeDialogEditing"

namespace NarrativeDialogEditing
{
	namespace
	{
		bool IsTyped(const UNarrativeDialogDef& Card, const TSoftObjectPtr<UNarrativeBasisVector>& Axis)
		{
			return Card.Location.GetBasis().Contains(Axis);
		}

		void RemoveCoordinate(UNarrativeDialogDef& Card, const TSoftObjectPtr<UNarrativeBasisVector>& Axis)
		{
			const int32 Index = Card.Location.GetBasis().IndexOfByKey(Axis);
			if (Index != INDEX_NONE)
			{
				Card.Location.RemoveAxis(Index);
			}
		}
	}

	EAxisState GetAxisState(const UNarrativeDialogDef& Card, const TSoftObjectPtr<UNarrativeBasisVector>& Axis)
	{
		const bool bTyped = IsTyped(Card, Axis);
		const bool bMasked = Card.Mask.Contains(Axis);
		if (bTyped && bMasked) { return EAxisState::About; }
		if (bTyped) { return EAxisState::Ignored; }
		return bMasked ? EAxisState::Blank : EAxisState::Absent;
	}

	TOptional<float> GetCoordinate(const UNarrativeDialogDef& Card, const TSoftObjectPtr<UNarrativeBasisVector>& Axis)
	{
		return IsTyped(Card, Axis) ? TOptional<float>(Card.Location.GetCoordinate(Axis)) : TOptional<float>();
	}

	void SetAbout(UNarrativeDialogDef& Card, const TSoftObjectPtr<UNarrativeBasisVector>& Axis, bool bAbout)
	{
		if (bAbout)
		{
			Card.Mask.Add(Axis);
			Card.Location.SetCoordinate(Axis, Card.Location.GetCoordinate(Axis));
			return;
		}

		Card.Mask.Axes.Remove(Axis);
		RemoveCoordinate(Card, Axis);
	}

	void SetCoordinate(UNarrativeDialogDef& Card, const TSoftObjectPtr<UNarrativeBasisVector>& Axis, float Value)
	{
		Card.Location.SetCoordinate(Axis, Value);
		Card.Mask.Add(Axis);
	}

	void Tidy(UNarrativeDialogDef& Card)
	{
		for (const TSoftObjectPtr<UNarrativeBasisVector>& Axis : TArray<TSoftObjectPtr<UNarrativeBasisVector>>(Card.Location.GetBasis()))
		{
			if (!Card.Mask.Contains(Axis))
			{
				RemoveCoordinate(Card, Axis);
			}
		}
		for (const TSoftObjectPtr<UNarrativeBasisVector>& Axis : Card.Mask.Axes)
		{
			if (!IsTyped(Card, Axis))
			{
				Card.Location.SetCoordinate(Axis, 0.f);
			}
		}
	}

	bool NeedsTidy(const UNarrativeDialogDef& Card)
	{
		for (const TSoftObjectPtr<UNarrativeBasisVector>& Axis : Card.Location.GetBasis())
		{
			if (!Card.Mask.Contains(Axis)) { return true; }
		}
		for (const TSoftObjectPtr<UNarrativeBasisVector>& Axis : Card.Mask.Axes)
		{
			if (!IsTyped(Card, Axis)) { return true; }
		}
		return false;
	}

	FText AxisLabel(const TSoftObjectPtr<UNarrativeBasisVector>& Axis)
	{
		const UNarrativeBasisVector* Loaded = Axis.Get();
		return Loaded && !Loaded->ArchetypeTitle.IsEmpty() ? Loaded->ArchetypeTitle : FText::FromString(Axis.ToSoftObjectPath().GetAssetName());
	}

	TArray<TSoftObjectPtr<UNarrativeBasisVector>> GatherAxes(const UNarrativeDialogDef& Card)
	{
		TArray<TSoftObjectPtr<UNarrativeBasisVector>> Axes;
		TArray<FAssetData> Assets;
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get().GetAssetsByClass(
			UNarrativeBasisVector::StaticClass()->GetClassPathName(), Assets, true);
		for (const FAssetData& Asset : Assets)
		{
			Axes.AddUnique(TSoftObjectPtr<UNarrativeBasisVector>(Asset.GetSoftObjectPath()));
		}

		// An axis the card names but the registry lacks still needs a row, or it can't be removed.
		for (const TSoftObjectPtr<UNarrativeBasisVector>& Axis : Card.Location.GetBasis()) { Axes.AddUnique(Axis); }
		for (const TSoftObjectPtr<UNarrativeBasisVector>& Axis : Card.Mask.Axes) { Axes.AddUnique(Axis); }

		Axes.Sort([](const TSoftObjectPtr<UNarrativeBasisVector>& A, const TSoftObjectPtr<UNarrativeBasisVector>& B)
		{
			return AxisLabel(A).CompareTo(AxisLabel(B)) < 0;
		});
		return Axes;
	}

	TOptional<float> StrengthCap(float Sigma)
	{
		if (NarrativeLints::TierForReach(Sigma) == ENarrativeTier::Legendary)
		{
			return {};
		}
		return static_cast<float>(NarrativeLints::StrengthCap(Sigma));
	}

	FLinearColor TierColor(ENarrativeTier Tier)
	{
		switch (Tier)
		{
		case ENarrativeTier::Common: return FLinearColor::FromSRGBColor(FColor(0xA0, 0xA0, 0xA0));
		case ENarrativeTier::Uncommon: return FLinearColor::FromSRGBColor(FColor(0x3D, 0xBE, 0x4A));
		case ENarrativeTier::Rare: return FLinearColor::FromSRGBColor(FColor(0x3C, 0x8D, 0xFF));
		case ENarrativeTier::Epic: return FLinearColor::FromSRGBColor(FColor(0xA8, 0x55, 0xF7));
		default: return FLinearColor::FromSRGBColor(FColor(0xFF, 0x9F, 0x1C));
		}
	}

	FText TierName(ENarrativeTier Tier)
	{
		return FText::FromString(NarrativeLints::LexToString(Tier));
	}

	TConstArrayView<ENarrativeTier> TiersByReach()
	{
		static const ENarrativeTier Tiers[] = {ENarrativeTier::Legendary, ENarrativeTier::Epic, ENarrativeTier::Rare, ENarrativeTier::Uncommon, ENarrativeTier::Common};
		return Tiers;
	}

	FString CollapsePhrase(const FString& Phrase, int32 MaxLength)
	{
		FString Result;
		bool bWordStart = true;
		for (const TCHAR Char : Phrase)
		{
			// Apostrophes join a word ("I've" is one word), so they vanish without starting a new one.
			if (Char == TEXT('\'') || Char == TEXT('\u2019') || Char == TEXT('`')) { continue; }
			// Asset names stay ASCII; anything else (spaces, punctuation, wild card braces) splits words.
			const bool bAscii = Char < 128 && FChar::IsAlnum(Char);
			if (!bAscii)
			{
				bWordStart = true;
				continue;
			}
			Result.AppendChar(bWordStart ? FChar::ToUpper(Char) : Char);
			bWordStart = false;
			if (Result.Len() >= MaxLength) { break; }
		}
		return Result;
	}

	FString LabelFromFamily(const FGameplayTag& Family)
	{
		if (!Family.IsValid()) { return FString(); }
		FString Leaf = Family.GetTagName().ToString();
		int32 Dot = INDEX_NONE;
		if (Leaf.FindLastChar(TEXT('.'), Dot)) { Leaf.RightChopInline(Dot + 1); }
		return CollapsePhrase(Leaf);
	}

	FString CardAssetName(const FString& Phrase, const FGameplayTag& Family)
	{
		FString Name = TEXT("DA");
		const FString Label = LabelFromFamily(Family);
		const FString Collapsed = CollapsePhrase(Phrase);
		if (!Label.IsEmpty()) { Name += TEXT("_") + Label; }
		if (!Collapsed.IsEmpty()) { Name += TEXT("_") + Collapsed; }
		return Name == TEXT("DA") ? TEXT("DA_NewDialog") : Name;
	}
}

#undef LOCTEXT_NAMESPACE
