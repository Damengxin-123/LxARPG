#if WITH_DEV_AUTOMATION_TESTS
#include "LxRoundBuildingActor.h"
#include "../GeneratedMesh/LxGeneratedMeshRenderComponent.h"
#include "../WoodenDoor/LxWoodenDoorActor.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/World.h"
#include "ProceduralMeshComponent.h"
#include "Materials/Material.h"
#include "Engine/StaticMesh.h"

namespace LxRoundBuildingTests
{
 /** 使用实际三角形碰撞验证洞口和楼板通路。 */
 bool Trace(ALxRoundBuildingActor* A,FVector Start,FVector End,FHitResult* Out=nullptr)
 {
  FHitResult Hit; FCollisionQueryParams Params; Params.bTraceComplex=true;
  const bool bHit=A->BuildingMesh->LineTraceComponent(Hit,Start,End,Params);
  if(Out) { *Out=Hit; } return bHit;
 }
 /** 创建轻量测试建筑，避免编辑测试执行 Nanite 烘焙。 */
 ALxRoundBuildingActor* Building(UWorld* World,FVector P=FVector::ZeroVector)
 { auto* A=World->SpawnActor<ALxRoundBuildingActor>(P,FRotator::ZeroRotator); A->OptimizedMesh->bEnableNanite=false; return A; }
}

