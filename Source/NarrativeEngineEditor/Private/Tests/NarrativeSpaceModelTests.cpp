#include "NarrativeSpaceModel.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "AssetRegistry/AssetRegistryModule.h"
#include "Editor.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/FileManager.h"
#include "ImageUtils.h"
#include "Misc/App.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "NarrativeStaticData.h"
#include "SNarrativeSpaceEditor.h"
#include "SNarrativeSpaceViewport.h"
#include "InputCoreTypes.h"
#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"
#include "Widgets/SWindow.h"

namespace NarrativeSpaceTests
{
TSharedPtr<SNarrativeSpaceViewport> FindViewport(const TSharedRef<SWidget>& Widget)
{
	if (Widget->GetType() == FName("SNarrativeSpaceViewport")) { return StaticCastSharedRef<SNarrativeSpaceViewport>(Widget); }
	FChildren* Children = Widget->GetChildren();
	for (int32 Index = 0; Index < Children->Num(); ++Index)
	{
		if (auto Found = FindViewport(Children->GetChildAt(Index))) { return Found; }
	}
	return nullptr;
}

struct FFixture
{
	FString Root = TEXT("/Game/__NarrativeSpaceTest_") + FGuid::NewGuid().ToString(EGuidFormats::Digits);
	TArray<TStrongObjectPtr<UNarrativeDataAsset>> Assets;
	UNarrativeBasisVector* X = Add<UNarrativeBasisVector>(TEXT("X"));
	UNarrativeBasisVector* Y = Add<UNarrativeBasisVector>(TEXT("Y"));
	UNarrativeBasisVector* Z = Add<UNarrativeBasisVector>(TEXT("Z"));
	UNarrativeEntityDef* Entity = Add<UNarrativeEntityDef>(TEXT("Entity"));
	UNarrativeDialogDef* Dialog = Add<UNarrativeDialogDef>(TEXT("Dialog"));

