// Copyright 2022 Escape Entertainment & Froyok


#include "PostProcessSubsystem.h"
#include "PostProcessDataAsset.h"
#include "Interfaces/IPluginManager.h"
#include "RenderGraph.h"
#include "SystemTextures.h"
#include "ScreenPass.h"
#include "PostProcess/PostProcessing.h"
#include "PostProcess/DrawRectangle.h"

// defines for UE4 single FP compatibility
#if ENGINE_MAJOR_VERSION >= 5
#define VECTOR2 FVector2f
#define VECTOR4 FVector4f
#else
#define VECTOR2 FVector2D
#define VECTOR4 FVector4
#endif

namespace
{
    // RDG buffer input shared by all passes
    BEGIN_SHADER_PARAMETER_STRUCT(FCustomPostProcessParameters, )
    SHADER_PARAMETER_RDG_TEXTURE(Texture2D, InputTexture)
    RENDER_TARGET_BINDING_SLOTS()
    END_SHADER_PARAMETER_STRUCT()

    // The vertex shader to draw a rectangle.
    class FCustomScreenPassVS : public FGlobalShader
    {
    public:
        DECLARE_GLOBAL_SHADER(FCustomScreenPassVS);

        static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters&)
        {
            return true;
        }

        FCustomScreenPassVS() = default;
        FCustomScreenPassVS(const ShaderMetaType::CompiledShaderInitializerType & Initializer)
            : FGlobalShader(Initializer)
        {}
    };
    IMPLEMENT_GLOBAL_SHADER(FCustomScreenPassVS, "/CustomShaders/ScreenPass.usf", "CustomScreenPassVS", SF_Vertex);

#if WITH_EDITOR
    // Editor rescale shader
    class FEditorRescalePS : public FGlobalShader
    {
    public:
        DECLARE_GLOBAL_SHADER(FEditorRescalePS);
        SHADER_USE_PARAMETER_STRUCT(FEditorRescalePS, FGlobalShader);

        BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER_STRUCT_INCLUDE(FCustomPostProcessParameters, Pass)
        SHADER_PARAMETER_SAMPLER(SamplerState, InputSampler)
        SHADER_PARAMETER(VECTOR2, InputViewportSize)
        END_SHADER_PARAMETER_STRUCT()

        static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
        {
            return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
        }
    };
    IMPLEMENT_GLOBAL_SHADER(FEditorRescalePS, "/CustomShaders/EditorRescale.usf", "RescalePS", SF_Pixel);
