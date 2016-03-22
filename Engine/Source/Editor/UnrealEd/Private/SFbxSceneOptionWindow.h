// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SlateBasics.h"
#include "AssetRegistryModule.h"
#include "Fbx/SSceneImportNodeTreeView.h"
#include "Fbx/SSceneReimportNodeTreeView.h"
#include "Fbx/SSceneImportStaticMeshListView.h"
#include "Fbx/SSceneReimportStaticMeshListView.h"
#include "Fbx/SSceneMaterialsListView.h"
#include "Factories/FbxSceneImportFactory.h"

class IDetailsView;

class SFbxSceneOptionWindow : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SFbxSceneOptionWindow)
		: _SceneInfo(nullptr)
		, _SceneInfoOriginal(nullptr)
		, _MeshStatusMap(nullptr)
		, _CanReimportHierarchy(false)
		, _NodeStatusMap(nullptr)
		, _GlobalImportSettings(nullptr)
		, _SceneImportOptionsDisplay(nullptr)
		, _SceneImportOptionsStaticMeshDisplay(nullptr)
		, _OverrideNameOptionsMap(nullptr)
		, _SceneImportOptionsSkeletalMeshDisplay(nullptr)
		, _SceneImportOptionsAnimationDisplay(nullptr)
		, _SceneImportOptionsMaterialDisplay(nullptr)
		, _OwnerWindow()
		, _FullPath(TEXT(""))
		{}

		SLATE_ARGUMENT( TSharedPtr<FFbxSceneInfo>, SceneInfo )
		SLATE_ARGUMENT( TSharedPtr<FFbxSceneInfo>, SceneInfoOriginal)
		SLATE_ARGUMENT( FbxSceneReimportStatusMapPtr, MeshStatusMap)
		SLATE_ARGUMENT( bool, CanReimportHierarchy)
		SLATE_ARGUMENT( FbxSceneReimportStatusMapPtr, NodeStatusMap)
		SLATE_ARGUMENT( UnFbx::FBXImportOptions*, GlobalImportSettings)
		SLATE_ARGUMENT( class UFbxSceneImportOptions*, SceneImportOptionsDisplay)
		SLATE_ARGUMENT( class UFbxSceneImportOptionsStaticMesh*, SceneImportOptionsStaticMeshDisplay)
		SLATE_ARGUMENT( ImportOptionsNameMapPtr, OverrideNameOptionsMap)
		SLATE_ARGUMENT( class UFbxSceneImportOptionsSkeletalMesh*, SceneImportOptionsSkeletalMeshDisplay)
		SLATE_ARGUMENT( class UFbxSceneImportOptionsAnimation*, SceneImportOptionsAnimationDisplay)
		SLATE_ARGUMENT( class UFbxSceneImportOptionsMaterial*, SceneImportOptionsMaterialDisplay)
		SLATE_ARGUMENT( TSharedPtr<SWindow>, OwnerWindow)
		SLATE_ARGUMENT( FString, FullPath )
	SLATE_END_ARGS()

