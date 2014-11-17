// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "IntroTutorialsPrivatePCH.h"
#include "STutorialOverlay.h"
#include "STutorialContent.h"
#include "EditorTutorial.h"
#include "IntroTutorials.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "LevelEditor.h"
#include "BlueprintEditorUtils.h"
#include "Guid.h"

static FName IntroTutorialsModuleName("IntroTutorials");

void STutorialOverlay::Construct(const FArguments& InArgs, UEditorTutorial* InTutorial, FTutorialStage* const InStage)
{
	ParentWindow = InArgs._ParentWindow;
	bIsStandalone = InArgs._IsStandalone;
	OnClosed = InArgs._OnClosed;
	bHasValidContent = InStage != nullptr;
	OnWidgetWasDrawn = InArgs._OnWidgetWasDrawn;

	TSharedPtr<SOverlay> Overlay;

	ChildSlot
	[
		SAssignNew(Overlay, SOverlay)
		+SOverlay::Slot()
		[
			SAssignNew(OverlayCanvas, SCanvas)
		]
	];

	if(InStage != nullptr)
	{
		// add non-widget content, if any
		if(InArgs._AllowNonWidgetContent && InStage->Content.Type != ETutorialContent::None)
		{
			Overlay->AddSlot()
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Center)
				[
					SNew(STutorialContent, InTutorial, InStage->Content)
					.OnClosed(InArgs._OnClosed)
					.OnNextClicked(InArgs._OnNextClicked)
					.OnHomeClicked(InArgs._OnHomeClicked)
					.OnBackClicked(InArgs._OnBackClicked)
					.IsBackEnabled(InArgs._IsBackEnabled)
					.IsHomeEnabled(InArgs._IsHomeEnabled)
					.IsNextEnabled(InArgs._IsNextEnabled)
					.IsStandalone(InArgs._IsStandalone)
					.WrapTextAt(600.0f)
				]
			];
		}

		if(InStage->WidgetContent.Num() > 0)
		{
			FIntroTutorials& IntroTutorials = FModuleManager::Get().GetModuleChecked<FIntroTutorials>("IntroTutorials");

			// now add canvas slots for widget-bound content
			for (const FTutorialWidgetContent& WidgetContent : InStage->WidgetContent)
			{
				if (WidgetContent.Content.Type != ETutorialContent::None)
				{
					TSharedPtr<STutorialContent> ContentWidget =
						SNew(STutorialContent, InTutorial, WidgetContent.Content)
						.HAlign(WidgetContent.HorizontalAlignment)
						.VAlign(WidgetContent.VerticalAlignment)
						.Offset(WidgetContent.Offset)
						.IsStandalone(bIsStandalone)
						.OnClosed(InArgs._OnClosed)
						.OnNextClicked(InArgs._OnNextClicked)
						.OnHomeClicked(InArgs._OnHomeClicked)
						.OnBackClicked(InArgs._OnBackClicked)
						.IsBackEnabled(InArgs._IsBackEnabled)
						.IsHomeEnabled(InArgs._IsHomeEnabled)
						.IsNextEnabled(InArgs._IsNextEnabled)
						.WrapTextAt(WidgetContent.ContentWidth)
						.Anchor(WidgetContent.WidgetAnchor)
						.AllowNonWidgetContent(InArgs._AllowNonWidgetContent)
						.OnWasWidgetDrawn(InArgs._OnWasWidgetDrawn)
						.NextButtonText(InStage->NextButtonText);
					
					PerformWidgetInteractions(WidgetContent); 					

					OverlayCanvas->AddSlot()
						.Position(TAttribute<FVector2D>::Create(TAttribute<FVector2D>::FGetter::CreateSP(ContentWidget.Get(), &STutorialContent::GetPosition)))
						.Size(TAttribute<FVector2D>::Create(TAttribute<FVector2D>::FGetter::CreateSP(ContentWidget.Get(), &STutorialContent::GetSize)))
						[
							ContentWidget.ToSharedRef()
						];

					OnPaintNamedWidget.AddSP(ContentWidget.Get(), &STutorialContent::HandlePaintNamedWidget);
					OnResetNamedWidget.AddSP(ContentWidget.Get(), &STutorialContent::HandleResetNamedWidget);
					OnCacheWindowSize.AddSP(ContentWidget.Get(), &STutorialContent::HandleCacheWindowSize);
				}
			}
		}
	}
}