#endif

    //----------------------------------------------------------
    // Bloom shaders
    //----------------------------------------------------------

    // Bloom downsample
    class FDownsamplePS : public FGlobalShader
    {
    public:
        DECLARE_GLOBAL_SHADER(FDownsamplePS);
        SHADER_USE_PARAMETER_STRUCT(FDownsamplePS, FGlobalShader);

        BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER_STRUCT_INCLUDE(FCustomPostProcessParameters, Pass)
        SHADER_PARAMETER_SAMPLER(SamplerState, InputSampler)
        SHADER_PARAMETER(VECTOR2, InputSize)
        END_SHADER_PARAMETER_STRUCT()

        static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
        {
            return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
        }
    };
    IMPLEMENT_GLOBAL_SHADER(FDownsamplePS, "/CustomShaders/Downsample.usf", "DownsamplePS", SF_Pixel);

    // Bloom upsample + combine
    class FUpsampleCombinePS : public FGlobalShader
    {
    public:
        DECLARE_GLOBAL_SHADER(FUpsampleCombinePS);
        SHADER_USE_PARAMETER_STRUCT(FUpsampleCombinePS, FGlobalShader);

        BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER_STRUCT_INCLUDE(FCustomPostProcessParameters, Pass)
        SHADER_PARAMETER_SAMPLER(SamplerState, InputSampler)
        SHADER_PARAMETER(VECTOR2, InputSize)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, PreviousTexture)
        SHADER_PARAMETER(float, Radius)
        END_SHADER_PARAMETER_STRUCT()

        static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
        {
            return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
        }
    };
    IMPLEMENT_GLOBAL_SHADER(FUpsampleCombinePS, "/CustomShaders/Upsample.usf", "UpsampleCombinePS", SF_Pixel);

    //----------------------------------------------------------
    // Flare shaders
    //----------------------------------------------------------

    // Blur shader (use Dual Kawase method)
    class FKawaseBlurDownPS : public FGlobalShader
    {
    public:
        DECLARE_GLOBAL_SHADER(FKawaseBlurDownPS);
        SHADER_USE_PARAMETER_STRUCT(FKawaseBlurDownPS, FGlobalShader);

        BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER_STRUCT_INCLUDE(FCustomPostProcessParameters, Pass)
        SHADER_PARAMETER_SAMPLER(SamplerState, InputSampler)
        SHADER_PARAMETER(VECTOR2, BufferSize)
        END_SHADER_PARAMETER_STRUCT()

        static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
        {
            return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
        }
    };
    class FKawaseBlurUpPS : public FGlobalShader
    {
    public:
        DECLARE_GLOBAL_SHADER(FKawaseBlurUpPS);
        SHADER_USE_PARAMETER_STRUCT(FKawaseBlurUpPS, FGlobalShader);

        BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER_STRUCT_INCLUDE(FCustomPostProcessParameters, Pass)
        SHADER_PARAMETER_SAMPLER(SamplerState, InputSampler)
        SHADER_PARAMETER(VECTOR2, BufferSize)
        END_SHADER_PARAMETER_STRUCT()

        static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
        {
            return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
        }
    };
    IMPLEMENT_GLOBAL_SHADER(FKawaseBlurDownPS, "/CustomShaders/DualKawaseBlur.usf", "KawaseBlurDownsamplePS", SF_Pixel);
    IMPLEMENT_GLOBAL_SHADER(FKawaseBlurUpPS, "/CustomShaders/DualKawaseBlur.usf", "KawaseBlurUpsamplePS", SF_Pixel);

    // Chromatic shift shader
    class FLensFlareChromaPS : public FGlobalShader
    {
    public:
        DECLARE_GLOBAL_SHADER(FLensFlareChromaPS);
        SHADER_USE_PARAMETER_STRUCT(FLensFlareChromaPS, FGlobalShader);

        BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER_STRUCT_INCLUDE(FCustomPostProcessParameters, Pass)
        SHADER_PARAMETER_SAMPLER(SamplerState, InputSampler)
        SHADER_PARAMETER(float, ChromaShift)
        END_SHADER_PARAMETER_STRUCT()

        static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
        {
            return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
        }
    };
    IMPLEMENT_GLOBAL_SHADER(FLensFlareChromaPS, "/CustomShaders/Chroma.usf", "ChromaPS", SF_Pixel);

    // Ghost shader
    class FLensFlareGhostsPS : public FGlobalShader
    {
    public:
        DECLARE_GLOBAL_SHADER(FLensFlareGhostsPS);
        SHADER_USE_PARAMETER_STRUCT(FLensFlareGhostsPS, FGlobalShader);

        BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER_STRUCT_INCLUDE(FCustomPostProcessParameters, Pass)
        SHADER_PARAMETER_SAMPLER(SamplerState, InputSampler)
        SHADER_PARAMETER_ARRAY(VECTOR4, GhostColors, [8])
			// temporary evil hack due to the float param being converted to Vector4D with SHADER_PARAMETER_SCALAR_ARRAY
        SHADER_PARAMETER_ARRAY(VECTOR4, GhostScales, [8])
        SHADER_PARAMETER(float, Intensity)
        SHADER_PARAMETER(float, ChromaShift)
        SHADER_PARAMETER(float, Compression)
		SHADER_PARAMETER(VECTOR2, InputScreenSize)
		SHADER_PARAMETER_TEXTURE(Texture2D, StarburstTexture)
		SHADER_PARAMETER_SAMPLER(SamplerState, StarburstSampler)
        END_SHADER_PARAMETER_STRUCT()

        static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
        {
            return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
        }
    };
    IMPLEMENT_GLOBAL_SHADER(FLensFlareGhostsPS, "/CustomShaders/Ghosts.usf", "GhostsPS", SF_Pixel);

    class FLensFlareHaloPS : public FGlobalShader
    {
    public:
        DECLARE_GLOBAL_SHADER(FLensFlareHaloPS);
        SHADER_USE_PARAMETER_STRUCT(FLensFlareHaloPS, FGlobalShader);

        BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
            SHADER_PARAMETER_STRUCT_INCLUDE(FCustomPostProcessParameters, Pass)
            SHADER_PARAMETER_SAMPLER(SamplerState, InputSampler)
            SHADER_PARAMETER(float, Width)
            SHADER_PARAMETER(float, Mask)
            SHADER_PARAMETER(float, Compression)
            SHADER_PARAMETER(float, Intensity)
            SHADER_PARAMETER(float, ChromaShift)
			SHADER_PARAMETER(VECTOR2, InputScreenSize)
			SHADER_PARAMETER_TEXTURE(Texture2D, StarburstTexture)
			SHADER_PARAMETER_SAMPLER(SamplerState, StarburstSampler)
            END_SHADER_PARAMETER_STRUCT()

            static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
        {
            return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
        }
    };
    IMPLEMENT_GLOBAL_SHADER(FLensFlareHaloPS, "/CustomShaders/Halo.usf", "HaloPS", SF_Pixel);

    //----------------------------------------------------------
    // Glare shaders
    //----------------------------------------------------------

    // Glare shader pass
    class FGlareVS : public FGlobalShader
    {
    public:
        DECLARE_GLOBAL_SHADER(FGlareVS);
        SHADER_USE_PARAMETER_STRUCT(FGlareVS, FGlobalShader);

        BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER_STRUCT_INCLUDE(FCustomPostProcessParameters, Pass)
        SHADER_PARAMETER_SAMPLER(SamplerState, InputSampler)
        SHADER_PARAMETER(FIntPoint, TileCount)
        SHADER_PARAMETER(VECTOR4, PixelSize)
        SHADER_PARAMETER(VECTOR2, BufferSize)
        END_SHADER_PARAMETER_STRUCT()
    };
    class FGlareGS : public FGlobalShader
    {
    public:
        DECLARE_GLOBAL_SHADER(FGlareGS);
        SHADER_USE_PARAMETER_STRUCT(FGlareGS, FGlobalShader);

        BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER(VECTOR4, PixelSize)
        SHADER_PARAMETER(VECTOR2, BufferSize)
        SHADER_PARAMETER(VECTOR2, BufferRatio)
        SHADER_PARAMETER(float, GlareIntensity)
        SHADER_PARAMETER(float, GlareDivider)
        SHADER_PARAMETER(VECTOR4, GlareTint)
			// this was [3] float array before, consolidated out of Vector4 evil hack
        SHADER_PARAMETER(VECTOR4, GlareScales)
        END_SHADER_PARAMETER_STRUCT()
    };
    class FGlarePS : public FGlobalShader
    {
    public:
        DECLARE_GLOBAL_SHADER(FGlarePS);
        SHADER_USE_PARAMETER_STRUCT(FGlarePS, FGlobalShader);

        BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER_SAMPLER(SamplerState, GlareSampler)
        SHADER_PARAMETER_TEXTURE(Texture2D, GlareTexture)
        END_SHADER_PARAMETER_STRUCT()

        static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
        {
            return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
        }
    };
    IMPLEMENT_GLOBAL_SHADER(FGlareVS, "/CustomShaders/Glare.usf", "GlareVS", SF_Vertex);
    IMPLEMENT_GLOBAL_SHADER(FGlareGS, "/CustomShaders/Glare.usf", "GlareGS", SF_Geometry);
    IMPLEMENT_GLOBAL_SHADER(FGlarePS, "/CustomShaders/Glare.usf", "GlarePS", SF_Pixel);

    //----------------------------------------------------------

    // Final bloom mix shader
    class FMixPS : public FGlobalShader
    {
    public:
        DECLARE_GLOBAL_SHADER(FMixPS);
        SHADER_USE_PARAMETER_STRUCT(FMixPS, FGlobalShader);

        BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER_STRUCT_INCLUDE(FCustomPostProcessParameters, Pass)
        SHADER_PARAMETER_SAMPLER(SamplerState, InputSampler)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, BloomTexture)
        SHADER_PARAMETER(float, BloomIntensity)
		SHADER_PARAMETER(VECTOR2, InputScreenSize)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, GlareTexture)
        SHADER_PARAMETER_TEXTURE(Texture2D, GradientTexture)
        SHADER_PARAMETER_SAMPLER(SamplerState, GradientSampler)
        SHADER_PARAMETER(VECTOR4, FlareTint)
        SHADER_PARAMETER(VECTOR2, InputViewportSize)
        SHADER_PARAMETER(VECTOR2, BufferSize)
        SHADER_PARAMETER(VECTOR2, PixelSize)
        SHADER_PARAMETER(FIntVector, MixPass)
        SHADER_PARAMETER(float, FlareIntensity)
        END_SHADER_PARAMETER_STRUCT()

        static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
        {
            return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
        }
    };
    IMPLEMENT_GLOBAL_SHADER(FMixPS, "/CustomShaders/Mix.usf", "MixPS", SF_Pixel);
}

//----------------------------------------------------------
// Console vars
//----------------------------------------------------------

TAutoConsoleVariable<int32> CVarBloomPassAmount(
    TEXT("r.PrettyPostProcess.BloomPassAmount"),
    7,
    TEXT("Maximum number of passes to render bloom"),
    ECVF_RenderThreadSafe);

