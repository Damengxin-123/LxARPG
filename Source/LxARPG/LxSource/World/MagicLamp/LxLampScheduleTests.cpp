#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/World.h"
#include "UObject/StrongObjectPtr.h"
#include "Components/PointLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "LxLampScheduleComponent.h"
#include "LxMagicLampActor.h"
#include "LxMagicStreetLampActor.h"
#include "../../Systems/SaveSystem/LxSaveManager.h"
#include "../../Systems/SaveSystem/LxGameSaveData.h"
#include "../../Systems/WorldInfo/LxWorldInfoManager.h"

/** 验证跨夜边界、真实时间通知、连续渐变、双侧同步与逐帧更新按需关闭。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxLampScheduleTest,"LxARPG.World.MagicLamp.AutoSchedule",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxLampScheduleTest::RunTest(const FString& Parameters)
{
 TestTrue(TEXT("18点开始开启"),ULxLampScheduleComponent::IsWithinHours(18,18,6));
 TestTrue(TEXT("跨午夜仍开启"),ULxLampScheduleComponent::IsWithinHours(24,18,6));
 TestTrue(TEXT("六点前仍开启"),ULxLampScheduleComponent::IsWithinHours(5.999,18,6));
 TestFalse(TEXT("6点开始关闭"),ULxLampScheduleComponent::IsWithinHours(6,18,6));
 TestFalse(TEXT("白天关闭"),ULxLampScheduleComponent::IsWithinHours(12,18,6));
 TestTrue(TEXT("支持白天时段"),ULxLampScheduleComponent::IsWithinHours(12,9,17));
 TestFalse(TEXT("白天时段不包含结束点"),ULxLampScheduleComponent::IsWithinHours(17,9,17));
 TestTrue(TEXT("起止相同表示全天"),ULxLampScheduleComponent::IsWithinHours(12,6,6));
 UWorld* World=UWorld::CreateWorld(EWorldType::Editor,false);
 TStrongObjectPtr<ULxSaveManager> Saves(NewObject<ULxSaveManager>());
 TStrongObjectPtr<ULxGameSaveData> Data(NewObject<ULxGameSaveData>());
 TStrongObjectPtr<ULxWorldInfoManager> Time(NewObject<ULxWorldInfoManager>());
 Saves->InitializeSession(Data.Get(),FLxPersistSaveSession::CreateLambda([](const ULxGameSaveData*){ return true; }));
 Time->Initialize(Saves.Get());
 ON_SCOPE_EXIT { Time->Deinitialize(); World->DestroyWorld(false); };
 ALxBoxMagicLampActor* Lamp=World->SpawnActor<ALxBoxMagicLampActor>();
 ALxMagicStreetLampActor* Street=World->SpawnActor<ALxMagicStreetLampActor>();
 Street->Layout=ELxStreetLampLayout::Double; Street->OnConstruction(Street->GetActorTransform());
 ULxLampScheduleComponent* LampSchedule=Lamp->FindComponentByClass<ULxLampScheduleComponent>();
 ULxLampScheduleComponent* StreetSchedule=Street->FindComponentByClass<ULxLampScheduleComponent>();
 if(!TestNotNull(TEXT("魔灯调度组件"),LampSchedule) || !TestNotNull(TEXT("路灯调度组件"),StreetSchedule)) return false;
 const int32 LampBuilds=Lamp->GeometryBuildCount, StreetBuilds=Street->GeometryBuildCount;
 const float FullBrightness=Lamp->LampLight->ComputeLightBrightness();
 TestTrue(TEXT("自动启用默认开启"),Lamp->bAutoEnable && Street->bAutoEnable);
 TestTrue(TEXT("编辑器仍能正常布光"),Lamp->LampLight->IsVisible());
 LampSchedule->BindWorldTime(Time.Get()); StreetSchedule->BindWorldTime(Time.Get());
 TestFalse(TEXT("运行白天关闭魔灯"),Lamp->LampLight->IsVisible());
 TestFalse(TEXT("运行白天关闭路灯"),Street->RoadLight->IsVisible());
 TestFalse(TEXT("稳定状态无需逐帧更新"),LampSchedule->IsComponentTickEnabled());
 Time->SetTimeOfDayHours(18);
 TestEqual(TEXT("开灯边界不瞬间全亮"),LampSchedule->GetBrightnessAlpha(),0.f);
 TestTrue(TEXT("仅渐变期间开启逐帧更新"),LampSchedule->IsComponentTickEnabled());
 for(ULxLampScheduleComponent* Schedule:{LampSchedule,StreetSchedule}) Schedule->TickComponent(1,LEVELTICK_All,nullptr);
 TestEqual(TEXT("两秒渐变中途为半亮"),LampSchedule->GetBrightnessAlpha(),.5f);
 TestTrue(TEXT("EV按真实线性光强衰减"),FMath::IsNearlyEqual(Lamp->LampLight->ComputeLightBrightness(),FullBrightness*.5f,1.f));
 TestTrue(TEXT("路灯两侧同步渐亮"),Street->RoadLight->IsVisible() && Street->SidewalkLight->IsVisible());
 TestEqual(TEXT("作者设置EV不受渐变修改"),Lamp->BrightnessEV,Lamp->LampLight->Intensity+1.f);
 Time->SetTimeOfDayHours(20);
 for(ULxLampScheduleComponent* Schedule:{LampSchedule,StreetSchedule}) Schedule->TickComponent(1,LEVELTICK_All,nullptr);
 TestEqual(TEXT("时间通知不会反复重置过渡"),LampSchedule->GetBrightnessAlpha(),1.f);
 TestFalse(TEXT("亮度稳定后停止逐帧更新"),StreetSchedule->IsComponentTickEnabled());
 Time->SetTimeOfDayHours(6);
 TestEqual(TEXT("关灯边界不瞬间熄灭"),LampSchedule->GetBrightnessAlpha(),1.f);
 LampSchedule->TickComponent(1,LEVELTICK_All,nullptr);
 TestEqual(TEXT("关灯中途仍有半亮"),LampSchedule->GetBrightnessAlpha(),.5f);
 Time->SetTimeOfDayHours(18);
 TestEqual(TEXT("中途反转保持亮度连续"),LampSchedule->GetBrightnessAlpha(),.5f);
 LampSchedule->TickComponent(1,LEVELTICK_All,nullptr);
 TestEqual(TEXT("反转后从当前亮度平滑亮起"),LampSchedule->GetBrightnessAlpha(),.75f);
 Time->SetTimeOfDayHours(6);
 for(ULxLampScheduleComponent* Schedule:{LampSchedule,StreetSchedule}) Schedule->TickComponent(2,LEVELTICK_All,nullptr);
 TestFalse(TEXT("结束后光源彻底熄灭"),Lamp->LampLight->IsVisible());
 TestFalse(TEXT("两侧彻底熄灭"),Street->RoadLight->IsVisible() || Street->SidewalkLight->IsVisible());
 Lamp->bAutoEnable=false; Lamp->OnConstruction(Lamp->GetActorTransform());
 LampSchedule->TickComponent(2,LEVELTICK_All,nullptr);
 TestTrue(TEXT("关闭自动后白天可常亮"),Lamp->LampLight->IsVisible());
 TestFalse(TEXT("关闭一盏自动不影响其他灯"),Street->RoadLight->IsVisible());
 Lamp->bLightEnabled=false; Lamp->OnConstruction(Lamp->GetActorTransform());
 TestFalse(TEXT("原有照明总开关继续生效"),Lamp->LampLight->IsVisible());
 TestEqual(TEXT("渐变不重建魔灯"),Lamp->GeometryBuildCount,LampBuilds);
 TestEqual(TEXT("渐变不重建路灯"),Street->GeometryBuildCount,StreetBuilds);
 return true;
}
#endif
