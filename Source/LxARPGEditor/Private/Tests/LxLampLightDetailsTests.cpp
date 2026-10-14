#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/World.h"
#include "PropertyEditorModule.h"
#include "IPropertyRowGenerator.h"
#include "IDetailTreeNode.h"
#include "LxARPG/LxSource/World/MagicLamp/LxMagicLampActor.h"
#include "LxARPG/LxSource/World/MagicLamp/LxMagicStreetLampActor.h"
#include "Components/PointLightComponent.h"
#include "Components/SpotLightComponent.h"

/** 使用真实详情行生成器验证内部光源参数隐藏，灯具参数保留。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxLampLightDetailsTest,"LxARPG.World.MagicLamp.Details",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxLampLightDetailsTest::RunTest(const FString& Parameters)
{
 UWorld* World=UWorld::CreateWorld(EWorldType::Editor,false);
 ON_SCOPE_EXIT { World->DestroyWorld(false); };
 ALxBoxMagicLampActor* Lamp=World->SpawnActor<ALxBoxMagicLampActor>();
 ALxMagicStreetLampActor* Street=World->SpawnActor<ALxMagicStreetLampActor>();
 FPropertyEditorModule& Module=FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));
 for(UObject* Object:{static_cast<UObject*>(Lamp->LampLight.Get()),static_cast<UObject*>(Street->RoadLight.Get()),static_cast<UObject*>(Street->SidewalkLight.Get())})
 {
  TSharedRef<IPropertyRowGenerator> Rows=Module.CreatePropertyRowGenerator(FPropertyRowGeneratorArgs());
  Rows->SetObjects({Object});
  TestEqual(TEXT("内部光源不显示组件设置分类"),Rows->GetRootTreeNodes().Num(),0);
  for(const TSharedRef<IDetailTreeNode>& Node:Rows->GetRootTreeNodes())
  {
   AddInfo(FString::Printf(TEXT("内部光源剩余分类：%s"),*Node->GetNodeName().ToString()));
   TestFalse(TEXT("不显示内部光源照明参数"),Node->GetNodeName()==TEXT("Light") || Node->GetNodeName()==TEXT("Light Profiles"));
  }
 }
 for(UObject* Object:{static_cast<UObject*>(Lamp),static_cast<UObject*>(Street)})
 {
  TSharedRef<IPropertyRowGenerator> Rows=Module.CreatePropertyRowGenerator(FPropertyRowGeneratorArgs());
  Rows->SetObjects({Object});
  TestTrue(TEXT("灯具自身仍有详情参数"),Rows->GetRootTreeNodes().Num()>0);
 }
 return true;
}
#endif