TAutoConsoleVariable<int32> CVarBloomResLimit(
	TEXT("r.PrettyPostProcess.BloomResLimit"),
	16,
	TEXT("Minimum downscaling size for the Bloom. This will affect how large the bloom will be."),
	ECVF_RenderThreadSafe);

TAutoConsoleVariable<float> CVarBloomRadius(
    TEXT("r.PrettyPostProcess.BloomRadius"),
    0.85,
    TEXT("Size/Scale of the Bloom"),
    ECVF_RenderThreadSafe);


TAutoConsoleVariable<int32> CVarRenderFlarePass(
    TEXT("r.PrettyPostProcess.RenderFlare"),
    1,
    TEXT(" 0: Don't render flare pass\n")
    TEXT(" 1: Render flare pass"),
    ECVF_RenderThreadSafe);

TAutoConsoleVariable<int32> CVarRenderHaloPass(
    TEXT("r.PrettyPostProcess.RenderHalo"),
    1,
    TEXT(" 0: Don't render halo pass\n")
    TEXT(" 1: Render halo pass (ring effect)"),
    ECVF_RenderThreadSafe);

TAutoConsoleVariable<int32> CVarRenderGlarePass(
    TEXT("r.PrettyPostProcess.RenderGlare"),
    1,
    TEXT(" 0: Don't render glare pass\n")
    TEXT(" 1: Render glare pass (star shape)"),
    ECVF_RenderThreadSafe);

//----------------------------------------------------------

DECLARE_GPU_STAT(PrettyPostProcess)

//----------------------------------------------------------
// Subsystem core functions
//----------------------------------------------------------

void UPostProcessSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    //--------------------------------
    // Setup delegate
    //--------------------------------
    FPP_CustomBloomFlare::FDelegate Delegate = FPP_CustomBloomFlare::FDelegate::CreateLambda(
        [=, this](FRDGBuilder& GraphBuilder, const FViewInfo& View, const FScreenPassTexture& SceneColor, FScreenPassTexture& Output)
        {
            Render(GraphBuilder, View, SceneColor, Output);
        });

    ENQUEUE_RENDER_COMMAND(BindRenderThreadDelegates)([Delegate](FRHICommandListImmediate& RHICmdList)
        {
            PP_CustomBloomFlare.Add(Delegate);
        });

    //--------------------------------
    // Data asset loading
    // NOTE: Ideally this would be something that Developer Setting would set,
	//		 however, it crashes everytime you adjust the values. This is not
	//		 ideal, but works.
    //--------------------------------
    FString Path = "PostProcessDataAsset'/PrettyPostProcess/DA_PostProcess_Default.DA_PostProcess_Default'";
    
    PostProcessDataAsset = LoadObject<UPostProcessDataAsset>(nullptr, *Path);
}

void UPostProcessSubsystem::Deinitialize()
{
    ClearBlendState = nullptr;
    AdditiveBlendState = nullptr;
    BilinearClampSampler = nullptr;
    BilinearBorderSampler = nullptr;
    BilinearRepeatSampler = nullptr;
    NearestRepeatSampler = nullptr;
}


void UPostProcessSubsystem::InitStates()
{
    if (ClearBlendState != nullptr)
    {
        return;
    }

    // Blend modes from:
    // '/Engine/Source/Runtime/RenderCore/Private/ClearQuad.cpp'
    // '/Engine/Source/Runtime/Renderer/Private/PostProcess/PostProcessMaterial.cpp'
    ClearBlendState = TStaticBlendState<>::GetRHI();
    AdditiveBlendState = TStaticBlendState<CW_RGB, BO_Add, BF_One, BF_One>::GetRHI();

    BilinearClampSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
    BilinearBorderSampler = TStaticSamplerState<SF_Bilinear, AM_Border, AM_Border, AM_Border>::GetRHI();
    BilinearRepeatSampler = TStaticSamplerState<SF_Bilinear, AM_Wrap, AM_Wrap, AM_Wrap>::GetRHI();
    NearestRepeatSampler = TStaticSamplerState<SF_Point, AM_Wrap, AM_Wrap, AM_Wrap>::GetRHI();
}

// The function that draw a shader into a given RenderGraph texture
template<typename TShaderParameters, typename TShaderClassVertex, typename TShaderClassPixel>
inline void DrawShaderPass(
    FRDGBuilder& GraphBuilder,
    const FString& PassName,
    TShaderParameters* PassParameters,
    TShaderMapRef<TShaderClassVertex> VertexShader,
    TShaderMapRef<TShaderClassPixel> PixelShader,
    FRHIBlendState* BlendState,
    const FIntRect& Viewport
)
{
    const FScreenPassPipelineState PipelineState(VertexShader, PixelShader, BlendState);

    GraphBuilder.AddPass(
        FRDGEventName(TEXT("%s"), *PassName),
        PassParameters,
        ERDGPassFlags::Raster,
        [PixelShader, PassParameters, Viewport, PipelineState](FRHICommandListImmediate& RHICmdList)
        {
            RHICmdList.SetViewport(
                Viewport.Min.X, Viewport.Min.Y, 0.0f,
                Viewport.Max.X, Viewport.Max.Y, 1.0f
            );

            SetScreenPassPipelineState(RHICmdList, PipelineState);

            SetShaderParameters(
                RHICmdList,
                PixelShader,
                PixelShader.GetPixelShader(),
                *PassParameters
            );

            UE::Renderer::PostProcess::DrawRectangle(
                RHICmdList,                             // FRHICommandList
                PipelineState.VertexShader,             // const TShaderRefBase VertexShader
                0.0f, 0.0f,                             // float X, float Y
                Viewport.Width(), Viewport.Height(),    // float SizeX, float SizeY
                Viewport.Min.X, Viewport.Min.Y,         // float U, float V
                Viewport.Width(),                       // float SizeU
                Viewport.Height(),                      // float SizeV
                Viewport.Size(),                        // FIntPoint TargetSize
                Viewport.Size(),                        // FIntPoint TextureSize
                EDrawRectangleFlags::EDRF_Default       // EDrawRectangleFlags Flags
            );

        });
}

FVector2f GetInputViewportSize(const FIntRect& Input, const FIntPoint& Extent)
{
    // Based on GetScreenPassTextureViewportParameters()
    // Engine/Source/Runtime/Renderer/Private/ScreenPass.cpp

    FVector2f ExtentInverse = FVector2f(1.0f / Extent.X, 1.0f / Extent.Y);

    FVector2f RectMin = FVector2f(Input.Min);
    FVector2f RectMax = FVector2f(Input.Max);

    FVector2f Min = RectMin * ExtentInverse;
    FVector2f Max = RectMax * ExtentInverse;

    return (Max - Min);
}

//----------------------------------------------------------
// Render functions - Bloom
//----------------------------------------------------------

