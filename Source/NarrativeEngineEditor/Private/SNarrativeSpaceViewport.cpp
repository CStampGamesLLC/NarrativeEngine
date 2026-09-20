#include "SNarrativeSpaceViewport.h"

#include "DragAndDrop/AssetDragDropOp.h"
#include "Editor.h"
#include "InputCoreTypes.h"
#include "Rendering/DrawElements.h"
#include "Styling/AppStyle.h"

namespace NarrativeSpaceDraw
{
	/** Card edge lengths, in pixels, at which the viewport switches level of detail. */
	constexpr double GlyphCardSize = 12.0;   // Below this a card is a bare box, with no icon.
	constexpr double DetailCardSize = 48.0;  // Below this cards cluster into dots; at or above they get labels.
	constexpr double LargeCardSize = 140.0;  // At or above this cards also show coordinates and class name.

	/** Gap between stacked number-line lanes, and between a lane entry and its neighbour. */
	constexpr double LaneGap = 8.0;

	constexpr int32 RingSegments = 64;

	const FVector2D ClusterHalfSize(23, 16);

	const FLinearColor BackgroundColor(0.018f, 0.024f, 0.033f);
	const FLinearColor GridLineColor(0.07f, 0.085f, 0.1f);
	const FLinearColor CardInteriorColor(0.035f, 0.045f, 0.06f);
	const FLinearColor SelectedCardColor(1.f, 0.75f, 0.2f);
	const FLinearColor MarqueeFillColor(0.15f, 0.45f, 0.9f, 0.12f);
	const FLinearColor MarqueeOutlineColor(0.3f, 0.6f, 1.f);
	const FLinearColor ClassTextColor(0.5f, 0.6f, 0.7f);
	const FLinearColor HudTextColor(0.6f, 0.7f, 0.8f);
	const FLinearColor AxisColors[] = {FLinearColor(1.f, 0.3f, 0.25f), FLinearColor(0.3f, 0.85f, 0.4f), FLinearColor(0.3f, 0.6f, 1.f)};

	/** Opacity applied to an entry that leaves one of the plotted axes undefined, when dimmed. */
	constexpr float DimOpacity = 0.25f;

	/** Gap held between an axis label and both its own axis line and the viewport edge. */
	constexpr double AxisLabelMargin = 12.0;

	/** Rough glyph advance in SmallFont, used only to keep a label inside the viewport. */
	constexpr double GlyphWidth = 7.0;

	/**
	 * Clips the infinite line Origin + t * Direction against the rect (0,0)..Size, returning the
	 * parameter range that lies inside it. False when the line misses the viewport entirely.
	 */
	bool ClipLineToRect(const FVector2D& Origin, const FVector2D& Direction, const FVector2D& Size,
		double& OutEnter, double& OutExit)
	{
		double Enter = -TNumericLimits<double>::Max();
		double Exit = TNumericLimits<double>::Max();
		for (int32 Component = 0; Component < 2; ++Component)
		{
			if (FMath::IsNearlyZero(Direction[Component]))
			{
				// Parallel to this pair of edges: either wholly inside them or wholly outside.
				if (Origin[Component] < 0.0 || Origin[Component] > Size[Component]) { return false; }
				continue;
			}
			double Near = (0.0 - Origin[Component]) / Direction[Component];
			double Far = (Size[Component] - Origin[Component]) / Direction[Component];
			if (Near > Far) { Swap(Near, Far); }
			Enter = FMath::Max(Enter, Near);
			Exit = FMath::Min(Exit, Far);
		}
		if (Enter > Exit) { return false; }
		OutEnter = Enter;
		OutExit = Exit;
		return true;
	}

	/** Display letter for a query axis index. */
	const TCHAR* AxisLabel(const int32 Axis)
	{
		switch (Axis)
		{
		case 0:  return TEXT("X");
		case 1:  return TEXT("Y");
		default: return TEXT("Z");
		}
	}
}

void SNarrativeSpaceViewport::Construct(const FArguments& Args)
{
	Model = Args._Model;
	SetClipping(EWidgetClipping::ClipToBounds);
}

SNarrativeSpaceViewport::~SNarrativeSpaceViewport()
{
	FinishInteraction(true);
}

void SNarrativeSpaceViewport::ResetCamera()
{
	FinishInteraction(true);
	Camera = FNarrativeSpaceCamera();
	bFramePending = true;
}

