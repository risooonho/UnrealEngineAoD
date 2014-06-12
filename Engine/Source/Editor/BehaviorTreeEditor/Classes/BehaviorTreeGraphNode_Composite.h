// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "BehaviorTreeGraphNode_Composite.generated.h"

UCLASS()
class UBehaviorTreeGraphNode_Composite : public UBehaviorTreeGraphNode
{
	GENERATED_UCLASS_BODY()
	
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	virtual bool RefreshNodeClass() OVERRIDE{ return false; }

	/** Gets a list of actions that can be done to this particular node */
	virtual void GetContextMenuActions(const FGraphNodeContextMenuBuilder& Context) const OVERRIDE;

	/** check if node can accept breakpoints */
	virtual bool CanPlaceBreakpoints() const OVERRIDE { return true; }
};
