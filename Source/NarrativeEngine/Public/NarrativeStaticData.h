#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "NarrativeCoreData.h"
#include "NarrativeMask.h"
#include "NarrativeStaticData.generated.h"

class UArchetypeDef;
class UNarrativeDialogDef;

UCLASS()
class NARRATIVEENGINE_API UNarrativeEventDef : public UNarrativeDataAsset
{
	GENERATED_BODY()

	NARRATIVE_DATA_HELPERS(UNarrativeEventDef)
};

/**
 * How a character reads others and holds a stance (spec §13.2; TDD Patterns 12, 16, 18). Stance
 * parameters are per-character data, never constants: a calibrated character's sheet may differ
 * from the default in any of them. The defaults are the spec's. Every duration is game time.
 */
USTRUCT(BlueprintType)
struct NARRATIVEENGINE_API FNarrativeMindSheet
{
	GENERATED_BODY()

	/** Whether this entity keeps a mind at all. Without one its hostility stays on the faction table. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mind")
	bool bHasMind = false;

	/** π: how far a first impression leans toward "they're like me" rather than the stereotype (§4.2). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mind|First Impressions", meta = (ClampMin = "0", ClampMax = "1"))
	float PriorSelfWeight = 0.2f;

	/** s_prior: how uncertain a first impression is, per axis (§4.2). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mind|First Impressions", meta = (ClampMin = "0"))
	float PriorSpread = 3.f;

	/** Where this character believes each archetype sits, when it differs from the archetype's own
	 *  location: its worldview (TDD Pattern 13). Archetypes not listed sit where they are authored. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mind|First Impressions")
	TMap<TSoftObjectPtr<UArchetypeDef>, FVectorND> WorldviewOverrides;

	/** w_pos: how much "are we alike?" counts. Negative for a contrarian (§6.1). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mind|Readings")
	float PositionWeight = 1.f;

	/** w_reg: how much deeds count against likeness (§6.1). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mind|Readings")
	float RegardWeight = 0.6f;

	/** The character's own baseline bias, summed with its faction biases and clamped (§6.1). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mind|Readings", meta = (ClampMin = "-1", ClampMax = "1"))
	float Bias = 0.f;

	/** θH: below this the reading turns Hostile. How quick to anger (§6.2). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mind|Stance")
	float HostileThreshold = -0.25f;

	/** θF: above this the reading turns Friendly. How slow to trust (§6.2). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mind|Stance")
	float FriendlyThreshold = 0.35f;

	/** δ: hysteresis on both thresholds. The bands must not overlap (θH + δ < θF - δ). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mind|Stance", meta = (ClampMin = "0"))
	float Hysteresis = 0.1f;

	/** T_z, game seconds: the memory of the reading's noise. How steady (§6.2). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mind|Stance", meta = (ClampMin = "0.01", Units = "s"))
	float NoiseMemory = 60.f;

	/** Δs, game seconds between this mind's decisions. Part of the calibration: T1-T7 assume 1 s. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mind|Stance", meta = (ClampMin = "0.05", Units = "s"))
	float DecisionInterval = 1.f;

	/** T_R, game seconds: how long regard lingers (§8.5). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mind|Regard", meta = (ClampMin = "0", Units = "s"))
	float RegardMemory = 7200.f;

	/** L: stored regard is clipped to ±Loyalty, so goodwill and grievance can't bank without limit (TDD Pattern 16). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mind|Regard", meta = (ClampMin = "0"))
	float Loyalty = 2.f;

	/** How long the warning before a social strike plays when no tell ability can (TDD Pattern 17). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mind|Tell", meta = (ClampMin = "0", Units = "s"))
	float TellWarningDuration = 1.5f;

	/** The ability that plays the warning, activated by tag on the character's ability system. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mind|Tell", meta = (Categories = "Ability"))
	FGameplayTag TellAbilityTag;

	/** An optional line barked as the warning starts. Barks are presentation, never deeds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mind|Tell")
	TSoftObjectPtr<UNarrativeDialogDef> TellCard;

	/** θ per gate, read against this character's reading (Gate.Join, Gate.Obey; P2). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mind|Gates", meta = (Categories = "Gate"))
	TMap<FGameplayTag, float> Gates;

	/** Detailed relations this mind keeps; the least recently touched unprotected one is forgotten first (§4.1). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mind|Capacity", meta = (ClampMin = "1"))
	int32 RelationCapacity = 48;

	/** Of those, how many may be protected (party, active threats, live grudges or tells). Past it, new optional relations are refused. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mind|Capacity", meta = (ClampMin = "1"))
	int32 ProtectedCapacity = 16;
};

UCLASS()
class NARRATIVEENGINE_API UNarrativeEntityDef : public UNarrativeDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVectorND StartingCoordinates;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVectorND PreviousCoordinates;

	/** M_A: the axes this character cares about. Likeness and approval are judged only there (§2, §6.1). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Meaning")
	FNarrativeMask Mask;

	/** How this character reads others and holds a stance. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mind")
	FNarrativeMindSheet Mind;

	/** One particular character, whom minds form relations with. Off for objects and for kinds that many
	 *  actors share (a mob's entity asset): minds keep no relation with those, and read them from the faction tables. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mind")
	bool bIndividual = false;

	/** The stereotype others meet first (§4.2; TDD Pattern 13), and the class shown in the character
	 *  menu. Shared UArchetypeDef assets carry the location, name and icon. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Menu")
	TObjectPtr<UArchetypeDef> ArchetypeAsset;

#if WITH_EDITOR
	virtual FName GetSpacePlacementProperty() const override { return GET_MEMBER_NAME_CHECKED(UNarrativeEntityDef, StartingCoordinates); }

	/** Live authoring: an edit to the authored point re-seeds anything already simulating from it. */
	virtual void PostEditChangeProperty(FPropertyChangedEvent& Event) override;
	virtual void PostEditUndo() override;
#endif

