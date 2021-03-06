// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AssetData.h"
#include "AssetManagerTypes.h"
#include "AssetRegistryInterface.h"
#include "StreamableManager.h"
#include "AssetBundleData.h"
#include "AssetRegistryModule.h"
#include "AssetManager.generated.h"

/** Defined in C++ file */
struct FPrimaryAssetTypeData;
struct FPrimaryAssetData;

/** 
 * A singleton UObject that is responsible for loading and unloading PrimaryAssets, and maintaining game-specific asset references
 * Games should override this class and change the class reference
 */
UCLASS()
class ENGINE_API UAssetManager : public UObject
{
	GENERATED_BODY()

public:
	/** Constructor */
	UAssetManager();

	/** Returns true if there is a current asset manager */
	static bool IsValid();

	/** Returns the current AssetManager object */
	static UAssetManager& Get();

	/** Accesses the StreamableManager used by this Asset Manager. Static for easy access */
	static FStreamableManager& GetStreamableManager() { return Get().StreamableManager; }

	/** Asset Type of UWorld assets */
	static const FPrimaryAssetType MapType;

	/** Asset Type of Label used to tag other assets */
	static const FPrimaryAssetType PrimaryAssetLabelType;

	// BUILDING ASSET DIRECTORY

	/** 
	 * Scans a list of paths and reads asset data for all primary assets of a specific type.
	 * If done in the editor it will load the data off disk, in cooked games it will load out of the asset registry cache
	 *
	 * @param PrimaryAssetType	Type of asset to look for. If the scanned asset matches GetPrimaryAssetType with this it will be added to directory
	 * @param Paths				List of file system paths to scan
	 * @param BaseClass			Base class of all loaded assets, if the scanned asset isn't a child of this class it will be skipped
	 * @param bHasBlueprintClasses	If true, the assets are blueprints that subclass BaseClass. If false they are base UObject assets
	 * @param bIsEditorOnly		If true, assets will only be scanned in editor builds, and will not be stored into the asset registry
	 * @param bForceSynchronousScan If true will scan the disk synchronously, otherwise will wait for asset registry scan to complete
	 * @return					Number of primary assets found
	 */
	virtual int32 ScanPathsForPrimaryAssets(FPrimaryAssetType PrimaryAssetType, const TArray<FString>& Paths, UClass* BaseClass, bool bHasBlueprintClasses, bool bIsEditorOnly = false, bool bForceSynchronousScan = true);

	/** Single path wrapper */
	virtual int32 ScanPathForPrimaryAssets(FPrimaryAssetType PrimaryAssetType, const FString& Path, UClass* BaseClass, bool bHasBlueprintClasses, bool bIsEditorOnly = false, bool bForceSynchronousScan = true);
	
	/** Call this before many calls to ScanPaths to improve load performance. It will defer expensive updates until StopBulkScanning is called */
	virtual void StartBulkScanning();
	virtual void StopBulkScanning();

	/** 
	 * Adds or updates a Dynamic asset, which is a runtime-specified asset that has no on disk representation, so has no FAssetData. But it can have bundle state and a path.
	 *
	 * @param FPrimaryAssetId	Type/Name of the asset. The type info will be created if it doesn't already exist
	 * @param AssetPath			Path to the object representing this asset, this is optional for dynamic assets
	 * @param BundleData		List of Name->asset paths that represent the possible bundle states for this asset
	 * @return					True if added
	 */
	virtual bool AddDynamicAsset(const FPrimaryAssetId& PrimaryAssetId, const FStringAssetReference& AssetPath, const FAssetBundleData& BundleData);

	/** This will expand out references in the passed in AssetBundleData that are pointing to other primary assets with bundles. This is useful to preload entire webs of assets */
	virtual void RecursivelyExpandBundleData(FAssetBundleData& BundleData);

	// ACCESSING ASSET DIRECTORY

	/** Gets the FAssetData for a primary asset with the specified type/name, will only work for once that have been scanned for already. Returns true if it found a valid data */
	virtual bool GetPrimaryAssetData(const FPrimaryAssetId& PrimaryAssetId, FAssetData& AssetData) const;

	/** Gets list of all FAssetData for a primary asset type, returns true if any were found */
	virtual bool GetPrimaryAssetDataList(FPrimaryAssetType PrimaryAssetType, TArray<FAssetData>& AssetDataList) const;