FRDGTextureRef UPostProcessSubsystem::RenderDownsample(
    FRDGBuilder& GraphBuilder,
    const FString& PassName,
    const FViewInfo& View,
    FRDGTextureRef InputTexture,
    const FIntRect& Viewport
)
{
    // Build texture
    FRDGTextureDesc Description = InputTexture->Desc;
    Description.Reset();
    Description.Extent = Viewport.Size();
    Description.Format = PF_FloatRGB;
    Description.ClearValue = FClearValueBinding(FLinearColor::Black);
    FRDGTextureRef TargetTexture = GraphBuilder.CreateTexture(Description, *PassName);

    // Render shader
    TShaderMapRef<FCustomScreenPassVS> VertexShader(View.ShaderMap);
    TShaderMapRef<FDownsamplePS> PixelShader(View.ShaderMap);

    FDownsamplePS::FParameters* PassParameters = GraphBuilder.AllocParameters<FDownsamplePS::FParameters>();

    PassParameters->Pass.InputTexture = InputTexture;
    PassParameters->Pass.RenderTargets[0] = FRenderTargetBinding(TargetTexture, ERenderTargetLoadAction::ENoAction);
    PassParameters->InputSampler = BilinearBorderSampler;
    PassParameters->InputSize = FVector2f(Viewport.Size());

    DrawShaderPass(
        GraphBuilder,
        PassName,
        PassParameters,
        VertexShader,
        PixelShader,
        ClearBlendState,
        Viewport
    );

    return TargetTexture;
}



FRDGTextureRef UPostProcessSubsystem::RenderUpsampleCombine(
    FRDGBuilder& GraphBuilder,
    const FString& PassName,
    const FViewInfo& View,
    const FScreenPassTexture& InputTexture,
    const FScreenPassTexture& PreviousTexture,
    float Radius
)
{
    // Build texture
    FRDGTextureDesc Description = InputTexture.Texture->Desc;
    Description.Reset();
    Description.Extent = InputTexture.ViewRect.Size();
    Description.Format = PF_FloatRGB;
    Description.ClearValue = FClearValueBinding(FLinearColor::Black);
    FRDGTextureRef TargetTexture = GraphBuilder.CreateTexture(Description, *PassName);

    TShaderMapRef<FCustomScreenPassVS> VertexShader(View.ShaderMap);
    TShaderMapRef<FUpsampleCombinePS> PixelShader(View.ShaderMap);

    FUpsampleCombinePS::FParameters* PassParameters = GraphBuilder.AllocParameters<FUpsampleCombinePS::FParameters>();

    PassParameters->Pass.InputTexture = InputTexture.Texture;
    PassParameters->Pass.RenderTargets[0] = FRenderTargetBinding(TargetTexture, ERenderTargetLoadAction::ENoAction);
    PassParameters->InputSampler = BilinearClampSampler;
    PassParameters->InputSize = FVector2f(PreviousTexture.ViewRect.Size());
    PassParameters->PreviousTexture = PreviousTexture.Texture;
    PassParameters->Radius = Radius;

    DrawShaderPass(
        GraphBuilder,
        PassName,
        PassParameters,
        VertexShader,
        PixelShader,
        ClearBlendState,
        InputTexture.ViewRect
    );

    return TargetTexture;
}

//----------------------------------------------------------
// Render functions - Flare
//----------------------------------------------------------

FRDGTextureRef UPostProcessSubsystem::RenderBlur(
    FRDGBuilder& GraphBuilder,
    FRDGTextureRef InputTexture,
    const FViewInfo& View,
    const FIntRect& Viewport,
    int BlurSteps
)
{
    // Shader setup
    TShaderMapRef<FCustomScreenPassVS>  VertexShader(View.ShaderMap);
    TShaderMapRef<FKawaseBlurDownPS>    PixelShaderDown(View.ShaderMap);
    TShaderMapRef<FKawaseBlurUpPS>      PixelShaderUp(View.ShaderMap);

    // Data setup
    FRDGTextureRef PreviousBuffer = InputTexture;
    const FRDGTextureDesc& InputDescription = InputTexture->Desc;

    const FString PassDownName = TEXT("Down");
    const FString PassUpName = TEXT("Up");
    const int32 ArraySize = BlurSteps * 2;

    // Viewport resolutions
    int32 Divider = 2;
    TArray<FIntRect> Viewports;
    for (int32 i = 0; i < ArraySize; i++)
    {
        FIntRect NewRect = FIntRect(
            0,
            0,
            Viewport.Width() / Divider,
            Viewport.Height() / Divider
        );

        Viewports.Add(NewRect);

        if (i < (BlurSteps - 1))
        {
            Divider *= 2;
        }
        else
        {
            Divider /= 2;
        }
    }

    // Render
    for (int32 i = 0; i < ArraySize; i++)
    {
        // Build texture
        FRDGTextureDesc BlurDesc = InputDescription;
        BlurDesc.Reset();
        BlurDesc.Extent = Viewports[i].Size();
        BlurDesc.Format = PF_FloatRGB;
        BlurDesc.NumMips = 1;
        BlurDesc.ClearValue = FClearValueBinding(FLinearColor::Transparent);

        FVector2f ViewportResolution = FVector2f(
            Viewports[i].Width(),
            Viewports[i].Height()
        );

        const FString PassName =
            FString("KawaseBlur")
            + FString::Printf(TEXT("_%i_"), i)
            + ((i < BlurSteps) ? PassDownName : PassUpName)
            + FString::Printf(TEXT("_%ix%i"), Viewports[i].Width(), Viewports[i].Height());

        FRDGTextureRef Buffer = GraphBuilder.CreateTexture(BlurDesc, *PassName);

        // Render shader
        if (i < BlurSteps)
        {
            FKawaseBlurDownPS::FParameters* PassDownParameters = GraphBuilder.AllocParameters<FKawaseBlurDownPS::FParameters>();
            PassDownParameters->Pass.InputTexture = PreviousBuffer;
            PassDownParameters->Pass.RenderTargets[0] = FRenderTargetBinding(Buffer, ERenderTargetLoadAction::ENoAction);
            PassDownParameters->InputSampler = BilinearClampSampler;
            PassDownParameters->BufferSize = ViewportResolution;

            DrawShaderPass(
                GraphBuilder,
                PassName,
                PassDownParameters,
                VertexShader,
                PixelShaderDown,
                ClearBlendState,
                Viewports[i]
            );
        }
        else
        {
            FKawaseBlurUpPS::FParameters* PassUpParameters = GraphBuilder.AllocParameters<FKawaseBlurUpPS::FParameters>();
            PassUpParameters->Pass.InputTexture = PreviousBuffer;
            PassUpParameters->Pass.RenderTargets[0] = FRenderTargetBinding(Buffer, ERenderTargetLoadAction::ENoAction);
            PassUpParameters->InputSampler = BilinearClampSampler;
            PassUpParameters->BufferSize = ViewportResolution;

            DrawShaderPass(
                GraphBuilder,
                PassName,
                PassUpParameters,
                VertexShader,
                PixelShaderUp,
                ClearBlendState,
                Viewports[i]
            );
        }

        PreviousBuffer = Buffer;
    }

    return PreviousBuffer;
}