int32 STutorialOverlay::OnPaint( const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const
{
	if(ParentWindow.IsValid())
	{
		bool bIsPicking = false;
		FName WidgetNameToHighlight = NAME_None;
		FIntroTutorials& IntroTutorials = FModuleManager::Get().GetModuleChecked<FIntroTutorials>(IntroTutorialsModuleName);
		if(IntroTutorials.OnIsPicking().IsBound())
		{
			bIsPicking = IntroTutorials.OnIsPicking().Execute(WidgetNameToHighlight);
		}

		if(bIsPicking || bHasValidContent)
		{
			TSharedPtr<SWindow> PinnedWindow = ParentWindow.Pin();
			OnResetNamedWidget.Broadcast();
			OnCacheWindowSize.Broadcast(PinnedWindow->GetWindowGeometryInWindow().Size);
			LayerId = TraverseWidgets(PinnedWindow.ToSharedRef(), PinnedWindow->GetWindowGeometryInWindow(), MyClippingRect, OutDrawElements, LayerId);
		}
	}
	
	return SCompoundWidget::OnPaint(Args, AllottedGeometry, MyClippingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
}

int32 STutorialOverlay::TraverseWidgets(TSharedRef<SWidget> InWidget, const FGeometry& InGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId) const
{
	TSharedPtr<FTagMetaData> MetaData = InWidget->GetMetaData<FTagMetaData>();
	const FName Tag = (MetaData.IsValid() && MetaData->Tag.IsValid()) ? MetaData->Tag : InWidget->GetTag();	
	if(Tag != NAME_None || MetaData.IsValid())
	{
		// we are a named widget - ask it to draw
		OnPaintNamedWidget.Broadcast(InWidget, InGeometry);
		OnWidgetWasDrawn.ExecuteIfBound(Tag);

		// if we are picking, we need to draw an outline here
		FName WidgetNameToHighlight = NAME_None;
		bool bIsPicking = false;
		FIntroTutorials& IntroTutorials = FModuleManager::Get().GetModuleChecked<FIntroTutorials>(IntroTutorialsModuleName);
		if(IntroTutorials.OnIsPicking().IsBound())
		{
			bIsPicking = IntroTutorials.OnIsPicking().Execute(WidgetNameToHighlight);
		}
	
		if(WidgetNameToHighlight != NAME_None || bIsPicking)
		{
			const bool bHighlight = WidgetNameToHighlight != NAME_None && WidgetNameToHighlight == Tag;
			if(bIsPicking || (!bIsPicking && bHighlight))
			{
				const FLinearColor Color = bIsPicking && bHighlight ? FLinearColor::Green : FLinearColor::White;
				FSlateDrawElement::MakeBox(OutDrawElements, LayerId++, InGeometry.ToPaintGeometry(), FCoreStyle::Get().GetBrush(TEXT("Debug.Border")), MyClippingRect, ESlateDrawEffect::None, Color);
			}
		}	
	}

	FArrangedChildren ArrangedChildren(EVisibility::Visible);
	InWidget->ArrangeChildren(InGeometry, ArrangedChildren);
	for(int32 ChildIndex = 0; ChildIndex < ArrangedChildren.Num(); ChildIndex++)
	{
		const FArrangedWidget& ArrangedWidget = ArrangedChildren[ChildIndex];
		LayerId = TraverseWidgets(ArrangedWidget.Widget, ArrangedWidget.Geometry, MyClippingRect, OutDrawElements, LayerId);
	}

	return LayerId;
}

void STutorialOverlay::PerformWidgetInteractions(const FTutorialWidgetContent &WidgetContent)
{
	// Open any browser we need too
	OpenBrowserForWidgetAnchor(WidgetContent);

	FocusOnAnyBlueprintNodes(WidgetContent);
}

void STutorialOverlay::OpenBrowserForWidgetAnchor(const FTutorialWidgetContent &WidgetContent)
{
	bool bTabOpened = false;
	// Open the required tab if we found it in the map
	if (WidgetContent.WidgetAnchor.TabTypeToOpen.IsEmpty() == false)
	{
		FLevelEditorModule& LevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>(TEXT("LevelEditor"));
		TSharedPtr<FTabManager> LevelEditorTabManager = LevelEditorModule.GetLevelEditorTabManager();
		TSharedPtr<SDockTab> DocTab = LevelEditorTabManager->InvokeTab(FName(*WidgetContent.WidgetAnchor.TabTypeToOpen));
		bTabOpened = DocTab.IsValid();
	}
	if (bTabOpened == false)
	{
		//bTabOpened = FAssetEditorManager::Get().OpenEditorForAsset()
	}
	
}

void STutorialOverlay::FocusOnAnyBlueprintNodes(const FTutorialWidgetContent &WidgetContent)
{
	if (WidgetContent.bAutoFocus == false)
	{
		return;
	}
	FString Name = WidgetContent.WidgetAnchor.OuterName;
	int32 NameIndex;
	Name.FindLastChar(TEXT('.'), NameIndex);
	FString BlueprintName = Name.RightChop(NameIndex + 1);
	UBlueprint* Blueprint = FindObject<UBlueprint>(ANY_PACKAGE, *BlueprintName);
	// If we find a blueprint
	if (Blueprint != nullptr)
	{
		// Try to grab guid
		FGuid NodeGuid;
		FGuid::Parse(WidgetContent.WidgetAnchor.GUIDString, NodeGuid);
		UEdGraphNode* OutNode = NULL;
		if (UEdGraphNode* GraphNode = FBlueprintEditorUtils::GetNodeByGUID(Blueprint, NodeGuid))
		{
			FKismetEditorUtilities::BringKismetToFocusAttentionOnObject(GraphNode, false);
		}
	}
}