void SNarrativeSpaceViewport::FrameAll(const bool bSelectionOnly)
{
	// Before the first arrange there is no size to frame against; Tick retries once there is.
	if (ViewportSize.X <= 0 || ViewportSize.Y <= 0)
	{
		bFramePending = true;
		return;
	}

	TArray<FVector> Positions;
	for (const FNarrativeSpacePoint& Point : Model->GetPoints())
	{
		if (IsHidden(Point)) { continue; }
		if (!bSelectionOnly || Model->IsSelected(Point.Asset.Get())) { Positions.Add(Point.Position); }
	}
	Camera.Frame(Positions, ViewportSize);
}

void SNarrativeSpaceViewport::SetView(const ENarrativeSpaceView View)
{
	FinishInteraction(true);
	Camera.SetView(Model->GetQuery().Axes.Num() < 3 ? ENarrativeSpaceView::XY : View);
	FrameAll();
}

double SNarrativeSpaceViewport::CardSize() const
{
	// Cards grow with the square root of zoom so a deep zoom stays readable without swamping the view.
	return FMath::Clamp(64.0 * FMath::Sqrt(Camera.Zoom / 24.0), 5.0, 200.0);
}

double SNarrativeSpaceViewport::ColumnWidth() const
{
	const double Extent = CardSize();
	return FMath::Max(Extent < NarrativeSpaceDraw::DetailCardSize ? Extent : Extent * 2.0, 1.0);
}

bool SNarrativeSpaceViewport::IsIncomplete(const FNarrativeSpacePoint& Point) const
{
	const int32 AxisCount = Model->GetQuery().Axes.Num();
	return Point.PresentAxes != uint8((1 << AxisCount) - 1);
}

bool SNarrativeSpaceViewport::IsHidden(const FNarrativeSpacePoint& Point) const
{
	return IncompleteDisplay == ENarrativeSpaceIncomplete::Hide && IsIncomplete(Point);
}

void SNarrativeSpaceViewport::SetIncompleteDisplay(const ENarrativeSpaceIncomplete Mode)
{
	if (IncompleteDisplay == Mode) { return; }
	// Hiding can drop the entry under an in-flight gesture, so settle first.
	FinishInteraction(true);
	IncompleteDisplay = Mode;
}

void SNarrativeSpaceViewport::SelectNext(const int32 Step)
{
	if (ViewportSize.X <= 0 || ViewportSize.Y <= 0 || Step == 0) { return; }

	const TArray<FNarrativeSpacePoint>& Points = Model->GetPoints();
	TArray<int32> Order;
	TArray<FVector2D> Screens;
	for (int32 Index = 0; Index < Points.Num(); ++Index)
	{
		if (!Points[Index].Asset.IsValid() || IsHidden(Points[Index])) { continue; }
		Order.Add(Index);
		Screens.Add(Camera.Project(Points[Index].Position, ViewportSize));
	}
	if (Order.IsEmpty()) { return; }

	// Column-major reading order: all the way down one column, then the next column to the right.
	// Entries are bucketed by card width because exact screen X values almost never coincide.
	const double Column = ColumnWidth();
	TArray<int32> Slots;
	for (int32 Slot = 0; Slot < Order.Num(); ++Slot) { Slots.Add(Slot); }
	Slots.Sort([&](const int32 A, const int32 B)
	{
		const int32 ColumnA = FMath::FloorToInt(Screens[A].X / Column);
		const int32 ColumnB = FMath::FloorToInt(Screens[B].X / Column);
		if (ColumnA != ColumnB) { return ColumnA < ColumnB; }
		if (Screens[A].Y != Screens[B].Y) { return Screens[A].Y < Screens[B].Y; }
		return Screens[A].X < Screens[B].X;
	});

	// Walk on from the last selected entry in that order, so repeated presses advance.
	int32 Current = INDEX_NONE;
	for (int32 Position = 0; Position < Slots.Num(); ++Position)
	{
		if (Model->IsSelected(Points[Order[Slots[Position]]].Asset.Get())) { Current = Position; }
	}
	const int32 Next = Current == INDEX_NONE
		? (Step > 0 ? 0 : Slots.Num() - 1)
		: ((Current + Step) % Slots.Num() + Slots.Num()) % Slots.Num();

	const int32 Chosen = Slots[Next];
	Model->Select({Points[Order[Chosen]].Asset.Get()});

	// Bring an off-screen entry into view without disturbing the zoom the user set.
	const FVector2D Screen = Screens[Chosen];
	if (Screen.X < 0 || Screen.Y < 0 || Screen.X > ViewportSize.X || Screen.Y > ViewportSize.Y)
	{
		Camera.Center = Points[Order[Chosen]].Position;
	}
}

