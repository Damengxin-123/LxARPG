#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Containers/Ticker.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "Editor.h"
#include "UnrealEdGlobals.h"
#include "Editor/UnrealEdEngine.h"
#include "ProceduralMeshComponent.h"
#include "../LxStoneTerraceVisualizer.h"
#include "LxARPG/LxSource/World/StoneTerrace/LxStoneTerraceActor.h"
#include "LxARPG/LxSource/World/StoneTerrace/LxStoneTerraceHandleComponent.h"
#include "LxARPG/LxSource/World/SplineRoad/LxSplineRoadActor.h"
#include "LxARPG/LxSource/World/GeneratedMesh/LxGeneratedMeshBuild.h"
#include "LxARPG/LxSource/World/GeneratedMesh/LxGeneratedMeshRenderComponent.h"

/** 使用实际尺寸手柄输入路径验证旋转、独立两轴、微小位移累计、连接以及撤销重做。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxTerraceHandlesTest,"LxARPG.World.StoneTerrace.Handles",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FLxTerraceHandlesTest::RunTest(const FString& Parameters)
{
 UWorld* World=UWorld::CreateWorld(EWorldType::Editor,false);
 ON_SCOPE_EXIT { World->DestroyWorld(false); };
 TestTrue(TEXT("编辑器已注册台地尺寸手柄"),GUnrealEd && GUnrealEd->FindComponentVisualizer(ULxStoneTerraceHandleComponent::StaticClass()).IsValid());
 auto* A=World->SpawnActor<ALxStoneTerraceActor>(FVector(400,300,0),FRotator(0,35,0));A->SetFlags(RF_Transactional);
 const FVector Center=A->GetActorLocation();
 auto* Road=World->SpawnActor<ALxSplineRoadActor>(A->GetActorTransform().TransformPosition(FVector(1300,0,100)),A->GetActorRotation());
 Road->RoadWidth=400;Road->RebuildRoad();
 TestTrue(TEXT("修改尺寸前尚未连接"),A->Connections.IsEmpty());
 FLxStoneTerraceVisualizer Visualizer;
 TestTrue(TEXT("选中长度手柄"),Visualizer.SelectHandle(A->DimensionHandles,0));
 FRotator Rotation=FRotator::ZeroRotator;FVector Scale=FVector::ZeroVector;
 FVector Delta=A->GetActorTransform().TransformVector(FVector(200,0,0));
 Visualizer.HandleInputDelta(nullptr,nullptr,Delta,Rotation,Scale);
 for(int32 I=0;I<5;++I)
 {
  Delta=A->GetActorTransform().TransformVector(FVector(.1,0,0));
  Visualizer.HandleInputDelta(nullptr,nullptr,Delta,Rotation,Scale);
 }
 FTSTicker::GetCoreTicker().Tick(.06f);
 TestTrue(TEXT("旋转后的手柄按局部X改变长度并累计微小位移"),FMath::IsNearlyEqual(A->LengthMeters,24.01f,.001f));
 TestEqual(TEXT("长度手柄不改变宽度"),A->WidthMeters,12.f);
 TestTrue(TEXT("台地中心保持不变"),A->GetActorLocation().Equals(Center));
 TestEqual(TEXT("拖动尺寸实时刷新道路接入"),A->Connections.Num(),1);
 FVector Widget;TestTrue(TEXT("尺寸手柄具有有效位置"),Visualizer.GetWidgetLocation(nullptr,Widget));
 TestTrue(TEXT("手柄随新边界移动"),Widget.Equals(A->GetActorTransform().TransformPosition(FVector(A->Size.X*.5,0,A->TerraceHeight)),.01));
 TestEqual(TEXT("手柄拖动不转换静态网格"),A->OptimizedMesh->GeometryConversionCount,0);
 const auto* Preview=A->TerraceMesh->GetProcMeshSection(0);
 TestTrue(TEXT("轻量预览包含真实砖缝比例"),Preview && Preview->ProcVertexBuffer[0].UV2.X>0);
 Visualizer.TrackingStopped(nullptr,true);Visualizer.EndEditing();
 GEditor->UndoTransaction();
 TestEqual(TEXT("撤销恢复原长宽"),A->LengthMeters,20.f);
 TestTrue(TEXT("撤销同时断开尺寸变化产生的连接"),A->Connections.IsEmpty());
 GEditor->RedoTransaction();
 TestTrue(TEXT("重做恢复拖动结果"),FMath::IsNearlyEqual(A->LengthMeters,24.01f,.001f));
 Visualizer.SelectHandle(A->DimensionHandles,1);
 Delta=A->GetActorTransform().TransformVector(FVector(0,150,500));
 Visualizer.HandleInputDelta(nullptr,nullptr,Delta,Rotation,Scale);Visualizer.TrackingStopped(nullptr,true);Visualizer.EndEditing();
 TestEqual(TEXT("宽度手柄独立控制宽度"),A->WidthMeters,15.f);
 TestEqual(TEXT("垂直分量不能拉弯台地或改变高度"),A->TerraceHeight,100.f);
 LxGeneratedMeshBuild::PrepareWorld(World);
 TestTrue(TEXT("手柄编辑后保存完整Nanite"),A->OptimizedMesh->GetStaticMesh() && A->OptimizedMesh->GetStaticMesh()->HasValidNaniteData());
 // 旧接口或旧关卡尺寸仍能准确换算为米，防止已有台地缩回默认大小。
 A->SetTerraceDimensions(3200,1800,200);
 TestEqual(TEXT("旧尺寸兼容长度"),A->LengthMeters,32.f);TestEqual(TEXT("旧尺寸兼容宽度"),A->WidthMeters,18.f);
 A->SetFootprintMeters(17.126f,9.344f);
 TestTrue(TEXT("米制接口量化到两位小数"),A->Size.Equals(FVector2D(1713,934),.01));
 return true;
}

/** 验证砖面色差、砖缝坐标和真实倒角，并检查小圆角及连接处不生成非法顶点。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxTerracePavingDetailTest,"LxARPG.World.StoneTerrace.PavingAndBevel",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FLxTerracePavingDetailTest::RunTest(const FString& Parameters)
{
 UWorld* World=UWorld::CreateWorld(EWorldType::Game,false);
 ON_SCOPE_EXIT { World->DestroyWorld(false); };
 auto* A=World->SpawnActor<ALxStoneTerraceActor>();
 A->OptimizedMesh->bEnableNanite=false;A->RebuildTerrace();
 const auto* Paving=A->TerraceMesh->GetProcMeshSection(0);
 int32 Min=255,Max=0;
 for(const auto& V:Paving->ProcVertexBuffer) { if(V.Color.A>0) { Min=FMath::Min(Min,int32(V.Color.R));Max=FMath::Max(Max,int32(V.Color.R)); } }
 TestTrue(TEXT("每块砖有明显但克制的明度差"),Max-Min>40);
 TestTrue(TEXT("铺砖数据包含实际砖缝比例"),Paving->ProcVertexBuffer[0].UV2.X>.01);
 const auto* Stone=A->TerraceMesh->GetProcMeshSection(1);
 TestTrue(TEXT("大边石包含真实斜向倒角面"),Stone->ProcVertexBuffer.ContainsByPredicate([](const FProcMeshVertex& V){return V.Normal.Z>.15 && V.Normal.Z<.9;}));
 const int32 BeveledTriangles=Stone->ProcIndexBuffer.Num();
 A->BorderBevelMeters=0;A->RebuildTerrace();
 TestTrue(TEXT("关闭倒角减少实际几何"),A->TerraceMesh->GetProcMeshSection(1)->ProcIndexBuffer.Num()<BeveledTriangles);
 A->SetFootprintMeters(2,1);A->CornerRadius=50;A->BorderBevelMeters=.1f;A->RebuildTerrace();
 for(int32 Slot=0;Slot<2;++Slot) for(const auto& V:A->TerraceMesh->GetProcMeshSection(Slot)->ProcVertexBuffer)
 { if(V.Position.ContainsNaN() || V.Normal.ContainsNaN()) { AddError(TEXT("极限尺寸倒角出现非法顶点"));return false; } }
 return true;
}
#endif