	/** Gets the in-memory UObject for a primary asset id, returning nullptr if it's not in memory. Will return blueprint class for blueprint assets. This works even if the asset wasn't loaded explicitly */
	virtual UObject* GetPrimaryAssetObject(const FPrimaryAssetId& PrimaryAssetId) const;

	/** Templated versions of above */
	template<class AssetType> 
	FORCEINLINE AssetType* GetPrimaryAssetObject(const FPrimaryAssetId& PrimaryAssetId) const
	{
		UObject* ObjectReturn = GetPrimaryAssetObject(PrimaryAssetId);
		return Cast<AssetType>(ObjectReturn);
	}

	template<class AssetType>
	FORCEINLINE TSubclassOf<AssetType> GetPrimaryAssetObjectClass(const FPrimaryAssetId& PrimaryAssetId) const
	{
		TSubclassOf<AssetType> ReturnClass;
		ReturnClass = Cast<UClass>(GetPrimaryAssetObject(PrimaryAssetId));
		return ReturnClass;
	}

	/** Gets list of all loaded objects for a primary asset type, returns true if any were found. Will return blueprint class for blueprint assets. This works even if the asset wasn't loaded explicitly */
	virtual bool GetPrimaryAssetObjectList(FPrimaryAssetType PrimaryAssetType, TArray<UObject*>& ObjectList) const;

	/** Gets the FStringAssetReference for a primary asset type and name, returns invalid if not found */
	virtual FStringAssetReference GetPrimaryAssetPath(const FPrimaryAssetId& PrimaryAssetId) const;

	/** Gets the list of all FStringAssetReferences for a given type, returns true if any found */
	virtual bool GetPrimaryAssetPathList(FPrimaryAssetType PrimaryAssetType, TArray<FStringAssetReference>& AssetPathList) const;

	/** Sees if the passed in object path is a registered primary asset, if so return it. Returns invalid Identifier if not found */
	virtual FPrimaryAssetId GetPrimaryAssetIdForPath(const FStringAssetReference& ObjectPath) const;
	virtual FPrimaryAssetId GetPrimaryAssetIdForPath(FName ObjectPath) const;

	/** Sees if the package has a primary asset, useful if only the package name is available */
	virtual FPrimaryAssetId GetPrimaryAssetIdForPackage(FName PackagePath) const;

	/** Parses AssetData to extract the primary type/name from it. This works even if it isn't in the directory yet */
	virtual FPrimaryAssetId GetPrimaryAssetIdFromData(const FAssetData& AssetData, FPrimaryAssetType SuggestedType = NAME_None) const;

	/** Gets list of all FPrimaryAssetId for a primary asset type, returns true if any were found */
	virtual bool GetPrimaryAssetIdList(FPrimaryAssetType PrimaryAssetType, TArray<FPrimaryAssetId>& PrimaryAssetIdList) const;

	/** Gets metadata for a specific asset type, returns false if not found */
	virtual bool GetPrimaryAssetTypeInfo(FPrimaryAssetType PrimaryAssetType, FPrimaryAssetTypeInfo& AssetTypeInfo) const;

	/** Gets list of all primary asset types infos */
	virtual void GetPrimaryAssetTypeInfoList(TArray<FPrimaryAssetTypeInfo>& AssetTypeInfoList) const;

	// ASYNC LOADING PRIMARY ASSETS

	/** 
	 * Loads a list of Primary Assets. This will start an async load of those assets, calling callback on completion.
	 * These assets will stay in memory until explicitly unloaded.
	 * You can wait on the returned streamable request or poll as needed.
	 * If there is no work to do, returned handle will be null and delegate will get called before function returns.
	 *
	 * @param AssetsToLoad		List of primary assets to load
	 * @param LoadBundles		List of bundles to load for those assets
	 * @param DelegateToCall	Delegate that will be called on completion, may be called before function returns if assets are already loaded
	 * @param Priority			Async loading priority for this request
	 * @return					Streamable Handle that can be used to poll or wait
	 */
	virtual TSharedPtr<FStreamableHandle> LoadPrimaryAssets(const TArray<FPrimaryAssetId>& AssetsToLoad, const TArray<FName>& LoadBundles = TArray<FName>(), FStreamableDelegate DelegateToCall = FStreamableDelegate(), TAsyncLoadPriority Priority = FStreamableManager::DefaultAsyncLoadPriority);