FRDGTextureRef UPostProcessSubsystem::RenderGhosts(
    FRDGBuilder& GraphBuilder,
    const FString& PassName,
    const FViewInfo& View,
    FRDGTextureRef InputTexture,
    const FIntRect& Viewport
)
{
    FRDGTextureRef TargetTexture = nullptr;
    FRDGTextureRef GhostsTexture = nullptr;
    {
        //----------------------------------------------------------
        // Ghosts
        //----------------------------------------------------------

        // Build buffer
        FRDGTextureDesc Description = InputTexture->Desc;
        Description.Reset();
        Description.Extent = Viewport.Size();
        Description.Format = PF_FloatRGB;
        Description.ClearValue = FClearValueBinding(FLinearColor::Transparent);
        GhostsTexture = GraphBuilder.CreateTexture(Description, *PassName);

        // Shader parameters
        TShaderMapRef<FCustomScreenPassVS> VertexShader(View.ShaderMap);
        TShaderMapRef<FLensFlareGhostsPS> PixelShader(View.ShaderMap);

        FLensFlareGhostsPS::FParameters* PassParameters = GraphBuilder.AllocParameters<FLensFlareGhostsPS::FParameters>();
        PassParameters->Pass.InputTexture = InputTexture;
        PassParameters->Pass.RenderTargets[0] = FRenderTargetBinding(GhostsTexture, ERenderTargetLoadAction::ENoAction);
        PassParameters->InputSampler = BilinearBorderSampler;
        PassParameters->Intensity = PostProcessDataAsset->GhostIntensity;
        PassParameters->ChromaShift = PostProcessDataAsset->GhostChromaShift;
        PassParameters->Compression = PostProcessDataAsset->GhostCompression;
		PassParameters->InputScreenSize = FVector2f(Viewport.Size());

		// Starburst
		PassParameters->StarburstTexture = GWhiteTexture->TextureRHI;
		PassParameters->StarburstSampler = BilinearRepeatSampler;

		if (PostProcessDataAsset->StarburstNoise != nullptr)
		{
			const FTextureRHIRef TextureRHI = PostProcessDataAsset->StarburstNoise->GetResource()->TextureRHI;
			PassParameters->StarburstTexture = TextureRHI;
		}

        PassParameters->GhostColors[0] = PostProcessDataAsset->Ghost1.Color;
        PassParameters->GhostColors[1] = PostProcessDataAsset->Ghost2.Color;
        PassParameters->GhostColors[2] = PostProcessDataAsset->Ghost3.Color;
        PassParameters->GhostColors[3] = PostProcessDataAsset->Ghost4.Color;
        PassParameters->GhostColors[4] = PostProcessDataAsset->Ghost5.Color;
        PassParameters->GhostColors[5] = PostProcessDataAsset->Ghost6.Color;
        PassParameters->GhostColors[6] = PostProcessDataAsset->Ghost7.Color;
        PassParameters->GhostColors[7] = PostProcessDataAsset->Ghost8.Color;

		// temporary evil hack due to the float param being converted to FVector4f with SHADER_PARAMETER_SCALAR_ARRAY in 5.1
        PassParameters->GhostScales[0].X = PostProcessDataAsset->Ghost1.Scale;
        PassParameters->GhostScales[1].X = PostProcessDataAsset->Ghost2.Scale;
        PassParameters->GhostScales[2].X = PostProcessDataAsset->Ghost3.Scale;
        PassParameters->GhostScales[3].X = PostProcessDataAsset->Ghost4.Scale;
        PassParameters->GhostScales[4].X = PostProcessDataAsset->Ghost5.Scale;
        PassParameters->GhostScales[5].X = PostProcessDataAsset->Ghost6.Scale;
        PassParameters->GhostScales[6].X = PostProcessDataAsset->Ghost7.Scale;
        PassParameters->GhostScales[7].X = PostProcessDataAsset->Ghost8.Scale;

        // Render
        DrawShaderPass(
            GraphBuilder,
            PassName,
            PassParameters,
            VertexShader,
            PixelShader,
            ClearBlendState,
            Viewport
        );

        TargetTexture = GhostsTexture;
    }

    return TargetTexture;
}

FRDGTextureRef UPostProcessSubsystem::RenderHalo(
    FRDGBuilder& GraphBuilder,
    const FString& PassName,
    const FViewInfo& View,
    const FScreenPassTexture& InputTexture
)
{
    // Build buffer
    FRDGTextureDesc Description = InputTexture.Texture->Desc;
    Description.Reset();
    Description.Extent = InputTexture.ViewRect.Size();
    Description.Format = PF_FloatRGB;
    Description.ClearValue = FClearValueBinding(FLinearColor::Black);
    FRDGTextureRef TargetTexture = GraphBuilder.CreateTexture(Description, *PassName);

    // Shader parameters
    TShaderMapRef<FCustomScreenPassVS> VertexShader(View.ShaderMap);
    TShaderMapRef<FLensFlareHaloPS> PixelShader(View.ShaderMap);

    FLensFlareHaloPS::FParameters* PassParameters = GraphBuilder.AllocParameters<FLensFlareHaloPS::FParameters>();
    PassParameters->Pass.InputTexture = InputTexture.Texture;
    PassParameters->Pass.RenderTargets[0] = FRenderTargetBinding(TargetTexture, ERenderTargetLoadAction::ENoAction);
    PassParameters->InputSampler = BilinearBorderSampler;
    PassParameters->Intensity = PostProcessDataAsset->HaloIntensity;
    PassParameters->Width = PostProcessDataAsset->HaloWidth;
    PassParameters->Mask = PostProcessDataAsset->HaloMask;
    PassParameters->Compression = PostProcessDataAsset->HaloCompression;
    PassParameters->ChromaShift = PostProcessDataAsset->HaloChromaShift;
	PassParameters->InputScreenSize = FVector2f(InputTexture.ViewRect.Size());

	// Starburst
	PassParameters->StarburstTexture = GWhiteTexture->TextureRHI;
	PassParameters->StarburstSampler = BilinearRepeatSampler;

	if (PostProcessDataAsset->StarburstNoise != nullptr)
	{
		const FTextureRHIRef TextureRHI = PostProcessDataAsset->StarburstNoise->GetResource()->TextureRHI;
		PassParameters->StarburstTexture = TextureRHI;
	}

    // Render
    DrawShaderPass(
        GraphBuilder,
        PassName,
        PassParameters,
        VertexShader,
        PixelShader,
        ClearBlendState,
        InputTexture.ViewRect
    );

    return TargetTexture;
}



//----------------------------------------------------------
// Render functions - Glare
//----------------------------------------------------------

