// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GCObject.h"

// Forward declarations
class UBlueprintNodeSpawner;
class UBlueprint;
class UEdGraphNode;
class FReferenceCollector;

/**
 * Serves as a centralized data-store for all UBlueprintNodeSpawner node-
 * templates. Implemented this way (rather than internal to UBlueprintNodeSpawner) 
 * since node-templates require a UEdGraph/UBlueprint outer chain. Instead of 
 * instantiating a bunch of graphs/blueprints per UBlueprintNodeSpawner, we'd 
 * rather have a small centralized set here.
 */
class FBlueprintNodeTemplateCache : public FGCObject
{
public:
	/** */
	FBlueprintNodeTemplateCache();

	/**
	 * Retrieves a cached template associated with the supplied spawner. Will
	 * instantiate a new template if one didn't already exist. If the
	 * template-node is not compatible with any cached UEdGraph outer, then
	 * we use TargetGraph as a model to create one that will work.
	 * 
	 * @param  NodeSpawner	Acts as the key for the template lookup; takes care of spawning the template-node if one didn't already exist.
	 * @param  TargetGraph	Optional param that defines a compatible graph outer (used as an achetype if we don't have a compatible outer on hand).
	 * @return Should return a new/cached template-node (but could be null, or some pre-existing node... depends on the sub-class's Invoke() method).
	 */
	UEdGraphNode* GetNodeTemplate(UBlueprintNodeSpawner const* NodeSpawner, UEdGraph* TargetGraph = nullptr);

	/**
	 * Retrieves a cached template associated with the supplied spawner. Does 
	 * NOT attempt to allocate one if it doesn't exist.
	 * 
	 * @param  NodeSpawner	The spawner you want a template node for.
	 * @param  ENoInit Signifies to use this function over the other (mutating) GetNodeTemplate().
	 * @return Should return the cached template-node (if one exists, otherwise false).
	 */
	UEdGraphNode* GetNodeTemplate(UBlueprintNodeSpawner const* NodeSpawner, ENoInit) const;

	/**
	 * Wipes any nodes that were cached on behalf of the specified spawner 
	 * (should be called when NodeSpawner is destroyed, in case 
	 * GetNodeTemplate() was called for it).
	 * 
	 * @param  NodeSpawner	The spawner we want cached node-templates cleared for.
	 */
	void ClearCachedTemplate(UBlueprintNodeSpawner const* NodeSpawner);

	// FGCObject interface
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	// End FGCObject interface

private:
	/** 
	 * Unfortunately, we cannot nest template-nodes in the transient package. 
	 * Certain nodes operate on the assumption that they have a UEdGraph outer, 
	 * while a certain subset expect the graph to have a UBlueprint outer. This
	 * means we cannot spawn templates without a blueprint/graph to add them to.
	 * 
	 * This array holds intermediate blueprints that we use to parent the 
	 * template-nodes. Ideally we only need a small handful that are compatible 
	 * with all nodes.
	 */
	TArray<UBlueprint*> TemplateOuters;

	/** */
	TMap<UBlueprintNodeSpawner const*, UEdGraphNode*> NodeTemplateCache;
};