	/** Single asset wrapper */
	virtual TSharedPtr<FStreamableHandle> LoadPrimaryAsset(const FPrimaryAssetId& AssetToLoad, const TArray<FName>& LoadBundles = TArray<FName>(), FStreamableDelegate DelegateToCall = FStreamableDelegate(), TAsyncLoadPriority Priority = FStreamableManager::DefaultAsyncLoadPriority);

	/** Loads all assets of a given type, useful for cooking */
	virtual TSharedPtr<FStreamableHandle> LoadPrimaryAssetsWithType(FPrimaryAssetType PrimaryAssetType, const TArray<FName>& LoadBundles = TArray<FName>(), FStreamableDelegate DelegateToCall = FStreamableDelegate(), TAsyncLoadPriority Priority = FStreamableManager::DefaultAsyncLoadPriority);

	/** 
	 * Unloads a list of Primary Assets. This will drop hard references to these assets, but they may be in memory due to other references or GC delay
	 *
	 * @param AssetsToUnload	List of primary assets to load
	 * @return					Number of assets unloaded
	 */
	virtual int32 UnloadPrimaryAssets(const TArray<FPrimaryAssetId>& AssetsToUnload);

	/** Single asset wrapper */
	virtual int32 UnloadPrimaryAsset(const FPrimaryAssetId& AssetToUnload);

	/** Loads all assets of a given type, useful for cooking */
	virtual int32 UnloadPrimaryAssetsWithType(FPrimaryAssetType PrimaryAssetType);

	/** 
	 * Changes the bundle state of a set of loaded primary assets.
	 * You can wait on the returned streamable request or poll as needed.
	 * If there is no work to do, returned handle will be null and delegate will get called before function returns.
	 *
	 * @param AssetsToChange	List of primary assets to change state of
	 * @param AddBundles		List of bundles to add
	 * @param RemoveBundles		Explicit list of bundles to remove
	 * @param RemoveAllBundles	If true, remove all existing bundles even if not in remove list
	 * @param DelegateToCall	Delegate that will be called on completion, may be called before function returns if assets are already loaded
	 * @param Priority			Async loading priority for this request
	 * @return					Streamable Handle that can be used to poll or wait
	 */
	virtual TSharedPtr<FStreamableHandle> ChangeBundleStateForPrimaryAssets(const TArray<FPrimaryAssetId>& AssetsToChange, const TArray<FName>& AddBundles, const TArray<FName>& RemoveBundles, bool bRemoveAllBundles = false, FStreamableDelegate DelegateToCall = FStreamableDelegate(), TAsyncLoadPriority Priority = FStreamableManager::DefaultAsyncLoadPriority);

	/** 
	 * Returns the loading handle associated with the primary asset, it can then be checked for progress or waited on
	 *
	 * @param PrimaryAssetId	Asset to get handle for
	 * @param bForceCurrent		If true, returns the current handle. If false, will return pending if active, or current if not
	 * @param Bundles			If not null, will fill in with a list of the requested bundle state
	 * @return					Streamable Handle that can be used to poll or wait
	 */
	TSharedPtr<FStreamableHandle> GetPrimaryAssetHandle(const FPrimaryAssetId& PrimaryAssetId, bool bForceCurrent = false, TArray<FName>* Bundles = nullptr);

	/** 
	 * Returns a list of primary assets that are in the given bundle state. Only assets that are loaded or being loaded are valid
	 *
	 * @param PrimaryAssetList	Any valid assets are added to this list
	 * @param ValidTypes		List of types that are allowed. If empty, all types allowed
	 * @param RequiredBundles	Adds to list if the bundle state has all of these bundles. If empty will return all loaded
	 * @param ExcludedBundles	Doesn't add if the bundle state has any of these bundles
	 * @param bForceCurrent		If true, only use the current state. If false, use the current or pending
	 * @return					True if any found
	 */
	bool GetPrimaryAssetsWithBundleState(TArray<FPrimaryAssetId>& PrimaryAssetList, const TArray<FName>& ValidTypes, const TArray<FName>& RequiredBundles, const TArray<FName>& ExcludedBundles = TArray<FName>(), bool bForceCurrent = false);

	/** Fills in a TMap with the pending/active loading state of every asset */
	void GetPrimaryAssetBundleStateMap(TMap<FPrimaryAssetId, TArray<FName>>& BundleStateMap, bool bForceCurrent = false);