TArray<SNarrativeSpaceViewport::FVisibleItem> SNarrativeSpaceViewport::VisibleItems(const FVector2D& Size) const
{
	using namespace NarrativeSpaceDraw;

	TArray<FVisibleItem> Result;
	TMap<FIntPoint, int32> Bins;
	TArray<TArray<FVector2D>> Lanes;

	const double Extent = CardSize();
	const TArray<FNarrativeSpacePoint>& Points = Model->GetPoints();
	const int32 AxisCount = Model->GetQuery().Axes.Num();
	const FVector Normal = Camera.Right.Cross(Camera.Up);

	for (int32 Index = 0; Index < Points.Num(); ++Index)
	{
		const FNarrativeSpacePoint& Point = Points[Index];
		if (!Point.Asset.IsValid() || IsHidden(Point)) { continue; }
		const bool bIncomplete = IsIncomplete(Point);

		FVector2D Screen = Camera.Project(Point.Position, Size);
		const FVector2D Half = Extent < DetailCardSize
			? FVector2D(Extent * 0.5)
			: FVector2D(Extent, Extent >= LargeCardSize ? 42.0 : 18.0);

		if (AxisCount == 1 && Extent >= DetailCardSize)
		{
			// Screen-only lanes keep coincident number-line points individually editable.
			int32 Lane = 0;
			for (; Lane < Lanes.Num(); ++Lane)
			{
				const bool bOccupied = Lanes[Lane].ContainsByPredicate([&](const FVector2D& Range)
				{
					return Screen.X + Half.X + LaneGap >= Range.X && Screen.X - Half.X - LaneGap <= Range.Y;
				});
				if (!bOccupied) { break; }
			}
			if (Lane == Lanes.Num()) { Lanes.AddDefaulted(); }
			Lanes[Lane].Add(FVector2D(Screen.X - Half.X, Screen.X + Half.X));
			Screen.Y += (Lane % 2 == 0 ? Lane / 2 : -(Lane + 1) / 2) * (Half.Y * 2 + LaneGap);
		}

		const bool bOffscreen = Screen.X + Half.X < 0 || Screen.Y + Half.Y < 0
			|| Screen.X - Half.X > Size.X || Screen.Y - Half.Y > Size.Y;
		if (bOffscreen) { continue; }

		if (Extent < DetailCardSize)
		{
			// Bins are one detail card wide, so clusters stay roughly card-sized on screen.
			const FIntPoint Bin(FMath::FloorToInt(Screen.X / DetailCardSize), FMath::FloorToInt(Screen.Y / DetailCardSize));
			if (int32* Existing = Bins.Find(Bin))
			{
				FVisibleItem& Cluster = Result[*Existing];
				Cluster.Screen = (Cluster.Screen * Cluster.Indices.Num() + Screen) / (Cluster.Indices.Num() + 1);
				Cluster.Indices.Add(Index);
				Cluster.HalfSize = ClusterHalfSize;
				// A cluster only reads as incomplete when nothing in it defines every axis.
				Cluster.bIncomplete &= bIncomplete;
				continue;
			}
			Bins.Add(Bin, Result.Num());
		}

		FVisibleItem& Item = Result.AddDefaulted_GetRef();
		Item.Screen = Screen;
		Item.HalfSize = Half;
		Item.Indices.Add(Index);
		Item.bIncomplete = bIncomplete;
		Item.Depth = (Point.Position - Camera.Center).Dot(Normal);
	}

	Result.StableSort([](const FVisibleItem& A, const FVisibleItem& B) { return A.Depth < B.Depth; });
	return Result;
}

TArray<int32> SNarrativeSpaceViewport::Hit(const FVector2D& At, const FVector2D& Size) const
{
	const TArray<FVisibleItem> Items = VisibleItems(Size);
	for (int32 Index = Items.Num() - 1; Index >= 0; --Index)
	{
		const FVisibleItem& Item = Items[Index];
		// Tiny cards keep a minimum grab radius so single dots stay clickable.
		if (FMath::Abs(At.X - Item.Screen.X) <= FMath::Max(6.0, Item.HalfSize.X)
			&& FMath::Abs(At.Y - Item.Screen.Y) <= FMath::Max(6.0, Item.HalfSize.Y))
		{
			return Item.Indices;
		}
	}
	return {};
}

void SNarrativeSpaceViewport::ApplyDrag(const FVector2D& At, const bool bSnap)
{
	Model->UpdateDrag(Camera.DragDelta(At - MouseDown, AxisLock), AxisLock, bSnap ? Camera.GridStep() : 0.0);
}

