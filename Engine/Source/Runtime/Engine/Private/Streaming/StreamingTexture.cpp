// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
StreamingTexture.cpp: Definitions of classes used for texture.
=============================================================================*/

#include "EnginePrivate.h"
#include "StreamingTexture.h"
#include "StreamingManagerTexture.h"
#include "Engine/LightMapTexture2D.h"
#include "Engine/ShadowMapTexture2D.h"

FStreamingTexture::FStreamingTexture(UTexture2D* InTexture, float GlobalMipBias, const int32 NumStreamedMips[TEXTUREGROUP_MAX], bool bOnlyStreamIn, int32 HLODStrategy)
: Texture(InTexture)
{
	UpdateStaticData(GlobalMipBias);
	UpdateDynamicData(NumStreamedMips, bOnlyStreamIn, FMath::FloorToInt(GlobalMipBias), HLODStrategy);

	InstanceRemovedTimestamp = -FLT_MAX;
	LastRenderTimeRefCountTimestamp = -FLT_MAX;
	LastRenderTimeRefCount = 0;
	DynamicBoostFactor = 1.f;

	bForceFullyLoadHeuristic = false;
	NumMissingMips = 0;
	bLooksLowRes = false;
	VisibleWantedMips = MinAllowedMips;
	HiddenWantedMips = MinAllowedMips;
	RetentionPriority = 0;
	BudgetedMips = MinAllowedMips;
	LoadOrderPriority = 0;
	WantedMips = MinAllowedMips;
}

void FStreamingTexture::UpdateStaticData(float GlobalMipBias)
{
	if (Texture)
	{
		LODGroup = (TextureGroup)Texture->LODGroup;
		NumNonStreamingMips = Texture->GetNumNonStreamingMips();
		MipCount = Texture->GetNumMips();
		BoostFactor = GetExtraBoost(LODGroup, GlobalMipBias);

		bIsCharacterTexture = (LODGroup == TEXTUREGROUP_Character || LODGroup == TEXTUREGROUP_CharacterSpecular || LODGroup == TEXTUREGROUP_CharacterNormalMap);
		bIsTerrainTexture = (LODGroup == TEXTUREGROUP_Terrain_Heightmap || LODGroup == TEXTUREGROUP_Terrain_Weightmap);

		for (int32 MipIndex=0; MipIndex < MAX_TEXTURE_MIP_COUNT; ++MipIndex)
		{
			TextureSizes[MipIndex] = Texture->CalcTextureMemorySize(FMath::Min(MipIndex + 1, MipCount));
		}
	}
	else
	{
		LODGroup = TEXTUREGROUP_World;
		NumNonStreamingMips = 0;
		MipCount = 0;
		BoostFactor = 1.f;

		bIsCharacterTexture = false;
		bIsTerrainTexture = false;

		for (int32 MipIndex=0; MipIndex < MAX_TEXTURE_MIP_COUNT; ++MipIndex)
		{
			TextureSizes[MipIndex] = 0;
		}
	}
}

void FStreamingTexture::UpdateDynamicData(const int32 NumStreamedMips[TEXTUREGROUP_MAX], bool bOnlyStreamIn, int32 GlobalMipBias, int32 HLODStrategy)
{
	// Note that those values are read from the async task and must not be assigned temporary values!!
	if (Texture)
	{
		UpdateStreamingStatus();

		// The last render time of this texture. Can be FLT_MAX when texture has no resource.
		const float LastRenderTimeForTexture = Texture->GetLastRenderTimeForStreaming();
		LastRenderTime = (FApp::GetCurrentTime() > LastRenderTimeForTexture) ? float( FApp::GetCurrentTime() - LastRenderTimeForTexture ) : 0.0f;

		bForceFullyLoad = Texture->ShouldMipLevelsBeForcedResident() || (bOnlyStreamIn && LastRenderTime < 300)|| LODGroup == TEXTUREGROUP_Skybox || (LODGroup == TEXTUREGROUP_HierarchicalLOD && HLODStrategy == 2);

		const int32 NumCinematicMipLevels = (bForceFullyLoad && Texture->bUseCinematicMipLevels) ? Texture->NumCinematicMipLevels : 0;


		int32 LODBias = FMath::Max<int32>(Texture->GetCachedLODBias() - NumCinematicMipLevels, 0);

		// Reduce the max allowed resolution according to LODBias if the texture group allows it.
		if (LODGroup != TEXTUREGROUP_HierarchicalLOD && !bIsTerrainTexture)
		{
			LODBias += GlobalMipBias;
		}

		// The max mip count is affected by the texture bias and cinematic bias settings.
		MaxAllowedMips = FMath::Clamp<int32>(FMath::Min<int32>(MipCount - LODBias, GMaxTextureMipCount), NumNonStreamingMips, MipCount);

		if (LODGroup == TEXTUREGROUP_HierarchicalLOD && HLODStrategy == 1)
		{
			MinAllowedMips = FMath::Clamp<int32>(MaxAllowedMips - 1, NumNonStreamingMips, MaxAllowedMips);
		}
		else if (NumStreamedMips[LODGroup] > 0)
		{
			MinAllowedMips = FMath::Clamp<int32>(MipCount - NumStreamedMips[LODGroup], NumNonStreamingMips, MaxAllowedMips);
		}
		else
		{
			MinAllowedMips = NumNonStreamingMips;
		}
	}
	else
	{
		bReadyForStreaming = false;
		bInFlight = false;
		bForceFullyLoad = false;
		ResidentMips = 0;
		RequestedMips = 0;
		MinAllowedMips = 0;
		MaxAllowedMips = 0;
		LastRenderTime = FLT_MAX;	
	}
}

