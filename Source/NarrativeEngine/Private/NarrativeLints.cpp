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
						FString::Printf(TEXT("Mask lists %s twice: it would weigh double in every reading."), *AxisName(Axis)));
				}
				else
				{
					Seen.Add(Axis.ToSoftObjectPath());
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
				if (Card.Strength > 0.f)
				{
					Add(OutIssues, TEXT("L14"), ENarrativeLintSeverity::Warning, Card, FString::Printf(
						TEXT("J %.2f on the offer: a gift's regard comes from its completed transfer (db.Narrative.Outcome.GiftStrength), so J here values it even when nothing changes hands."),
						Card.Strength));
				}
			}
			else if (Mechanic == TEXT("Recruit") && Card.Strength > 0.f)
			{
				Add(OutIssues, TEXT("L14"), ENarrativeLintSeverity::Warning, Card,
					FString::Printf(TEXT("J %.2f on the ask: an invitation is a request (J = 0); its ability runs the join."), Card.Strength));
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
			// L8: with no reading weighted, the stance is the clamped bias alone, fixed forever.
			Add(OutIssues, TEXT("L8"), ENarrativeLintSeverity::Warning, Entity,
				TEXT("Both reading weights are 0: this mind reads only its bias, so its stance never moves."));
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