void SNarrativeSpaceViewport::FinishInteraction(const bool bCancel)
{
	Model->EndDrag(bCancel);
	bPan = false;
	bOrbit = false;
	bPendingDrag = false;
	bMarquee = false;
	AxisLock = INDEX_NONE;
}

int32 SNarrativeSpaceViewport::OnPaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& CullingRect,
	FSlateWindowElementList& OutElements, int32 LayerId, const FWidgetStyle& Style, bool bParentEnabled) const
{
	using namespace NarrativeSpaceDraw;

	const FVector2D Size = Geometry.GetLocalSize();
	const FSlateBrush* White = FAppStyle::GetBrush("WhiteBrush");
	const FSlateFontInfo Font = FAppStyle::GetFontStyle("SmallFont");

	auto Box = [&](FVector2D Pos, FVector2D Dimensions, FLinearColor Color, int32 Z)
	{
		FSlateDrawElement::MakeBox(OutElements, Z, Geometry.ToPaintGeometry(Dimensions, FSlateLayoutTransform(Pos)),
			White, ESlateDrawEffect::None, Color);
	};
	auto Line = [&](const TArray<FVector2D>& Vertices, FLinearColor Color, int32 Z, float Thickness = 1.f)
	{
		FSlateDrawElement::MakeLines(OutElements, Z, Geometry.ToPaintGeometry(), Vertices,
			ESlateDrawEffect::None, Color, true, Thickness);
	};
	auto Text = [&](FVector2D Pos, const FString& Value, FLinearColor Color, int32 Z)
	{
		FSlateDrawElement::MakeText(OutElements, Z, Geometry.ToPaintGeometry(Size, FSlateLayoutTransform(Pos)),
			Value, Font, ESlateDrawEffect::None, Color);
	};

	const FNarrativeSpaceQuery& Query = Model->GetQuery();
	const int32 AxisCount = Query.Axes.Num();
	const TArray<FNarrativeSpacePoint>& Points = Model->GetPoints();

	// Background and grid.
	Box(FVector2D::ZeroVector, Size, BackgroundColor, LayerId);
	const double Step = Camera.GridStep() * Camera.Zoom;
	const FVector2D Origin = Camera.Project(FVector::ZeroVector, Size);
	for (double X = FMath::Fmod(Origin.X, Step); X < Size.X; X += Step)
	{
		Line({FVector2D(X, 0), FVector2D(X, Size.Y)}, GridLineColor, LayerId + 1);
	}
	if (AxisCount > 1)
	{
		for (double Y = FMath::Fmod(Origin.Y, Step); Y < Size.Y; Y += Step)
		{
			Line({FVector2D(0, Y), FVector2D(Size.X, Y)}, GridLineColor, LayerId + 1);
		}
	}

	// Axis lines through the origin.
	for (int32 Axis = 0; Axis < AxisCount && Axis < 3; ++Axis)
	{
		FVector2D Direction(Camera.Right[Axis], -Camera.Up[Axis]);
		if (!Direction.IsNearlyZero())
		{
			Direction.Normalize();
			Line({Origin - Direction * Size.Size() * 2, Origin + Direction * Size.Size() * 2},
				AxisColors[Axis] * 0.45f, LayerId + 2);
		}
	}

	// Cards, back to front.
	const double Extent = CardSize();
	int32 NextLayer = LayerId + 3;
	for (const FVisibleItem& Item : VisibleItems(Size))
	{
		// Keep all primitives for a card together so nearer cards occlude far labels too.
		const int32 CardLayer = NextLayer;
		NextLayer += 6;

		const FNarrativeSpacePoint& Point = Points[Item.Indices[0]];
		const bool bSelected = Item.Indices.ContainsByPredicate([&](int32 Index)
		{
			return Model->IsSelected(Points[Index].Asset.Get());
		});

		FLinearColor Color = bSelected ? SelectedCardColor : Point.Color;
		if (AxisCount == 3 && !bSelected)
		{
			// Fade with distance from the focal plane so depth reads without a perspective projection.
			Color.A = FMath::Clamp(1.0 / (1.0 + FMath::Abs(Item.Depth) * Camera.Zoom / 600.0), 0.35, 1.0);
		}

		// Entries missing one of the plotted axes recede instead of competing with complete ones.
		const float IncompleteFade = Item.bIncomplete && IncompleteDisplay == ENarrativeSpaceIncomplete::Dim ? DimOpacity : 1.f;
		Color.A *= IncompleteFade;

		if (Item.Indices.Num() == 1 && Extent >= LargeCardSize && Point.Radius > 0)
		{
			const double Radius = Point.Radius * Camera.Zoom;
			if (Radius < Size.Size() * 4)
			{
				TArray<FVector2D> Ring;
				Ring.Reserve(RingSegments + 1);
				for (int32 Segment = 0; Segment <= RingSegments; ++Segment)
				{
					const double Angle = Segment * UE_DOUBLE_TWO_PI / RingSegments;
					Ring.Add(Item.Screen + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius);
				}
				Line(Ring, Color.CopyWithNewOpacity(0.3f), CardLayer);
			}
		}

		const FVector2D TopLeft = Item.Screen - Item.HalfSize;
		Box(TopLeft, Item.HalfSize * 2, Color, CardLayer + 1);

		// Hollow out the card so the border reads as an outline rather than a solid blob.
		if (Item.bIncomplete || Extent >= DetailCardSize || Item.Indices.Num() > 1)
		{
			Box(TopLeft + FVector2D(2), Item.HalfSize * 2 - FVector2D(4), CardInteriorColor, CardLayer + 2);
		}

		if (Item.Indices.Num() > 1)
		{
			Text(TopLeft + FVector2D(5, 7), FString::Printf(TEXT("x%d"), Item.Indices.Num()), Color, CardLayer + 3);
			continue;
		}

		if (Extent >= GlyphCardSize)
		{
			const double IconSize = FMath::Min(Extent - 6, 26.0);
			const FVector2D IconPos = Extent < DetailCardSize ? Item.Screen - FVector2D(IconSize * 0.5) : TopLeft + FVector2D(6, 5);
			if (Point.Icon.GetResourceObject())
			{
				FSlateDrawElement::MakeBox(OutElements, CardLayer + 3,
					Geometry.ToPaintGeometry(FVector2D(IconSize), FSlateLayoutTransform(IconPos)),
					&Point.Icon, ESlateDrawEffect::None, FLinearColor::White.CopyWithNewOpacity(Color.A));
			}
			else
			{
				Text(IconPos + FVector2D(2), Point.Asset->GetClass()->GetName().Left(1), Color, CardLayer + 3);
			}
		}

		if (Extent < DetailCardSize) { continue; }

		// Per-axis filled/hollow markers remain visible even when the card has an icon.
		for (int32 Axis = 0; Axis < AxisCount; ++Axis)
		{
			const FVector2D Marker = Item.Screen + Item.HalfSize - FVector2D(10 + Axis * 9, 8);
			Box(Marker, FVector2D(6), AxisColors[Axis].CopyWithNewOpacity(IncompleteFade), CardLayer + 4);
			if (!(Point.PresentAxes & (1 << Axis)))
			{
				Box(Marker + FVector2D(1), FVector2D(4), CardInteriorColor, CardLayer + 5);
			}
		}

		const int32 MaxChars = FMath::Max(4, int32((Item.HalfSize.X * 2 - 42) / 7));
		FString Label = Point.Label.ToString();
		if (Label.Len() > MaxChars) { Label = Label.Left(MaxChars - 3) + TEXT("..."); }
		Text(TopLeft + FVector2D(38, 9), Label, FLinearColor::White.CopyWithNewOpacity(Color.A), CardLayer + 3);

		if (Extent >= LargeCardSize)
		{
			FString Coordinates;
			for (int32 Axis = 0; Axis < AxisCount; ++Axis)
			{
				Coordinates += FString::Printf(TEXT("%s %s  "), AxisLabel(Axis),
					(Point.PresentAxes & (1 << Axis)) ? *FString::SanitizeFloat(Point.Position[Axis], 2) : TEXT("unset"));
			}
			Text(TopLeft + FVector2D(8, 36), Coordinates, Color, CardLayer + 3);
			Text(TopLeft + FVector2D(8, 59), Point.Asset->GetClass()->GetName(),
				ClassTextColor.CopyWithNewOpacity(IncompleteFade), CardLayer + 3);
		}
	}

	if (bMarquee)
	{
		const FVector2D A(FMath::Min(MouseDown.X, MarqueeEnd.X), FMath::Min(MouseDown.Y, MarqueeEnd.Y));
		const FVector2D B(FMath::Max(MouseDown.X, MarqueeEnd.X), FMath::Max(MouseDown.Y, MarqueeEnd.Y));
		Box(A, B - A, MarqueeFillColor, NextLayer);
		Line({A, FVector2D(B.X, A.Y), B, FVector2D(A.X, B.Y), A}, MarqueeOutlineColor, NextLayer + 1);
	}

	// Axis labels ride their own axis line so each one is read in context. Above every card.
	for (int32 Axis = 0; Axis < AxisCount && Axis < 3; ++Axis)
	{
		FVector2D Direction(Camera.Right[Axis], -Camera.Up[Axis]);
		if (Direction.IsNearlyZero()) { continue; }
		Direction.Normalize();

		double Enter = 0.0;
		double Exit = 0.0;
		if (!ClipLineToRect(Origin, Direction, Size, Enter, Exit)) { continue; }

		const FString Label = FString::Printf(TEXT("%s: %s"), AxisLabel(Axis), *Query.Axes[Axis].GetAssetName());
		const double Length = Label.Len() * GlyphWidth;
		// Skip rather than spill a label out of the short end of a barely visible axis.
		if (Exit - Enter < Length + AxisLabelMargin * 2.0) { continue; }

		// Glyphs never read right to left or bottom to top, so a vertical axis reads downward.
		const bool bFlip = Direction.X < -UE_DOUBLE_SMALL_NUMBER
			|| (FMath::Abs(Direction.X) <= UE_DOUBLE_SMALL_NUMBER && Direction.Y < 0.0);
		const FVector2D Reading = bFlip ? -Direction : Direction;

		// Anchor at the positive end of the visible segment. Unflipped text runs towards that end,
		// so it starts a label's length back from it; flipped text already runs back from there.
		FVector2D Anchor = Origin + Direction * (Exit - AxisLabelMargin);
		if (!bFlip) { Anchor -= Direction * Length; }
		// Glyphs hang off the anchor along the rotated frame's +Y, so nudge that way to clear the line.
		Anchor += FVector2D(-Reading.Y, Reading.X) * 4.0;

		FSlateDrawElement::MakeText(OutElements, NextLayer + 2,
			Geometry.ToPaintGeometry(FVector2D(Length, 16.0), FSlateLayoutTransform(Anchor),
				FSlateRenderTransform(FQuat2D(float(FMath::Atan2(Reading.Y, Reading.X)))), FVector2D::ZeroVector),
			Label, Font, ESlateDrawEffect::None, AxisColors[Axis]);
	}
	if (Points.IsEmpty())
	{
		Text(FVector2D(24, Size.Y * 0.5), Model->GetStatus().ToString(), FLinearColor::White, NextLayer + 2);
	}
	const FString LockLabel = AxisLock == INDEX_NONE
		? FString(TEXT("View-plane drag"))
		: FString::Printf(TEXT("%s locked"), AxisLabel(AxisLock));
	Text(FVector2D(14, Size.Y - 24),
		FString::Printf(TEXT("Grid %g | Zoom %.2f | %s"), Camera.GridStep(), Camera.Zoom, *LockLabel),
		HudTextColor, NextLayer + 2);

	return NextLayer + 3;
}