	/** Quick wrapper to async load some non primary assets with the primary streamable manager. This will not auto release the handle, release it if needed */
	virtual TSharedPtr<FStreamableHandle> LoadAssetList(const TArray<FStringAssetReference>& AssetList);

	/** Returns a single AssetBundleInfo, matching Scope and Name */
	virtual FAssetBundleEntry GetAssetBundleEntry(const FPrimaryAssetId& BundleScope, FName BundleName) const;

	/** Appends all AssetBundleInfos inside a given scope */
	virtual bool GetAssetBundleEntries(const FPrimaryAssetId& BundleScope, TArray<FAssetBundleEntry>& OutEntries) const;

	
	// FUNCTIONS FOR MANAGEMENT/COOK RULES

	/** Changes the default management rules for a specified type */
	virtual void SetPrimaryAssetTypeRules(FPrimaryAssetType PrimaryAssetType, const FPrimaryAssetRules& Rules);

	/** Changes the management rules for a specific asset, this overrides the type rules. If passed in Rules is default, delete override */
	virtual void SetPrimaryAssetRules(FPrimaryAssetId PrimaryAssetId, const FPrimaryAssetRules& Rules);

	/** Gets the management rules for a specific asset, this will merge the type and individual values */
	virtual FPrimaryAssetRules GetPrimaryAssetRules(FPrimaryAssetId PrimaryAssetId) const;

	/** Returns list of asset packages managed by primary asset */
	virtual bool GetManagedPackageList(FPrimaryAssetId PrimaryAssetId, TArray<FName>& AssetPackageList) const;

	/** Returns list of PrimaryAssetIds that manage a package. Will optionally recurse up the management chain */
	virtual bool GetPackageManagerList(FName PackageName, bool bRecurseToParents, TArray<FPrimaryAssetId>& ManagerList) const;


	// GENERAL ASSET UTILITY FUNCTIONS

	/** Gets the FAssetData at a specific path, handles redirectors and blueprint classes correctly. Returns true if it found a valid data */
	virtual bool GetAssetDataForPath(const FStringAssetReference& ObjectPath, FAssetData& AssetData) const;

	/** Turns an FAssetData into FStringAssetReference, handles adding _C as necessary */
	virtual FStringAssetReference GetAssetPathForData(const FAssetData& AssetData) const;

	/** Tries to redirect a Primary Asset Id, using list in AssetManagerSettings */
	virtual FPrimaryAssetId GetRedirectedPrimaryAssetId(const FPrimaryAssetId& OldId) const;

	/** Reads redirector list and gets a list of the redirected previous names for a Primary Asset Id */
	virtual void GetPreviousPrimaryAssetIds(const FPrimaryAssetId& NewId, TArray<FPrimaryAssetId>& OutOldIds) const;

	/** Reads AssetManagerSettings for specifically redirected asset paths. This is useful if you need to convert older saved data */
	virtual FName GetRedirectedAssetPath(FName OldPath) const;
	virtual FStringAssetReference GetRedirectedAssetPath(const FStringAssetReference& OldPath) const;

	/** Extracts all FStringAssetReferences from a Class/Struct */
	virtual void ExtractStringAssetReferences(const UStruct* Struct, const void* StructValue, TArray<FStringAssetReference>& FoundAssetReferences, const TArray<FName>& PropertiesToSkip = TArray<FName>()) const;

	/** Dumps out summary of managed types to log */
	static void DumpAssetTypeSummary();

	/** Dumps out list of loaded asset bundles to log */
	static void DumpLoadedAssetState();

	/** Dumps out list of primary asset -> managed assets to log */
	static void DumpReferencersForPackage(const TArray< FString >& PackageNames);

	/** Starts initial load, gets called from InitializeObjectReferences */
	virtual void StartInitialLoading();

	/** Finishes initial loading, gets called from end of Engine::Init() */
	virtual void FinishInitialLoading();

	/** Accessor for asset registry */
	class IAssetRegistry& GetAssetRegistry() const;

	/** Return settings object */
	const class UAssetManagerSettings& GetSettings() const;

	// Overrides
	virtual void PostInitProperties() override;

#if WITH_EDITOR
	// EDITOR ONLY FUNCTIONALITY

	/** Gets package names to add to the cook. Do this instead of loading assets so RAM can be properly managed on build machines */
	virtual void ModifyCook(TArray<FName>& PackageNames);

