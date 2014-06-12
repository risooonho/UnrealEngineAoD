// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once
#include "BehaviorTree/BTDecorator.h"
#include "BTDecorator_Loop.generated.h"

struct FBTLoopDecoratorMemory
{
	uint8 RemainingExecutions;
};

UCLASS(HideCategories=(Condition))
class AIMODULE_API UBTDecorator_Loop : public UBTDecorator
{
	GENERATED_UCLASS_BODY()

	/** number of executions */
	UPROPERTY(Category=Decorator, EditAnywhere)
	int32 NumLoops;

	/** infinite loop */
	UPROPERTY(Category=Decorator, EditAnywhere)
	bool bInfiniteLoop;

	virtual uint16 GetInstanceMemorySize() const OVERRIDE;
	virtual void DescribeRuntimeValues(const class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, EBTDescriptionVerbosity::Type Verbosity, TArray<FString>& Values) const OVERRIDE;
	virtual FString GetStaticDescription() const OVERRIDE;

#if WITH_EDITOR
	virtual FName GetNodeIconName() const override;
#endif // WITH_EDITOR

protected:

	virtual void OnNodeActivation(struct FBehaviorTreeSearchData& SearchData) OVERRIDE;
};
