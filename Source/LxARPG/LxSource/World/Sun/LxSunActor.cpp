#include "LxSunActor.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/PostProcessComponent.h"
#include "HAL/IConsoleManager.h"
#include "UObject/UnrealType.h"
#include "Curves/CurveFloat.h"
#include "Curves/CurveLinearColor.h"
#include "Engine/World.h"
#include "Engine/SkyLight.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "EngineUtils.h"
#include "LxARPG/LxSource/Systems/LxGameInstanceSubsystem.h"
#include "LxARPG/LxSource/Systems/WorldInfo/LxWorldInfoManager.h"

ALxSunActor::ALxSunActor()
{
	PrimaryActorTick.bCanEverTick = false;
#if WITH_EDITORONLY_DATA
	// 与引擎方向光保持一致：太阳影响整个世界，不能跟随其摆放位置流送。
	bIsSpatiallyLoaded = false;
#endif
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("太阳根节点"));
	SetRootComponent(SceneRoot);
	SunLight = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("太阳光"));
	SunLight->SetupAttachment(SceneRoot);
	SunLight->SetMobility(EComponentMobility::Movable);
	SunLight->SetAtmosphereSunLight(true);
	SunLight->SetAtmosphereSunLightIndex(0);
	Exposure = CreateDefaultSubobject<UPostProcessComponent>(TEXT("稳定曝光"));
	Exposure->SetupAttachment(SceneRoot);
	Exposure->bUnbound = true;
	Exposure->Priority = -10.0f;
	Exposure->BlendWeight = 1.0f;
}

void ALxSunActor::PostLoad()
{
	Super::PostLoad();
#if WITH_EDITORONLY_DATA
	bIsSpatiallyLoaded = false;
#endif
}

#if WITH_EDITOR
void ALxSunActor::PostEditChangeProperty(FPropertyChangedEvent& Event)
{
	Super::PostEditChangeProperty(Event);
	if (!IsTemplate()) RefreshSun();
}

void ALxSunActor::PostEditUndo()
{
	Super::PostEditUndo();
	if (!IsTemplate()) RefreshSun();
}

void ALxSunActor::GetStreamingBounds(FBox& OutRuntimeBounds, FBox& OutEditorBounds) const
{
	OutRuntimeBounds.Init();
	OutEditorBounds.Init();
}
#endif

void ALxSunActor::RefreshExposure()
{
	if (!Exposure) return;
	Exposure->bEnabled = bStabilizeExposure;
	if (!bStabilizeExposure) return;
	FPostProcessSettings& Settings = Exposure->Settings;
	const float BaseEV = FMath::IsFinite(ExposureEV100) ? FMath::Clamp(ExposureEV100, -10.0f, 20.0f) : 11.0f;
	const float Range = FMath::IsFinite(ExposureAdaptationRange) ? FMath::Clamp(ExposureAdaptationRange, 0.0f, 3.0f) : 0.0f;
	const IConsoleVariable* ExtendedRange = IConsoleManager::Get().FindConsoleVariable(TEXT("r.DefaultFeature.AutoExposure.ExtendDefaultLuminanceRange"));
	const bool bUseEV100 = ExtendedRange && ExtendedRange->GetInt() != 0;
	const float MinEV = FMath::Clamp(BaseEV - Range, -10.0f, 20.0f);
	const float MaxEV = FMath::Clamp(BaseEV + Range, -10.0f, 20.0f);
	Settings.bOverride_AutoExposureMethod = true;
	Settings.AutoExposureMethod = EAutoExposureMethod::AEM_Histogram;
	Settings.bOverride_AutoExposureMinBrightness = true;
	Settings.bOverride_AutoExposureMaxBrightness = true;
	// 与引擎眼部适应的饱和亮度换算一致，兼容关闭扩展范围的项目。
	const IConsoleVariable* LensAttenuation = IConsoleManager::Get().FindConsoleVariable(TEXT("r.EyeAdaptation.LensAttenuation"));
	const float LuminanceMax = 0.78f / FMath::Max(LensAttenuation ? LensAttenuation->GetFloat() : 0.78f, 0.01f);
	Settings.AutoExposureMinBrightness = bUseEV100 ? MinEV : LuminanceMax * FMath::Pow(2.0f, MinEV);
	Settings.AutoExposureMaxBrightness = bUseEV100 ? MaxEV : LuminanceMax * FMath::Pow(2.0f, MaxEV);
	Settings.bOverride_AutoExposureBias = true;
	Settings.AutoExposureBias = 0.0f;
	Settings.bOverride_AutoExposureBiasCurve = true;
	Settings.AutoExposureBiasCurve = nullptr;
	Settings.bOverride_AutoExposureSpeedUp = true;
	Settings.bOverride_AutoExposureSpeedDown = true;
	Settings.AutoExposureSpeedUp = 1.0f;
	Settings.AutoExposureSpeedDown = 1.0f;
	Settings.bOverride_LocalExposureShadowContrastScale = true;
	Settings.LocalExposureShadowContrastScale = FMath::IsFinite(ShadowContrast) ? FMath::Clamp(ShadowContrast, 0.0f, 1.0f) : 1.0f;
}