	/** Returns cook rule for a package name using Management rules, games should override this to take into account their individual workflows */
	virtual EPrimaryAssetCookRule GetPackageCookRule(FName PackageName) const;

	/** Returns true if the specified asset package can be cooked, will error and return false if it is disallowed */
	virtual bool VerifyCanCookPackage(FName PackageName, bool bLogError = true) const;

	/** For a given package and platform, return what Chunks it should be assigned to, games can override this as needed. Returns false if no information found */
	virtual bool GetPackageChunkIds(FName PackageName, const class ITargetPlatform* TargetPlatform, const TArray<int32>& ExistingChunkList, TArray<int32>& OutChunkList) const;

	/** Runs game-specific code to update the source hash on a package at cook time. This is added to the base package guid check */
	virtual void UpdatePackageSourceHash(FName PackageName, FMD5& PackageSourceHash) const;

	/** Refresh the entire set of asset data, can call from editor when things have changed dramatically */
	virtual void RefreshPrimaryAssetDirectory();

	/** Updates the asset management database if needed */
	virtual void UpdateManagementDatabase(bool bForceRefresh = false);

	/** Handles applying Asset Labels and should be overridden to do any game-specific labelling */
	virtual void ApplyPrimaryAssetLabels();

	/** Refreshes cache of asset data for in memory object */
	virtual void RefreshAssetData(UObject* ChangedObject);

	/** 
	 * Initializes asset bundle data from a passed in struct or class, this will read the AssetBundles metadata off the UProperties. As an example this property definition:
	 *
	 * UPROPERTY(EditDefaultsOnly, Category=Data, meta = (AssetBundles = "Client,Server"))
	 * TAssetPtr<class UCurveTable> CurveTableReference;
	 *
	 * Would add the value of CurveTableReference to both the Client and Server asset bundles
	 *
	 * @param Struct		UScriptStruct or UClass representing the property hierarchy
	 * @param StructValue	Location in memory of Struct or Object
	 * @param AssetBundle	Bundle that will be filled out
	 */
	virtual void InitializeAssetBundlesFromMetadata(const UStruct* Struct, const void* StructValue, FAssetBundleData& AssetBundle) const;

	/** UObject wrapper */
	virtual void InitializeAssetBundlesFromMetadata(const UObject* Object, FAssetBundleData& AssetBundle) const
	{
		InitializeAssetBundlesFromMetadata(Object->GetClass(), Object, AssetBundle);
	}

#endif

protected:
	friend class FAssetManagerEditorModule;

	/** Internal helper function that attempts to get asset data from the specified path; Accounts for possibility of blueprint classes ending in _C */
	virtual void GetAssetDataForPathInternal(class IAssetRegistry& AssetRegistry, const FString& AssetPath, OUT FAssetData& OutAssetData) const;

	/** Updates the asset data cached on the name data */
	virtual void UpdateCachedAssetData(const FPrimaryAssetId& PrimaryAssetId, const FAssetData& NewAssetData, bool bAllowDuplicates);

	/** Returns the NameData for a specific type/name pair */
	FPrimaryAssetData* GetNameData(const FPrimaryAssetId& PrimaryAssetId, bool bCheckRedirector = true);
	const FPrimaryAssetData* GetNameData(const FPrimaryAssetId& PrimaryAssetId, bool bCheckRedirector = true) const;

	/** Loads the redirector maps */
	virtual void LoadRedirectorMaps();

	/** Rebuilds the ObjectReferenceList, needed after global object state has changed */
	virtual void RebuildObjectReferenceList();

	/** Called when an internal load handle finishes, handles setting to pending state */
	virtual void OnAssetStateChangeCompleted(FPrimaryAssetId PrimaryAssetId, TSharedPtr<FStreamableHandle> BoundHandle, FStreamableDelegate WrappedDelegate);

	/** Helper function to write out asset reports */
	virtual bool WriteCustomReport(FString FileName, TArray<FString>& FileLines) const;

	/** Function used during creating Management references to decide when to recurse and set references */
	virtual EAssetSetManagerResult::Type ShouldSetManager(const FAssetIdentifier& Manager, const FAssetIdentifier& Source, const FAssetIdentifier& Target, EAssetRegistryDependencyType::Type DependencyType, EAssetSetManagerFlags::Type Flags) const;

