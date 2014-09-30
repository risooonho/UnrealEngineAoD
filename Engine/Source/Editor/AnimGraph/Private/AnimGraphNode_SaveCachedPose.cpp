// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AnimGraphPrivatePCH.h"

#include "GraphEditorActions.h"
#include "ScopedTransaction.h"
#include "Kismet2NameValidators.h"
#include "K2ActionMenuBuilder.h" // for FK2ActionMenuBuilder::AddNewNodeAction()
#include "AnimGraphNode_SaveCachedPose.h"
#include "BlueprintNodeSpawner.h"
#include "BlueprintActionDatabaseRegistrar.h"

/////////////////////////////////////////////////////
// FCachedPoseNameValidator

class FCachedPoseNameValidator : public FStringSetNameValidator
{
public:
	FCachedPoseNameValidator(class UBlueprint* InBlueprint, const FString& InExistingName)
		: FStringSetNameValidator(InExistingName)
	{
		TArray<UAnimGraphNode_SaveCachedPose*> Nodes;
		FBlueprintEditorUtils::GetAllNodesOfClass<UAnimGraphNode_SaveCachedPose>(InBlueprint, Nodes);

		for (auto Node : Nodes)
		{
			Names.Add(Node->CacheName);
		}
	}
};

/////////////////////////////////////////////////////
// UAnimGraphNode_ComponentToLocalSpace

#define LOCTEXT_NAMESPACE "A3Nodes"

UAnimGraphNode_SaveCachedPose::UAnimGraphNode_SaveCachedPose(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bCanRenameNode = true;
}

FText UAnimGraphNode_SaveCachedPose::GetTooltipText() const
{
	return LOCTEXT("SaveCachedPose_Tooltip", "Denotes an animation tree that can be referenced elsewhere in the blueprint, which will be evaluated at most once per frame and then cached.");
}

FText UAnimGraphNode_SaveCachedPose::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (TitleType == ENodeTitleType::EditableTitle)
	{
		return FText::FromString(CacheName);
	}
	else if ((TitleType == ENodeTitleType::MenuTitle) && CacheName.IsEmpty())
	{
		return LOCTEXT("NewSaveCachedPose", "New Save cached pose...");
	}
	// @TODO: don't know enough about this node type to comfortably assert that
	//        the CacheName won't change after the node has spawned... until
	//        then, we'll leave this optimization off
	else //if (CachedNodeTitle.IsOutOfDate())
	{
		FFormatNamedArguments Args;
		Args.Add(TEXT("NodeTitle"), FText::FromString(CacheName));
		CachedNodeTitle = FText::Format(LOCTEXT("AnimGraphNode_SaveCachedPose_Title", "Save cached pose '{NodeTitle}'"), Args);
	}
	return CachedNodeTitle;
}

FString UAnimGraphNode_SaveCachedPose::GetNodeCategory() const
{
	return TEXT("Cached Poses");
}

void UAnimGraphNode_SaveCachedPose::GetMenuEntries(FGraphContextMenuBuilder& ContextMenuBuilder) const
{
	//@TODO: Check the type of the from pin to make sure it's a pose
	if ((ContextMenuBuilder.FromPin == NULL) || (ContextMenuBuilder.FromPin->Direction == EGPD_Output))
	{
		// Disallow the graph being cached from residing in a state for safety and sanity; the tree is in effect it's own graph entirely and should be treated that way
		const bool bNotInStateMachine = ContextMenuBuilder.CurrentGraph->GetOuter()->IsA(UAnimBlueprint::StaticClass());

		if (bNotInStateMachine)
		{
			// Offer save cached pose
			UAnimGraphNode_SaveCachedPose* SaveCachedPose = NewObject<UAnimGraphNode_SaveCachedPose>();
			SaveCachedPose->CacheName = TEXT("SavedPose") + FString::FromInt(FMath::Rand());

			TSharedPtr<FEdGraphSchemaAction_K2NewNode> SaveCachedPoseAction = FK2ActionMenuBuilder::AddNewNodeAction(ContextMenuBuilder, GetNodeCategory(), LOCTEXT("NewSaveCachedPose", "New Save cached pose..."), SaveCachedPose->GetTooltipText().ToString(), 0, SaveCachedPose->GetKeywords());
			SaveCachedPoseAction->NodeTemplate = SaveCachedPose;
		}
	}
}

void UAnimGraphNode_SaveCachedPose::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	auto PostSpawnSetupLambda = [](UEdGraphNode* NewNode, bool bIsTemplateNode)
	{
		UAnimGraphNode_SaveCachedPose* CachedPoseNode = CastChecked<UAnimGraphNode_SaveCachedPose>(NewNode);
		// we use an empty CacheName in GetNodeTitle() to relay the proper menu title
		if (!bIsTemplateNode)
		{
			// @TODO: is the idea that this name is unique? what if Rand() hit twice? why not MakeUniqueObjectName()?			
			CachedPoseNode->CacheName = TEXT("SavedPose") + FString::FromInt(FMath::Rand());
		}
	};

	// actions get registered under specific object-keys; the idea is that 
	// actions might have to be updated (or deleted) if their object-key is  
	// mutated (or removed)... here we use the node's class (so if the node 
	// type disappears, then the action should go with it)
	UClass* ActionKey = GetClass();
	// to keep from needlessly instantiating a UBlueprintNodeSpawner, first   
	// check to make sure that the registrar is looking for actions of this type
	// (could be regenerating actions for a specific asset, and therefore the 
	// registrar would only accept actions corresponding to that asset)
	if (ActionRegistrar.IsOpenForRegistration(ActionKey))
	{
		UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
		NodeSpawner->CustomizeNodeDelegate = UBlueprintNodeSpawner::FCustomizeNodeDelegate::CreateStatic(PostSpawnSetupLambda);

		ActionRegistrar.AddBlueprintAction(ActionKey, NodeSpawner);
	}
}

bool UAnimGraphNode_SaveCachedPose::IsCompatibleWithGraph(const UEdGraph* TargetGraph) const
{
	//EGraphType GraphType = TargetGraph->GetSchema()->GetGraphType(TargetGraph);
	//bool const bIsNotStateMachine = (GraphType != GT_StateMachine);

	bool const bIsNotStateMachine = TargetGraph->GetOuter()->IsA(UAnimBlueprint::StaticClass());
	return bIsNotStateMachine && Super::IsCompatibleWithGraph(TargetGraph);
}

void UAnimGraphNode_SaveCachedPose::OnRenameNode(const FString& NewName)
{
	CacheName = NewName;
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(GetBlueprint());
}

TSharedPtr<class INameValidatorInterface> UAnimGraphNode_SaveCachedPose::MakeNameValidator() const
{
	return MakeShareable(new FCachedPoseNameValidator(GetBlueprint(), CacheName));
}

#undef LOCTEXT_NAMESPACE