void SNarrativeSpaceViewport::Tick(const FGeometry& Geometry, double CurrentTime, float DeltaTime)
{
	ViewportSize = Geometry.GetLocalSize();
	Model->Tick();
	if (bFramePending && Geometry.GetLocalSize().X > 0)
	{
		bFramePending = false;
		FrameAll();
	}
}

FReply SNarrativeSpaceViewport::OnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event)
{
	if (HasMouseCapture()) { return FReply::Handled(); }

	MouseDown = LastMouse = MarqueeEnd = Geometry.AbsoluteToLocal(Event.GetScreenSpacePosition());
	bAddSelection = Event.IsShiftDown();

	if (Event.GetEffectingButton() == EKeys::RightMouseButton || Event.GetEffectingButton() == EKeys::MiddleMouseButton)
	{
		bOrbit = Event.IsAltDown() && Event.GetEffectingButton() == EKeys::RightMouseButton && Model->GetQuery().Axes.Num() == 3;
		bPan = !bOrbit;
	}
	else if (Event.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		const TArray<int32> Indices = Hit(MouseDown, Geometry.GetLocalSize());
		if (Indices.IsEmpty())
		{
			if (!bAddSelection) { Model->Select({}); }
			bMarquee = true;
		}
		else
		{
			TArray<UNarrativeDataAsset*> Assets;
			for (int32 Index : Indices) { Assets.Add(Model->GetPoints()[Index].Asset.Get()); }

			// A cluster is too dense to drag meaningfully, so clicking it zooms in on its contents instead.
			if (Indices.Num() > 1)
			{
				Model->Select(Assets, bAddSelection);
				FrameAll(true);
				return FReply::Handled().SetUserFocus(AsShared(), EFocusCause::Mouse);
			}

			if (bAddSelection || !Model->IsSelected(Assets[0])) { Model->Select(Assets, bAddSelection); }
			bPendingDrag = Model->CanEdit();
		}
	}
	else { return FReply::Unhandled(); }

	return FReply::Handled().CaptureMouse(AsShared()).SetUserFocus(AsShared(), EFocusCause::Mouse);
}

