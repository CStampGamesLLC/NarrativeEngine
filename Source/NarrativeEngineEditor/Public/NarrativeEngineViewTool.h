// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EditorUtilityWidget.h"
#include "Components/CanvasPanelSlot.h"
#include "UObject/Object.h"
#include "NarrativeEngineViewTool.generated.h"

/*
 * Visual representation of an FNarrativeEntityInstance
 */
UCLASS(Blueprintable)
class UNarrativeEntityViewWidget : public UUserWidget
{
	GENERATED_BODY()
};
/**
 * 
 */
UCLASS(Blueprintable)
class NARRATIVEENGINEEDITOR_API UNarrativeEngineViewTool : public UEditorUtilityWidget
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftClassPtr<UNarrativeEntityViewWidget> ViewWidgetClass;

	TWeakObjectPtr<UWorld> GameWorld;
	
	UPROPERTY(VisibleAnywhere)
	TMap<FName, UNarrativeEntityViewWidget*> EntityViews;

	void InitializeViewWidgets();
	void NativeConstruct() override;
	void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
};
