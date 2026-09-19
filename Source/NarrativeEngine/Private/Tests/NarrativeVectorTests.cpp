#include "NarrativeCoreData.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "NarrativeVectorLegacy.h"
#include "NarrativeVectorTestTypes.h"
#include "Misc/AutomationTest.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "Serialization/StructuredArchiveAdapters.h"
#include "UObject/UnrealType.h"

namespace NarrativeVectorTests
{
	const TSoftObjectPtr<UNarrativeBasisVector> A(FSoftObjectPath(TEXT("/Game/VectorTests/A.A")));
	const TSoftObjectPtr<UNarrativeBasisVector> B(FSoftObjectPath(TEXT("/Game/VectorTests/B.B")));
	const TSoftObjectPtr<UNarrativeBasisVector> C(FSoftObjectPath(TEXT("/Game/VectorTests/C.C")));

	bool LoadLegacy(const FNarrativeVectorLegacy& Source, const FNarrativeVectorLegacy* Defaults, FVectorND& Destination)
	{
		TArray<uint8> Bytes;
		{
			FMemoryWriter Memory(Bytes);
			FObjectAndNameAsStringProxyArchive Writer(Memory, false);
			FNarrativeVectorLegacy::StaticStruct()->SerializeItem(Writer, const_cast<FNarrativeVectorLegacy*>(&Source), Defaults);
		}
		FMemoryReader Memory(Bytes);
		FObjectAndNameAsStringProxyArchive Reader(Memory, false);
		FNarrativeVectorTestContainer Loaded;
		Loaded.Components = Destination;
		FNarrativeVectorTestContainer::StaticStruct()->SerializeItem(Reader, &Loaded, nullptr);
		Destination = Loaded.Components;
		return !Reader.IsError();
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNarrativeVectorArithmeticTest, "Narrative.VectorND.AxisIdentity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNarrativeVectorArithmeticTest::RunTest(const FString& Parameters)
{
	using namespace NarrativeVectorTests;
	FVectorND Left({A, B});
	Left[0] = 3.f;
	Left[1] = 4.f;
	FVectorND Right({B, A});
	Right[0] = 4.f;
	Right[1] = 3.f;
	TestEqual(TEXT("Reordered axes have zero distance"), Left.Distance(Right), 0.f);
	TestEqual(TEXT("Dot matches identities, not indices"), Left.Dot(Right), 25.f);
	TestEqual(TEXT("Normalized magnitude"), Left.GetSafeNormal().Length(), 1.f);
	FVectorND Additional({C}, 2.f);
	const FVectorND Sum = Left + Additional;
	TestEqual(TEXT("Union includes new axis"), Sum.Num(), 3);
	TestEqual(TEXT("Existing coordinate retained"), Sum.GetCoordinate(A), 3.f);
	TestEqual(TEXT("New coordinate added"), Sum.GetCoordinate(C), 2.f);
	const FVectorND Rebased = Left.Rebased({C, B, A});
	TestEqual(TEXT("New axis zero-filled"), Rebased[0], 0.f);
	TestEqual(TEXT("Reordered coordinate retained"), Rebased[2], 3.f);
	TestEqual(TEXT("Default vector is zero without a registry"), FVectorND().Length(), 0.f);
	TestEqual(TEXT("Empty vector behaves as additive identity"), (FVectorND() + Left).Distance(Left), 0.f);
	TestEqual(TEXT("Zero normalization retains basis"), FVectorND({A, B}).GetSafeNormal().Num(), 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNarrativeVectorSerializationTest, "Narrative.VectorND.DenseRoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNarrativeVectorSerializationTest::RunTest(const FString& Parameters)
{
	using namespace NarrativeVectorTests;
	FVectorND Original({B, A});
	Original[0] = -2.5f;
	Original[1] = 0.f;
	TArray<uint8> Bytes;
	{
		FMemoryWriter Memory(Bytes);
		FObjectAndNameAsStringProxyArchive Writer(Memory, false);
		FVectorND::StaticStruct()->SerializeItem(Writer, &Original, nullptr);
	}
	FVectorND Loaded;
	{
		FMemoryReader Memory(Bytes);
		FObjectAndNameAsStringProxyArchive Reader(Memory, false);
		FVectorND::StaticStruct()->SerializeItem(Reader, &Loaded, nullptr);
		TestFalse(TEXT("No archive error"), Reader.IsError());
	}
	TestTrue(TEXT("Axis order survives"), Loaded.GetBasis() == Original.GetBasis());
	TestTrue(TEXT("Dense values survive, including explicit zero"), Loaded.GetCoordinates() == Original.GetCoordinates());
	TestFalse(TEXT("New format is not legacy"), Loaded.WasLoadedFromLegacyMap());
	TestFalse(TEXT("Unloaded reference stays unloaded"), A.IsLoaded());
	FString Text;
	FVectorND::StaticStruct()->ExportText(Text, &Original, nullptr, nullptr, PPF_None, nullptr);
	FVectorND Pasted;
	TestNotNull(TEXT("Copy/paste import succeeds"), FVectorND::StaticStruct()->ImportText(*Text, &Pasted, nullptr, PPF_None, GWarn, TEXT("VectorTest")));
	TestTrue(TEXT("Copy/paste preserves dense representation"), Pasted.GetBasis() == Original.GetBasis() && Pasted.GetCoordinates() == Original.GetCoordinates());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNarrativeVectorMigrationTest, "Narrative.VectorND.LegacyMapMigration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNarrativeVectorMigrationTest::RunTest(const FString& Parameters)
{
	using namespace NarrativeVectorTests;
	FNarrativeVectorLegacy Legacy;
	Legacy.Components.Add(B, -4.f);
	Legacy.Components.Add(A, 0.f);
	FVectorND Loaded;
	TestTrue(TEXT("Read old map wire format"), LoadLegacy(Legacy, nullptr, Loaded));
	TestTrue(TEXT("Migration provenance recorded"), Loaded.WasLoadedFromLegacyMap());
	TestEqual(TEXT("Unloaded reference and negative value survive"), Loaded.GetCoordinate(B), -4.f);
	TestEqual(TEXT("Explicit zero retained"), Loaded.Num(), 2);
	TestTrue(TEXT("Deterministic path ordering"), Loaded.GetBasis()[0] == A);

	FNarrativeVectorLegacy Defaults;
	Defaults.Components.Add(A, 3.f);
	Defaults.Components.Add(B, 4.f);
	Defaults.Components.Add(C, 5.f);
	Legacy.Components = Defaults.Components;
	Legacy.Components.Remove(A);
	Legacy.Components[B] = 8.f;
	FVectorND Inherited({A, B, C});
	Inherited[0] = 3.f;
	Inherited[1] = 4.f;
	Inherited[2] = 5.f;
	TestTrue(TEXT("Read delta against inherited values"), LoadLegacy(Legacy, &Defaults, Inherited));
	TestEqual(TEXT("Removal applied"), Inherited.GetBasis().IndexOfByKey(A), INDEX_NONE);
	TestEqual(TEXT("Override applied"), Inherited.GetCoordinate(B), 8.f);
	TestEqual(TEXT("Unchanged inherited value preserved"), Inherited.GetCoordinate(C), 5.f);
	Legacy.Components.Reset();
	Inherited = FVectorND({A, B, C});
	Inherited[0] = 3.f;
	Inherited[1] = 4.f;
	Inherited[2] = 5.f;
	TestTrue(TEXT("Empty override removes all inherited entries"), LoadLegacy(Legacy, &Defaults, Inherited));
	TestEqual(TEXT("All inherited entries removed"), Inherited.Num(), 0);
	Loaded = FVectorND();
	TestTrue(TEXT("Empty map converts"), LoadLegacy(Legacy, nullptr, Loaded));
	TestEqual(TEXT("Empty map remains empty"), Loaded.Num(), 0);
	Legacy.Components.Add(TSoftObjectPtr<UNarrativeBasisVector>(), 9.f);
	TestTrue(TEXT("Invalid legacy entry is preserved for diagnosis"), LoadLegacy(Legacy, nullptr, Loaded));
	TestFalse(TEXT("Null basis blocks migration save"), Loaded.IsValid());
	TestEqual(TEXT("Bad entry value not discarded"), Loaded[0], 9.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNarrativeVectorTaggedMigrationTest, "Narrative.VectorND.TaggedPropertyMigration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNarrativeVectorTaggedMigrationTest::RunTest(const FString& Parameters)
{
	using namespace NarrativeVectorTests;
	FNarrativeVectorLegacy Legacy;
	Legacy.Components.Add(A, 7.f);
	Legacy.Components.Add(B, -3.f);
	TArray<uint8> Bytes;
	{
		FMemoryWriter Memory(Bytes);
		FObjectAndNameAsStringProxyArchive Writer(Memory, false);
		FNarrativeVectorLegacy::StaticStruct()->SerializeItem(Writer, &Legacy, nullptr);
	}
	FNarrativeVectorTestContainer Loaded;
	{
		FMemoryReader Memory(Bytes);
		FObjectAndNameAsStringProxyArchive Reader(Memory, false);
		FNarrativeVectorTestContainer::StaticStruct()->SerializeItem(Reader, &Loaded, nullptr);
		TestFalse(TEXT("Tagged conversion has no archive errors"), Reader.IsError());
		TestEqual(TEXT("Entire tagged stream consumed"), Reader.Tell(), static_cast<int64>(Bytes.Num()));
	}
	TestTrue(TEXT("Property system invoked map-to-struct conversion"), Loaded.Components.WasLoadedFromLegacyMap());
	TestEqual(TEXT("Tagged property preserves first axis"), Loaded.Components.GetCoordinate(A), 7.f);
	TestEqual(TEXT("Tagged property preserves second axis"), Loaded.Components.GetCoordinate(B), -3.f);
	return true;
}
#endif