FReply SNarrativeSpaceViewport::OnMouseButtonUp(const FGeometry& Geometry, const FPointerEvent& Event)
{
	if (!HasMouseCapture()) { return FReply::Unhandled(); }

	// Camera gestures end on their own button; selection and dragging end on the left button.
	if ((bPan || bOrbit) && Event.GetEffectingButton() == EKeys::LeftMouseButton) { return FReply::Handled(); }
	if (!(bPan || bOrbit) && Event.GetEffectingButton() != EKeys::LeftMouseButton) { return FReply::Handled(); }

	if (bMarquee)
	{
		MarqueeEnd = Geometry.AbsoluteToLocal(Event.GetScreenSpacePosition());
		const FVector2D MarqueeMin(FMath::Min(MouseDown.X, MarqueeEnd.X), FMath::Min(MouseDown.Y, MarqueeEnd.Y));
		const FVector2D MarqueeMax(FMath::Max(MouseDown.X, MarqueeEnd.X), FMath::Max(MouseDown.Y, MarqueeEnd.Y));

		TArray<UNarrativeDataAsset*> Assets;
		for (const FVisibleItem& Item : VisibleItems(Geometry.GetLocalSize()))
		{
			if (Item.Screen.X >= MarqueeMin.X && Item.Screen.X <= MarqueeMax.X
				&& Item.Screen.Y >= MarqueeMin.Y && Item.Screen.Y <= MarqueeMax.Y)
			{
				for (int32 Index : Item.Indices) { Assets.Add(Model->GetPoints()[Index].Asset.Get()); }
			}
		}
		Model->Select(Assets, bAddSelection);
	}

	FinishInteraction(false);
	return FReply::Handled().ReleaseMouseCapture();
}