	template <typename T> T* Add(const TCHAR* Name)
	{
		UPackage* Package = CreatePackage(*(Root / Name));
		T* Asset = NewObject<T>(Package, Name, RF_Public | RF_Standalone | RF_Transactional);
		Assets.Emplace(Asset);
		FAssetRegistryModule::AssetCreated(Asset);
		return Asset;
	}
	FNarrativeSpaceQuery Query() const
	{
		FNarrativeSpaceQuery Result;
		Result.ContentPath = Root;
		Result.Axes = {X, Y};
		return Result;
	}
	~FFixture()
	{
		for (const auto& Asset : Assets)
		{
			FAssetRegistryModule::AssetDeleted(Asset.Get());
			Asset->ClearFlags(RF_Standalone);
			Asset->GetPackage()->SetDirtyFlag(false);
		}
	}
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNarrativeSpaceCameraTest, "Narrative.Space.Camera",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNarrativeSpaceCameraTest::RunTest(const FString& Parameters)
{
	FNarrativeSpaceCamera Camera;
	Camera.Center = FVector(17, -23, 51);
	const FVector2D Size(1024, 768), Screen(173, 294);
	for (auto View : {ENarrativeSpaceView::XY, ENarrativeSpaceView::XZ, ENarrativeSpaceView::YZ, ENarrativeSpaceView::Iso})
	{
		Camera.SetView(View);
		TestTrue(TEXT("Projection round trip on view plane"), Camera.Project(Camera.Unproject(Screen, Size), Size).Equals(Screen, 1.e-6));
		const FVector Before = Camera.Unproject(Screen, Size);
		Camera.ZoomAt(Screen, Size, 3);
		TestTrue(TEXT("Zoom preserves cursor location"), Camera.Unproject(Screen, Size).Equals(Before, 1.e-6));
		const FVector Delta = Camera.DragDelta(FVector2D(40, -10), 0);
		TestEqual(TEXT("Axis lock leaves Y untouched"), Delta.Y, 0.0);
		TestEqual(TEXT("Axis lock leaves Z untouched"), Delta.Z, 0.0);
	}
	Camera.SetView(ENarrativeSpaceView::XY);
	TestTrue(TEXT("End-on axis cannot be dragged"), Camera.DragDelta(FVector2D(90, 80), 2).IsNearlyZero());
	Camera.Orbit(FVector2D(56, 32));
	TestTrue(TEXT("Orbit preserves orthonormal basis"), FMath::IsNearlyZero(Camera.Right.Dot(Camera.Up), 1.e-6));
	TestTrue(TEXT("Orbit round trip"), Camera.Project(Camera.Unproject(Screen, Size), Size).Equals(Screen, 1.e-6));
	Camera.Frame({FVector(-100, -50, 0), FVector(100, 50, 0)}, Size);
	TestTrue(TEXT("Frame produces a positive grid"), Camera.GridStep() > 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNarrativeSpaceOffsetTest, "Narrative.Space.CoordinateEditing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNarrativeSpaceOffsetTest::RunTest(const FString& Parameters)
{
	NarrativeSpaceTests::FFixture Fixture;
	const TArray<TSoftObjectPtr<UNarrativeBasisVector>> Axes{Fixture.X, Fixture.Y};
	FVectorND Original({Fixture.Z, Fixture.X});
	Original.SetCoordinate(Fixture.Z, 99);
	Original.SetCoordinate(Fixture.X, 2);
	const FVectorND Locked = FNarrativeSpaceModel::Offset(Original, Axes, FVector(3, 7, 9), 0, 0);
	TestEqual(TEXT("X changes by identity despite reordered basis"), Locked.GetCoordinate(Fixture.X), 5.f);
	TestEqual(TEXT("Hidden Z preserved"), Locked.GetCoordinate(Fixture.Z), 99.f);
	TestEqual(TEXT("Axis lock does not append missing Y"), Locked.Num(), 2);
	const FVectorND Moved = FNarrativeSpaceModel::Offset(Original, Axes, FVector(0, 3.6, 0), INDEX_NONE, 1);
	TestEqual(TEXT("Moving missing Y appends it"), Moved.Num(), 3);
	TestEqual(TEXT("Snap is in coordinate units"), Moved.GetCoordinate(Fixture.Y), 4.f);
	TestEqual(TEXT("Unmoved X preserved"), Moved.GetCoordinate(Fixture.X), 2.f);
	const FVectorND NoMove = FNarrativeSpaceModel::Offset(Original, Axes, FVector::ZeroVector, INDEX_NONE, 1);
	TestTrue(TEXT("No movement preserves sparse layout"), NoMove.GetBasis() == Original.GetBasis());
	const FVectorND OneAxis = FNarrativeSpaceModel::Offset(Original, {Fixture.X}, FVector(1, 200, 500), INDEX_NONE, 0);
	TestEqual(TEXT("1D stacking cannot write other coordinates"), OneAxis.GetCoordinate(Fixture.Z), 99.f);
	TestEqual(TEXT("1D only writes X"), OneAxis.Num(), 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNarrativeSpaceQueryTest, "Narrative.Space.QueryAndRefresh",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNarrativeSpaceQueryTest::RunTest(const FString& Parameters)
{
	NarrativeSpaceTests::FFixture Fixture;
	FNarrativeSpaceModel Model;
	auto Query = Fixture.Query();
	FText QueryError;
	FNarrativeSpaceQuery RootQuery = Query;
	RootQuery.ContentPath = TEXT("/Game");
	TestTrue(TEXT("Default content root is valid"), RootQuery.Validate(QueryError));
	TestTrue(TEXT("Valid query"), Model.SetQuery(Query));
	TestEqual(TEXT("Only assets with placement fields are plotted"), Model.GetPoints().Num(), 2);
	TestEqual(TEXT("Entity resolves StartingCoordinates"), Model.ResolvePlacement(Fixture.Entity)->GetFName(), GET_MEMBER_NAME_CHECKED(UNarrativeEntityDef, StartingCoordinates));
	TestEqual(TEXT("Dialog resolves Location"), Model.ResolvePlacement(Fixture.Dialog)->GetFName(), GET_MEMBER_NAME_CHECKED(UNarrativeDialogDef, Location));
	Model.Select({Fixture.Entity});
	Fixture.Entity->StartingCoordinates.SetCoordinate(Fixture.X, 11);
	FPropertyChangedEvent Event(Model.ResolvePlacement(Fixture.Entity));
	Fixture.Entity->PostEditChangeProperty(Event);
	Model.Tick();
	for (const auto& Point : Model.GetPoints())
	{
		if (Point.Asset == Fixture.Entity)
		{
			TestEqual(TEXT("External property edit refreshes point"), Point.Position.X, 11.0);
			TestEqual(TEXT("Missing axis is distinguished from zero"), Point.PresentAxes, uint8(1));
		}
	}
	Fixture.Add<UNarrativeDialogDef>(TEXT("Added"));
	Model.Tick();
	TestEqual(TEXT("Registry add refreshes model"), Model.GetPoints().Num(), 3);
	Query.Classes = {UNarrativeDialogDef::StaticClass()};
	Model.SetQuery(Query);
	TestEqual(TEXT("Class filter"), Model.GetPoints().Num(), 2);
	TestTrue(TEXT("Filtered selection removed"), Model.GetSelection().IsEmpty());
	Query.Axes = {Fixture.X, Fixture.X};
	TestFalse(TEXT("Duplicate axes rejected"), Model.SetQuery(Query));
	TestTrue(TEXT("Invalid query clears old points"), Model.GetPoints().IsEmpty());
	Query = Fixture.Query(); Query.Axes.Reset();
	TestFalse(TEXT("Empty axes rejected"), Model.SetQuery(Query));
	Query = Fixture.Query(); Query.ContentPath = TEXT("C:/NotContent");
	TestFalse(TEXT("Disk paths rejected"), Model.SetQuery(Query));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNarrativeSpaceTransactionTest, "Narrative.Space.UndoRedoAndCancel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNarrativeSpaceTransactionTest::RunTest(const FString& Parameters)
{
	NarrativeSpaceTests::FFixture Fixture;
	FNarrativeSpaceModel Model;
	Fixture.Entity->StartingCoordinates.SetCoordinate(Fixture.X, 2);
	Fixture.Entity->StartingCoordinates.SetCoordinate(Fixture.Z, 99);
	Fixture.Entity->PreviousCoordinates.SetCoordinate(Fixture.X, -10);
	Model.SetQuery(Fixture.Query());
	Model.Select({Fixture.Entity, Fixture.Dialog});
	if (!TestTrue(TEXT("Begin multi-asset drag"), Model.BeginDrag())) { return false; }
	Model.UpdateDrag(FVector(3, 4, 0), INDEX_NONE, 0);
	Model.UpdateDrag(FVector(5, 6, 0), INDEX_NONE, 0);
	Model.EndDrag();
	TestEqual(TEXT("Drag uses original, not accumulated delta"), Fixture.Entity->StartingCoordinates.GetCoordinate(Fixture.X), 7.f);
	TestEqual(TEXT("Second selected asset moved"), Fixture.Dialog->Location.GetCoordinate(Fixture.Y), 6.f);
	TestEqual(TEXT("PreviousCoordinates untouched"), Fixture.Entity->PreviousCoordinates.GetCoordinate(Fixture.X), -10.f);
	GEditor->UndoTransaction(); Model.Tick();
	TestEqual(TEXT("One undo restores first asset"), Fixture.Entity->StartingCoordinates.GetCoordinate(Fixture.X), 2.f);
	TestEqual(TEXT("Undo removes appended missing axis"), Fixture.Entity->StartingCoordinates.Num(), 2);
	TestEqual(TEXT("One undo restores second asset"), Fixture.Dialog->Location.Num(), 0);
	GEditor->RedoTransaction(); Model.Tick();
	TestEqual(TEXT("Redo reapplies whole drag"), Fixture.Entity->StartingCoordinates.GetCoordinate(Fixture.X), 7.f);
	TestEqual(TEXT("Redo reapplies second asset"), Fixture.Dialog->Location.GetCoordinate(Fixture.Y), 6.f);
	Fixture.Entity->GetPackage()->SetDirtyFlag(false);
	Fixture.Dialog->GetPackage()->SetDirtyFlag(true);
	Model.BeginDrag(); Model.UpdateDrag(FVector(100, 100, 0), INDEX_NONE, 0); Model.EndDrag(true);
	TestEqual(TEXT("Cancel restores coordinates"), Fixture.Entity->StartingCoordinates.GetCoordinate(Fixture.X), 7.f);
	TestFalse(TEXT("Cancel restores clean package"), Fixture.Entity->GetPackage()->IsDirty());
	TestTrue(TEXT("Cancel preserves previously dirty package"), Fixture.Dialog->GetPackage()->IsDirty());
	Model.BeginDrag(); Model.UpdateDrag(FVector(0, 0, 0), INDEX_NONE, 0); Model.EndDrag();
	TestFalse(TEXT("No-op does not dirty clean package"), Fixture.Entity->GetPackage()->IsDirty());
	GEditor->UndoTransaction(); Model.Tick();
	TestEqual(TEXT("Canceled/no-op drag did not add undo entries"), Fixture.Entity->StartingCoordinates.GetCoordinate(Fixture.X), 2.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNarrativeSpaceWidgetTest, "Narrative.Space.EditorWidget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNarrativeSpaceWidgetTest::RunTest(const FString& Parameters)
{
	const TSharedRef<SNarrativeSpaceEditor> Widget = SNew(SNarrativeSpaceEditor);
	Widget->SlatePrepass();
	TestTrue(TEXT("Native editor constructs its query, viewport and details panels"), Widget->GetDesiredSize().X > 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNarrativeSpaceRenderTest, "Narrative.Space.Render",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNarrativeSpaceRenderTest::RunTest(const FString& Parameters)
{
	if (!FApp::CanEverRender()) { AddInfo(TEXT("Rendering check requires a rendering RHI; model tests support NullRHI.")); return true; }
	auto Fixture = MakeShared<NarrativeSpaceTests::FFixture>();
	Fixture->Entity->StartingCoordinates.SetCoordinate(Fixture->X, -8);
	Fixture->Entity->StartingCoordinates.SetCoordinate(Fixture->Y, 4);
	Fixture->Dialog->Location.SetCoordinate(Fixture->X, 6);
	Fixture->Dialog->Location.SetCoordinate(Fixture->Y, -3);
	Fixture->Dialog->Label = FText::FromString(TEXT("A moment of courage"));
	for (int32 Index = 0; Index < 7; ++Index)
	{
		auto* Asset = Fixture->Add<UArchetypeDef>(*FString::Printf(TEXT("Archetype_%d"), Index));
		Asset->DisplayName = FText::FromString(FString::Printf(TEXT("Archetype %d"), Index + 1));
		Asset->Location.SetCoordinate(Fixture->X, (Index - 3) * 4);
		if (Index % 2 == 0) { Asset->Location.SetCoordinate(Fixture->Y, (Index % 3 - 1) * 6); }
	}
	const auto Widget = SNew(SNarrativeSpaceEditor).InitialQuery(Fixture->Query());
	const auto Window = SNew(SWindow).Title(FText::FromString(TEXT("Narrative Space automation preview")))
		.ClientSize(FVector2D(1440, 900)).SupportsMaximize(false).SupportsMinimize(false)[Widget];
	FSlateApplication::Get().AddWindow(Window);
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Fixture, Widget, Window]()
	{
		const auto Viewport = NarrativeSpaceTests::FindViewport(Widget);
		if (TestTrue(TEXT("Editor contains reusable viewport"), Viewport.IsValid()))
		{
			const FGeometry Geometry = Viewport->GetCachedGeometry();
			TArray<FVector> Positions;
			for (const auto& Point : Widget->GetModel()->GetPoints()) { Positions.Add(Point.Position); }
			FNarrativeSpaceCamera ExpectedCamera;
			ExpectedCamera.Frame(Positions, Geometry.GetLocalSize());
			const FVector2D Screen = Geometry.LocalToAbsolute(ExpectedCamera.Project(FVector(-8, 4, 0), Geometry.GetLocalSize()));
			const TSet<FKey> Buttons{EKeys::LeftMouseButton};
			Viewport->OnMouseButtonDown(Geometry, FPointerEvent(0, Screen, Screen, Buttons, EKeys::LeftMouseButton, 0, FModifierKeysState()));
			TestTrue(TEXT("Initial framing uses arranged viewport size and hit testing selects the entity"), Widget->GetModel()->IsSelected(Fixture->Entity));
			Viewport->OnKeyDown(Geometry, FKeyEvent(EKeys::Escape, FModifierKeysState(), 0, false, 0, 0));
		}
		TArray<FColor> Pixels;
		FIntVector Size;
		if (TestTrue(TEXT("Editor paints to a screenshot"), FSlateApplication::Get().TakeScreenshot(Widget, Pixels, Size)))
		{
			TArray64<uint8> Png;
			FImageUtils::PNGCompressImageArray(Size.X, Size.Y, Pixels, Png);
			const FString Directory = FPaths::ProjectSavedDir() / TEXT("Automation/NarrativeSpace");
			IFileManager::Get().MakeDirectory(*Directory, true);
			TestTrue(TEXT("Save visual review artifact"), FFileHelper::SaveArrayToFile(Png, *(Directory / TEXT("Editor.png"))));
		}
		Window->RequestDestroyWindow();
	}, 1.0f));
	return true;
}
#endif