	/* Presence in the space: deeds done to this entity weigh this much (TDD Pattern 16), and, once
	 * the force law lands, how much it resists forces. At least 1 (lint L7). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	double Mass = 1.f;

	/* Tendency to naturally progress along the entity's telos */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	double Drift = 0.005f;

	/* κ: resistance to forces that oppose the entity's telos. Was "Alignment", a name the spec
	 * reserves for the disposition of §6 (redirected in DefaultEngine.ini). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	double Rigidity = 1.f;

	/* Affects how quickly the entity comes to rest */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	double Damping = 2.f;

	NARRATIVE_DATA_HELPERS(UNarrativeEntityDef)
};

/* A named, illustrated point in the narrative Hilbert space - e.g. an archetype entities can be measured against or drift towards */
UCLASS()
class NARRATIVEENGINE_API UArchetypeDef : public UNarrativeDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVectorND Location;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftObjectPtr<UTexture2D> Icon;

#if WITH_EDITOR
	virtual FText GetSpaceLabel() const override { return DisplayName.IsEmpty() ? Super::GetSpaceLabel() : DisplayName; }
	virtual TSoftObjectPtr<UTexture2D> GetSpaceIcon() const override { return Icon; }
#endif

	NARRATIVE_DATA_HELPERS(UArchetypeDef)
};

UCLASS()
class NARRATIVEENGINE_API UNarrativeActionDef : public UNarrativeDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVectorND Force;

	NARRATIVE_DATA_HELPERS(UNarrativeActionDef)
};


/*
 * Emplace some phrase for characters to "say", as if this is one possible incoming thought that's simulated for a character.
 * The radius is a sort of entropy field for simulating wave function collapse in the narrative state hilbert space
 */
UCLASS()
class NARRATIVEENGINE_API UNarrativeDialogDef : public UNarrativeDataAsset
{
	GENERATED_BODY()

public:
	// todo it'd be cool to break this phrase into a grammer where data fed into the grammar structure is determined via wave function collapse
	/* What's actually said in its entirety */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText Phrase;
	/* One word description that indicates the emotion or action associated with delivering this phrase*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText Label;

#pragma region Narrative Engine Encounter
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVectorND Location;

	// Reach σ: the 1/e distance of the card's kernel, and so how specific it is. A higher sigma
	// means characters can be further away along the axis. The tier (and its cap on Strength)
	// follows from it (§13.4).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Reach (Sigma)"))
	float SpaceSigma = 5.f;

	/** M_y: the axes this card is about. A typed 0 counts; a blank doesn't (§2, §3.1). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FNarrativeMask Mask;

	/** J: how hard the deed lands on those who witness it. 0 means saying it is not a deed (§3.1, §8.5). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0"))
	float Strength = 0.f;

	/** h: +1 for what is done to friends, -1 for what is done to foes (§2). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "-1", ClampMax = "1"))
	int32 Stance = 1;

	/** g(y): the idea this card is one phrasing of. Empty means the card is its own family (§3.2). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Categories = "Dialog.Family"))
	FGameplayTag Family;
#if WITH_EDITOR
	virtual FText GetSpaceLabel() const override { return Label.IsEmpty() ? Super::GetSpaceLabel() : Label; }
	virtual float GetSpaceRadius() const override { return SpaceSigma; }
#endif
#pragma endregion

	/* Scriptable gameplay mechanic entrypoint - fire a montage, and script mechanics through anim notifies */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTagContainer AbilityTags;


	NARRATIVE_DATA_HELPERS(UNarrativeDialogDef)
};
