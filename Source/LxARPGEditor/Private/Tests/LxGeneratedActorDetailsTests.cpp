#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "PropertyEditorModule.h"
#include "IPropertyRowGenerator.h"
#include "IDetailTreeNode.h"
#include "PropertyHandle.h"
#include "../LxGeneratedActorDetails.h"
#include "LxARPG/LxSource/World/SplineRoad/LxSplineRoadActor.h"
#include "Components/SplineComponent.h"
#include "ProceduralMeshComponent.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "Misc/ScopeExit.h"
#include "LxARPG/LxSource/World/RoundBuilding/LxRoundBuildingActor.h"
#include "Tools/LegacyEdModeWidgetHelpers.h"

/** 验证自动化类型只显示预设参数，内部组件隐藏，普通演员不受影响。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxGeneratedActorDetailsTest,"LxARPG.World.GeneratedMesh.SimpleDetails",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxGeneratedActorDetailsTest::RunTest(const FString& Parameters)
{
 FPropertyEditorModule& Module=FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));
 UWorld* World=UWorld::CreateWorld(EWorldType::Editor,false);
 ON_SCOPE_EXIT { World->DestroyWorld(false); };
 for(FName Name : LxGeneratedActorDetails::Classes())
 {
  UClass* Class=FindObject<UClass>(nullptr,*(TEXT("/Script/LxARPG.")+Name.ToString()));
  if(!TestNotNull(TEXT("自动化类型存在"),Class)) { continue; }
  TSharedRef<IPropertyRowGenerator> Rows=Module.CreatePropertyRowGenerator(FPropertyRowGeneratorArgs());
  UObject* Object=Name==TEXT("LxSunActor") ? static_cast<UObject*>(World->SpawnActor<AActor>(Class)) : Class->GetDefaultObject();
  Rows->SetObjects({Object});
  TestTrue(FString::Printf(TEXT("%s保留功能分类"),*Name.ToString()),Rows->GetRootTreeNodes().Num()>0);
  for(const TSharedRef<IDetailTreeNode>& Node : Rows->GetRootTreeNodes())
  {
   const FName Category=Node->GetNodeName();
   TestFalse(FString::Printf(TEXT("%s隐藏引擎分类%s"),*Name.ToString(),*Category.ToString()),Category==TEXT("Replication")
    || Category==TEXT("Actor Tick") || Category==TEXT("Navigation") || Category==TEXT("HLOD") || Category==TEXT("WorldPartition"));
  }
 }
 const ALxSplineRoadActor* Road=GetDefault<ALxSplineRoadActor>();
 for(UObject* Component : {static_cast<UObject*>(Road->RoadSpline.Get()),static_cast<UObject*>(Road->RoadMesh.Get())})
 {
  TSharedRef<IPropertyRowGenerator> Rows=Module.CreatePropertyRowGenerator(FPropertyRowGeneratorArgs());
  Rows->SetObjects({Component});
  TestEqual(TEXT("内部样条和网格组件不暴露引擎参数"),Rows->GetRootTreeNodes().Num(),0);
 }
 USplineComponent* Ordinary=NewObject<USplineComponent>(GetTransientPackage());
 TSharedRef<IPropertyRowGenerator> Rows=Module.CreatePropertyRowGenerator(FPropertyRowGeneratorArgs());
 Rows->SetObjects({Ordinary});
 TestTrue(TEXT("普通样条组件保留原有详情"),Rows->GetRootTreeNodes().Num()>0);
 ALxSplineRoadActor* Instance=World->SpawnActor<ALxSplineRoadActor>();
 Rows->SetObjects({Instance});
 TSet<FName> Properties;
 /** 递归读取实际生成的详情行，确认功能参数与内部对象的可见性。 */
 TFunction<void(const TArray<TSharedRef<IDetailTreeNode>>&)> Collect;
 Collect=[&](const TArray<TSharedRef<IDetailTreeNode>>& Nodes)
 {
  for(const TSharedRef<IDetailTreeNode>& Node : Nodes)
  {
   if(TSharedPtr<IPropertyHandle> Handle=Node->CreatePropertyHandle())
   { if(Handle->GetProperty()) { Properties.Add(Handle->GetProperty()->GetFName()); } }
   TArray<TSharedRef<IDetailTreeNode>> Children; Node->GetChildren(Children); Collect(Children);
  }
 };
 Collect(Rows->GetRootTreeNodes());
 TestTrue(TEXT("道路宽度功能参数保留"),Properties.Contains(TEXT("RoadWidth")));
 TestTrue(TEXT("自动连接开关保留"),Properties.Contains(TEXT("bAutoConnect")));
 for(FName Name : {FName(TEXT("bStartCurb")),FName(TEXT("bEndCurb")),FName(TEXT("StartCurbRadius")),FName(TEXT("EndCurbRadius"))})
 { TestTrue(TEXT("端部路缘石预设参数可见"),Properties.Contains(Name)); }
 TestFalse(TEXT("内部样条引用不再展开"),Properties.Contains(TEXT("RoadSpline")));
 TestFalse(TEXT("内部网格引用不再展开"),Properties.Contains(TEXT("RoadMesh")));
 TestFalse(TEXT("父类复制属性隐藏"),Properties.Contains(TEXT("bReplicates")));
 auto* Building=World->SpawnActor<ALxRoundBuildingActor>();
 Building->Openings.SetNum(3);
 /** 暴露引擎实际手柄枚举入口，验证结构体数组中的每个洞口均可编辑。 */
 struct FBuildingWidgetHelper : FLegacyEdModeWidgetHelper
 { using FLegacyEdModeWidgetHelper::GetPropertyWidgetInfos; };
 FBuildingWidgetHelper Helper;
 TArray<FLegacyEdModeWidgetHelper::FPropertyWidgetInfo> Widgets;
 Helper.GetPropertyWidgetInfos(Building->GetClass(),Building,Widgets);
 TestEqual(TEXT("每个门窗洞口均有独立拖动手柄"),Widgets.Num(),3);
 Rows->SetObjects({Building}); Properties.Reset(); Collect(Rows->GetRootTreeNodes());
 TestTrue(TEXT("圆形建筑洞口参数可见"),Properties.Contains(TEXT("Openings")));
 TestTrue(TEXT("圆形建筑半径参数可见"),Properties.Contains(TEXT("Radius")));
 TestFalse(TEXT("圆形建筑内部网格隐藏"),Properties.Contains(TEXT("BuildingMesh")));
 return true;
}
#endif
