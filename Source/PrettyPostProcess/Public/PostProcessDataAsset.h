// Copyright 2021 Invasion Games. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PostProcessDataAsset.generated.h"

// This custom struct is used to more easily
// setup and organize the settings for the Ghosts
USTRUCT(BlueprintType)
struct FLensFlareGhostSettings
{
    GENERATED_BODY()

        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flare")
        FLinearColor Color = FLinearColor::White;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flare")
        float Scale = 1.0f;
};

/**
 * 
 */
UCLASS()
class PRETTYPOSTPROCESS_API UPostProcessDataAsset : public UDataAsset
{
	GENERATED_BODY()
	
public:

    /** Intensity of the overall flare effect */
    UPROPERTY(EditAnywhere, Category = "Flare", meta = (UIMin = "0.0", UIMax = "10.0"))
    float FlareIntensity = 1.0f;

    /** Tint of the overall flare effect */
    UPROPERTY(EditAnywhere, Category = "Flare")
    FLinearColor FlareTint = FLinearColor(1.0f, 0.85f, 0.7f, 1.0f);

    /** 1D circular gradient of the overall flare effect */
    UPROPERTY(EditAnywhere, Category = "Flare")
	TObjectPtr<class UTexture2D> FlareGradient = nullptr;

	/** Circular starburst noise for the overall flare effect */
	UPROPERTY(EditAnywhere, Category = "Flare")
	TObjectPtr<class UTexture2D> StarburstNoise = nullptr;

    /** Blur steps for the ghosts */
    UPROPERTY(EditAnywhere, Category = "Ghosts", meta = (ClampMin = "0", ClampMax = "8", UIMin = "0", UIMax = "8"))
    int32 BlurSteps = 2;

    /** Compression of the flare ghosts's fisheye distortion. Larger numbers means less distortion */
    UPROPERTY(EditAnywhere, Category = "Ghosts", meta = (UIMin = "0.0", UIMax = "1.0"))
    float GhostCompression = 0.65f;

    /** Intensity of the flare ghosts */
    UPROPERTY(EditAnywhere, Category = "Ghosts", meta = (UIMin = "0.0", UIMax = "1.0"))
    float GhostIntensity = 1.0f;

    /** Intensity of the flare starburst filter */
    UPROPERTY(EditAnywhere, Category = "Ghosts", meta = (UIMin = "0.0", UIMax = "1.0"))
    float StarburstIntensity = 0.3f;

    /** Offset of the flare starburst filter */
    UPROPERTY(EditAnywhere, Category = "Ghosts", meta = (UIMin = "0.0", UIMax = "1.0"))
    float StarburstOffset = 0.0f;

    /** Chroma shift amount of the flare ghosts */
    UPROPERTY(EditAnywhere, Category = "Ghosts", meta = (UIMin = "0.0", UIMax = "1.0"))
    float GhostChromaShift = 0.015f;
    
    /** Tint and size of the first flare ghost pass */
    UPROPERTY(EditAnywhere, Category = "Ghosts")
    FLensFlareGhostSettings Ghost1 = { FLinearColor(1.0f, 0.8f, 0.4f, 1.0f), -1.5 };
    
    /** Tint and size of the second flare ghost pass */
    UPROPERTY(EditAnywhere, Category = "Ghosts")
    FLensFlareGhostSettings Ghost2 = { FLinearColor(1.0f, 1.0f, 0.6f, 1.0f),  2.5 };

    /** Tint and size of the third flare ghost pass */
    UPROPERTY(EditAnywhere, Category = "Ghosts")
    FLensFlareGhostSettings Ghost3 = { FLinearColor(0.8f, 0.8f, 1.0f, 1.0f), -5.0 };

    /** Tint and size of the fourth flare ghost pass */
    UPROPERTY(EditAnywhere, Category = "Ghosts")
    FLensFlareGhostSettings Ghost4 = { FLinearColor(0.5f, 1.0f, 0.4f, 1.0f), 10.0 };

    /** Tint and size of the fifth flare ghost pass */
    UPROPERTY(EditAnywhere, Category = "Ghosts")
    FLensFlareGhostSettings Ghost5 = { FLinearColor(0.5f, 0.8f, 1.0f, 1.0f),  0.7 };

    /** Tint and size of the sixth flare ghost pass */
    UPROPERTY(EditAnywhere, Category = "Ghosts")
    FLensFlareGhostSettings Ghost6 = { FLinearColor(0.9f, 1.0f, 0.8f, 1.0f), -0.4 };

    /** Tint and size of the seventh flare ghost pass */
    UPROPERTY(EditAnywhere, Category = "Ghosts")
    FLensFlareGhostSettings Ghost7 = { FLinearColor(1.0f, 0.8f, 0.4f, 1.0f), -0.2 };

    /** Tint and size of the eighth flare ghost pass */
    UPROPERTY(EditAnywhere, Category = "Ghosts")
    FLensFlareGhostSettings Ghost8 = { FLinearColor(0.9f, 0.7f, 0.7f, 1.0f), -0.1 };

    /** Intensity of the halo bloom */
    UPROPERTY(EditAnywhere, Category = "Halo", meta = (UIMin = "0.0", UIMax = "3.0"))
    float HaloIntensity = 1.0f;

    /** Width of the halo bloom */
    UPROPERTY(EditAnywhere, Category = "Halo", meta = (UIMin = "0.0", UIMax = "1.0"))
    float HaloWidth = 0.6f;

    /** Center mask of the halo bloom */
    UPROPERTY(EditAnywhere, Category = "Halo", meta = (UIMin = "0.0", UIMax = "1.0"))
    float HaloMask = 0.5f;

    /** Compression of the halo bloom's fisheye distortion. Larger numbers means less distortion */
    UPROPERTY(EditAnywhere, Category = "Halo", meta = (UIMin = "0.0", UIMax = "1.0"))
    float HaloCompression = 0.65f;

    /** Chroma shift amount of the halo bloom */
    UPROPERTY(EditAnywhere, Category = "Halo", meta = (UIMin = "0.0", UIMax = "1.0"))
    float HaloChromaShift = 0.015f;

    /** Intensity of the glare effect */
    UPROPERTY(EditAnywhere, Category = "Glare", meta = (UIMin = "0", UIMax = "10"))
    float GlareIntensity = 0.02f;

    UPROPERTY(EditAnywhere, Category = "Glare", meta = (UIMin = "0.01", UIMax = "200"))
    float GlareDivider = 60.0f;

    /** Size of each glare bars */
    UPROPERTY(EditAnywhere, Category = "Glare", meta = (UIMin = "0.0", UIMax = "10.0"))
    FVector GlareScale = FVector(1.0f, 1.0f, 1.0f);

    /** Tint of the glare effect */
    UPROPERTY(EditAnywhere, Category = "Glare")
    FLinearColor GlareTint = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);

    UPROPERTY(EditAnywhere, Category = "Glare")
    TObjectPtr<class UTexture2D> GlareLineMask = nullptr;
};