FReply SNarrativeSpaceViewport::OnMouseMove(const FGeometry& Geometry, const FPointerEvent& Event)
{
	const FVector2D At = Geometry.AbsoluteToLocal(Event.GetScreenSpacePosition());
	if (HasMouseCapture())
	{
		if (bPan) { Camera.Center -= Camera.DragDelta(At - LastMouse); }
		else if (bOrbit) { Camera.Orbit(At - LastMouse); }
		else if (bMarquee) { MarqueeEnd = At; }
		else if (bPendingDrag || Model->IsDragging())
		{
			// Only open a transaction once the pointer clears the click threshold.
			if (bPendingDrag && (At - MouseDown).Size() >= 4.0)
			{
				bPendingDrag = false;
				Model->BeginDrag();
			}
			if (Model->IsDragging()) { ApplyDrag(At, Event.IsControlDown()); }
		}
		LastMouse = At;
		return FReply::Handled();
	}

	const TArray<int32> Indices = Hit(At, Geometry.GetLocalSize());
	FString Tip;
	if (Indices.Num() == 1)
	{
		const FNarrativeSpacePoint& Point = Model->GetPoints()[Indices[0]];
		Tip = Point.Label.ToString() + TEXT("\n") + Point.Asset->GetPathName();
	}
	else if (Indices.Num() > 1)
	{
		Tip = FString::Printf(TEXT("%d assets - click to select and frame"), Indices.Num());
	}
	SetToolTipText(FText::FromString(Tip));
	return FReply::Unhandled();
}

FReply SNarrativeSpaceViewport::OnMouseWheel(const FGeometry& Geometry, const FPointerEvent& Event)
{
	if (!HasMouseCapture())
	{
		Camera.ZoomAt(Geometry.AbsoluteToLocal(Event.GetScreenSpacePosition()), Geometry.GetLocalSize(), Event.GetWheelDelta());
	}
	return FReply::Handled();
}

