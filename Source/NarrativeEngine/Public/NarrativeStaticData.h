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

/** Whom a card may be said to (TDD Pattern 8, check 2). */
UENUM(BlueprintType, meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class ENarrativeDialogTarget : uint8
{
	None = 0 UMETA(Hidden),
	/** The speaker: talking to oneself, as toward a wordless thing. */
	Self = 1 << 0,
	/** A character outside the speaker's party. */
	NonMember = 1 << 1,
	/** A member of the speaker's party. */
	PartyMember = 1 << 2,
};
ENUM_CLASS_FLAGS(ENarrativeDialogTarget);

/** Which record carries a mechanic card's material value (TDD Pattern 11). */
UENUM(BlueprintType)
enum class ENarrativeOutcomePolicy : uint8
{
	/** From the card's mechanic tag: GiveItem a gift, Recruit a recruit, RequestItem an item request, EndDialog a farewell, else a line. */
	Auto,
	/** The line carries the deed, as every card without a mechanic does. */
	Line,
	/** The line carries J = 0; the transfer carries the card's J, once, as the line's child. */
	Gift,
	/** Asks the addressee into the party. Both the line and its outcome carry J = 0 (MD7): an invitation awards no goodwill. */
	Recruit,
	/** The line only; it ends the conversation. */
	Farewell,
	/**
	 * Asks the addressee for an item. The line carries its own J like any line. Legal only while the addressee could
	 * legally give the speaker something the card asks for; how well the ask lands makes their gift likelier.
	 */
	RequestItem,
};

/**
 * What a character may say (TDD Pattern 10, D3): every family is granted unless denied, except mechanic families
 * (a card carrying a Dialog.Mechanic tag), which need a grant. Single cards may be granted or denied on top.
 */
USTRUCT(BlueprintType)
struct NARRATIVEENGINE_API FNarrativeRepertoire
{
	GENERATED_BODY()

	/** Mechanic families this character may use (Recruit, Item.Give, Depart, SeeWares), and with bExplicitOnly every family it may. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repertoire", meta = (Categories = "Dialog.Family"))
	FGameplayTagContainer GrantedFamilies;

	/** Families withheld from this character's voice although granted by default. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repertoire", meta = (Categories = "Dialog.Family"))
	FGameplayTagContainer DeniedFamilies;

	/** Cards granted whatever their family. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repertoire")
	TArray<TSoftObjectPtr<UNarrativeDialogDef>> GrantedCards;

	/** Cards withheld whatever their family. A denial beats a grant. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repertoire")
	TArray<TSoftObjectPtr<UNarrativeDialogDef>> DeniedCards;

	/** D3's alternative: only GrantedFamilies and GrantedCards, nothing by default. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repertoire")
	bool bExplicitOnly = false;

	/** Whether this repertoire lets its character say Card. */
	bool Grants(const UNarrativeDialogDef& Card) const;
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
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mind|Affinity")
	float PositionWeight = 1.f;

	/** w_reg: how much deeds count against likeness (§6.1). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mind|Affinity")
	float RegardWeight = 0.6f;

	/** The character's own baseline bias, summed with its faction biases and clamped (§6.1). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mind|Affinity", meta = (ClampMin = "-1", ClampMax = "1"))
	float Bias = 0.f;

	/** θH: below this the affinity turns Hostile. How quick to anger (§6.2). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mind|Stance")
	float HostileThreshold = -0.25f;

	/** θF: above this the affinity turns Friendly. How slow to trust (§6.2). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mind|Stance")
	float FriendlyThreshold = 0.35f;

	/** δ: hysteresis on both thresholds. The bands must not overlap (θH + δ < θF - δ). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mind|Stance", meta = (ClampMin = "0"))
	float Hysteresis = 0.1f;

	/** T_z, game seconds: the memory of the affinity's noise. How steady (§6.2). */
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

	/** θ per gate, read against this character's affinity (Gate.Join, Gate.Obey; P2). */
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

	/** What this character may say (TDD Pattern 10, D3): its voice, as opposed to the whole catalog. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialog")
	FNarrativeRepertoire Repertoire;

	/** Real seconds between a line and this character's reply showing (D4): presentation only, keyed, the same on replay. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialog")
	FFloatInterval ReplyDelay = FFloatInterval(1.f, 2.5f);

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

/**
 * A located thing a dialog card's wild card can name: "Care for a cup of {Item.Consumable}?" draws one
 * subject whose SubjectTag matches the token. Which one is the choice rule's call, the same rule that
 * chose the card (NarrativeChoice::PickSubject): a subject near the speaker's own traits resonates, one
 * near what the speaker believes of the addressee is relevant, and its reach decides how near is near.
 *
 * Abstract: the game derives concrete subjects (UItemAsset) and says how wide each one reaches.
 */
UCLASS(Abstract)
class NARRATIVEENGINE_API UNarrativeSubjectDef : public UNarrativeDataAsset
{
	GENERATED_BODY()

public:
	/** What wild cards find this by. A token selects every subject whose tag matches it, parents included:
	 *  {Item.Equipable} draws from every equipable, {Item.Consumable.Coffee} from coffees alone. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative")
	FGameplayTag SubjectTag;

	/** l: where this subject sits in the space. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative")
	FVectorND Location;

	/** The axes this subject is about. Blank reads its typed axes, as a card's does (L13). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative")
	FNarrativeMask Mask;

	/** σ: how far from its location this subject still resonates, and so how specific it is. */
	virtual float GetSubjectReach() const { return 5.f; }

	/** The name the space editor and the choice trace show. */
	virtual FText GetSubjectDisplayName() const { return FText::FromName(GetFName()); }

#if WITH_EDITOR
	virtual FName GetSpacePlacementProperty() const override { return GET_MEMBER_NAME_CHECKED(UNarrativeSubjectDef, Location); }
	virtual FName GetSpaceMaskProperty() const override { return GET_MEMBER_NAME_CHECKED(UNarrativeSubjectDef, Mask); }
	virtual FText GetSpaceLabel() const override
	{
		const FText Name = GetSubjectDisplayName();
		return Name.IsEmpty() ? Super::GetSpaceLabel() : Name;
	}
	virtual float GetSpaceRadius() const override { return GetSubjectReach(); }
#endif

	NARRATIVE_DATA_HELPERS(UNarrativeSubjectDef)
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

	/** Whom this card may be said to (TDD Pattern 8, check 2). Toward a wordless thing only Self cards are legal. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Legality", meta = (Bitmask, BitmaskEnum = "/Script/NarrativeEngine.ENarrativeDialogTarget"))
	int32 TargetTypes = static_cast<int32>(ENarrativeDialogTarget::NonMember | ENarrativeDialogTarget::PartyMember);

	/** Game seconds before the speaker may say this card again (Pattern 8, check 6). 0: its family's, if any. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Legality", meta = (ClampMin = "0", Units = "s"))
	float Cooldown = 0.f;

	/** Which record carries a mechanic card's value (TDD Pattern 11). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Legality")
	ENarrativeOutcomePolicy OutcomePolicy = ENarrativeOutcomePolicy::Auto;

	/** The policy in force: Auto read from the mechanic tags. */
	ENarrativeOutcomePolicy GetOutcomePolicy() const;

	/** A mechanic card: one of its ability tags is a Dialog.Mechanic tag. Its family needs a grant (D3) and is offered as a verb (D7). */
	bool IsMechanic() const;

	/** Whether the card may be said to this kind of addressee. */
	bool AllowsTarget(ENarrativeDialogTarget Target) const { return (TargetTypes & static_cast<int32>(Target)) != 0; }

	/** Bumped whenever any card loads, unloads or is edited: the dealer's cached catalog rebuilds on a change (TDD §9). */
	static uint32 GetCatalogRevision() { return CatalogRevision; }

	virtual void PostLoad() override { Super::PostLoad(); ++CatalogRevision; }
	virtual void BeginDestroy() override { ++CatalogRevision; Super::BeginDestroy(); }
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& Event) override { Super::PostEditChangeProperty(Event); ++CatalogRevision; }
	virtual void PostEditUndo() override { Super::PostEditUndo(); ++CatalogRevision; }
#endif

private:
	static inline uint32 CatalogRevision = 0;

public:
	NARRATIVE_DATA_HELPERS(UNarrativeDialogDef)
};

/** An author's record that a family's phrasings say one thing (TDD Pattern 7, L10). */
USTRUCT(BlueprintType)
struct NARRATIVEENGINE_API FNarrativeFamilySignOff
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sign-off")
	FString Author;

	/** ISO date, e.g. 2026-11-14. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sign-off")
	FString Date;

	/** The family's phrasings when signed (NarrativeLints::FamilyHash). A changed family needs signing again. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sign-off")
	FString CatalogHash;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sign-off", meta = (MultiLine = "true"))
	FString Note;

	bool IsSigned() const { return !Author.IsEmpty() && !CatalogHash.IsEmpty(); }
};

/** A family of cards (TDD §7 Family definition): its cooldown and its author sign-off. Families without an asset have neither. */
UCLASS()
class NARRATIVEENGINE_API UNarrativeFamilyDef : public UNarrativeDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Family", meta = (Categories = "Dialog.Family"))
	FGameplayTag Family;

	/** Game seconds before the speaker may say any phrasing of this family again (Pattern 8, check 6). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Family", meta = (ClampMin = "0", Units = "s"))
	float Cooldown = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Family")
	FNarrativeFamilySignOff SignOff;

	/** The asset for Family, or null. */
	static const UNarrativeFamilyDef* Find(const FGameplayTag& Family);

	NARRATIVE_DATA_HELPERS(UNarrativeFamilyDef)
};
