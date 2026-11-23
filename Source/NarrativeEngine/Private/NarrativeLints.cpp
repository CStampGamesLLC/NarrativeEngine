#include "NarrativeLints.h"

#include "NarrativeStaticData.h"

FString FNarrativeLintIssue::ToString() const
{
	return FString::Printf(TEXT("%s %s [%s] %s"),
		Severity == ENarrativeLintSeverity::Error ? TEXT("Error") : TEXT("Warning"), *Rule, *Asset, *Message);
}

namespace NarrativeLints
{
	namespace
	{
		void Add(TArray<FNarrativeLintIssue>& OutIssues, const TCHAR* Rule, ENarrativeLintSeverity Severity, const UObject& Asset, FString Message)
		{
			FNarrativeLintIssue& Issue = OutIssues.AddDefaulted_GetRef();
			Issue.Rule = Rule;
			Issue.Severity = Severity;
			Issue.Asset = Asset.GetPathName();
			Issue.Message = MoveTemp(Message);
		}

		FString AxisName(const TSoftObjectPtr<UNarrativeBasisVector>& Axis)
		{
			return Axis.ToSoftObjectPath().GetAssetName();
		}

		/** L1 and L13 on one located, masked thing: a card, or a character's start. */
		void LintMaskedVector(const UObject& Asset, const FVectorND& Vector, const FNarrativeMask& Mask, TArray<FNarrativeLintIssue>& OutIssues)
		{
			// A null axis is never read; a repeated one counts twice in every cosine.
			TSet<FSoftObjectPath> Seen;
			for (const TSoftObjectPtr<UNarrativeBasisVector>& Axis : Mask.Axes)
			{
				if (Axis.IsNull())
				{
					Add(OutIssues, TEXT("L13"), ENarrativeLintSeverity::Error, Asset, TEXT("Mask has an empty axis entry: remove it."));
				}
				else if (Seen.Contains(Axis.ToSoftObjectPath()))
				{
					Add(OutIssues, TEXT("L13"), ENarrativeLintSeverity::Error, Asset,
						FString::Printf(TEXT("Mask lists %s twice: it would weigh double in every affinity."), *AxisName(Axis)));
				}
				else
				{
					Seen.Add(Axis.ToSoftObjectPath());
				}
			}

			// CO-C14: an axis the mask reads must have a typed coordinate, or every gather reads an accidental 0 there.
			for (const TSoftObjectPtr<UNarrativeBasisVector>& Axis : Mask.Axes)
			{
				if (!Axis.IsNull() && !Vector.GetBasis().Contains(Axis))
				{
					Add(OutIssues, TEXT("C14"), ENarrativeLintSeverity::Error, Asset,
						FString::Printf(TEXT("The mask reads %s, which has no coordinate: type one (0 if neutral) or drop the axis."), *AxisName(Axis)));
				}
			}

			if (Mask.IsEmpty())
			{
				Add(OutIssues, TEXT("L13"), ENarrativeLintSeverity::Error, Asset,
					TEXT("Mask is empty: nothing says which axes this is about. Run the mask migration or author one."));
			}
			else if (Mask.Num() > MaxMaskAxes)
			{
				Add(OutIssues, TEXT("L13"), ENarrativeLintSeverity::Warning, Asset,
					FString::Printf(TEXT("Mask has %d axes; more than %d brings back the uncertainty penalty. Decide which matter."), Mask.Num(), MaxMaskAxes));
			}

			// Authored data: basis and coordinates may disagree on length, so walk only what both hold.
			const int32 Count = FMath::Min(Vector.Num(), Vector.GetBasis().Num());
			for (int32 Index = 0; Index < Count; ++Index)
			{
				const TSoftObjectPtr<UNarrativeBasisVector>& Axis = Vector.GetBasis()[Index];
				if (Mask.IsEmpty() || Mask.Contains(Axis))
				{
					continue;
				}
				if (Vector[Index] != 0.f)
				{
					// L1: outside the mask an axis is irrelevant, so a value typed there is never read.
					Add(OutIssues, TEXT("L1"), ENarrativeLintSeverity::Warning, Asset,
						FString::Printf(TEXT("%s is %.2f but outside the mask, so the model ignores it."), *AxisName(Axis), Vector[Index]));
				}
				else
				{
					// L13: a typed 0 outside the mask could mean "deliberately neutral" or "not about this".
					Add(OutIssues, TEXT("L13"), ENarrativeLintSeverity::Warning, Asset,
						FString::Printf(TEXT("%s is a typed 0 outside the mask: neutral (add it to the mask) or irrelevant (remove it)?"), *AxisName(Axis)));
				}
			}
		}
	}