void FStreamingTexture::UpdateStreamingStatus()
{
	if (Texture)
	{
		bReadyForStreaming = Texture->IsReadyForStreaming();
		if (bReadyForStreaming)
		{
			bInFlight = Texture->UpdateStreamingStatus(true);
		}
		else
		{
			ensure(Texture->PendingMipChangeRequestStatus.GetValue() <= TexState_ReadyFor_Requests); // Could also be TexState_InProgress_Initialization
			bInFlight = false;
		}

		// This must be updated after UpdateStreamingStatus
		ResidentMips = Texture->ResidentMips;
		RequestedMips = Texture->RequestedMips;

		ensure(bInFlight || ResidentMips == RequestedMips);
	}
	else
	{
		bReadyForStreaming = false;
		bInFlight = false;
	}
}

float FStreamingTexture::GetExtraBoost(TextureGroup	LODGroup, float GlobalBias)
{
	// When accurate distance computation, we need to relax the distance otherwhise it get's too conservative. (ex 513 goes to 1024)
	const bool bUseApproxDistance = CVarStreamingUseNewMetrics.GetValueOnAnyThread() == 0;
	const float DistanceScale = bUseApproxDistance ? 1.f : .71f;

	if (LODGroup == TEXTUREGROUP_Terrain_Heightmap || LODGroup == TEXTUREGROUP_Terrain_Weightmap) 
	{
		// Terrain are not affected by any kind of scale. Important since instance can use hardcoded resolution.
		// Used the Distance Scale from the new metrics is not big enough to affect which mip gets selected.
		return DistanceScale;
	}
	else if (LODGroup == TEXTUREGROUP_Lightmap)
	{
		return FMath::Min<float>(DistanceScale, GLightmapStreamingFactor);
	}
	else if (LODGroup == TEXTUREGROUP_Shadowmap)
	{
		return FMath::Min<float>(DistanceScale, GShadowmapStreamingFactor);
	}
	else
	{
		return DistanceScale * FMath::Exp2(-GlobalBias);
	}
}

int32 FStreamingTexture::GetWantedMipsFromSize(float Size, int32 MipBias) const
{
	float WantedMipsFloat = 1.0f + FMath::Log2(FMath::Max(1.f, Size));
	// Using the new metrics has very conservative distance computation. Rounding to mip here compensates.
	int32 WantedMipsInt = FMath::CeilToInt(WantedMipsFloat);

	if (LODGroup != TEXTUREGROUP_HierarchicalLOD  && LODGroup != TEXTUREGROUP_Terrain_Heightmap)
	{
		return FMath::Max<int32>(FMath::Min<int32>(WantedMipsInt, MaxAllowedMips - MipBias), MinAllowedMips);
	}
	else
	{
		return FMath::Clamp<int32>(WantedMipsInt, MinAllowedMips, MaxAllowedMips);
	}
}

/** Set the wanted mips from the async task data */
void FStreamingTexture::SetPerfectWantedMips_Async(float MaxSize, float MaxSize_VisibleOnly, float MipBias, bool InLooksLowRes)
{
	const float HiddenScale = CVarStreamingHiddenPrimitiveScale.GetValueOnAnyThread();

	bForceFullyLoadHeuristic = (MaxSize == FLT_MAX || MaxSize_VisibleOnly == FLT_MAX);
	VisibleWantedMips = GetWantedMipsFromSize(MaxSize_VisibleOnly, MipBias);
	bLooksLowRes = InLooksLowRes; // Things like lightmaps, HLOD and close instances.

	// Terrain, Forced Fully Load and Things that already look bad are not affected by hidden scale.
	if (bIsTerrainTexture || bForceFullyLoadHeuristic || bLooksLowRes)
	{
		HiddenWantedMips = GetWantedMipsFromSize(MaxSize, MipBias);
		NumMissingMips = 0; // No impact for terrains as there are not allowed to drop mips.
	}
	else
	{
		HiddenWantedMips = GetWantedMipsFromSize(MaxSize * HiddenScale, MipBias);
		NumMissingMips = FMath::Max<int32>(GetWantedMipsFromSize(MaxSize, MipBias) - FMath::Max<int32>(VisibleWantedMips, HiddenWantedMips), 0);
	}
}

