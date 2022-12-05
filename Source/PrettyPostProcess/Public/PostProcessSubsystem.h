// Copyright 2021 Escape Entertainment & Froyok

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/EngineSubsystem.h"
#include "PostProcess/PostProcessing.h" // For PostProcess delegate
#include "PostProcessSubsystem.generated.h"

DECLARE_MULTICAST_DELEGATE_FourParams(FPP_CustomBloomFlare, FRDGBuilder&, const FViewInfo&, const FScreenPassTexture&, FScreenPassTexture&);
extern RENDERER_API FPP_CustomBloomFlare PP_CustomBloomFlare;

class UPostProcessDataAsset;
/**
 * 
 */
UCLASS()
class PRETTYPOSTPROCESS_API UPostProcessSubsystem : public UEngineSubsystem
{
	GENERATED_BODY()
	
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    virtual void Deinitialize() override;

private:
	TObjectPtr<UTexture2D> StarburstNoise;
	TObjectPtr<UTexture2D> FlareGradient;
	TObjectPtr<UTexture2D> GlareLineMask;

    //------------------------------------
    // Helpers
    //------------------------------------
    // Internal blending and sampling states;
    FRHIBlendState* ClearBlendState = nullptr;
    FRHIBlendState* AdditiveBlendState = nullptr;

    FRHISamplerState* BilinearClampSampler = nullptr;
    FRHISamplerState* BilinearBorderSampler = nullptr;
    FRHISamplerState* BilinearRepeatSampler = nullptr;
    FRHISamplerState* NearestRepeatSampler = nullptr;

    void InitStates();

    //------------------------------------
    // Main function
    //------------------------------------
    void Render(
        FRDGBuilder& GraphBuilder,
        const FViewInfo& View,
        const FScreenPassTexture& SceneColor,
        FScreenPassTexture& Output
    );

    TArray<FScreenPassTexture> MipMapsDownsample;
    TArray<FScreenPassTexture> MipMapsUpsample;


    //------------------------------------
    // Bloom
    //------------------------------------
    FScreenPassTexture RenderBloom(
        FRDGBuilder& GraphBuilder,
        const FViewInfo& View,
        const FScreenPassTexture& SceneColor,
        int32 PassAmount
    );

    FRDGTextureRef RenderDownsample(
        FRDGBuilder& GraphBuilder,
        const FString& PassName,
        const FViewInfo& View,
        FRDGTextureRef InputTexture,
        const FIntRect& Viewport
    );

    FRDGTextureRef RenderUpsampleCombine(
        FRDGBuilder& GraphBuilder,
        const FString& PassName,
        const FViewInfo& View,
        const FScreenPassTexture& InputTexture,
        const FScreenPassTexture& PreviousTexture,
        float Radius
    );

    //------------------------------------
    // Flare
    //------------------------------------

    // The reference to the data asset storing flare settings
    UPROPERTY(Transient)
    TObjectPtr<UPostProcessDataAsset> PostProcessDataAsset;

	UPROPERTY(Transient)
	TObjectPtr<class UPostProcessDevSettings> Settings;

    FRDGTextureRef RenderGhosts(
        FRDGBuilder& GraphBuilder,
        const FString& PassName,
        const FViewInfo& View,
        FRDGTextureRef InputTexture,
        const FIntRect& Viewport
    );

    FRDGTextureRef RenderHalo(
        FRDGBuilder& GraphBuilder,
        const FString& PassName,
        const FViewInfo& View,
        const FScreenPassTexture& InputTexture
    );

    FScreenPassTexture RenderFlarePass(
        FRDGBuilder& GraphBuilder,
        const FViewInfo& View,
        const FScreenPassTexture& SceneColor
    );

    // Sub-pass for flare blurring
    FRDGTextureRef RenderBlur(
        FRDGBuilder& GraphBuilder,
        FRDGTextureRef InputTexture,
        const FViewInfo& View,
        const FIntRect& Viewport,
        int BlurSteps
    );

    // Downsampled texture to be fed by flare
    FScreenPassTexture DownsampleTextureFlare;

    //------------------------------------
    // Glare
    //------------------------------------

    FRDGTextureRef RenderGlare(
        FRDGBuilder& GraphBuilder,
        const FString& PassName,
        const FViewInfo& View,
        FRDGTextureRef InputTexture,
        const FIntRect& Viewport
    );

    FScreenPassTexture RenderGlarePass(
        FRDGBuilder& GraphBuilder,
        const FViewInfo& View,
        const FScreenPassTexture& SceneColor
    );

    // Downsampled texture to be fed by flare
    FScreenPassTexture DownsampleTextureGlare;
};