	double TierReachFloor(ENarrativeTier Tier)
	{
		switch (Tier)
		{
		case ENarrativeTier::Common: return 8.0;
		case ENarrativeTier::Uncommon: return 5.0;
		case ENarrativeTier::Rare: return 3.0;
		case ENarrativeTier::Epic: return 2.0;
		default: return MinReach;
		}
	}

	ENarrativeTier TierForReach(double Sigma)
	{
		for (const ENarrativeTier Tier : {ENarrativeTier::Common, ENarrativeTier::Uncommon, ENarrativeTier::Rare, ENarrativeTier::Epic})
		{
			if (Sigma >= TierReachFloor(Tier)) { return Tier; }
		}
		return ENarrativeTier::Legendary;
	}

	const TCHAR* LexToString(ENarrativeTier Tier)
	{
		switch (Tier)
		{
		case ENarrativeTier::Common: return TEXT("Common");
		case ENarrativeTier::Uncommon: return TEXT("Uncommon");
		case ENarrativeTier::Rare: return TEXT("Rare");
		case ENarrativeTier::Epic: return TEXT("Epic");
		default: return TEXT("Legendary");
		}
	}

	double StrengthCap(double Sigma)
	{
		double TierCap = 0.0;
		switch (TierForReach(Sigma))
		{
		case ENarrativeTier::Common: TierCap = 0.2; break;
		case ENarrativeTier::Uncommon: TierCap = 0.35; break;
		case ENarrativeTier::Rare: TierCap = 0.6; break;
		case ENarrativeTier::Epic: TierCap = 0.9; break;
		default: return TNumericLimits<double>::Max();
		}
		return FMath::Min(TierCap, 1.8 / Sigma);
	}

	FTierInfo TierOf(double Sigma)
	{
		FTierInfo Info;
		Info.Tier = TierForReach(Sigma);
		Info.Effort = static_cast<int32>(Info.Tier) + 1;
		Info.StrengthCap = StrengthCap(FMath::Max(Sigma, UE_DOUBLE_SMALL_NUMBER));
		return Info;
	}

	bool AreStanceBandsValid(const FNarrativeMindSheet& Sheet)
	{
		return Sheet.HostileThreshold + Sheet.Hysteresis < Sheet.FriendlyThreshold - Sheet.Hysteresis;
	}