FRDGTextureRef UPostProcessSubsystem::RenderGlare(
    FRDGBuilder& GraphBuilder,
    const FString& PassName,
    const FViewInfo& View,
    FRDGTextureRef InputTexture,
    const FIntRect& Viewport
)
{
    FRDGTextureRef TargetTexture = nullptr;

    // Only render the Glare if its intensity is different from 0
    if (PostProcessDataAsset->GlareIntensity > SMALL_NUMBER)
    {
        // This compute the number of point that will be drawn
        // Since we want one point for 2 by 2 pixel block we just 
        // need to divide the resolution by two to get this value.
        FIntPoint TileCount = Viewport.Size();
        TileCount.X = TileCount.X / 2;
        TileCount.Y = TileCount.Y / 2;
        int32 Amount = TileCount.X * TileCount.Y;

        // Compute the ratio between the width and height
        // to know how to adjust the scaling of the quads.
        // (This assume width is bigger than height.)
        FVector2f BufferRatio = FVector2f(
            float(Viewport.Height()) / float(Viewport.Width()),
            1.0f
        );

        // Build the buffer
        FRDGTextureDesc Description = InputTexture->Desc;
        Description.Reset();
        Description.Extent = Viewport.Size();
        Description.Format = PF_FloatRGB;
        Description.ClearValue = FClearValueBinding(FLinearColor::Transparent);
        FRDGTextureRef GlareTexture = GraphBuilder.CreateTexture(Description, *PassName);

        // Setup a few other variables that will 
        // be needed by the shaders.
        FVector4f PixelSize = FVector4f(0, 0, 0, 0);
        PixelSize.X = 1.0f / float(Viewport.Width());
        PixelSize.Y = 1.0f / float(Viewport.Height());
        PixelSize.Z = PixelSize.X;
        PixelSize.W = PixelSize.Y * -1.0f;

        FVector2f BufferSize = FVector2f(Description.Extent);

        // Setup shader
        FCustomPostProcessParameters* PassParameters = GraphBuilder.AllocParameters<FCustomPostProcessParameters>();
        PassParameters->InputTexture = InputTexture;
        PassParameters->RenderTargets[0] = FRenderTargetBinding(GlareTexture, ERenderTargetLoadAction::EClear);

        // Vertex shader
        FGlareVS::FParameters VertexParameters;
        VertexParameters.Pass = *PassParameters;
        VertexParameters.InputSampler = BilinearBorderSampler;
        VertexParameters.TileCount = TileCount;
        VertexParameters.PixelSize = PixelSize;
        VertexParameters.BufferSize = BufferSize;

        // Geometry shader
        FGlareGS::FParameters GeometryParameters;
        GeometryParameters.BufferSize = BufferSize;
        GeometryParameters.BufferRatio = BufferRatio;
        GeometryParameters.PixelSize = PixelSize;
        GeometryParameters.GlareIntensity = PostProcessDataAsset->GlareIntensity;
        GeometryParameters.GlareTint = FVector4f(PostProcessDataAsset->GlareTint);
        GeometryParameters.GlareScales.X = PostProcessDataAsset->GlareScale.X;
        GeometryParameters.GlareScales.Y = PostProcessDataAsset->GlareScale.Y;
        GeometryParameters.GlareScales.Z = PostProcessDataAsset->GlareScale.Z;
        GeometryParameters.GlareDivider = FMath::Max(PostProcessDataAsset->GlareDivider, 0.01f);

        // Pixel shader
        FGlarePS::FParameters PixelParameters;
        PixelParameters.GlareSampler = BilinearClampSampler;
        PixelParameters.GlareTexture = GWhiteTexture->TextureRHI;

        if (PostProcessDataAsset->GlareLineMask != nullptr)
        {
            const FTextureRHIRef TextureRHI = PostProcessDataAsset->GlareLineMask->GetResource()->TextureRHI;
            PixelParameters.GlareTexture = TextureRHI;
        }

        TShaderMapRef<FGlareVS> VertexShader(View.ShaderMap);
        TShaderMapRef<FGlareGS> GeometryShader(View.ShaderMap);
        TShaderMapRef<FGlarePS> PixelShader(View.ShaderMap);

        // Required for Lambda capture
        FRHIBlendState* BlendState = this->AdditiveBlendState;

        GraphBuilder.AddPass(
            RDG_EVENT_NAME("%s", *PassName),
            PassParameters,
            ERDGPassFlags::Raster,
            [
                VertexShader, VertexParameters,
                GeometryShader, GeometryParameters,
                PixelShader, PixelParameters,
                BlendState, Viewport, Amount
            ] (FRHICommandListImmediate& RHICmdList)
            {
                RHICmdList.SetViewport(
                    Viewport.Min.X, Viewport.Min.Y, 0.0f,
                    Viewport.Max.X, Viewport.Max.Y, 1.0f
                );

                FGraphicsPipelineStateInitializer GraphicsPSOInit;
                RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
                GraphicsPSOInit.BlendState = BlendState;
                GraphicsPSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
                GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
                GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GEmptyVertexDeclaration.VertexDeclarationRHI;
                GraphicsPSOInit.BoundShaderState.VertexShaderRHI = VertexShader.GetVertexShader();
				GraphicsPSOInit.BoundShaderState.SetGeometryShader(GeometryShader.GetGeometryShader());
                GraphicsPSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();
                GraphicsPSOInit.PrimitiveType = PT_PointList;
				// TODO: Get StencilRef for this
                SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit);

                SetShaderParameters(RHICmdList, VertexShader, VertexShader.GetVertexShader(), VertexParameters);
                SetShaderParameters(RHICmdList, GeometryShader, GeometryShader.GetGeometryShader(), GeometryParameters);
                SetShaderParameters(RHICmdList, PixelShader, PixelShader.GetPixelShader(), PixelParameters);

                RHICmdList.SetStreamSource(0, nullptr, 0);
                RHICmdList.DrawPrimitive(0, 1, Amount);
            });

        TargetTexture = GlareTexture;

    }

    return TargetTexture;
}



//----------------------------------------------------------
// Render passes
//----------------------------------------------------------