/**
 * Once the wanted mips are computed, the async task will check if everything fits in the budget.
 *  This only consider the highest mip that will be requested eventually, so that slip requests are stables.
 */
int64 FStreamingTexture::UpdateRetentionPriority_Async()
{
	// Reserve the budget for the max mip that will be loaded eventually (ignore the effect of split requests)
	BudgetedMips = GetPerfectWantedMips();
	RetentionPriority = 0;

	if (Texture)
	{
		const bool bShouldKeep = bIsTerrainTexture || bForceFullyLoadHeuristic || bLooksLowRes;
		const bool bIsSmall   = GetSize(BudgetedMips) <= 200 * 1024; 
		const bool bIsVisible = VisibleWantedMips >= HiddenWantedMips; // Whether the first mip dropped would be a visible mip or not.

		// Here we try to have a minimal amount of priority flags for last render time to be meaningless.
		// We mostly want thing not seen from a long time to go first to avoid repeating load / unload patterns.

		if (bShouldKeep)						RetentionPriority += 1024; // Keep forced fully load as much as possible.
		if (bIsVisible)							RetentionPriority += 512; // Keep visible things as much as possible.
		if (bIsCharacterTexture || bIsSmall)	RetentionPriority += 256; // Try to keep character of small texture as they don't pay off.
		if (!bIsVisible)						RetentionPriority += FMath::Clamp<int32>(255 - (int32)LastRenderTime, 1, 255); // Keep last visible first.

		return GetSize(BudgetedMips);
	}
	else
	{
		return 0;
	}
}

int64 FStreamingTexture::DropOneMip_Async()
{
	if (Texture && BudgetedMips > MinAllowedMips)
	{
		--BudgetedMips;
		return GetSize(BudgetedMips + 1) - GetSize(BudgetedMips);
	}
	else
	{
		return 0;
	}
}

int64 FStreamingTexture::KeepOneMip_Async()
{
	if (Texture && BudgetedMips < FMath::Min<int32>(ResidentMips, MaxAllowedMips))
	{
		++BudgetedMips;
		return GetSize(BudgetedMips) - GetSize(BudgetedMips - 1);
	}
	else
	{
		return 0;
	}
}

bool FStreamingTexture::UpdateLoadOrderPriority_Async()
{
	LoadOrderPriority = 0;
	WantedMips = BudgetedMips;

	// If the entry is valid and we need to send a new request to load/drop the right mip.
	if (Texture && WantedMips != RequestedMips)
	{
		const bool bIsVisible			= WantedMips <= VisibleWantedMips; // Otherwise it means we are loading mips that are only usefull for non visible primitives.
		const bool bMustLoadFirst		= bForceFullyLoadHeuristic || bIsTerrainTexture || bIsCharacterTexture;
		const bool bMipIsImportant		= WantedMips - ResidentMips > (bLooksLowRes ? 1 : 2);

		if (bIsVisible)				LoadOrderPriority += 1024;
		if (bMustLoadFirst)			LoadOrderPriority += 512; 
		if (bMipIsImportant)		LoadOrderPriority += 256;
		if (!bIsVisible)			LoadOrderPriority += FMath::Clamp<int32>(255 - (int32)LastRenderTime, 1, 255);

		return true;
	}
	else
	{
		return false;
	}
}

void FStreamingTexture::CancelPendingMipChangeRequest()
{
	if (Texture && bReadyForStreaming && bInFlight)
	{
		// Try to cancel, but don't try anything else before next update.
		bReadyForStreaming = false;

		if (Texture->CancelPendingMipChangeRequest() )
		{
			bInFlight = false;
			RequestedMips = ResidentMips;
		}
	}
}

void FStreamingTexture::StreamWantedMips(FStreamingManagerTexture& Manager)
{
	if (Texture && bReadyForStreaming && !bInFlight && WantedMips != ResidentMips)
	{
		ensure(ResidentMips == RequestedMips);

		// Try to load/unload, but don't try anything else before next update.
		bReadyForStreaming = false;

		if (Texture->PendingMipChangeRequestStatus.GetValue() == TexState_ReadyFor_Requests)
		{
			bInFlight = true;
			RequestedMips = WantedMips;
			Texture->RequestedMips = WantedMips;

			FTexture2DResource* Texture2DResource = (FTexture2DResource*)Texture->Resource;
			Texture2DResource->BeginUpdateMipCount( (bForceFullyLoadHeuristic || bIsTerrainTexture || bIsCharacterTexture) && WantedMips <= VisibleWantedMips );
			TrackTextureEvent( this, Texture, bForceFullyLoadHeuristic != 0, &Manager );
		}
		else
		{
			// Did UpdateStreamingTextures() miss a texture? Should never happen!
			UE_LOG(LogContentStreaming, Warning, TEXT("BeginUpdateMipCount failure! PendingMipChangeRequestStatus == %d, Resident=%d, Requested=%d, Wanted=%d"), Texture->PendingMipChangeRequestStatus.GetValue(), Texture->ResidentMips, Texture->RequestedMips, WantedMips );
		}
	}
}