void ALxSunActor::RefreshSkyLight()
{
	UWorld* World = GetWorld();
	if (!bSynchronizeSkyLight || !World || IsTemplate()) return;
	// 实时天空捕获需要天空大气；没有时不擅自添加环境或改变手工布光。
	TActorIterator<ASkyAtmosphere> Atmosphere(World);
	if (!Atmosphere) return;
	ASkyLight* Target = SkyLightActor;
	if (!Target)
	{
		for (TActorIterator<ASkyLight> It(World); It; ++It)
		{
			if (Target) return;
			Target = *It;
		}
	}
	if (!IsValid(Target) || Target->GetWorld() != World) return;
	USkyLightComponent* Light = Target->GetLightComponent();
	if (!Light || Light->SourceType != SLS_CapturedScene) return;
	Light->SetMobility(EComponentMobility::Movable);
	Light->SetRealTimeCaptureEnabled(true);
}

void ALxSunActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
#if WITH_EDITORONLY_DATA
	bIsSpatiallyLoaded = false;
#endif
	RefreshSun();
}

void ALxSunActor::BeginPlay()
{
	Super::BeginPlay();
	RefreshSun();
}

void ALxSunActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ULxWorldInfoManager* Manager = WorldInfoManager.Get())
	{
		Manager->OnTimeChanged.RemoveDynamic(this, &ThisClass::HandleTimeChanged);
		Manager->ClearTestTimeSpeedMultiplier(this);
	}
	WorldInfoManager.Reset();
	Super::EndPlay(EndPlayReason);
}

void ALxSunActor::SetTestTimeSpeedMultiplier(double InMultiplier)
{
	TestTimeSpeedMultiplier = FMath::IsFinite(InMultiplier) ? FMath::Max(0.0, InMultiplier) : 1.0;
	if (!IsTemplate()) RefreshSun();
}

void ALxSunActor::RefreshSun()
{
	RefreshExposure();
	RefreshSkyLight();
	ULxGameInstanceSubsystem* Global = GetWorld() && GetWorld()->IsGameWorld()
		? ULxGameInstanceSubsystem::GetInstance(GetWorld()) : nullptr;
	ULxWorldInfoManager* Manager = Global ? Global->GetWorldInfoManager() : nullptr;
	if (WorldInfoManager.Get() != Manager)
	{
		if (ULxWorldInfoManager* Previous = WorldInfoManager.Get())
		{
			Previous->OnTimeChanged.RemoveDynamic(this, &ThisClass::HandleTimeChanged);
			Previous->ClearTestTimeSpeedMultiplier(this);
		}
		WorldInfoManager = Manager;
	}
	if (Manager)
	{
		Manager->OnTimeChanged.AddUniqueDynamic(this, &ThisClass::HandleTimeChanged);
		Manager->SetTestTimeSpeedMultiplier(this, TestTimeSpeedMultiplier);
		ApplyTimeOfDay(Manager->GetTimeOfDayHours());
	}
	else ApplyTimeOfDay(PreviewHour);
}

void ALxSunActor::HandleTimeChanged(double TotalGameSeconds)
{
	ApplyTimeOfDay(FMath::Fmod(TotalGameSeconds, 86400.0) / 3600.0);
}

void ALxSunActor::ApplyTimeOfDay(double InHours)
{
	if (!SunLight || !FMath::IsFinite(InHours)) return;
	const double Hours = FMath::Fmod(FMath::Fmod(InHours, 24.0) + 24.0, 24.0);
	const double Phase = (Hours - 6.0) * PI / 12.0;
	const float Height = static_cast<float>(FMath::Sin(Phase));
	const float Daylight = FMath::Clamp(Height, 0.0f, 1.0f);
	const float Elevation = Height * FMath::Clamp(NoonElevation, 1.0f, 90.0f);
	const double Azimuth = SunriseAzimuth + (Hours - 6.0) * 15.0;
	// 光线从太阳射向地面，因此光源前向量与太阳所在方向相反。
	SunLight->SetWorldRotation(FRotator(-Elevation, Azimuth + 180.0, 0.0));
	const float CurveFactor = IntensityCurve ? IntensityCurve->GetFloatValue(static_cast<float>(Hours)) : Daylight;
	const float IntensityFactor = Daylight > KINDA_SMALL_NUMBER && FMath::IsFinite(CurveFactor)
		? FMath::Clamp(CurveFactor, 0.0f, 1.0f) : 0.0f;
	SunLight->SetIntensity(FMath::Max(0.0f, NoonIntensity) * IntensityFactor);
	const FLinearColor Color = ColorCurve ? ColorCurve->GetLinearColorValue(static_cast<float>(Hours))
		: FMath::Lerp(HorizonColor, DayColor, FMath::Clamp(Daylight * 2.0f, 0.0f, 1.0f));
	SunLight->SetLightColor(Color * LightTint.GetClamped());
	SunLight->SetUseTemperature(bUseTemperature);
	SunLight->SetTemperature(FMath::IsFinite(Temperature) ? FMath::Clamp(Temperature, 1700.0f, 12000.0f) : 6500.0f);
	// 编辑器构造预览只更新组件，不执行运行蓝图事件。
	if (HasActorBegunPlay()) OnSunUpdated(Hours, Daylight);
}