FScreenPassTexture UPostProcessSubsystem::RenderBloom(
    FRDGBuilder& GraphBuilder,
    const FViewInfo& View,
    const FScreenPassTexture& SceneColor,
    int32 PassAmount
)
{
    check(SceneColor.IsValid());

    if (PassAmount <= 1)
    {
        return FScreenPassTexture();
    }

    RDG_EVENT_SCOPE(GraphBuilder, "BloomPass");

    //----------------------------------------------------------
    // Downsample
    //----------------------------------------------------------
    int32 Width = View.ViewRect.Width();
    int32 Height = View.ViewRect.Height();
    int32 Divider = 2;
    FRDGTextureRef PreviousTexture = SceneColor.Texture;

    for (int32 i = 0; i < PassAmount; i++)
    {
        FIntRect Size{
            0,
            0,
            FMath::Max(Width / Divider, 1),
            FMath::Max(Height / Divider, 1)
        };

        const FString PassName = "Downsample_"
            + FString::FromInt(i)
            + "_(1/"
            + FString::FromInt(Divider)
            + ")_"
            + FString::FromInt(Size.Width())
            + "x"
            + FString::FromInt(Size.Height());

        FRDGTextureRef Texture = nullptr;

        // The SceneColor input is already downscaled by the engine
        // so we just reference it and continue.
        if (i == 0)
        {
            Texture = PreviousTexture;
        }
        else
        {
            Texture = RenderDownsample(
                GraphBuilder,
                PassName,
                View,
                PreviousTexture,
                Size
            );
        }

        //Store the first downsample pass for flares
        if (i == 0)
        {
            FScreenPassTexture OutputTexture(Texture, Size);
            DownsampleTextureFlare = OutputTexture;
        }

        //Store the second downsample pass for glares
        if (i == 1)
        {
            FScreenPassTexture OutputTexture(Texture, Size);
            DownsampleTextureGlare = OutputTexture;
        }

        FScreenPassTexture DownsampleTexture(Texture, Size);

        MipMapsDownsample.Add(DownsampleTexture);
        PreviousTexture = Texture;
        Divider *= 2;
    }

    //----------------------------------------------------------
    // Upsample
    //----------------------------------------------------------
    float Radius = CVarBloomRadius.GetValueOnRenderThread();

    // Copy downsamples into upsample so that
    // we can easily access current and previous
    // inputs during the upsample process
    MipMapsUpsample.Append(MipMapsDownsample);

    // Starts at -2 since we need the last buffer
    // as the previous input (-2) and the one just
    // before as the current input (-1).
    // We also go from end to start of array to
    // go from small to big texture (going back up the mips)
    for (int32 i = PassAmount - 2; i >= 0; i--)
    {
        FIntRect CurrentSize = MipMapsUpsample[i].ViewRect;

        const FString PassName = "UpsampleCombine_"
            + FString::FromInt(i)
            + "_"
            + FString::FromInt(CurrentSize.Width())
            + "x"
            + FString::FromInt(CurrentSize.Height());

        // mix Halo pass into the upscaling process
        if (i == 1 && CVarRenderHaloPass.GetValueOnRenderThread())
        {
            FRDGTextureRef HaloTexture = RenderHalo(
                GraphBuilder,
                "HaloPass" + FString::FromInt(i),
                View,
                MipMapsUpsample[i]
            );
            FScreenPassTexture HaloMixTexture(HaloTexture, CurrentSize);
            MipMapsUpsample[i] = HaloMixTexture;
        }
        FRDGTextureRef ResultTexture = RenderUpsampleCombine(
            GraphBuilder,
            PassName,
            View,
            MipMapsUpsample[i],     // Current texture
            MipMapsUpsample[i + 1], // Previous texture,
            Radius
        );

        FScreenPassTexture NewTexture(ResultTexture, CurrentSize);
        MipMapsUpsample[i] = NewTexture;
    }

    return MipMapsUpsample[0];
}

FScreenPassTexture UPostProcessSubsystem::RenderFlarePass(
    FRDGBuilder& GraphBuilder,
    const FViewInfo& View,
    const FScreenPassTexture& SceneColor
)
{
    check(SceneColor.IsValid());

    if (!CVarRenderFlarePass.GetValueOnRenderThread())
    {
        return FScreenPassTexture();
    }

    RDG_EVENT_SCOPE(GraphBuilder, "FlarePass");

    int32 Width = SceneColor.ViewRect.Width();
    int32 Height = SceneColor.ViewRect.Height();

    FIntRect Size{
            0,
            0,
            Width,
            Height
    };


    FRDGTextureRef FlareTexture = nullptr;

    FlareTexture = RenderGhosts(
        GraphBuilder,
        "FlareGhosts",
        View,
        SceneColor.Texture,
        Size
    );

    FlareTexture = RenderBlur(
        GraphBuilder,
        FlareTexture,
        View,
        Size,
        1
    );
    
    FScreenPassTexture OutputTexture(FlareTexture, Size);
    return OutputTexture;
}

FScreenPassTexture UPostProcessSubsystem::RenderGlarePass(
    FRDGBuilder& GraphBuilder,
    const FViewInfo& View,
    const FScreenPassTexture& SceneColor
)
{
    check(SceneColor.IsValid());

    if (!CVarRenderGlarePass.GetValueOnRenderThread())
    {
        return FScreenPassTexture();
    }

    RDG_EVENT_SCOPE(GraphBuilder, "GlarePass");

    int32 Width = SceneColor.ViewRect.Width();
    int32 Height = SceneColor.ViewRect.Height();
    
    FIntRect Size{
            0,
            0,
            Width,
            Height
    };

    FRDGTextureRef GlareTexture = nullptr;
    
    GlareTexture = RenderGlare(
        GraphBuilder,
        "GlareRenderPass",
        View,
        SceneColor.Texture,
        Size
    );

    FScreenPassTexture OutputTexture(GlareTexture, Size);
    return OutputTexture;
}


//----------------------------------------------------------
// Final delegate render function
//----------------------------------------------------------