	void LintMechanics(const UNarrativeDialogDef& Card, TArray<FNarrativeLintIssue>& OutIssues)
	{
		static const FString MechanicPrefix(TEXT("Dialog.Mechanic."));
		// The family each mechanic's card belongs to; the two tag trees are named independently.
		static const TMap<FString, FString> MechanicFamily = {
			{TEXT("EndDialog"), TEXT("Dialog.Family.Depart")},
			{TEXT("GiveItem"), TEXT("Dialog.Family.Item.Give")},
			{TEXT("Recruit"), TEXT("Dialog.Family.Recruit")},
			{TEXT("RequestItem"), TEXT("Dialog.Family.Item.Request")},
			{TEXT("Duel"), TEXT("Dialog.Family.Duel")},
			{TEXT("EndDuel"), TEXT("Dialog.Family.Duel.Yield")},
		};
		for (const FGameplayTag& Tag : Card.AbilityTags)
		{
			const FString TagName = Tag.ToString();
			if (!TagName.StartsWith(MechanicPrefix))
			{
				continue;
			}

			const FString Mechanic = TagName.RightChop(MechanicPrefix.Len());
			const FString* FoundFamily = MechanicFamily.Find(Mechanic);
			if (!FoundFamily)
			{
				Add(OutIssues, TEXT("L14"), ENarrativeLintSeverity::Warning, Card,
					FString::Printf(TEXT("Runs %s, which has no family on record: add it to L14's mechanic table."), *TagName));
				continue;
			}

			const FString& Expected = *FoundFamily;
			if (!Card.Family.IsValid())
			{
				Add(OutIssues, TEXT("L14"), ENarrativeLintSeverity::Warning, Card,
					FString::Printf(TEXT("Runs %s but has no family: confirm it is a %s card, or remove a copied tag."), *TagName, *Expected));
			}
			else if (Card.Family.ToString() != Expected)
			{
				Add(OutIssues, TEXT("L14"), ENarrativeLintSeverity::Error, Card,
					FString::Printf(TEXT("Runs %s but means %s: a copied tag? A %s card belongs to %s."),
						*TagName, *Card.Family.ToString(), *Mechanic, *Expected));
			}

			if (Mechanic == TEXT("GiveItem"))
			{
				if (Card.Stance < 0)
				{
					Add(OutIssues, TEXT("L14"), ENarrativeLintSeverity::Error, Card, TEXT("A gift with h = -1: giving is done to friends."));
				}
				// Pattern 11 (P0-L): a gift's J is valued on its transfer, once, as the line's child; the line itself carries 0.
				if (Card.Strength <= 0.f)
				{
					Add(OutIssues, TEXT("L14"), ENarrativeLintSeverity::Warning, Card,
						TEXT("A gift with J = 0: its transfer carries the card's J (Pattern 11), so this gift is worth nothing to the receiver."));
				}
			}
			else if (Mechanic == TEXT("Recruit") && Card.Strength > 0.f)
			{
				Add(OutIssues, TEXT("L14"), ENarrativeLintSeverity::Warning, Card,
					FString::Printf(TEXT("J %.2f on the ask: an invitation is a request (J = 0); its ability runs the join."), Card.Strength));
			}
		}
	}

	FString FamilyHash(const FGameplayTag& Family, TConstArrayView<const UNarrativeDialogDef*> Cards)
	{
		TArray<const UNarrativeDialogDef*> Members;
		for (const UNarrativeDialogDef* Card : Cards)
		{
			if (Card && Card->Family == Family)
			{
				Members.Add(Card);
			}
		}
		Members.Sort([](const UNarrativeDialogDef& A, const UNarrativeDialogDef& B) { return A.GetPathName() < B.GetPathName(); });
		FString Signature = Family.ToString();
		for (const UNarrativeDialogDef* Card : Members)
		{
			Signature += FString::Printf(TEXT("|%s|%s|%.4f|%.4f|%d|%s"), *Card->GetPathName(), *Card->Phrase.ToString(), Card->SpaceSigma, Card->Strength, Card->Stance,
				*Card->AbilityTags.ToStringSimple());
			for (const TSoftObjectPtr<UNarrativeBasisVector>& Axis : Card->Mask.Axes)
			{
				Signature += FString::Printf(TEXT(",%s=%.3f"), *AxisName(Axis), Card->Location.GetCoordinate(Axis));
			}
		}
		return FString::Printf(TEXT("%08x"), FCrc::StrCrc32(*Signature));
	}