public:

	SFbxSceneOptionWindow();
	~SFbxSceneOptionWindow();

	void Construct(const FArguments& InArgs);
	virtual bool SupportsKeyboardFocus() const override { return true; }

	void CloseFbxSceneOption();

	FReply OnImport()
	{
		bShouldImport = true;
		CloseFbxSceneOption();
		return FReply::Handled();
	}

	FReply OnCancel()
	{
		bShouldImport = false;
		CloseFbxSceneOption();
		return FReply::Handled();
	}

	virtual FReply OnKeyDown( const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent ) override
	{
		if( InKeyEvent.GetKey() == EKeys::Escape )
		{
			return OnCancel();
		}

		return FReply::Unhandled();
	}

	bool ShouldImport() const
	{
		return bShouldImport;
	}

	void OnToggleReimportHierarchy(ECheckBoxState CheckType);
	ECheckBoxState IsReimportHierarchyChecked() const;

	//Material UI
	FText GetMaterialPrefixName() const;
	void OnMaterialPrefixCommited(const FText& InText, ETextCommit::Type InCommitType);
	FSlateColor GetMaterialPrefixTextColor() const;

	static void CopyFbxOptionsToFbxOptions(UnFbx::FBXImportOptions *SourceOptions, UnFbx::FBXImportOptions *DestinationOptions);

	static void CopyStaticMeshOptionsToFbxOptions(UnFbx::FBXImportOptions *ImportSettings, class UFbxSceneImportOptionsStaticMesh* StaticMeshOptions);
	static void CopyFbxOptionsToStaticMeshOptions(UnFbx::FBXImportOptions *ImportSettings, class UFbxSceneImportOptionsStaticMesh* StaticMeshOptions);

	static void CopySkeletalMeshOptionsToFbxOptions(UnFbx::FBXImportOptions *ImportSettings, class UFbxSceneImportOptionsSkeletalMesh* SkeletalMeshOptions);
	static void CopyFbxOptionsToSkeletalMeshOptions(UnFbx::FBXImportOptions *ImportSettings, class UFbxSceneImportOptionsSkeletalMesh* SkeletalMeshOptions);

	static void CopyAnimationOptionsToFbxOptions(UnFbx::FBXImportOptions *ImportSettings, class UFbxSceneImportOptionsAnimation* AnimationOptions);
	static void CopyFbxOptionsToAnimationOptions(UnFbx::FBXImportOptions *ImportSettings, class UFbxSceneImportOptionsAnimation* AnimationOptions);

	static void CopyMaterialOptionsToFbxOptions(UnFbx::FBXImportOptions *ImportSettings, class UFbxSceneImportOptionsMaterial* MaterialOptions);
	static void CopyFbxOptionsToMaterialOptions(UnFbx::FBXImportOptions *ImportSettings, class UFbxSceneImportOptionsMaterial* MaterialOptions);

	void OnFinishedChangingPropertiesSceneTabDetailView(const FPropertyChangedEvent& PropertyChangedEvent);

private:

	bool CanImport() const;

	void InitAllTabs();
	TSharedPtr<SWidget> SpawnDockTab();
	TSharedRef<SDockTab> SpawnSceneTab(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnStaticMeshTab(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnSkeletalMeshTab(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnMaterialTab(const FSpawnTabArgs& Args);

	TSharedRef<SDockTab> SpawnSceneReimportTab(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnStaticMeshReimportTab(const FSpawnTabArgs& Args);

private:
	//SFbxSceneOptionWindow Arguments
	
	TSharedPtr< FFbxSceneInfo > SceneInfo;
	TSharedPtr< FFbxSceneInfo > SceneInfoOriginal;
	FbxSceneReimportStatusMapPtr MeshStatusMap;
	FbxSceneReimportStatusMapPtr NodeStatusMap;
	UnFbx::FBXImportOptions *GlobalImportSettings;
	class UFbxSceneImportOptions* SceneImportOptionsDisplay;
	class UFbxSceneImportOptionsStaticMesh* SceneImportOptionsStaticMeshDisplay;
	ImportOptionsNameMapPtr OverrideNameOptionsMap;
	class UFbxSceneImportOptionsSkeletalMesh* SceneImportOptionsSkeletalMeshDisplay;
	class UFbxSceneImportOptionsAnimation* SceneImportOptionsAnimationDisplay;
	class UFbxSceneImportOptionsMaterial* SceneImportOptionsMaterialDisplay;
	TSharedPtr< SWindow > OwnerWindow;
	FString FullPath;

	bool bCanReimportHierarchy;

	
	//Variables

	TSharedPtr<FTabManager> FbxSceneImportTabManager;
	TSharedPtr<FTabManager::FLayout> Layout;
	bool			bShouldImport;
	
	
	//Scene tab variables
	TSharedPtr<SFbxSceneTreeView> SceneTabTreeview;
	TSharedPtr<IDetailsView> SceneTabDetailsView;

	//Material tab Variables
	TSharedPtr<SFbxSceneMaterialsListView> MaterialsTabListView;
	FbxTextureInfoArray TexturesArray;
	FString MaterialPrefixName;

	//StaticMesh tab Variables
	TSharedPtr<SFbxSceneStaticMeshListView> StaticMeshTabListView;
	TSharedPtr<IDetailsView> StaticMeshTabDetailsView;

	//Scene Reimport tab variables
	TSharedPtr<SFbxReimportSceneTreeView> SceneReimportTreeview;

	//StaticMesh Reimport tab variables
	TSharedPtr<SFbxSceneStaticMeshReimportListView> StaticMeshReimportListView;
	TSharedPtr<IDetailsView> StaticMeshReimportDetailsView;
};