void UPostProcessSubsystem::Render(
    FRDGBuilder& GraphBuilder,
    const FViewInfo& View,
    const FScreenPassTexture& SceneColor,
    FScreenPassTexture& Output
)
{
    check(SceneColor.IsValid());

    if (PostProcessDataAsset == nullptr)
    {
        return;
    }

    InitStates();

    RDG_GPU_STAT_SCOPE(GraphBuilder, PrettyPostProcess)
    RDG_EVENT_SCOPE(GraphBuilder, "PrettyPostProcess");

    int32 PassAmount = 1;
	int32 ViewWidth = View.ViewRect.Width();
	int32 ViewHeight = View.ViewRect.Height();

	// Limit pass amount to not going smaller than specified resolution limit
	while (PassAmount < CVarBloomPassAmount.GetValueOnRenderThread()
		&& ViewWidth > CVarBloomResLimit.GetValueOnRenderThread() && ViewHeight > CVarBloomResLimit.GetValueOnRenderThread())
	{
		PassAmount++;
		ViewWidth /= 2;
		ViewHeight /= 2;

		// failsafe
		if (PassAmount > CVarBloomPassAmount.GetValueOnRenderThread()) break;
	}

    // Buffers setup
    const FScreenPassTexture BlackDummy{
        GraphBuilder.RegisterExternalTexture(
            GSystemTextures.BlackDummy,
            TEXT("BlackDummy")
        )
    };

    FScreenPassTexture BloomTexture;
    FScreenPassTexture FlareTexture;
    FScreenPassTexture GlareTexture;
    FScreenPassTexture InputTexture(SceneColor.Texture);

    // Scene color setup
    // We need to pass a FScreenPassTexture into FScreenPassTextureViewport()
    // and not a FRDGTextureRef (aka SceneColor.Texture) to ensure we can compute
    // the right Rect vs Extent sub-region. Otherwise only the full buffer 
    // resolution is gonna be reported leading to NaNs/garbage in the rendering.
    const FScreenPassTextureViewport SceneColorViewport(SceneColor);
    const FVector2f SceneColorViewportSize = GetInputViewportSize(SceneColorViewport.Rect, SceneColorViewport.Extent);

    //----------------------------------------------------------
    // Editor buffer rescale
    //----------------------------------------------------------
#if WITH_EDITOR
    // Rescale the Scene Color to fit the whole texture and not use a sub-region.
    // This is to simplify the render pass (shaders) that come after.
    // This part is skipped when built without the editor because
    // it is not needed (unless splitscreen needs it ?).
    if (SceneColorViewport.Rect.Width() != SceneColorViewport.Extent.X
        || SceneColorViewport.Rect.Height() != SceneColorViewport.Extent.Y)
    {
        const FString PassName("SceneColorRescale");

        // Build texture
        FRDGTextureDesc Desc = SceneColor.Texture->Desc;
        Desc.Reset();
        Desc.Extent = SceneColorViewport.Rect.Size();
        Desc.Format = PF_FloatRGB;
        Desc.ClearValue = FClearValueBinding(FLinearColor::Transparent);
        FRDGTextureRef RescaleTexture = GraphBuilder.CreateTexture(Desc, *PassName);

        // Render shader
        TShaderMapRef<FCustomScreenPassVS> VertexShader(View.ShaderMap);
        TShaderMapRef<FEditorRescalePS> PixelShader(View.ShaderMap);

        FEditorRescalePS::FParameters* PassParameters = GraphBuilder.AllocParameters<FEditorRescalePS::FParameters>();
        PassParameters->Pass.InputTexture = SceneColor.Texture;
        PassParameters->Pass.RenderTargets[0] = FRenderTargetBinding(RescaleTexture, ERenderTargetLoadAction::ENoAction);
        PassParameters->InputSampler = BilinearClampSampler;
        PassParameters->InputViewportSize = SceneColorViewportSize;

        DrawShaderPass(
            GraphBuilder,
            PassName,
            PassParameters,
            VertexShader,
            PixelShader,
            ClearBlendState,
            SceneColorViewport.Rect
        );

        InputTexture.Texture = RescaleTexture;
        InputTexture.ViewRect = SceneColorViewport.Rect;
    }
#endif

	

    //----------------------------------------------------------
    // Render passes
    //----------------------------------------------------------

    // Bloom
    {
        BloomTexture = RenderBloom(
            GraphBuilder,
            View,
            InputTexture,
            PassAmount
        );
    }

    // Flare
    {
        FlareTexture = RenderFlarePass(
            GraphBuilder,
            View,
            DownsampleTextureFlare
        );
    }

    // Glare
    {
        GlareTexture = RenderGlarePass(
            GraphBuilder,
            View,
            DownsampleTextureGlare
        );
    }

    //----------------------------------------------------------
    // Composite Bloom pass
    //----------------------------------------------------------
    FRDGTextureRef MixTexture = nullptr;
    FIntRect MixViewport{
        0,
        0,
        View.ViewRect.Width() / 2,
        View.ViewRect.Height() / 2
    };

    {
        RDG_EVENT_SCOPE(GraphBuilder, "MixPass");

        const FString PassName("Mix");

        float BloomIntensity = 1.0f;

        // If the internal blending for the upsample pass is additive
        // (aka not using the lerp) then uncomment this line to
        // normalize the final bloom intensity.

        BloomIntensity = 1.0f / float( FMath::Max( PassAmount, 1 ) );

        FVector2f BufferSize{
            float(MixViewport.Width()),
            float(MixViewport.Height())
        };

        FIntVector BuffersValidity{
            (BloomTexture.IsValid()),
            (FlareTexture.IsValid()),
            (GlareTexture.IsValid())
        };

        // Create texture
        FRDGTextureDesc Description = SceneColor.Texture->Desc;
        Description.Reset();
        Description.Extent = MixViewport.Size();
        Description.Format = PF_FloatRGB;
        Description.ClearValue = FClearValueBinding(FLinearColor::Black);
        MixTexture = GraphBuilder.CreateTexture(Description, *PassName);

        // Render shader
        TShaderMapRef<FCustomScreenPassVS> VertexShader(View.ShaderMap);
        TShaderMapRef<FMixPS> PixelShader(View.ShaderMap);

        FMixPS::FParameters* PassParameters = GraphBuilder.AllocParameters<FMixPS::FParameters>();
        PassParameters->Pass.RenderTargets[0] = FRenderTargetBinding(MixTexture, ERenderTargetLoadAction::ENoAction);
        PassParameters->InputSampler = BilinearClampSampler;
        PassParameters->MixPass = BuffersValidity;

		// Get screen size for aspect ratio calculation
		PassParameters->InputScreenSize = FVector2f(MixViewport.Size());

        // Bloom
        PassParameters->BloomTexture = BlackDummy.Texture;
        PassParameters->BloomIntensity = BloomIntensity;

        // Glare
        PassParameters->GlareTexture = BlackDummy.Texture;
        PassParameters->PixelSize = FVector2f(1.0f, 1.0f) / BufferSize;

        // Flare
        PassParameters->Pass.InputTexture = BlackDummy.Texture;
        PassParameters->FlareIntensity = PostProcessDataAsset->FlareIntensity;
        PassParameters->FlareTint = FVector4f(PostProcessDataAsset->FlareTint);
        PassParameters->GradientTexture = GWhiteTexture->TextureRHI;
        PassParameters->GradientSampler = BilinearClampSampler;

        if (BuffersValidity.X)
        {
            PassParameters->BloomTexture = BloomTexture.Texture;
        }

        if (BuffersValidity.Y)
        {
            PassParameters->Pass.InputTexture = FlareTexture.Texture;
        }

        if (BuffersValidity.Z)
        {
            PassParameters->GlareTexture = GlareTexture.Texture;
        }

		if (PostProcessDataAsset->FlareGradient != nullptr)
		{
			const FTextureRHIRef TextureRHI = PostProcessDataAsset->FlareGradient->GetResource()->TextureRHI;
			PassParameters->GradientTexture = TextureRHI;
		}

        // Render
        DrawShaderPass(
            GraphBuilder,
            PassName,
            PassParameters,
            VertexShader,
            PixelShader,
            ClearBlendState,
            MixViewport
        );

        // Reset texture lists
        MipMapsDownsample.Empty();
        MipMapsUpsample.Empty();
    }

    // Output
    Output.Texture = MixTexture;
    Output.ViewRect = MixViewport;

}