	void LintCatalog(TConstArrayView<const UNarrativeDialogDef*> Cards, TArray<FNarrativeLintIssue>& OutIssues)
	{
		TMap<FGameplayTag, TArray<const UNarrativeDialogDef*>> Families;
		for (const UNarrativeDialogDef* Card : Cards)
		{
			if (Card && Card->Family.IsValid())
			{
				Families.FindOrAdd(Card->Family).Add(Card);
			}
		}
		for (TPair<FGameplayTag, TArray<const UNarrativeDialogDef*>>& Pair : Families)
		{
			Pair.Value.Sort([](const UNarrativeDialogDef& A, const UNarrativeDialogDef& B) { return A.GetPathName() < B.GetPathName(); });
		}

		auto SameMask = [](const FNarrativeMask& A, const FNarrativeMask& B)
		{
			if (A.Num() != B.Num())
			{
				return false;
			}
			for (const TSoftObjectPtr<UNarrativeBasisVector>& Axis : A.Axes)
			{
				if (!B.Contains(Axis))
				{
					return false;
				}
			}
			return true;
		};
		auto MaskDistance = [](const UNarrativeDialogDef& A, const UNarrativeDialogDef& B)
		{
			double Sum = 0.0;
			for (const TSoftObjectPtr<UNarrativeBasisVector>& Axis : A.Mask.Axes)
			{
				Sum += FMath::Square(A.Location.GetCoordinate(Axis) - B.Location.GetCoordinate(Axis));
			}
			return FMath::Sqrt(Sum);
		};

		// L10: one family, one idea. Phrasings agree in mask, h, mechanic and tier; an author has signed it.
		for (const TPair<FGameplayTag, TArray<const UNarrativeDialogDef*>>& Pair : Families)
		{
			const TArray<const UNarrativeDialogDef*>& Members = Pair.Value;
			const UNarrativeDialogDef& Lead = *Members[0];
			int32 MinTier = TNumericLimits<int32>::Max();
			int32 MaxTier = 0;
			for (const UNarrativeDialogDef* Card : Members)
			{
				const int32 Tier = static_cast<int32>(TierForReach(Card->SpaceSigma));
				MinTier = FMath::Min(MinTier, Tier);
				MaxTier = FMath::Max(MaxTier, Tier);
				if (Card == &Lead)
				{
					continue;
				}
				if (!SameMask(Card->Mask, Lead.Mask))
				{
					Add(OutIssues, TEXT("L10"), ENarrativeLintSeverity::Error, *Card, FString::Printf(TEXT("%s: its mask differs from %s's. One family is one idea on one set of axes."),
						*Pair.Key.ToString(), *Lead.GetName()));
				}
				if ((Card->Strength > 0.f ? FMath::Sign(Card->Stance) : 0) != (Lead.Strength > 0.f ? FMath::Sign(Lead.Stance) : 0))
				{
					Add(OutIssues, TEXT("L10"), ENarrativeLintSeverity::Error, *Card, FString::Printf(TEXT("%s: it pushes the other way from %s (h, or J = 0). A mixed family is never reserved."),
						*Pair.Key.ToString(), *Lead.GetName()));
				}
				if (Card->IsMechanic() != Lead.IsMechanic() || Card->GetOutcomePolicy() != Lead.GetOutcomePolicy())
				{
					Add(OutIssues, TEXT("L10"), ENarrativeLintSeverity::Error, *Card, FString::Printf(TEXT("%s: its mechanic differs from %s's."), *Pair.Key.ToString(), *Lead.GetName()));
				}
			}
			if (MaxTier - MinTier > 1)
			{
				Add(OutIssues, TEXT("L10"), ENarrativeLintSeverity::Error, Lead, FString::Printf(TEXT("%s spans %s to %s: phrasings of one idea sit within one tier of each other."),
					*Pair.Key.ToString(), LexToString(static_cast<ENarrativeTier>(MinTier)), LexToString(static_cast<ENarrativeTier>(MaxTier))));
			}
			else if (MaxTier != MinTier)
			{
				Add(OutIssues, TEXT("L10"), ENarrativeLintSeverity::Warning, Lead, FString::Printf(TEXT("%s spans two tiers: its phrasings cost different Effort."), *Pair.Key.ToString()));
			}

			const FString Hash = FamilyHash(Pair.Key, Cards);
			const UNarrativeFamilyDef* Def = UNarrativeFamilyDef::Find(Pair.Key);
			if (!Def || !Def->SignOff.IsSigned())
			{
				Add(OutIssues, TEXT("L10"), ENarrativeLintSeverity::Warning, Lead, FString::Printf(
					TEXT("%s has no author sign-off: confirm its %d phrasing(s) mean the same, then sign it in a UNarrativeFamilyDef with CatalogHash %s."),
					*Pair.Key.ToString(), Members.Num(), *Hash));
			}
			else if (Def->SignOff.CatalogHash != Hash)
			{
				Add(OutIssues, TEXT("L10"), ENarrativeLintSeverity::Warning, *Def, FString::Printf(
					TEXT("%s changed since %s signed it (%s): review it and sign %s."), *Pair.Key.ToString(), *Def->SignOff.Author, *Def->SignOff.CatalogHash, *Hash));
			}
		}

		// L5: two families with phrasings that coincide in location and mask are one idea named twice.
		TArray<FGameplayTag> Keys;
		Families.GetKeys(Keys);
		Keys.Sort([](const FGameplayTag& A, const FGameplayTag& B) { return A.ToString() < B.ToString(); });
		for (int32 I = 0; I < Keys.Num(); ++I)
		{
			for (int32 J = I + 1; J < Keys.Num(); ++J)
			{
				for (const UNarrativeDialogDef* A : Families[Keys[I]])
				{
					const UNarrativeDialogDef* Twin = nullptr;
					for (const UNarrativeDialogDef* B : Families[Keys[J]])
					{
						if (!A->Mask.IsEmpty() && SameMask(A->Mask, B->Mask) && A->Stance == B->Stance && MaskDistance(*A, *B) < 0.5)
						{
							Twin = B;
							break;
						}
					}
					if (Twin)
					{
						Add(OutIssues, TEXT("L5"), ENarrativeLintSeverity::Warning, *A, FString::Printf(
							TEXT("%s and %s (%s) sit together on one mask: one idea named twice? Merge the families or move one."),
							*Keys[I].ToString(), *Keys[J].ToString(), *Twin->GetName()));
						break;
					}
				}
			}
		}
	}