	/** Scans all asset types specified in DefaultGame */
	virtual void ScanPrimaryAssetTypesFromConfig();

	/** Called after scanning is complete, either from FinishInitialLoading or after the AssetRegistry finishes */
	virtual void PostInitialAssetScan();

	/** Returns true if path should be excluded from primary asset scans */
	virtual bool IsPathExcludedFromScan(const FString& Path) const;

#if WITH_EDITOR
	/** Helper function which requests the asset register scan a list of directories/assets */
	virtual void ScanPathsSynchronous(const TArray<FString>& PathsToScan) const;

	/** Called when asset registry is done loading off disk, will finish any deferred loads */
	virtual void OnAssetRegistryFilesLoaded();

	/** Handles updating manager when a new asset is created */
	virtual void OnInMemoryAssetCreated(UObject *Object);

	/** Handles updating manager if deleted object is relevant*/
	virtual void OnInMemoryAssetDeleted(UObject *Object);

	/** When asset is renamed */
	virtual void OnAssetRenamed(const FAssetData& NewData, const FString& OldPath);

	/** Try to remove an old asset identifier when it has been deleted/renamed */
	virtual void RemovePrimaryAssetId(const FPrimaryAssetId& PrimaryAssetId);

	/** Called right before PIE starts, will refresh asset directory and can be overriden to preload assets */
	virtual void PreBeginPIE(bool bStartSimulate);

	/** Called after PIE ends, resets loading state */
	virtual void EndPIE(bool bStartSimulate);

	/** Copy of the asset state before PIE was entered, return to that when PIE completes */
	TMap<FPrimaryAssetId, TArray<FName>> PrimaryAssetStateBeforePIE;
#endif // WITH_EDITOR

	/** Per-type asset information */
	TMap<FName, TSharedRef<FPrimaryAssetTypeData>> AssetTypeMap;

	/** Map from object path to Primary Asset Id */
	TMap<FName, FPrimaryAssetId> AssetPathMap;

	/** Overridden asset management data for specific types */
	TMap<FPrimaryAssetId, FPrimaryAssetRules> AssetRuleOverrides;

	/** Map from PrimaryAssetId to list of PrimaryAssetIds that are the parent of this one, for determining chunking/cooking */
	TMap<FPrimaryAssetId, TArray<FPrimaryAssetId>> ManagementParentMap;

	/** Cached map of asset bundles, global and per primary asset */
	TMap<FPrimaryAssetId, TMap<FName, FAssetBundleEntry>> CachedAssetBundles;

	/** List of directories that have already been synchronously scanned */
	mutable TArray<FString> AlreadyScannedDirectories;

	/** The streamable manager used for all primary asset loading */
	FStreamableManager StreamableManager;

	/** List of UObjects that are being kept from being GCd, derived from the asset type map. Arrays are currently more efficient than Sets */
	UPROPERTY()
	TArray<UObject*> ObjectReferenceList;

	/** True if we are running a build that is already scanning assets globally so we can perhaps avoid scanning paths synchronously */
	UPROPERTY()
	bool bIsGlobalAsyncScanEnvironment;

	/** True if we should keep hard references to loaded assets, to stop them from garbage collecting */
	UPROPERTY()
	bool bShouldKeepHardRefs;

	/** True if PrimaryAssetType/Name will be implied for loading assets that don't have it saved on disk. Won't work for all projects */
	UPROPERTY()
	bool bShouldGuessTypeAndName;

	/** True if we should always use synchronous loads, this speeds up cooking */
	UPROPERTY()
	bool bShouldUseSynchronousLoad;

	/** True if we are currently in bulk scanning mode */
	UPROPERTY()
	bool bIsBulkScanning;

	/** True if the asset management database is up to date */
	UPROPERTY()
	bool bIsManagementDatabaseCurrent;

	/** True if the asset management database should be updated after scan completes */
	UPROPERTY()
	bool bUpdateManagementDatabaseAfterScan;

	/** Redirector maps loaded out of AssetMigrations.ini */
	TMap<FName, FName> PrimaryAssetTypeRedirects;
	TMap<FString, FString> PrimaryAssetIdRedirects;
	TMap<FName, FName> AssetPathRedirects;

private:
	mutable class IAssetRegistry* CachedAssetRegistry;
	mutable const class UAssetManagerSettings* CachedSettings;
};
