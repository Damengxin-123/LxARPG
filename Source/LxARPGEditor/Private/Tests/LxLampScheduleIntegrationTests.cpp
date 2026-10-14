#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Engine/Blueprint.h"
#include "UObject/StrongObjectPtr.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "ProceduralMeshComponent.h"
#include "LxARPG/LxSource/World/MagicLamp/LxLampScheduleComponent.h"
#include "LxARPG/LxSource/World/MagicLamp/LxMagicLampActor.h"
#include "LxARPG/LxSource/World/MagicLamp/LxMagicStreetLampActor.h"
#include "LxARPG/LxSource/Systems/LxGameInstanceSubsystem.h"
#include "LxARPG/LxSource/Systems/SettingSystem/LxGameSettings.h"
#include "LxARPG/LxSource/Systems/SaveSystem/LxSaveManager.h"
#include "LxARPG/LxSource/Systems/SaveSystem/LxGameSaveData.h"
#include "LxARPG/LxSource/Systems/WorldInfo/LxWorldInfoManager.h"
#include "Components/LocalLightComponent.h"

/** 在实际游戏实例中验证七类蓝图的自动绑定、灯芯同步、读档响应与销毁解绑。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxLampScheduleIntegrationTest,"LxARPG.World.MagicLamp.ScheduleIntegration",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxLampScheduleIntegrationTest::RunTest(const FString& Parameters)
{
 ULxGameSettings* Settings=GetMutableDefault<ULxGameSettings>();
 const FString PreviousSlot=Settings->SaveSlotName;
 const FString Slot=TEXT("LxLampSchedule_")+FGuid::NewGuid().ToString(EGuidFormats::Digits);
 Settings->SaveSlotName=Slot;
 ON_SCOPE_EXIT { Settings->SaveSlotName=PreviousSlot; UGameplayStatics::DeleteGameInSlot(Slot,Settings->SaveUserIndex); };
 TStrongObjectPtr<UGameInstance> Instance(NewObject<UGameInstance>(GEngine));
 Instance->InitializeStandalone();
 UWorld* World=Instance->GetWorld();
 ULxGameInstanceSubsystem* Global=Instance->GetSubsystem<ULxGameInstanceSubsystem>();
 ON_SCOPE_EXIT
 {
  if(Global && Global->GetSaveManager()) Global->GetSaveManager()->SetReadOnly(true);
  World->SetBegunPlay(false);
  Instance->Shutdown(); World->DestroyWorld(false); GEngine->DestroyWorldContext(World);
 };
 if(!TestNotNull(TEXT("实际全局子系统"),Global)) return false;
 ULxWorldInfoManager* Time=Global->GetWorldInfoManager();
 World->InitializeActorsForPlay(FURL());
 World->SetBegunPlay(true);
 Time->SetTimeScale(0);
 const TCHAR* Presets[]={
  TEXT("/Game/项目内容/美术资产/海港城市/魔灯/预设/塔顶巨晶灯"),
  TEXT("/Game/项目内容/美术资产/海港城市/魔灯/预设/方晶壁灯"),
  TEXT("/Game/项目内容/美术资产/海港城市/魔灯/预设/方晶顶灯"),
  TEXT("/Game/项目内容/美术资产/海港城市/魔灯/预设/花冠球灯"),
  TEXT("/Game/项目内容/美术资产/海港城市/魔灯/预设/星环球灯"),
  TEXT("/Game/项目内容/美术资产/海港城市/路灯/预设/单侧悬灯"),
  TEXT("/Game/项目内容/美术资产/海港城市/路灯/预设/双侧悬灯")};
 for(const TCHAR* Path:Presets)
 {
  UBlueprint* Blueprint=LoadObject<UBlueprint>(nullptr,Path);
  if(!TestNotNull(Path,Blueprint)) return false;
  Time->SetTimeOfDayHours(12);
  AActor* Actor=World->SpawnActor<AActor>(Blueprint->GeneratedClass);
  if(!TestNotNull(TEXT("实际灯具实例"),Actor)) return false;
  ULxLampScheduleComponent* Schedule=Actor->FindComponentByClass<ULxLampScheduleComponent>();
  if(!TestNotNull(TEXT("旧预设继承自动照明组件"),Schedule)) return false;
  TestTrue(TEXT("自动执行BeginPlay绑定"),Schedule->HasBegunPlay());
  TestEqual(TEXT("白天初始熄灭"),Schedule->GetBrightnessAlpha(),0.f);
  UProceduralMeshComponent* Mesh=Actor->FindComponentByClass<UProceduralMeshComponent>();
  const bool bStreet=Actor->IsA<ALxMagicStreetLampActor>();
  UMaterialInstanceDynamic* Crystal=Cast<UMaterialInstanceDynamic>(Mesh->GetMaterial(bStreet?3:2));
  if(!TestNotNull(TEXT("实际晶核动态材质"),Crystal)) return false;
  TestEqual(TEXT("白天晶核不发光"),Crystal->K2_GetScalarParameterValue(TEXT("发光强度")),0.f);
  Time->SetTimeOfDayHours(18);
  Schedule->TickComponent(1,LEVELTICK_All,nullptr);
  TestEqual(TEXT("实际蓝图在18点渐亮"),Schedule->GetBrightnessAlpha(),.5f);
  const float HalfGlow=Crystal->K2_GetScalarParameterValue(TEXT("发光强度"));
  TestTrue(TEXT("灯芯同步渐亮"),HalfGlow>0);
  Schedule->TickComponent(1,LEVELTICK_All,nullptr);
  TestTrue(TEXT("灯芯线性光强同步"),FMath::IsNearlyEqual(Crystal->K2_GetScalarParameterValue(TEXT("发光强度")),HalfGlow*2,.001f));
  TInlineComponentArray<ULocalLightComponent*> Lights(Actor);
  int32 VisibleLights=0;
  for(ULocalLightComponent* Light:Lights) if(Light->IsVisible()) ++VisibleLights;
  const ALxMagicStreetLampActor* Street=Cast<ALxMagicStreetLampActor>(Actor);
  TestEqual(TEXT("路灯仅开启当前布局对应的光源"),VisibleLights,Street && Street->Layout==ELxStreetLampLayout::Double?2:1);
  // 使用真实存档恢复通知跳到早晨，验证已放置灯具自动渐灭。
  TStrongObjectPtr<ULxGameSaveData> Data(NewObject<ULxGameSaveData>());
  Data->WorldTime.TotalGameSeconds=6*3600;
  Data->WorldTime.TimeScale=0;
  TStrongObjectPtr<ULxSaveManager> Session(NewObject<ULxSaveManager>(Global));
  Session->InitializeSession(Data.Get(),FLxPersistSaveSession::CreateLambda([](const ULxGameSaveData*){ return true; }));
  Global->SetSessionSaveManager(Session.Get());
  Schedule->TickComponent(1,LEVELTICK_All,nullptr);
  TestEqual(TEXT("读档调时触发渐灭"),Schedule->GetBrightnessAlpha(),.5f);
  Schedule->TickComponent(1,LEVELTICK_All,nullptr);
  TestEqual(TEXT("晨间灯芯彻底熄灭"),Crystal->K2_GetScalarParameterValue(TEXT("发光强度")),0.f);
  for(ULocalLightComponent* Light:Lights) TestFalse(TEXT("晨间真实光源关闭"),Light->IsVisible());
  Time->SetTimeOfDayHours(18);
  Schedule->TickComponent(2,LEVELTICK_All,nullptr);
  Schedule->EndPlay(EEndPlayReason::RemovedFromWorld);
  Schedule->BeginPlay();
  TestTrue(TEXT("关卡重入后恢复渐变更新"),Schedule->IsComponentTickEnabled());
  Schedule->TickComponent(2,LEVELTICK_All,nullptr);
  TestEqual(TEXT("关卡重入后正确恢复夜间照明"),Schedule->GetBrightnessAlpha(),1.f);
  Actor->Destroy();
  TestFalse(TEXT("销毁后停止渐变更新"),Schedule->IsComponentTickEnabled());
  Time->SetTimeOfDayHours(18);
  TestFalse(TEXT("销毁后不再响应世界时间"),Schedule->IsComponentTickEnabled());
 }
 return true;
}
#endif
