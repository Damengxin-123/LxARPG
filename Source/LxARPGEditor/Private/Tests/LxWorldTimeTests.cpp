#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/Engine.h"
#include "Engine/Blueprint.h"
#include <limits>
#include "LxARPG/LxSource/World/MagicLamp/LxMagicLampActor.h"
#include "LxARPG/LxSource/World/MagicLamp/LxLampScheduleComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/SkyLight.h"
#include "Components/SkyAtmosphereComponent.h"
#include "HAL/IConsoleManager.h"
#include "Curves/CurveFloat.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"
#include "LxARPG/LxSource/Systems/SaveSystem/LxGameSaveData.h"
#include "LxARPG/LxSource/Systems/SaveSystem/LxSaveManager.h"
#include "LxARPG/LxSource/Systems/WorldInfo/LxWorldInfoManager.h"
#include "LxARPG/LxSource/World/Sun/LxSunActor.h"
#include "LxARPG/LxSource/Systems/LxGameInstanceSubsystem.h"
#include "LxARPG/LxSource/Systems/SettingSystem/LxGameSettings.h"

/** 验证迟到的读档通知、只变时间时落盘、读盘精度和重复加载不会倒退运行时间。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxWorldTimePersistenceTest, "LxARPG.World.Time.Persistence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxWorldTimePersistenceTest::RunTest(const FString& Parameters)
{
	const FString Slot = TEXT("LxTimeTest_") + FGuid::NewGuid().ToString(EGuidFormats::Digits);
	ON_SCOPE_EXIT { UGameplayStatics::DeleteGameInSlot(Slot, 0); };
	TStrongObjectPtr<ULxSaveManager> Saves(NewObject<ULxSaveManager>());
	TStrongObjectPtr<ULxWorldInfoManager> Info(NewObject<ULxWorldInfoManager>());
	Saves->Initialize(Slot, 0);
	Info->Initialize(Saves.Get());
	ON_SCOPE_EXIT { Info->Deinitialize(); };
	TestFalse(TEXT("加载前不允许修改时间"), Info->SetTotalGameSeconds(1.0));
	if (!TestTrue(TEXT("空存档初始化"), Saves->LoadSave())) return false;
	TestEqual(TEXT("默认第一天八点"), Info->GetTimeOfDayHours(), 8.0);
	TestTrue(TEXT("设置含跨天和小数的时间"), Info->SetTotalGameSeconds(3.0 * 86400.0 + 12345.125));
	TestTrue(TEXT("设置流速"), Info->SetTimeScale(30.0));
	TestTrue(TEXT("没有角色和组件也能保存时间"), Saves->SaveAll());
	TestTrue(TEXT("时间单独变化创建磁盘文件"), UGameplayStatics::DoesSaveGameExist(Slot, 0));
	TStrongObjectPtr<ULxSaveManager> Reloaded(NewObject<ULxSaveManager>());
	Reloaded->Initialize(Slot, 0);
	if (!TestTrue(TEXT("读取实际磁盘存档"), Reloaded->LoadSave())) return false;
	TestEqual(TEXT("累计秒数无损恢复"), Reloaded->GetSaveData()->WorldTime.TotalGameSeconds, 3.0 * 86400.0 + 12345.125);
	TestEqual(TEXT("流速随地图保存"), Reloaded->GetSaveData()->WorldTime.TimeScale, 30.0);
	Info->SetTotalGameSeconds(4.0 * 86400.0);
	Saves->LoadSave();
	TestEqual(TEXT("重复加载不会回滚运行时间"), Info->GetDayNumber(), int64(5));
	TestTrue(TEXT("关闭阶段缓存入口同样采集最新时间"), Saves->SaveCachedData());
	TestEqual(TEXT("保存前采集更新的时间"), Saves->GetSaveData()->WorldTime.TotalGameSeconds, 4.0 * 86400.0);
	TestFalse(TEXT("拒绝负时间"), Info->SetTotalGameSeconds(-1.0));
	TestFalse(TEXT("拒绝越界小时"), Info->SetTimeOfDayHours(24.0));
	TestFalse(TEXT("拒绝负流速"), Info->SetTimeScale(-1.0));
	return true;
}

/** 验证真实世界的计时条件、跨天、菜单只读和切档恢复，并隔离编辑器世界。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxWorldTimeLifecycleTest, "LxARPG.World.Time.Lifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxWorldTimeLifecycleTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	TStrongObjectPtr<UGameInstance> Instance(NewObject<UGameInstance>(GEngine));
	FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
	Context.SetCurrentWorld(World);
	Context.OwningGameInstance = Instance.Get();
	World->SetGameInstance(Instance.Get());
	Instance->OnWorldChanged(nullptr, World);
	TStrongObjectPtr<ULxSaveManager> Saves(NewObject<ULxSaveManager>());
	TStrongObjectPtr<ULxGameSaveData> Data(NewObject<ULxGameSaveData>());
	TStrongObjectPtr<ULxWorldInfoManager> Info(NewObject<ULxWorldInfoManager>(Instance.Get()));
	ON_SCOPE_EXIT
	{
		Info->Deinitialize();
		World->SetBegunPlay(false);
		Instance->OnWorldChanged(World, nullptr);
		World->DestroyWorld(false);
		GEngine->DestroyWorldContext(World);
	};
	int32 Writes = 0;
	Saves->InitializeSession(Data.Get(), FLxPersistSaveSession::CreateLambda(
		[&Writes](const ULxGameSaveData*) { ++Writes; return true; }));
	Info->Initialize(Saves.Get());
	TestFalse(TEXT("开始游戏之前不计时"), Info->IsTickable());
	World->SetBegunPlay(true);
	TestTrue(TEXT("正式世界开始计时"), Info->IsTickable());
	Info->SetTotalGameSeconds(86370.0);
	Info->Tick(1.0f);
	TestEqual(TEXT("一分钟自然推进跨天"), Info->GetDayNumber(), int64(2));
	TestEqual(TEXT("跨天保留三十秒"), Info->GetWorldTime().TotalGameSeconds, 86430.0);
	Info->SetTimePaused(true);
	Info->Tick(1.0f);
	TestEqual(TEXT("历法暂停不推进"), Info->GetWorldTime().TotalGameSeconds, 86430.0);
	Info->SetTimePaused(false);
	Info->SetTimeScale(0.0);
	Info->Tick(1.0f);
	TestEqual(TEXT("零流速不推进"), Info->GetWorldTime().TotalGameSeconds, 86430.0);
	Info->SetTimeScale(60.0);
	Saves->SetReadOnly(true);
	Info->Tick(1.0f);
	TestEqual(TEXT("菜单世界不推进"), Info->GetWorldTime().TotalGameSeconds, 86430.0);
	TestFalse(TEXT("菜单不能手动改时"), Info->SetTimeOfDayHours(12.0));
	TestFalse(TEXT("菜单不能保存"), Saves->SaveAll());
	Data->WorldTime.TotalGameSeconds = 6.0 * 86400.0 + 18.0 * 3600.0;
	Data->WorldTime.TimeScale = 10.0;
	TestTrue(TEXT("原地替换地图档"), Saves->ReplaceSession(Data.Get(), true));
	TestEqual(TEXT("切档立即恢复时间"), Info->GetTimeOfDayHours(), 18.0);
	TestEqual(TEXT("切档立即恢复天数"), Info->GetDayNumber(), int64(7));
	TestEqual(TEXT("切档立即恢复流速"), Info->GetWorldTime().TimeScale, 10.0);
	TestEqual(TEXT("只读切档没有写盘"), Writes, 0);
	Saves->SetReadOnly(false);
	Info->Tick(1.0f);
	TestEqual(TEXT("切档后按新流速推进"), Info->GetWorldTime().TotalGameSeconds, Data->WorldTime.TotalGameSeconds + 10.0);
	World->SetBegunPlay(false);
	TestFalse(TEXT("世界停止后不计时"), Info->IsTickable());
	return true;
}

/** 验证太阳真实组件的朝向、日间照度、夜间归零及曲线覆盖。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxSunLightingTest, "LxARPG.World.Time.SunLighting",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxSunLightingTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Editor, false);
	ON_SCOPE_EXIT { World->DestroyWorld(false); };
	ALxSunActor* Sun = World->SpawnActor<ALxSunActor>();
	if (!TestNotNull(TEXT("创建太阳"), Sun)) return false;
	TestTrue(TEXT("太阳为可移动光源"), Sun->SunLight->Mobility == EComponentMobility::Movable);
	TestTrue(TEXT("启用天空大气太阳"), Sun->SunLight->bAtmosphereSunLight);
	TestTrue(TEXT("构造时已经应用八点预览"), Sun->SunLight->Intensity > 0.0f);
	Sun->ApplyTimeOfDay(12.0);
	TestEqual(TEXT("正午达到配置照度"), Sun->SunLight->Intensity, Sun->NoonIntensity);
	TestTrue(TEXT("正午光线照向地面"), Sun->SunLight->GetForwardVector().Z < -0.5);
	Sun->ApplyTimeOfDay(6.0);
	TestEqual(TEXT("日出地平线处照度为零"), Sun->SunLight->Intensity, 0.0f);
	TestTrue(TEXT("日出从正X方向照入"), Sun->SunLight->GetForwardVector().X < -0.99);
	Sun->ApplyTimeOfDay(18.0);
	TestTrue(TEXT("日落从负X方向照入"), Sun->SunLight->GetForwardVector().X > 0.99);
	Sun->ApplyTimeOfDay(0.0);
	TestEqual(TEXT("午夜太阳不照亮场景"), Sun->SunLight->Intensity, 0.0f);
	Sun->ApplyTimeOfDay(36.0);
	TestEqual(TEXT("跨日小时正确归一化"), Sun->SunLight->Intensity, Sun->NoonIntensity);
	Sun->IntensityCurve = NewObject<UCurveFloat>(Sun);
	Sun->IntensityCurve->FloatCurve.AddKey(0.0f, 0.25f);
	Sun->IntensityCurve->FloatCurve.AddKey(24.0f, 0.25f);
	Sun->ApplyTimeOfDay(12.0);
	TestEqual(TEXT("日间采用美术曲线"), Sun->SunLight->Intensity, Sun->NoonIntensity * 0.25f);
	Sun->ApplyTimeOfDay(0.0);
	TestEqual(TEXT("曲线不能让夜间太阳穿透地面"), Sun->SunLight->Intensity, 0.0f);
	Sun->IntensityCurve->FloatCurve.Reset();
	Sun->IntensityCurve->FloatCurve.AddKey(12.0f, 2.0f);
	Sun->ApplyTimeOfDay(12.0);
	TestEqual(TEXT("曲线不能超过正午最大亮度"), Sun->SunLight->Intensity, Sun->NoonIntensity);
	Sun->IntensityCurve = nullptr;
	Sun->PreviewHour = 12.0;
	Sun->NoonIntensity = 72000.0f;
	Sun->LightTint = FLinearColor(0.3f, 0.6f, 1.0f);
	Sun->bUseTemperature = true;
	Sun->Temperature = 4800.0f;
	Sun->PreEditChange(nullptr);
	FPropertyChangedEvent LightEdit(FindFProperty<FProperty>(ALxSunActor::StaticClass(), GET_MEMBER_NAME_CHECKED(ALxSunActor, LightTint)), EPropertyChangeType::Interactive);
	Sun->PostEditChangeProperty(LightEdit);
	TestEqual(TEXT("场景调节实时更新正午亮度"), Sun->SunLight->Intensity, 72000.0f);
	TestTrue(TEXT("颜色叠加在自动昼夜光色上"), Sun->SunLight->GetLightColor().Equals(Sun->DayColor * Sun->LightTint, 0.01f));
	TestTrue(TEXT("太阳色温独立启用"), bool(Sun->SunLight->bUseTemperature));
	TestEqual(TEXT("太阳色温同步"), Sun->SunLight->Temperature, 4800.0f);
	Sun->ApplyTimeOfDay(8.0);
	TestEqual(TEXT("调整最大值后早晨仍按昼夜比例变化"), Sun->SunLight->Intensity, 36000.0f);
	Sun->ApplyTimeOfDay(0.0);
	TestEqual(TEXT("染色和色温不会点亮午夜太阳"), Sun->SunLight->Intensity, 0.0f);
	ALxSunActor* OtherSun = World->SpawnActor<ALxSunActor>();
	TestEqual(TEXT("场景实例调亮不会影响另一太阳默认值"), OtherSun->NoonIntensity, 50000.0f);
	return true;
}
/** 验证真实游戏实例初始化、太阳创建自动读时、自然推进和更换会话的事件链。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxWorldTimeSunIntegrationTest, "LxARPG.World.Time.SunIntegration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxWorldTimeSunIntegrationTest::RunTest(const FString& Parameters)
{
	ULxGameSettings* Settings = GetMutableDefault<ULxGameSettings>();
	const FString PreviousSlot = Settings->SaveSlotName;
	const FString Slot = TEXT("LxTimeIntegration_") + FGuid::NewGuid().ToString(EGuidFormats::Digits);
	Settings->SaveSlotName = Slot;
	ON_SCOPE_EXIT
	{
		Settings->SaveSlotName = PreviousSlot;
		UGameplayStatics::DeleteGameInSlot(Slot, Settings->SaveUserIndex);
	};
	TStrongObjectPtr<UGameInstance> Instance(NewObject<UGameInstance>(GEngine));
	Instance->InitializeStandalone();
	UWorld* World = Instance->GetWorld();
	ULxGameInstanceSubsystem* Global = Instance->GetSubsystem<ULxGameInstanceSubsystem>();
	ON_SCOPE_EXIT
	{
		if (Global && Global->GetSaveManager()) Global->GetSaveManager()->SetReadOnly(true);
		World->SetBegunPlay(false);
		Instance->Shutdown();
		World->DestroyWorld(false);
		GEngine->DestroyWorldContext(World);
	};
	if (!TestNotNull(TEXT("游戏实例自动创建全局子系统"), Global)) return false;
	ULxWorldInfoManager* Info = Global->GetWorldInfoManager();
	if (!TestNotNull(TEXT("子系统自动创建世界信息管理器"), Info)) return false;
	// 初始化世界中的演员后，引擎才会分发动态委托的反射函数调用。
	World->InitializeActorsForPlay(FURL());
	TestTrue(TEXT("运行世界允许设置时间"), Info->SetTimeOfDayHours(0.0));
	ALxSunActor* Sun = World->SpawnActor<ALxSunActor>();
	if (!TestNotNull(TEXT("在运行世界创建太阳"), Sun)) return false;
	TestEqual(TEXT("创建时读取午夜而非编辑器八点预览"), Sun->SunLight->Intensity, 0.0f);
	Info->SetTimeOfDayHours(12.0);
	TestEqual(TEXT("全局调时立即更新已存在的太阳"), Sun->SunLight->Intensity, Sun->NoonIntensity);
	World->SetBegunPlay(true);
	Info->SetTimeOfDayHours(6.0);
	Info->Tick(60.0f);
	TestTrue(TEXT("自然推进驱动太阳升起"), Sun->SunLight->Intensity > 0.0f);
	TStrongObjectPtr<ULxGameSaveData> Data(NewObject<ULxGameSaveData>());
	Data->WorldTime.TotalGameSeconds = 23.0 * 3600.0;
	TStrongObjectPtr<ULxSaveManager> Session(NewObject<ULxSaveManager>(Global));
	Session->InitializeSession(Data.Get(), FLxPersistSaveSession::CreateLambda([](const ULxGameSaveData*) { return true; }));
	Session->SetReadOnly(true);
	Global->SetSessionSaveManager(Session.Get());
	TestTrue(TEXT("更换存档管理器保留唯一世界信息实例"), Info == Global->GetWorldInfoManager());
	TestEqual(TEXT("更换会话立即恢复二十三点"), Info->GetTimeOfDayHours(), 23.0);
	TestEqual(TEXT("已存在的太阳收到切档通知"), Sun->SunLight->Intensity, 0.0f);
	Data->WorldTime.TotalGameSeconds = 12.0 * 3600.0;
	Session->ReplaceSession(Data.Get(), true);
	TestEqual(TEXT("同一管理器切地图也通知太阳"), Sun->SunLight->Intensity, Sun->NoonIntensity);
	return true;
}

/** 验证太阳测试倍率、灯具联动、切档基础流速、暂停限制和来源卸载恢复。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxSunTimeSpeedTest, "LxARPG.World.Time.SunTimeSpeed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxSunTimeSpeedTest::RunTest(const FString& Parameters)
{
	ULxGameSettings* Settings = GetMutableDefault<ULxGameSettings>();
	const FString PreviousSlot = Settings->SaveSlotName;
	const FString Slot = TEXT("LxSunSpeed_") + FGuid::NewGuid().ToString(EGuidFormats::Digits);
	Settings->SaveSlotName = Slot;
	ON_SCOPE_EXIT
	{
		Settings->SaveSlotName = PreviousSlot;
		UGameplayStatics::DeleteGameInSlot(Slot, Settings->SaveUserIndex);
	};
	TStrongObjectPtr<UGameInstance> Instance(NewObject<UGameInstance>(GEngine));
	Instance->InitializeStandalone();
	UWorld* World = Instance->GetWorld();
	ULxGameInstanceSubsystem* Global = Instance->GetSubsystem<ULxGameInstanceSubsystem>();
	ON_SCOPE_EXIT
	{
		if (Global && Global->GetSaveManager()) Global->GetSaveManager()->SetReadOnly(true);
		World->SetBegunPlay(false);
		Instance->Shutdown();
		World->DestroyWorld(false);
		GEngine->DestroyWorldContext(World);
	};
	if (!TestNotNull(TEXT("游戏实例自动创建全局子系统"), Global)) return false;
	ULxWorldInfoManager* Info = Global->GetWorldInfoManager();
	if (!TestNotNull(TEXT("子系统自动创建世界信息管理器"), Info)) return false;
	World->InitializeActorsForPlay(FURL());
	World->SetBegunPlay(true);
	UBlueprint* Preset = LoadObject<UBlueprint>(nullptr, TEXT("/Game/项目内容/美术资产/照明/世界太阳"));
	if (!TestNotNull(TEXT("已有太阳蓝图"), Preset)) return false;
	ALxSunActor* Sun = World->SpawnActor<ALxSunActor>(Preset->GeneratedClass);
	if (!TestNotNull(TEXT("实际太阳实例"), Sun)) return false;
	TestEqual(TEXT("旧蓝图默认继承一倍流速"), Sun->TestTimeSpeedMultiplier, 1.0);
	Info->SetTotalGameSeconds(0.0);
	Info->SetTimeScale(60.0);
	Info->Tick(1.0f);
	TestEqual(TEXT("一倍保持基础流速"), Info->GetWorldTime().TotalGameSeconds, 60.0);
	Sun->SetTestTimeSpeedMultiplier(10.0);
	Sun->RefreshSun();
	Sun->RefreshSun();
	Info->Tick(1.0f);
	TestEqual(TEXT("十倍加速且重复刷新不叠乘"), Info->GetWorldTime().TotalGameSeconds, 660.0);
	Sun->SetTestTimeSpeedMultiplier(0.5);
	Info->Tick(1.0f);
	TestEqual(TEXT("半倍减速"), Info->GetWorldTime().TotalGameSeconds, 690.0);
	// 详情面板直接修改属性时也立即更新倍率，无需重启游戏。
	Sun->PreEditChange(nullptr);
	Sun->TestTimeSpeedMultiplier = 2.0;
	FPropertyChangedEvent SpeedEdit(FindFProperty<FProperty>(ALxSunActor::StaticClass(), GET_MEMBER_NAME_CHECKED(ALxSunActor, TestTimeSpeedMultiplier)), EPropertyChangeType::ValueSet);
	Sun->PostEditChangeProperty(SpeedEdit);
	Info->Tick(1.0f);
	TestEqual(TEXT("运行中详情面板修改立即生效"), Info->GetWorldTime().TotalGameSeconds, 810.0);
	Sun->SetTestTimeSpeedMultiplier(0.0);
	TestFalse(TEXT("零倍率暂停历法更新"), Info->IsTickable());
	Info->Tick(1.0f);
	TestEqual(TEXT("零倍率不推进"), Info->GetWorldTime().TotalGameSeconds, 810.0);
	Sun->SetTestTimeSpeedMultiplier(std::numeric_limits<double>::quiet_NaN());
	TestEqual(TEXT("无效倍率恢复默认"), Info->GetTestTimeSpeedMultiplier(), 1.0);
	Sun->SetTestTimeSpeedMultiplier(10.0);
	Info->SetTimePaused(true);
	Info->Tick(1.0f);
	TestEqual(TEXT("测试倍率尊重手动暂停"), Info->GetWorldTime().TotalGameSeconds, 810.0);
	Info->SetTimePaused(false);
	TestTrue(TEXT("加速期间可以保存"), Global->GetSaveManager()->SaveAll());
	TestEqual(TEXT("存档仍记录基础流速"), Global->GetSaveManager()->GetSaveData()->WorldTime.TimeScale, 60.0);

	Info->SetTimeOfDayHours(17.0);
	UBlueprint* LampPreset = LoadObject<UBlueprint>(nullptr, TEXT("/Game/项目内容/美术资产/海港城市/魔灯/预设/方晶壁灯"));
	if (!TestNotNull(TEXT("实际灯具蓝图"), LampPreset)) return false;
	ALxMagicLampActor* Lamp = World->SpawnActor<ALxMagicLampActor>(LampPreset->GeneratedClass);
	ULxLampScheduleComponent* Schedule = Lamp ? Lamp->FindComponentByClass<ULxLampScheduleComponent>() : nullptr;
	if (!TestNotNull(TEXT("灯具绑定同一时钟"), Schedule)) return false;
	TestEqual(TEXT("傍晚前灯具关闭"), Schedule->GetBrightnessAlpha(), 0.0f);
	Info->Tick(6.0f);
	TestEqual(TEXT("十倍流速六秒从十七点到十八点"), Info->GetTimeOfDayHours(), 18.0);
	TestEqual(TEXT("加速日落同步关闭太阳"), Sun->SunLight->Intensity, 0.0f);
	Schedule->TickComponent(1.0f, LEVELTICK_All, nullptr);
	TestEqual(TEXT("灯具到点开启且渐变时长不加速"), Schedule->GetBrightnessAlpha(), 0.5f);

	TStrongObjectPtr<ULxGameSaveData> Data(NewObject<ULxGameSaveData>());
	Data->WorldTime.TimeScale = 30.0;
	TStrongObjectPtr<ULxSaveManager> Session(NewObject<ULxSaveManager>(Global));
	Session->InitializeSession(Data.Get(), FLxPersistSaveSession::CreateLambda([](const ULxGameSaveData*) { return true; }));
	Global->SetSessionSaveManager(Session.Get());
	const double RestoredSeconds = Info->GetWorldTime().TotalGameSeconds;
	Info->Tick(1.0f);
	TestEqual(TEXT("切档后对新基础流速应用场景倍率"), Info->GetWorldTime().TotalGameSeconds, RestoredSeconds + 300.0);
	Session->SetReadOnly(true);
	Info->Tick(1.0f);
	TestEqual(TEXT("测试倍率不能推进只读菜单时钟"), Info->GetWorldTime().TotalGameSeconds, RestoredSeconds + 300.0);
	Session->SetReadOnly(false);
	ALxSunActor* OtherSun = World->SpawnActor<ALxSunActor>();
	OtherSun->SetTestTimeSpeedMultiplier(2.0);
	Sun->Destroy();
	TestEqual(TEXT("旧太阳销毁不清除新太阳的倍率"), Info->GetTestTimeSpeedMultiplier(), 2.0);
	OtherSun->Destroy();
	Info->Tick(1.0f);
	TestEqual(TEXT("太阳卸载后恢复基础流速"), Info->GetWorldTime().TotalGameSeconds, RestoredSeconds + 330.0);
	TestEqual(TEXT("太阳卸载后倍率恢复一"), Info->GetTestTimeSpeedMultiplier(), 1.0);
	return true;
}

/** 验证太阳曝光基线与场景亮暗无关，并保留关闭、有限适应及夜间变暗。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxSunExposureTest, "LxARPG.World.Time.SunExposure",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxSunExposureTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Editor, false);
	ON_SCOPE_EXIT { World->DestroyWorld(false); };
	ALxSunActor* Sun = World->SpawnActor<ALxSunActor>();
	if (!TestNotNull(TEXT("创建太阳"), Sun) || !TestNotNull(TEXT("创建稳定曝光组件"), Sun->Exposure.Get())) return false;
	TestFalse(TEXT("全局太阳不随距离流送"), Sun->GetIsSpatiallyLoaded());
	TestFalse(TEXT("不允许误设空间加载"), Sun->CanChangeIsSpatiallyLoadedFlag());
	TestTrue(TEXT("曝光影响整个场景"), Sun->Exposure->bUnbound && Sun->Exposure->bEnabled);
	TestTrue(TEXT("场景专用后处理可覆盖太阳基线"), Sun->Exposure->Priority < 0.0f);
	const FPostProcessSettings& Settings = Sun->Exposure->Settings;
	TestTrue(TEXT("明确覆盖最小和最大曝光"), Settings.bOverride_AutoExposureMinBrightness && Settings.bOverride_AutoExposureMaxBrightness);
	TestEqual(TEXT("默认固定曝光消除抽动"), Settings.AutoExposureMinBrightness, Settings.AutoExposureMaxBrightness);
	TestEqual(TEXT("不额外提亮暗部"), Settings.LocalExposureShadowContrastScale, 1.0f);
	const float DayExposure = Settings.AutoExposureMinBrightness;
	Sun->ApplyTimeOfDay(0.0);
	TestEqual(TEXT("夜间不自动抬升曝光"), Settings.AutoExposureMinBrightness, DayExposure);
	TestEqual(TEXT("保持真实夜间无太阳直射"), Sun->SunLight->Intensity, 0.0f);
	Sun->ExposureAdaptationRange = 0.5f;
	Sun->RefreshSun();
	TestTrue(TEXT("支持有限幅度的眼部适应"), Settings.AutoExposureMaxBrightness > Settings.AutoExposureMinBrightness);
	Sun->bStabilizeExposure = false;
	Sun->RefreshSun();
	TestFalse(TEXT("可交还场景曝光控制"), Sun->Exposure->bEnabled);
	Sun->bStabilizeExposure = true;
	Sun->ExposureAdaptationRange = 0.0f;
	Sun->RefreshSun();
	TestTrue(TEXT("重新启用恢复曝光"), Sun->Exposure->bEnabled);
	return true;
}

/** 验证天空捕获跟随昼夜，并尊重多天空光和自定义立方体贴图。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxSunSkyLightTest, "LxARPG.World.Time.SunSkyLight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxSunSkyLightTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Editor, false);
	ON_SCOPE_EXIT { World->DestroyWorld(false); };
	World->SpawnActor<ASkyAtmosphere>();
	ASkyLight* Sky = World->SpawnActor<ASkyLight>();
	Sky->GetLightComponent()->SetRealTimeCaptureEnabled(false);
	ALxSunActor* Sun = World->SpawnActor<ALxSunActor>();
	TestTrue(TEXT("唯一场景天空光自动跟随昼夜"), Sky->GetLightComponent()->bRealTimeCapture);
	TestTrue(TEXT("天空光允许动态更新"), Sky->GetLightComponent()->Mobility == EComponentMobility::Movable);
	Sky->GetLightComponent()->SetRealTimeCaptureEnabled(false);
	Sun->bSynchronizeSkyLight = false;
	Sun->RefreshSun();
	TestFalse(TEXT("关闭联动后尊重已有天空光"), Sky->GetLightComponent()->bRealTimeCapture);
	Sun->bSynchronizeSkyLight = true;
	Sky->GetLightComponent()->SourceType = SLS_SpecifiedCubemap;
	Sun->RefreshSun();
	TestFalse(TEXT("指定立方体贴图不被改成实时天空"), Sky->GetLightComponent()->bRealTimeCapture);
	Sky->GetLightComponent()->SourceType = SLS_CapturedScene;
	World->SpawnActor<ASkyLight>();
	Sun->RefreshSun();
	TestFalse(TEXT("存在多个天空光时不猜测目标"), Sky->GetLightComponent()->bRealTimeCapture);
	Sun->SkyLightActor = Sky;
	Sun->RefreshSun();
	TestTrue(TEXT("可明确指定需要联动的天空光"), Sky->GetLightComponent()->bRealTimeCapture);
	return true;
}

#endif