FReply SNarrativeSpaceViewport::OnMouseButtonDoubleClick(const FGeometry& Geometry, const FPointerEvent& Event)
{
	if (Event.GetEffectingButton() != EKeys::LeftMouseButton) { return FReply::Unhandled(); }

	FinishInteraction(true);
	const TArray<int32> Indices = Hit(Geometry.AbsoluteToLocal(Event.GetScreenSpacePosition()), Geometry.GetLocalSize());
	if (Indices.Num() == 1)
	{
		Model->Select({Model->GetPoints()[Indices[0]].Asset.Get()});
		Model->OpenSelection();
	}
	return FReply::Handled().ReleaseMouseCapture();
}

FReply SNarrativeSpaceViewport::OnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
	const FKey Key = Event.GetKey();
	if (Key == EKeys::Escape)
	{
		FinishInteraction(true);
		return FReply::Handled().ReleaseMouseCapture();
	}

	const int32 Axis = Key == EKeys::X ? 0 : Key == EKeys::Y ? 1 : Key == EKeys::Z ? 2 : INDEX_NONE;
	if (!Event.IsControlDown() && Axis >= 0 && Axis < Model->GetQuery().Axes.Num())
	{
		AxisLock = Axis;
		if (Model->IsDragging()) { ApplyDrag(LastMouse, Event.IsControlDown()); }
		return FReply::Handled();
	}

	// Camera, selection and undo shortcuts would fight an in-progress gesture.
	if (HasMouseCapture()) { return FReply::Handled(); }
	if (Key == EKeys::Tab) { SelectNext(Event.IsShiftDown() ? -1 : 1); return FReply::Handled(); }
	if (Key == EKeys::F) { FrameAll(true); return FReply::Handled(); }
	if (Key == EKeys::Home) { FrameAll(); return FReply::Handled(); }
	if (Event.IsControlDown() && GEditor)
	{
		if (Key == EKeys::Z) { GEditor->UndoTransaction(); return FReply::Handled(); }
		if (Key == EKeys::Y) { GEditor->RedoTransaction(); return FReply::Handled(); }
		if (Key == EKeys::S) { Model->Save(); return FReply::Handled(); }
	}
	return FReply::Unhandled();
}

FReply SNarrativeSpaceViewport::OnKeyUp(const FGeometry& Geometry, const FKeyEvent& Event)
{
	const FKey Key = Event.GetKey();
	const int32 Axis = Key == EKeys::X ? 0 : Key == EKeys::Y ? 1 : Key == EKeys::Z ? 2 : INDEX_NONE;
	if (Axis != INDEX_NONE && Axis == AxisLock)
	{
		AxisLock = INDEX_NONE;
		if (Model->IsDragging()) { ApplyDrag(LastMouse, Event.IsControlDown()); }
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

void SNarrativeSpaceViewport::OnMouseCaptureLost(const FCaptureLostEvent& Event)
{
	FinishInteraction(true);
}

FReply SNarrativeSpaceViewport::OnDragOver(const FGeometry& Geometry, const FDragDropEvent& Event)
{
	return Event.GetOperationAs<FAssetDragDropOp>().IsValid() && Model->CanEdit() ? FReply::Handled() : FReply::Unhandled();
}

FReply SNarrativeSpaceViewport::OnDrop(const FGeometry& Geometry, const FDragDropEvent& Event)
{
	const TSharedPtr<FAssetDragDropOp> Operation = Event.GetOperationAs<FAssetDragDropOp>();
	if (!Operation || !Model->CanEdit()) { return FReply::Unhandled(); }

	// Only assets already plotted by this query can be dropped; the drop moves them as a group.
	TArray<UNarrativeDataAsset*> Assets;
	FVector Center = FVector::ZeroVector;
	for (const FNarrativeSpacePoint& Point : Model->GetPoints())
	{
		const bool bDragged = Operation->GetAssets().ContainsByPredicate([&](const FAssetData& Data)
		{
			return Data.GetSoftObjectPath() == FSoftObjectPath(Point.Asset.Get());
		});
		if (bDragged)
		{
			Assets.Add(Point.Asset.Get());
			Center += Point.Position;
		}
	}
	if (Assets.IsEmpty()) { return FReply::Unhandled(); }

	Center /= Assets.Num();
	Model->Select(Assets);
	if (Model->BeginDrag())
	{
		const FVector2D At = Geometry.AbsoluteToLocal(Event.GetScreenSpacePosition());
		Model->UpdateDrag(Camera.DragDelta(At - Camera.Project(Center, Geometry.GetLocalSize())), INDEX_NONE,
			Event.IsControlDown() ? Camera.GridStep() : 0.0);
		Model->EndDrag();
	}
	return FReply::Handled().SetUserFocus(AsShared(), EFocusCause::Mouse);
}
