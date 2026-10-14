#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/World.h"
#include "ProceduralMeshComponent.h"
#include "PhysicsEngine/BodySetup.h"
#include "LxBrickWallActor.h"
#include "Components/BoxComponent.h"
#include "UObject/UnrealType.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "StaticMeshResources.h"
#if WITH_EDITOR
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"
#endif

/** 统计实际启用的分段碰撞盒，不把隐藏备用组件计入墙体。 */
static int32 CountWallCollision(const ALxBrickWallActor* Wall)
{
 int32 Count=0;
 for (const UBoxComponent* Box : Wall->CollisionBoxes) { Count+=Box->GetCollisionEnabled()!=ECollisionEnabled::NoCollision; }
 return Count;
}

/** 验证实心砖的闭合拓扑、朝外绕序、转角出口、碰撞清理以及超预算拒绝行为。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxBrickWallTest,"LxARPG.World.BrickWall.Geometry",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxBrickWallTest::RunTest(const FString& Parameters)
{
 UWorld* World=UWorld::CreateWorld(EWorldType::Game,false);
 if (!TestNotNull(TEXT("测试世界"),World)) { return false; }
 ON_SCOPE_EXIT { World->DestroyWorld(false); };
 ALxBrickWallActor* Wall=World->SpawnActor<ALxBrickWallActor>();
 if (!TestNotNull(TEXT("自动砖墙"),Wall)) { return false; }
 Wall->bGenerateStaticMesh=false;
 Wall->LengthHandle=FVector(60,30,50); Wall->WallHeight=24;
 Wall->SurfaceRelief=0; Wall->ColorVariation=0; Wall->Weathering=0;
 Wall->StartConnection=ELxWallConnection::None; Wall->EndConnection=ELxWallConnection::None;
 Wall->RebuildWall();
 TestTrue(TEXT("拖动时锁定水平与高度"),Wall->LengthHandle.Equals(FVector(60,0,0)));
 TestEqual(TEXT("单块完整石砖"),Wall->GeneratedBrickCount,1);
 const FProcMeshSection* Section=Wall->WallMesh->GetProcMeshSection(0);
 if (!TestNotNull(TEXT("石砖几何"),Section)) { return false; }
 const FVector Center(30,0,12);
 // 将重复顶点焊接后，每条无向边必须被恰好两个三角形引用。
 TArray<FVector> Unique;
 TArray<int32> Welded;
 TMap<uint64,int32> Edges;
 for (const FProcMeshVertex& Vertex : Section->ProcVertexBuffer)
 {
  const int32 Found=Unique.IndexOfByPredicate([&](const FVector& P) { return P.Equals(Vertex.Position,0.0001); });
  Welded.Add(Found==INDEX_NONE?Unique.Add(Vertex.Position):Found);
 }
 bool bNormals=true, bNonDegenerate=true, bOutward=true;
 for (int32 I=0; I<Section->ProcIndexBuffer.Num(); I+=3)
 {
  const uint32 A=Section->ProcIndexBuffer[I], B=Section->ProcIndexBuffer[I+1], C=Section->ProcIndexBuffer[I+2];
  const FVector P=Section->ProcVertexBuffer[A].Position, Q=Section->ProcVertexBuffer[B].Position, R=Section->ProcVertexBuffer[C].Position;
  const FVector N=FVector::CrossProduct(R-P,Q-P);
  bNonDegenerate &= N.SizeSquared()>0.000001;
  bNormals &= FVector::DotProduct(N.GetSafeNormal(),Section->ProcVertexBuffer[A].Normal)>0.999;
  bOutward &= FVector::DotProduct(N,(P+Q+R)/3-Center)>0;
  for (int32 J=0; J<3; ++J)
  {
   const int32 U=Welded[Section->ProcIndexBuffer[I+J]], V=Welded[Section->ProcIndexBuffer[I+(J+1)%3]];
   const uint64 Key=(static_cast<uint64>(FMath::Min(U,V))<<32)|static_cast<uint32>(FMath::Max(U,V));
   ++Edges.FindOrAdd(Key);
  }
 }
 TestTrue(TEXT("全部三角形非退化"),bNonDegenerate);
 TestTrue(TEXT("所有面均朝向实体外部"),bOutward);
 TestTrue(TEXT("法线与绕序一致"),bNormals);
 bool bClosed=true; for (const auto& Edge : Edges) { bClosed &= Edge.Value==2; }
 TestTrue(TEXT("砖块六面及倒角形成闭合实体"),bClosed);
 const FProcMeshSection* Mortar=Wall->WallMesh->GetProcMeshSection(2);
 if (TestNotNull(TEXT("内凹砂浆"),Mortar))
 {
  TestTrue(TEXT("砂浆端盖不与裸露石砖端面共面"),Mortar->SectionLocalBox.Min.X>0 && Mortar->SectionLocalBox.Max.X<60);
 }
 TestEqual(TEXT("无连接时没有柱体碰撞"),CountWallCollision(Wall),1);
 Wall->LengthHandle=FVector(613,0,0); Wall->WallHeight=245;
 Wall->StartConnection=ELxWallConnection::Left; Wall->EndConnection=ELxWallConnection::Right;
 Wall->RebuildWall();
 TestTrue(TEXT("不规则尺寸可生成"),Wall->GenerationDiagnostic.IsEmpty());
 TestEqual(TEXT("主墙、双柱、双侧墙共五个盒形碰撞"),CountWallCollision(Wall),5);
 TestTrue(TEXT("起点左转出口"),Wall->GetConnectionTransform(false).GetLocation().Equals(FVector(-30,-210,0),0.001));
 TestTrue(TEXT("终点右转出口"),Wall->GetConnectionTransform(true).GetLocation().Equals(FVector(643,210,0),0.001));
 const int32 Count=Wall->GeneratedBrickCount;
 const FVector First=Wall->WallMesh->GetProcMeshSection(0)->ProcVertexBuffer[0].Position;
 Wall->RebuildWall();
 TestEqual(TEXT("重建不累积砖块"),Wall->GeneratedBrickCount,Count);
 TestTrue(TEXT("相同种子结果稳定"),First.Equals(Wall->WallMesh->GetProcMeshSection(0)->ProcVertexBuffer[0].Position));
 Wall->bEnableCollision=false; Wall->StartConnection=ELxWallConnection::None; Wall->EndConnection=ELxWallConnection::None;
 Wall->RebuildWall();
 TestEqual(TEXT("关闭碰撞后所有盒形碰撞禁用"),CountWallCollision(Wall),0);
 TestEqual(TEXT("渲染组件不再烹饪凸体"),Wall->WallMesh->GetBodySetup()->AggGeom.ConvexElems.Num(),0);
 TestTrue(TEXT("移除连接后没有残留连接几何"),!Wall->WallMesh->GetProcMeshSection(1) || Wall->WallMesh->GetProcMeshSection(1)->ProcVertexBuffer.IsEmpty());
 Wall->LengthHandle.X=10000; Wall->WallHeight=3000; Wall->BrickLength=15; Wall->BrickHeight=8;
 Wall->RebuildWall();
 TestFalse(TEXT("超预算给出诊断"),Wall->GenerationDiagnostic.IsEmpty());
 TestEqual(TEXT("超预算不残留旧墙"),Wall->WallMesh->GetNumSections(),0);
 return true;
}

/** 验证隐藏面裁切与编辑交互缓存，防止移动墙体再次逐帧生成或重复烹饪。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxBrickWallPerformanceTest,"LxARPG.World.BrickWall.EditingPerformance",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxBrickWallPerformanceTest::RunTest(const FString& Parameters)
{
 UWorld* World=UWorld::CreateWorld(EWorldType::Game,false);
 if (!TestNotNull(TEXT("性能测试世界"),World)) { return false; }
 ON_SCOPE_EXIT { World->DestroyWorld(false); };
 ALxBrickWallActor* Wall=World->SpawnActor<ALxBrickWallActor>();
 Wall->bGenerateStaticMesh=false;
 Wall->StartConnection=ELxWallConnection::None; Wall->EndConnection=ELxWallConnection::None;
 Wall->RebuildWall();
 const int32 OptimizedTriangles=Wall->GeneratedTriangleCount;
 const int32 OriginalTriangles=(Wall->GeneratedBrickCount+1)*96;
 TestTrue(TEXT("三角形少于原完整砖版本的八成"),OptimizedTriangles<OriginalTriangles*0.8);
 // 所有被内层砂浆完全包住的三角形都必须剔除，边界槽壁允许保留。
 const FBox Core(FVector(1.7,-18.3,0.8),FVector(598.3,18.3,239.2));
 const FProcMeshSection* Section=Wall->WallMesh->GetProcMeshSection(0);
 bool bNoBuriedTriangles=true;
 for (int32 I=0; I<Section->ProcIndexBuffer.Num(); I+=3)
 {
  const FVector& A=Section->ProcVertexBuffer[Section->ProcIndexBuffer[I]].Position;
  const FVector& B=Section->ProcVertexBuffer[Section->ProcIndexBuffer[I+1]].Position;
  const FVector& C=Section->ProcVertexBuffer[Section->ProcIndexBuffer[I+2]].Position;
  bNoBuriedTriangles &= !(Core.IsInside(A)&&Core.IsInside(B)&&Core.IsInside(C));
 }
 TestTrue(TEXT("不提交深埋砂浆的砖面"),bNoBuriedTriangles);
 const FBox OptimizedBounds=Section->SectionLocalBox;
 Wall->bCullInternalFaces=false; Wall->RebuildWall();
 TestTrue(TEXT("剔除内部面不改变外部轮廓"),Wall->WallMesh->GetProcMeshSection(0)->SectionLocalBox.Equals(OptimizedBounds,0.001));
 Wall->bCullInternalFaces=true; Wall->RebuildWall();
 const int32 BeforeMove=Wall->GeometryBuildCount;
 const FProcMeshVertex* VertexStorage=Wall->WallMesh->GetProcMeshSection(0)->ProcVertexBuffer.GetData();
 const double Start=FPlatformTime::Seconds();
 for (int32 I=0; I<100; ++I)
 {
  Wall->SetActorLocation(FVector(I*10,0,0));
  Wall->OnConstruction(Wall->GetActorTransform());
 }
 AddInfo(FString::Printf(TEXT("100次移动构造检查：%.3f毫秒；优化后%d三角形，旧版%d三角形。"),
  (FPlatformTime::Seconds()-Start)*1000,OptimizedTriangles,OriginalTriangles));
 TestEqual(TEXT("移动不会重建几何"),Wall->GeometryBuildCount,BeforeMove);
 TestTrue(TEXT("移动复用同一顶点缓冲"),VertexStorage==Wall->WallMesh->GetProcMeshSection(0)->ProcVertexBuffer.GetData());
 Wall->bEnableCollision=false; Wall->OnConstruction(Wall->GetActorTransform());
 TestEqual(TEXT("切换碰撞不重建砖面"),Wall->GeometryBuildCount,BeforeMove);
 TestEqual(TEXT("切换碰撞立即生效"),CountWallCollision(Wall),0);
#if WITH_EDITOR
 FProperty* Handle=FindFProperty<FProperty>(ALxBrickWallActor::StaticClass(),GET_MEMBER_NAME_CHECKED(ALxBrickWallActor,LengthHandle));
 for (int32 I=0; I<20; ++I)
 {
  Wall->LengthHandle.X+=10;
  FPropertyChangedEvent Event(Handle);
  Wall->PostEditChangeProperty(Event);
 }
 TestEqual(TEXT("连续手柄事件不逐帧重建"),Wall->GeometryBuildCount,BeforeMove);
 Wall->PostEditMove(true);
 TestEqual(TEXT("松手后只提交一次最终长度"),Wall->GeometryBuildCount,BeforeMove+1);
 TestEqual(TEXT("最终长度没有丢失"),Wall->LengthHandle.X,800.0);
 TestTrue(TEXT("最终网格长度同步"),Wall->WallMesh->GetProcMeshSection(0)->SectionLocalBox.Max.X>=800.0);
 Wall->LengthHandle.X=600;
 Wall->PostEditUndo();
 TestEqual(TEXT("撤销后恢复尺寸"),Wall->LengthHandle.X,600.0);
#endif
 return true;
}
/** 对比静态输出与原始砖面，验证转角、材质槽、顶点颜色及失败清理。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxBrickWallStaticMeshTest,"LxARPG.World.BrickWall.MergedStaticMesh",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxBrickWallStaticMeshTest::RunTest(const FString& Parameters)
{
 UWorld* World=UWorld::CreateWorld(EWorldType::Game,false);
 if (!TestNotNull(TEXT("静态网格测试世界"),World)) { return false; }
 ON_SCOPE_EXIT { World->DestroyWorld(false); };
 ALxBrickWallActor* Wall=World->SpawnActor<ALxBrickWallActor>();
 if (!TestNotNull(TEXT("默认墙体"),Wall)) { return false; }
 TestTrue(TEXT("默认直接生成静态网格"),Wall->bGenerateStaticMesh);
 Wall->StartConnection=ELxWallConnection::Left; Wall->EndConnection=ELxWallConnection::Right;
 Wall->LengthHandle.X=613; Wall->WallHeight=245;
 Wall->bGenerateStaticMesh=false;
 Wall->RebuildWall();
 TArray<FProcMeshSection> Reference;
 for (int32 Slot=0; Slot<3; ++Slot) { Reference.Add(*Wall->WallMesh->GetProcMeshSection(Slot)); }
 const int32 Triangles=Wall->GeneratedTriangleCount;
 const FTransform Start=Wall->GetConnectionTransform(false), End=Wall->GetConnectionTransform(true);
 Wall->bGenerateStaticMesh=true;
 Wall->RebuildWall();
 UStaticMesh* Mesh=Wall->StaticWallMesh->GetStaticMesh();
 if (!TestNotNull(TEXT("整墙静态网格"),Mesh)) { return false; }
 TestEqual(TEXT("整墙只有一个渲染网格"),Wall->GeneratedMeshCount,1);
 TestEqual(TEXT("静态模式不保留重复程序化几何"),Wall->WallMesh->GetNumSections(),0);
 TestEqual(TEXT("合并不丢失凹凸三角形"),Mesh->GetRenderData()->LODResources[0].GetNumTriangles(),static_cast<uint32>(Triangles));
 TestEqual(TEXT("双转角仍只有三个材质分区"),Mesh->GetRenderData()->LODResources[0].Sections.Num(),3);
 TestEqual(TEXT("双转角仍用五个碰撞盒"),CountWallCollision(Wall),5);
 TestTrue(TEXT("合并不改变两个接续出口"),Start.Equals(Wall->GetConnectionTransform(false)) && End.Equals(Wall->GetConnectionTransform(true)));
#if WITH_EDITOR
 const FMeshDescription* Description=Mesh->GetMeshDescription(0);
 if (!TestNotNull(TEXT("编辑器保留可保存的网格描述"),Description)) { return false; }
 FStaticMeshConstAttributes Attributes(*Description);
 int32 VertexIndex=0, TriangleIndex=0;
 bool bPositions=true, bNormals=true, bColors=true, bUVs=true, bWinding=true;
 for (int32 Slot=0; Slot<Reference.Num(); ++Slot)
 {
  const FProcMeshSection& Section=Reference[Slot];
  const int32 Base=VertexIndex;
  for (const FProcMeshVertex& Vertex : Section.ProcVertexBuffer)
  {
   const FVertexID ID(VertexIndex);
   const FVertexInstanceID Instance(VertexIndex++);
   bPositions &= FVector(Attributes.GetVertexPositions()[ID]).Equals(Vertex.Position,0.001);
   bNormals &= FVector(Attributes.GetVertexInstanceNormals()[Instance]).Equals(Vertex.Normal,0.001);
   const FVector4f Color=Attributes.GetVertexInstanceColors()[Instance];
   bColors &= FLinearColor(Color.X,Color.Y,Color.Z,Color.W).ToFColor(false)==Vertex.Color;
   bUVs &= FVector2D(Attributes.GetVertexInstanceUVs().Get(Instance,0)).Equals(Vertex.UV0,0.001);
  }
  for (int32 I=0; I<Section.ProcIndexBuffer.Num(); I+=3)
  {
   const FTriangleID Triangle(TriangleIndex++);
   const TArrayView<const FVertexInstanceID> Instances=Description->GetTriangleVertexInstances(Triangle);
   for (int32 Corner=0; Corner<3; ++Corner)
   { bWinding &= Instances[Corner].GetValue()==Base+static_cast<int32>(Section.ProcIndexBuffer[I+Corner]); }
   TestEqual(TEXT("每个三角形保持原材质槽"),Description->GetTrianglePolygonGroup(Triangle).GetValue(),Slot);
  }
 }
 TestTrue(TEXT("主墙、侧墙与柱体的所有顶点保持原位置"),bPositions);
 TestTrue(TEXT("所有法线保持原倒角光照"),bNormals);
 TestTrue(TEXT("逐砖颜色与积尘保持一致"),bColors);
 TestTrue(TEXT("所有纹理坐标保持一致"),bUVs);
 TestTrue(TEXT("每个三角形保持外向绕序"),bWinding);
#endif
 // 模拟关卡重新加载时临时资源未序列化，只依赖注册生命周期恢复，不调用构造脚本。
 Wall->UnregisterAllComponents();
 Wall->StaticWallMesh->SetStaticMesh(nullptr);
 Wall->RegisterAllComponents();
 Mesh=Wall->StaticWallMesh->GetStaticMesh();
 if (!TestNotNull(TEXT("注册时自动恢复临时静态网格"),Mesh)) { return false; }
 const int32 Builds=Wall->GeometryBuildCount;
 Wall->SetActorLocation(FVector(50,100,20));
 Wall->OnConstruction(Wall->GetActorTransform());
 TestTrue(TEXT("移动复用同一个静态网格资源"),Wall->StaticWallMesh->GetStaticMesh()==Mesh);
 TestEqual(TEXT("移动没有重建"),Wall->GeometryBuildCount,Builds);
 Wall->bEnableCollision=false;
 Wall->OnConstruction(Wall->GetActorTransform());
 TestEqual(TEXT("关闭碰撞不重建静态网格"),Wall->GeometryBuildCount,Builds);
 TestEqual(TEXT("关闭碰撞立即生效"),CountWallCollision(Wall),0);
 Wall->StartConnection=ELxWallConnection::None; Wall->EndConnection=ELxWallConnection::None;
 Wall->RebuildWall();
 Mesh=Wall->StaticWallMesh->GetStaticMesh();
 TestEqual(TEXT("无柱墙仅提交石砖与砂浆两个分区"),Mesh->GetRenderData()->LODResources[0].Sections.Num(),2);
 TestEqual(TEXT("无柱墙的砂浆仍使用第三个材质槽"),Mesh->GetRenderData()->LODResources[0].Sections[1].MaterialIndex,2);
 Wall->LengthHandle.X=10000; Wall->WallHeight=3000; Wall->BrickLength=15; Wall->BrickHeight=8;
 Wall->RebuildWall();
 TestNull(TEXT("超预算移除旧静态网格"),Wall->StaticWallMesh->GetStaticMesh());
 TestEqual(TEXT("超预算没有渲染网格"),Wall->GeneratedMeshCount,0);
 TestFalse(TEXT("超预算有诊断"),Wall->GenerationDiagnostic.IsEmpty());
 return true;
}
#endif
