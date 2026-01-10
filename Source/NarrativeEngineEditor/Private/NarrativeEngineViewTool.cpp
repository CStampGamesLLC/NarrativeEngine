// Fill out your copyright notice in the Description page of Project Settings.


#include "NarrativeEngineViewTool.h"

#include "NarrativeSubsystem.h"
#include "Kismet/GameplayStatics.h"

void UNarrativeEngineViewTool::InitializeViewWidgets()
{
	UNarrativeSubsystem::ForeachEntity(GameWorld.Get(), [this](FNarrativeEntityInstance& Entity)
	{
		UNarrativeEntityViewWidget* NewViewWidget = CreateWidget<UNarrativeEntityViewWidget>(this, ViewWidgetClass.Get());
		EntityViews.Add(Entity.Name, NewViewWidget);
	});
}

void UNarrativeEngineViewTool::NativeConstruct()
{
	Super::NativeConstruct();

}

void UNarrativeEngineViewTool::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!IsValid(GEditor) || !GEditor->IsPlaySessionInProgress())
	{
		return;
	}

	GameWorld = GEditor->GetPIEWorldContext()->World();
	UNarrativeSubsystem* NarrativeSubsystem = GameWorld.IsValid() ? GameWorld->GetSubsystem<UNarrativeSubsystem>() : nullptr;
	if (!ensure(IsValid(NarrativeSubsystem)))
	{
		return;
	}

	if (EntityViews.Num() != NarrativeSubsystem->Scene.Entities.Num())
	{
		InitializeViewWidgets();
	}
				
	UNarrativeSubsystem::ForeachEntity(GameWorld.Get(), [this](FNarrativeEntityInstance& Entity)
	{
		if (!EntityViews.Contains(Entity.Name) || !IsValid(EntityViews[Entity.Name]))
		{
			return;
		}

		UNarrativeEntityViewWidget& View = *EntityViews[Entity.Name];

		// Assuming the entity has a 2D position stored as FVector or FVector2D
		FVector2D EntityPosition = FVector2D(Entity.Position[0], Entity.Position[1]);

		// Convert world-space position to UI-space coordinates (example scaling factor)
		constexpr float ScaleFactor = 10.0f;  // Adjust based on how large the grid should be
		FVector2D UISpacePosition = EntityPosition * ScaleFactor;

		// Get the parent Canvas Slot to move the widget
		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(View.Slot))
		{
			CanvasSlot->SetPosition(UISpacePosition);
		}
	});
}