	void LintCard(const UNarrativeDialogDef& Card, TArray<FNarrativeLintIssue>& OutIssues)
	{
		LintMaskedVector(Card, Card.Location, Card.Mask, OutIssues);
		LintMechanics(Card, OutIssues);

		const double Sigma = Card.SpaceSigma;
		if (Sigma < MinReach)
		{
			Add(OutIssues, TEXT("L11"), ENarrativeLintSeverity::Error, Card,
				FString::Printf(TEXT("Reach %.2f is below the floor %.1f; evidence this narrow is finer than the sampler resolves."), Sigma, MinReach));
		}

		const double Cap = StrengthCap(FMath::Max(Sigma, UE_DOUBLE_SMALL_NUMBER));
		if (Card.Strength > Cap + 1e-6)
		{
			Add(OutIssues, TEXT("L6"), ENarrativeLintSeverity::Error, Card,
				FString::Printf(TEXT("Strength %.2f exceeds the %s cap %.2f at reach %.2f (min(tier cap, 1.8/sigma))."),
					Card.Strength, LexToString(TierForReach(Sigma)), Cap, Sigma));
		}
	}

	void LintEntity(const UNarrativeEntityDef& Entity, TArray<FNarrativeLintIssue>& OutIssues)
	{
		LintMaskedVector(Entity, Entity.StartingCoordinates, Entity.Mask, OutIssues);

		if (Entity.Mass < 1.0)
		{
			Add(OutIssues, TEXT("L7"), ENarrativeLintSeverity::Error, Entity,
				FString::Printf(TEXT("Mass %.2f is below 1."), Entity.Mass));
		}

		if (Entity.Mind.bHasMind && Entity.Mind.PositionWeight == 0.f && Entity.Mind.RegardWeight == 0.f)
		{
			// L8: with no affinity weighted, the stance is the clamped bias alone, fixed forever.
			Add(OutIssues, TEXT("L8"), ENarrativeLintSeverity::Warning, Entity,
				TEXT("Both affinity weights are 0: this mind reads only its bias, so its stance never moves."));
		}

		if (Entity.Mind.bHasMind && Entity.Mind.Loyalty <= 0.f)
		{
			Add(OutIssues, TEXT("L8"), ENarrativeLintSeverity::Warning, Entity,
				TEXT("Loyalty 0 caps regard at 0: no deed can move this mind."));
		}

		if (Entity.Mind.bHasMind && Entity.Mind.ProtectedCapacity > Entity.Mind.RelationCapacity)
		{
			Add(OutIssues, TEXT("Sheet"), ENarrativeLintSeverity::Warning, Entity,
				FString::Printf(TEXT("ProtectedCapacity %d exceeds RelationCapacity %d: protected relations can never all fit."),
					Entity.Mind.ProtectedCapacity, Entity.Mind.RelationCapacity));
		}

		if (Entity.Mind.bHasMind && !AreStanceBandsValid(Entity.Mind))
		{
			Add(OutIssues, TEXT("Sheet"), ENarrativeLintSeverity::Error, Entity,
				FString::Printf(TEXT("Stance bands overlap: HostileThreshold + Hysteresis (%.2f) must stay below FriendlyThreshold - Hysteresis (%.2f)."),
					Entity.Mind.HostileThreshold + Entity.Mind.Hysteresis, Entity.Mind.FriendlyThreshold - Entity.Mind.Hysteresis));
		}
	}
}
