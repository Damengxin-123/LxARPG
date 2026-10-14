#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "StaticMeshResources.h"
#include "PhysicsEngine/BodySetup.h"
#include "LxTileRoofActor.h"
#if WITH_EDITOR
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"
#endif

/** 验证单坡几何、三个独立石沿开关、真实搭接与不封闭室内的简单碰撞。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxRoofGeometryTest,"LxARPG.World.TileRoof.Geometry",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxRoofGeometryTest::RunTest(const FString& Parameters)
{
 UWorld* World=UWorld::CreateWorld(EWorldType::Game,false);
 ON_SCOPE_EXIT { if (World) { World->DestroyWorld(false); } };
 if (!TestNotNull(TEXT("测试世界"),World)) { return false; }
 ALxTileRoofActor* Roof=World->SpawnActor<ALxTileRoofActor>();
 if (!TestNotNull(TEXT("屋顶实例"),Roof)) { return false; }
 Roof->bCloseSides=false;
 TInlineComponentArray<UStaticMeshComponent*> Components(Roof);
 TestEqual(TEXT("所有瓦片和石沿只有一个网格组件"),Components.Num(),1);
 for (int32 Mask=0; Mask<8; ++Mask)
 {
  Roof->bLeftStone=(Mask&1)!=0; Roof->bRightStone=(Mask&2)!=0; Roof->bTopStone=(Mask&4)!=0;
  Roof->SizeHandle=FVector(823,517,80); Roof->RebuildRoof();
  UStaticMesh* Mesh=Roof->RoofMesh->GetStaticMesh();
  if (!TestNotNull(TEXT("合并屋顶"),Mesh)) { return false; }
  TestEqual(TEXT("尺寸手柄保持水平"),Roof->SizeHandle.Z,0.0);
  const FBox Bounds=Mesh->GetBoundingBox();
  TestTrue(TEXT("石沿开关不改变水平宽深"),FMath::IsNearlyZero(Bounds.Min.X,.001) && FMath::IsNearlyZero(Bounds.Min.Y,.001)
   && FMath::IsNearlyEqual(Bounds.Max.X,823.0,.001) && FMath::IsNearlyEqual(Bounds.Max.Y,517.0,.001));
  TestEqual(TEXT("只有有效材质分区"),Mesh->GetRenderData()->LODResources[0].Sections.Num(),Mask?2:1);
  TestEqual(TEXT("关闭石沿同时移除对应碰撞"),Mesh->GetBodySetup()->AggGeom.GetElementCount(),1+int(Roof->bLeftStone)+int(Roof->bRightStone)+int(Roof->bTopStone));
  TestTrue(TEXT("按坡向生成多排瓦"),Roof->GeneratedRowCount>3 && Roof->GeneratedTileCount>Roof->GeneratedRowCount);
  const FKConvexElem& RoofCollision=Mesh->GetBodySetup()->AggGeom.ConvexElems[0];
  const double Slope=FMath::Tan(FMath::DegreesToRadians(Roof->SlopeAngle));
  bool bThin=true;
  for (const FVector& P : RoofCollision.VertexData)
  { const double Height=P.Z-P.Y*Slope; bThin &= Height>=-Roof->BackingThickness-.001 && Height<15; }
  TestTrue(TEXT("屋面碰撞为斜薄板，不是封住室内的外包围盒"),bThin);
#if WITH_EDITOR
  const FMeshDescription* D=Mesh->GetMeshDescription(0);
  if (!TestNotNull(TEXT("可导出的源几何"),D)) { return false; }
  FStaticMeshConstAttributes A(*D);
  // 每个构件独立验算，以免相邻石料接触点影响闭合性统计。
  TMap<float,TMap<FVector3f,int32>> PieceVertices;
  TMap<float,TMap<uint64,int32>> PieceEdges;
  TMap<float,FBox> TileBounds;
  TMap<float,TMap<FVector3f,FVector3f>> Coordinates;
  bool bNormals=true,bContinuous=true,bUVs=true;
  for (FTriangleID T : D->Triangles().GetElementIDs())
  {
   const auto C=D->GetTriangleVertexInstances(T);
   const float Phase=A.GetVertexInstanceUVs().Get(C[0],1).Y;
   auto& Vertices=PieceVertices.FindOrAdd(Phase); auto& Edges=PieceEdges.FindOrAdd(Phase); auto& UVs=Coordinates.FindOrAdd(Phase);
   FVector P[3]; int32 IDs[3];
   for (int32 I=0; I<3; ++I)
   {
    const FVector3f Pos=A.GetVertexPositions()[D->GetVertexInstanceVertex(C[I])]; P[I]=FVector(Pos);
    IDs[I]=Vertices.FindOrAdd(Pos,Vertices.Num());
    const FVector2f UV=A.GetVertexInstanceUVs().Get(C[I],0),UV1=A.GetVertexInstanceUVs().Get(C[I],1);
    const FVector3f Coord(UV.X,UV.Y,UV1.X);
    if (const FVector3f* Old=UVs.Find(Pos)) { bContinuous &= Old->Equals(Coord,1.e-6f); } else { UVs.Add(Pos,Coord); }
    if (D->GetTrianglePolygonGroup(T).GetValue()==0)
    { if (!TileBounds.Contains(Phase)) { TileBounds.Add(Phase,FBox(ForceInit)); } TileBounds[Phase]+=P[I]; }
   }
   const FVector N=FVector::CrossProduct(P[2]-P[0],P[1]-P[0]).GetSafeNormal();
   for (int32 I=0; I<3; ++I)
   {
    bNormals &= FVector::DotProduct(N,FVector(A.GetVertexInstanceNormals()[C[I]]))>.99;
    const uint64 Key=(static_cast<uint64>(FMath::Min(IDs[I],IDs[(I+1)%3]))<<32)|static_cast<uint32>(FMath::Max(IDs[I],IDs[(I+1)%3]));
    ++Edges.FindOrAdd(Key);
   }
   const FVector2f U=A.GetVertexInstanceUVs().Get(C[1],2)-A.GetVertexInstanceUVs().Get(C[0],2);
   const FVector2f V=A.GetVertexInstanceUVs().Get(C[2],2)-A.GetVertexInstanceUVs().Get(C[0],2);
   bUVs &= FMath::Abs(U.X*V.Y-U.Y*V.X)>1.e-10;
  }
  bool bClosed=true;
  for (const auto& Piece : PieceEdges) for (const auto& Edge : Piece.Value) { bClosed &= Edge.Value==2; }
  TestTrue(TEXT("瓦片和石沿倒角均完整闭合"),bClosed);
  TestTrue(TEXT("斜切构件法线与绕序一致"),bNormals);
  TestTrue(TEXT("瓦片与石沿各自跨面纹理连续"),bContinuous);
  TestTrue(TEXT("专用光照坐标无退化面"),bUVs);
  // 同列第一排与第二排必须在 Y 范围有真实搭接，并有逐排抬高的前缘。
  TArray<FBox> FirstColumn;
  const double XMin=Roof->bLeftStone?Roof->EdgeWidth-2:0;
  for (const auto& Piece : TileBounds)
  { if (Piece.Value.GetSize().X<Roof->SizeHandle.X*.5 && FMath::IsNearlyEqual(Piece.Value.Min.X,XMin,.001)) { FirstColumn.Add(Piece.Value); } }
  FirstColumn.Sort([](const FBox& L,const FBox& R) { return L.Min.Y<R.Min.Y; });
  TestEqual(TEXT("第一列瓦片与排数相符"),FirstColumn.Num(),Roof->GeneratedRowCount);
  if (FirstColumn.Num()>1)
  { TestTrue(TEXT("相邻排瓦片存在实体搭接"),FirstColumn[0].Max.Y>FirstColumn[1].Min.Y+1); }
#endif
 }
 return true;
}

/** 验证双轴调节、移动缓存、碰撞开关和异常输入保护。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxRoofEditingTest,"LxARPG.World.TileRoof.Editing",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxRoofEditingTest::RunTest(const FString& Parameters)
{
 UWorld* World=UWorld::CreateWorld(EWorldType::Game,false);
 ON_SCOPE_EXIT { if (World) { World->DestroyWorld(false); } };
 if (!TestNotNull(TEXT("测试世界"),World)) { return false; }
 ALxTileRoofActor* Roof=World->SpawnActor<ALxTileRoofActor>();
 const int32 Builds=Roof->GeometryBuildCount;
 for (int32 I=0; I<20; ++I) { Roof->SetActorLocation(FVector(I*30,0,0)); Roof->OnConstruction(Roof->GetActorTransform()); }
 TestEqual(TEXT("整体移动不重新铺瓦"),Roof->GeometryBuildCount,Builds);
 Roof->bEnableCollision=false; Roof->OnConstruction(Roof->GetActorTransform());
 TestEqual(TEXT("碰撞开关不重新生成"),Roof->GeometryBuildCount,Builds);
 TestEqual(TEXT("碰撞关闭"),Roof->RoofMesh->GetCollisionEnabled(),ECollisionEnabled::NoCollision);
 Roof->SizeHandle=FVector(1100,430,0); Roof->RebuildRoof();
 const FBox Bounds=Roof->RoofMesh->GetStaticMesh()->GetBoundingBox();
 TestTrue(TEXT("两个水平方向尺寸独立生效"),FMath::IsNearlyEqual(Bounds.Max.X,1100.0,.001) && FMath::IsNearlyEqual(Bounds.Max.Y,430.0,.001));
 Roof->UnregisterAllComponents(); Roof->RoofMesh->SetStaticMesh(nullptr); Roof->RegisterAllComponents();
 TestNotNull(TEXT("重新注册恢复网格"),Roof->RoofMesh->GetStaticMesh().Get());
 Roof->bEnableCollision=true; Roof->SizeHandle=FVector(20000,20000,0); Roof->TileWidth=20; Roof->TileLength=30; Roof->RebuildRoof();
 TestNull(TEXT("预算超限清除旧网格"),Roof->RoofMesh->GetStaticMesh());
 TestEqual(TEXT("预算超限清除旧碰撞"),Roof->RoofMesh->GetCollisionEnabled(),ECollisionEnabled::NoCollision);
 Roof->SizeHandle.X=std::numeric_limits<double>::quiet_NaN(); Roof->RebuildRoof();
 TestFalse(TEXT("无效输入有中文提示"),Roof->GenerationDiagnostic.IsEmpty());
 return true;
}
/** 验证双坡对称、中央屋脊唯一、侧墙内凹及左右独立关闭。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxRoofGableTest,"LxARPG.World.TileRoof.GableAndSideWalls",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxRoofGableTest::RunTest(const FString& Parameters)
{
 UWorld* World=UWorld::CreateWorld(EWorldType::Game,false);
 ON_SCOPE_EXIT { if (World) { World->DestroyWorld(false); } };
 if (!TestNotNull(TEXT("测试世界"),World)) { return false; }
 ALxTileRoofActor* Roof=World->SpawnActor<ALxTileRoofActor>();
 if (!TestNotNull(TEXT("屋顶实例"),Roof)) { return false; }
 for (bool bGable : {false,true}) for (bool bClose : {false,true}) for (int32 Mask=0; Mask<8; ++Mask)
 {
  Roof->RoofType=bGable?ELxRoofType::Gable:ELxRoofType::SingleSlope;
  Roof->bCloseSides=bClose; Roof->bLeftStone=(Mask&1)!=0; Roof->bRightStone=(Mask&2)!=0; Roof->bTopStone=(Mask&4)!=0;
  Roof->SizeHandle=FVector(823,917,0); Roof->SideWallInset=4; Roof->SideWallDrop=15; Roof->RebuildRoof();
  UStaticMesh* Mesh=Roof->RoofMesh->GetStaticMesh();
  if (!TestNotNull(TEXT("生成完整屋顶"),Mesh)) { return false; }
  const int32 Planes=bGable?2:1,Sides=int(Roof->bLeftStone)+int(Roof->bRightStone);
  TestEqual(TEXT("侧墙及坡面分别使用贴合的简单碰撞"),Mesh->GetBodySetup()->AggGeom.GetElementCount(),Planes*(1+Sides+(bClose?Sides:0))+int(Roof->bTopStone));
  const FBox Bounds=Mesh->GetBoundingBox();
  TestTrue(TEXT("完整屋顶使用总宽深"),FMath::IsNearlyEqual(Bounds.Max.X,823.0,.001) && FMath::IsNearlyEqual(Bounds.Max.Y,917.0,.001));
  const double Slope=FMath::Tan(FMath::DegreesToRadians(Roof->SlopeAngle));
  int32 WallCount=0;
  for (const FKConvexElem& Shape : Mesh->GetBodySetup()->AggGeom.ConvexElems)
  {
   FBox B(ForceInit); for (const FVector& P : Shape.VertexData) { B+=P; }
   if (B.Min.Z < -Roof->BackingThickness-1)
   {
    ++WallCount;
    TestTrue(TEXT("墙身低至檐口以下并沿坡面封满"),FMath::IsNearlyEqual(B.Min.Z,-Roof->BackingThickness-15,.001)
     && B.Max.Z>917.0/Planes*Slope);
    TestTrue(TEXT("墙身比石沿两面各内凹四厘米"),FMath::IsNearlyEqual(B.GetSize().X,Roof->EdgeWidth-8,.001)
     && (FMath::IsNearlyEqual(B.Min.X,4.0,.001) || FMath::IsNearlyEqual(B.Max.X,819.0,.001)));
   }
  }
  TestEqual(TEXT("侧墙开关不遗留碰撞"),WallCount,bClose?Sides*Planes:0);
#if WITH_EDITOR
  if (bGable)
  {
   const FMeshDescription* D=Mesh->GetMeshDescription(0); FStaticMeshConstAttributes A(*D);
   double AreaFront=0,AreaBack=0; bool bFacing=true;
   for (FTriangleID T : D->Triangles().GetElementIDs())
   {
    const auto C=D->GetTriangleVertexInstances(T); FVector P[3];
    for (int32 I=0; I<3; ++I) { P[I]=FVector(A.GetVertexPositions()[D->GetVertexInstanceVertex(C[I])]); }
    const FVector Cross=FVector::CrossProduct(P[2]-P[0],P[1]-P[0]);
    bFacing &= Cross.Size()>1.e-8 && FVector::DotProduct(Cross.GetSafeNormal(),FVector(A.GetVertexInstanceNormals()[C[0]]))>.99;
    if (D->GetTrianglePolygonGroup(T).GetValue()==0
     && !(FMath::IsNearlyEqual(P[0].Y,458.5,.001) && FMath::IsNearlyEqual(P[1].Y,458.5,.001) && FMath::IsNearlyEqual(P[2].Y,458.5,.001)))
    { if ((P[0].Y+P[1].Y+P[2].Y)/3<458.5) { AreaFront+=Cross.Size(); } else { AreaBack+=Cross.Size(); } }
   }
   TestTrue(TEXT("镜像坡面法线和绕序一致，无退化面"),bFacing);
   TestTrue(TEXT("两个坡面的瓦片几何面积对称"),FMath::IsNearlyEqual(AreaFront,AreaBack,AreaFront*.00001));
   TestEqual(TEXT("仅有一条中央屋脊碰撞"),Mesh->GetBodySetup()->AggGeom.BoxElems.Num(),int(Roof->bTopStone));
  }
#endif
 }
 return true;
}
#endif