/** 验证房间边界、矩形门窗切口、安装变换及可选楼板。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxRoundBuildingGeometryTest,"LxARPG.World.RoundBuilding.GeometryAndOpenings",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxRoundBuildingGeometryTest::RunTest(const FString& Parameters)
{
 using namespace LxRoundBuildingTests;
 UWorld* World=UWorld::CreateWorld(EWorldType::Game,false); ON_SCOPE_EXIT { World->DestroyWorld(false); };
 auto* A=Building(World); A->RebuildBuilding();
 TestTrue(TEXT("默认建筑生成"),A->GenerationDiagnostic.IsEmpty());
 const auto* Floor=A->BuildingMesh->GetProcMeshSection(1);
 const auto* Ceiling=A->BuildingMesh->GetProcMeshSection(2);
 if(!TestNotNull(TEXT("地板存在"),Floor) || !TestNotNull(TEXT("天花板存在"),Ceiling)) { return false; }
 TestTrue(TEXT("地板加厚向上且底面保持零"),FMath::IsNearlyZero(Floor->SectionLocalBox.Min.Z) && FMath::IsNearlyEqual(Floor->SectionLocalBox.Max.Z,20.));
 TestTrue(TEXT("天花板加厚向下且顶面保持指定高度"),FMath::IsNearlyEqual(Ceiling->SectionLocalBox.Min.Z,280.) && FMath::IsNearlyEqual(Ceiling->SectionLocalBox.Max.Z,300.));
 A->AddDoorOpening();
 if(!TestTrue(*A->GenerationDiagnostic,A->GenerationDiagnostic.IsEmpty())) { return false; }
 TestFalse(TEXT("矩形门洞贯穿墙厚"),Trace(A,FVector(600,0,100),FVector(400,0,100)));
 TestTrue(TEXT("门洞旁墙体仍有碰撞"),Trace(A,FVector(600,70,100),FVector(400,70,100)));
 FHitResult Hit;
 TestTrue(TEXT("门洞侧框存在"),Trace(A,FVector(480,0,100),FVector(480,100,100),&Hit));
 TestTrue(TEXT("侧框是直线平面，精确保留134净宽"),FMath::IsNearlyEqual(Hit.ImpactPoint.Y,67.,0.05));
 TestTrue(TEXT("门洞顶部存在"),Trace(A,FVector(480,0,100),FVector(480,0,275),&Hit));
 TestTrue(TEXT("门洞顶梁水平，净高准确"),FMath::IsNearlyEqual(Hit.ImpactPoint.Z,264.,0.05));
 const FTransform Mount=A->GetOpeningTransform(0);
 TestTrue(TEXT("现有木门的宽度轴沿墙面切向"),FVector::DotProduct(Mount.GetUnitAxis(EAxis::X),FVector::RightVector)>0.999);
 TestTrue(TEXT("安装原点跟随地板顶面"),FMath::IsNearlyEqual(Mount.GetLocation().Z,20.));
 TestTrue(TEXT("可按现有木门调整净尺寸"),A->FitOpeningToDoor(0,GetMutableDefault<ALxWoodenDoorActor>(),2));
 TestEqual(TEXT("门框宽度留安装余量"),A->Openings[0].Width,134.f);
 FLxRoundBuildingOpening Window; Window.Name=TEXT("测试窗洞"); Window.bDoor=false; Window.Position=FVector(0,500,120); Window.Width=100; Window.Height=100;
 A->Openings.Add(Window); A->RebuildBuilding();
 TestTrue(TEXT("多个洞口生成成功"),A->GenerationDiagnostic.IsEmpty());
 TestFalse(TEXT("窗洞贯穿圆墙"),Trace(A,FVector(0,600,150),FVector(0,400,150)));
 TestTrue(TEXT("窗台以下仍为墙体"),Trace(A,FVector(0,600,80),FVector(0,400,80)));
 A->Openings[1].Position=FVector(-500,0,140); A->RebuildBuilding();
 TestTrue(TEXT("拖动后恢复旧窗洞"),Trace(A,FVector(0,600,150),FVector(0,400,150)));
 TestFalse(TEXT("拖动后新位置开窗"),Trace(A,FVector(-600,0,180),FVector(-400,0,180)));
 A->Openings.Reset(); A->bFloor=false; A->bCeiling=false; A->RebuildBuilding();
 TestFalse(TEXT("关闭地板和天花板后室内竖直贯通"),Trace(A,FVector(0,0,450),FVector(0,0,-10)));
 A->bFloor=true; A->bCeiling=true; A->bCeilingFollowsWall=false; A->CeilingHeight=1000; A->CeilingThickness=800; A->RebuildBuilding();
 Ceiling=A->BuildingMesh->GetProcMeshSection(2);
 if(!TestNotNull(TEXT("厚楼板仍生成"),Ceiling)) { return false; }
 TestTrue(TEXT("楼板不超出墙高"),Ceiling->SectionLocalBox.Max.Z<=A->WallHeight+0.001 && Ceiling->SectionLocalBox.Min.Z>=A->FloorThickness-0.001);
 return true;
}

/** 验证大厅与偏心高塔之间的竖向通道，排除普通相交并验证移动和删除恢复。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxRoundBuildingNestedTest,"LxARPG.World.RoundBuilding.NestedTower",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxRoundBuildingNestedTest::RunTest(const FString& Parameters)
{
 using namespace LxRoundBuildingTests;
 UWorld* World=UWorld::CreateWorld(EWorldType::Game,false); ON_SCOPE_EXIT { World->DestroyWorld(false); };
 auto* Hall=Building(World); Hall->Radius=600; Hall->RebuildBuilding();
 auto* Tower=Building(World,FVector(100,0,280)); Tower->Radius=180; Tower->WallHeight=800; Tower->CeilingTopOffset=40; Tower->RebuildBuilding();
 TestTrue(TEXT("大厅和高塔均正常生成"),Hall->GenerationDiagnostic.IsEmpty() && Tower->GenerationDiagnostic.IsEmpty());
 TestFalse(TEXT("大厅天花板在偏心高塔范围开孔"),Trace(Hall,FVector(100,0,350),FVector(100,0,250)));
 TestTrue(TEXT("高塔以外的大厅天花板保留"),Trace(Hall,FVector(400,0,350),FVector(400,0,250)));
 TestFalse(TEXT("高塔连接层地板不阻挡楼梯"),Trace(Tower,FVector(100,0,350),FVector(100,0,250)));
 TestTrue(TEXT("未相交的大厅地板保留"),Trace(Hall,FVector(100,0,50),FVector(100,0,-10)));
 TestTrue(TEXT("高于大厅的高塔天花板保留"),Trace(Tower,FVector(100,0,1100),FVector(100,0,1000)));
 TestTrue(TEXT("高塔墙体仍然存在"),Trace(Tower,FVector(300,0,500),FVector(200,0,500)));
 Tower->SetActorLocation(FVector(550,0,280)); Tower->RebuildBuilding();
 TestTrue(TEXT("普通两圆相交不挖大厅天花板"),Trace(Hall,FVector(400,0,350),FVector(400,0,250)));
 TestTrue(TEXT("离开包含范围恢复高塔地板"),Trace(Tower,FVector(550,0,350),FVector(550,0,250)));
 Tower->SetActorLocation(FVector(100,0,500)); Tower->RebuildBuilding();
 TestTrue(TEXT("无高度交集不挖孔"),Trace(Hall,FVector(100,0,350),FVector(100,0,250)));
 Tower->SetActorLocation(FVector(100,0,280)); Tower->RebuildBuilding(); Tower->Destroy();
 TestTrue(TEXT("删除高塔恢复大厅天花板"),Trace(Hall,FVector(100,0,350),FVector(100,0,250)));
 return true;
}

/** 验证城垛凹口不会侵入天花板，生成几何无反面和退化面。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxRoundBuildingBattlementsTest,"LxARPG.World.RoundBuilding.Battlements",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxRoundBuildingBattlementsTest::RunTest(const FString& Parameters)
{
 using namespace LxRoundBuildingTests;
 UWorld* World=UWorld::CreateWorld(EWorldType::Game,false); ON_SCOPE_EXIT { World->DestroyWorld(false); };
 auto* A=Building(World); A->bBattlements=true; A->NotchDepth=500; A->RebuildBuilding();
 TestTrue(TEXT("城垛生成成功"),A->GenerationDiagnostic.IsEmpty());
 const double HighAngle=UE_DOUBLE_TWO_PI/16*0.25, LowAngle=UE_DOUBLE_TWO_PI/16*0.75;
 for(double Angle : {HighAngle,LowAngle})
 {
  const FVector D(FMath::Cos(Angle),FMath::Sin(Angle),0);
  TestEqual(TEXT("高墙和凹口交替"),Trace(A,D*550+FVector(0,0,375),D*450+FVector(0,0,375)),Angle==HighAngle);
  TestTrue(TEXT("凹口以下墙面保留"),Trace(A,D*550+FVector(0,0,290),D*450+FVector(0,0,290)));
 }
 bool bValid=true;
 for(int Slot=0; Slot<A->BuildingMesh->GetNumSections(); ++Slot)
 {
  const auto* S=A->BuildingMesh->GetProcMeshSection(Slot);
  if(!S) { continue; }
  for(int I=0; I<S->ProcIndexBuffer.Num(); I+=3)
  {
   const auto& P=S->ProcVertexBuffer[S->ProcIndexBuffer[I]]; const auto& Q=S->ProcVertexBuffer[S->ProcIndexBuffer[I+1]]; const auto& R=S->ProcVertexBuffer[S->ProcIndexBuffer[I+2]];
   const FVector Cross=FVector::CrossProduct(FVector(R.Position-P.Position),FVector(Q.Position-P.Position));
   bValid &= Cross.SizeSquared()>1.e-10 && FVector::DotProduct(Cross,FVector(P.Normal))>0;
  }
 }
 TestTrue(TEXT("所有面非退化且法线匹配绕序"),bValid);
 A->bBattlements=false; A->RebuildBuilding();
 const auto* S=A->BuildingMesh->GetProcMeshSection(3);
 TestTrue(TEXT("关闭城垛清理旧网格"),!S || S->ProcVertexBuffer.IsEmpty());
 return true;
}
/** 验证墙高联动、外凸门窗框的净空以及砖墙圆周纹理的接缝展开。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxRoundBuildingTrimTest,"LxARPG.World.RoundBuilding.CeilingAndTrim",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxRoundBuildingTrimTest::RunTest(const FString& Parameters)
{
 using namespace LxRoundBuildingTests;
 UWorld* World=UWorld::CreateWorld(EWorldType::Game,false); ON_SCOPE_EXIT { World->DestroyWorld(false); };
 auto* A=Building(World); A->WallHeight=700; A->RebuildBuilding();
 auto* Ceiling=A->BuildingMesh->GetProcMeshSection(2);
 if(!TestNotNull(TEXT("调高后天花板存在"),Ceiling)) { return false; }
 TestTrue(TEXT("墙高增加300厘米时天花板同步升高300厘米"),FMath::IsNearlyEqual(Ceiling->SectionLocalBox.Max.Z,600.));
 A->bCeilingFollowsWall=false; A->CeilingHeight=450; A->RebuildBuilding();
 TestTrue(TEXT("手动高度模式仍可用"),FMath::IsNearlyEqual(A->BuildingMesh->GetProcMeshSection(2)->SectionLocalBox.Max.Z,450.));
 A->bCeilingFollowsWall=true; A->AddDoorOpening();
 if(!TestTrue(*A->GenerationDiagnostic,A->GenerationDiagnostic.IsEmpty())) { return false; }
 auto* Trim=A->BuildingMesh->GetProcMeshSection(4);
 if(!TestNotNull(TEXT("门框生成独立网格"),Trim)) { return false; }
 TestTrue(TEXT("石框凸出墙面"),Trim->SectionLocalBox.Max.X>A->Radius+7.9);
 TestFalse(TEXT("门框不生成挡路门槛"),Trace(A,FVector(550,0,21),FVector(400,0,21)));
 TestFalse(TEXT("门框不侵占矩形净宽"),Trace(A,FVector(550,66,100),FVector(400,66,100)));
 FHitResult Hit;
 TestTrue(TEXT("外凸门柱具有碰撞"),Trace(A,FVector(550,73,100),FVector(400,73,100),&Hit));
 TestTrue(TEXT("门柱前表面超出墙面8厘米"),FMath::IsNearlyEqual(Hit.ImpactPoint.X,508.,0.05));
 FLxRoundBuildingOpening Window; Window.bDoor=false; Window.Position=FVector(0,500,120); Window.Width=100; Window.Height=100;
 A->Openings.Add(Window); A->RebuildBuilding();
 TestTrue(TEXT("外挑窗台具有碰撞"),Trace(A,FVector(0,550,115),FVector(0,490,115)));
 TestFalse(TEXT("窗台不侵占窗洞净高"),Trace(A,FVector(0,550,121),FVector(0,400,121)));
 A->Openings.Reset(); A->RebuildBuilding();
 const auto* Wall=A->BuildingMesh->GetProcMeshSection(0); bool bContinuous=true;
 for(int I=0; I<Wall->ProcIndexBuffer.Num(); I+=3)
 {
  const auto& P=Wall->ProcVertexBuffer[Wall->ProcIndexBuffer[I]];
  const auto& Q=Wall->ProcVertexBuffer[Wall->ProcIndexBuffer[I+1]];
  const auto& R=Wall->ProcVertexBuffer[Wall->ProcIndexBuffer[I+2]];
  if(FMath::Abs(P.Normal.Z)<0.01) { bContinuous &= FMath::Abs(P.UV0.X-Q.UV0.X)<0.5 && FMath::Abs(P.UV0.X-R.UV0.X)<0.5; }
 }
 TestTrue(TEXT("圆周接缝不会将整圈砖纹挤入一个三角形"),bContinuous);
 A->AddDoorOpening(); A->bOpeningTrim=false; A->RebuildBuilding();
 Trim=A->BuildingMesh->GetProcMeshSection(4);
 TestTrue(TEXT("关闭装饰清理旧石框"),!Trim || Trim->ProcVertexBuffer.IsEmpty());
 return true;
}
/** 验证原生和蓝图放置、旧空材质恢复、自定义材质与静态转换的完整链路。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxRoundBuildingMaterialsTest,"LxARPG.World.RoundBuilding.MaterialDefaults",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxRoundBuildingMaterialsTest::RunTest(const FString& Parameters)
{
 UWorld* World=UWorld::CreateWorld(EWorldType::Editor,false); ON_SCOPE_EXIT { World->DestroyWorld(false); };
 UClass* Preset=LoadClass<ALxRoundBuildingActor>(nullptr,TEXT("/Game/项目内容/美术资产/海港城市/圆形建筑/自动圆形建筑.自动圆形建筑_C"));
 if(!TestNotNull(TEXT("保存的圆形建筑蓝图可重新加载"),Preset)) { return false; }
 UClass* UserPreset=LoadClass<ALxRoundBuildingActor>(nullptr,TEXT("/Game/项目内容/美术资产/自动化类型/自动建筑/自动圆形建筑.自动圆形建筑_C"));
 if(!TestNotNull(TEXT("自动化目录中的同名预设可重新加载"),UserPreset)) { return false; }
 const auto* Defaults=GetDefault<ALxRoundBuildingActor>();
 if(!TestNotNull(TEXT("原生墙材质默认值"),Defaults->WallMaterial.Get())
  || !TestNotNull(TEXT("原生装饰材质默认值"),Defaults->OpeningTrimMaterial.Get())
  || !TestNotNull(TEXT("原生地板材质默认值"),Defaults->FloorMaterial.Get())
  || !TestNotNull(TEXT("原生天花板材质默认值"),Defaults->CeilingMaterial.Get())) { return false; }
 TestTrue(TEXT("默认墙体使用实际错缝砖纹资产"),Defaults->WallMaterial->GetPathName().Contains(TEXT("灰白错缝砖墙")));
 TestTrue(TEXT("默认地板使用大块石板"),Defaults->FloorMaterial->GetPathName().Contains(TEXT("大块浅灰石板地面")));
 TestTrue(TEXT("默认天花板使用独立的大块石板"),Defaults->CeilingMaterial->GetPathName().Contains(TEXT("大块灰白石板天花")));
 UMaterialInterface* Legacy=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/项目内容/美术资产/海港城市/砖墙/材质/灰白石砖.灰白石砖"));
 if(!TestNotNull(TEXT("迁移测试使用真实旧石材"),Legacy)) { return false; }
 for(UClass* Type : {ALxRoundBuildingActor::StaticClass(),Preset,UserPreset})
 {
  TestTrue(TEXT("重新加载后预设本身已保存正确墙材质"),Type->GetDefaultObject<ALxRoundBuildingActor>()->WallMaterial==Defaults->WallMaterial);
  TestTrue(TEXT("两个目录的预设均保存大石板材质"),Type->GetDefaultObject<ALxRoundBuildingActor>()->FloorMaterial==Defaults->FloorMaterial && Type->GetDefaultObject<ALxRoundBuildingActor>()->CeilingMaterial==Defaults->CeilingMaterial);
  auto* A=World->SpawnActor<ALxRoundBuildingActor>(Type);
  A->OptimizedMesh->bEnableNanite=false;
  A->bNestedFloorHoles=false; A->bBattlements=true; A->AddDoorOpening();
  TestTrue(TEXT("新放置对象绑定砖墙和独立石框"),A->WallMaterial==Defaults->WallMaterial && A->OpeningTrimMaterial==Defaults->OpeningTrimMaterial);
  // 初版无接缝的旧石材分别升级为砖墙与大石板，不影响楼板几何和通道。
  A->WallMaterial=Legacy; A->FloorMaterial=Legacy; A->CeilingMaterial=Legacy;
  A->RebuildBuilding();
  TestTrue(TEXT("旧版非空石材升级为有砖缝的墙材质"),A->WallMaterial==Defaults->WallMaterial && A->BuildingMesh->GetMaterial(3)==Defaults->WallMaterial);
  TestTrue(TEXT("旧地板和天花板均迁移为大石板"),A->FloorMaterial==Defaults->FloorMaterial && A->CeilingMaterial==Defaults->CeilingMaterial);
  A->WallMaterial=nullptr; A->FloorMaterial=nullptr; A->CeilingMaterial=nullptr; A->OpeningTrimMaterial=nullptr;
  A->RebuildBuilding();
  const TArray<UMaterialInterface*> Materials={Defaults->WallMaterial,Defaults->FloorMaterial,Defaults->CeilingMaterial,Defaults->WallMaterial,Defaults->OpeningTrimMaterial};
  for(int Slot=0; Slot<Materials.Num(); ++Slot)
  { TestTrue(TEXT("旧对象空材质在预览中自动恢复"),A->BuildingMesh->GetMaterial(Slot)==Materials[Slot]); }
  A->OptimizedMesh->bEnableNanite=true;
  A->OptimizedMesh->FinalizeMesh();
  UStaticMesh* Mesh=A->OptimizedMesh->GetStaticMesh();
  if(TestNotNull(TEXT("静态转换完成"),Mesh))
  {
   for(int Slot=0; Slot<Materials.Num(); ++Slot)
   { TestTrue(TEXT("静态网格和渲染组件均保留分区材质"),Mesh->GetMaterial(Slot)==Materials[Slot] && A->OptimizedMesh->GetMaterial(Slot)==Materials[Slot]); }
  }
  UMaterialInterface* Custom=UMaterial::GetDefaultMaterial(MD_Surface);
  A->WallMaterial=Custom; A->FloorMaterial=Custom; A->CeilingMaterial=Custom; A->RebuildBuilding();
  TestTrue(TEXT("用户自定义材质不会被默认值覆盖"),A->WallMaterial==Custom && A->BuildingMesh->GetMaterial(0)==Custom && A->BuildingMesh->GetMaterial(1)==Custom && A->BuildingMesh->GetMaterial(2)==Custom);
 }
 return true;
}
#endif
