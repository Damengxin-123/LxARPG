#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "Materials/Material.h"
#include "UObject/PropertyPortFlags.h"
#include "LxARPG/LxSource/World/GeneratedMesh/LxCompactProceduralMeshComponent.h"

namespace
{
 /** 记录版本与对象路径，模拟磁盘持久化归档，不依赖组件私有缓存。 */
 struct FMeshSaveSnapshot
 {
  /** 实際持久化字节与使用的自定义版本。 */
  TArray<uint8> Bytes;
  /** 读取新格式时使用与写入一致的版本表。 */
  FCustomVersionContainer Versions;
 };
 /** 通过组件公开接口生成含四组纹理坐标、颜色、切线及隐藏分区的测试几何。 */
 ULxCompactProceduralMeshComponent* MakeTestMesh()
 {
  auto* Mesh=NewObject<ULxCompactProceduralMeshComponent>();
  FProcMeshSection Section;
  for(int32 I=0;I<4;++I)
  {
   FProcMeshVertex Vertex;
   Vertex.Position=FVector(I*113.123456789,I%2*107.987654321,I*0.123456789);
   Vertex.Normal=FVector(0.123456789,0.234567891,0.962345678);
   Vertex.Tangent=FProcMeshTangent(FVector(0.987654321,0.123456789,0.012345678),I%2!=0);
   Vertex.Color=FColor(17+I,71+I,137+I,201+I);
   Vertex.UV0={I*0.123456789,I*0.987654321}; Vertex.UV1={I+0.345678901,I+0.456789012};
   Vertex.UV2={I+0.567890123,I+0.678901234}; Vertex.UV3={I+0.789012345,I+0.890123456};
   Section.SectionLocalBox+=Vertex.Position;
   Section.ProcVertexBuffer.Add(Vertex);
  }
  Section.ProcIndexBuffer={0,1,2,2,1,3}; Section.bEnableCollision=true; Section.bSectionVisible=false;
  Mesh->SetProcMeshSection(0,Section);
  Mesh->SetProcMeshSection(2,Section); // 中间空分区也必须保留，不能改变材质槽编号。
  Mesh->SetMaterial(2,UMaterial::GetDefaultMaterial(MD_Surface));
  Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
  Mesh->bUseComplexAsSimpleCollision=true;
  return Mesh;
 }
 /** 独立检查每个公开网格字段的精确值，避免仅比较压缩数据自身。 */
 bool EqualGeometry(UProceduralMeshComponent* A,UProceduralMeshComponent* B)
 {
  if(A->GetNumSections()!=B->GetNumSections()) return false;
  for(int32 S=0;S<A->GetNumSections();++S)
  {
   const FProcMeshSection& X=*A->GetProcMeshSection(S); const FProcMeshSection& Y=*B->GetProcMeshSection(S);
   if(X.bEnableCollision!=Y.bEnableCollision || X.bSectionVisible!=Y.bSectionVisible
    || X.SectionLocalBox!=Y.SectionLocalBox || X.ProcIndexBuffer!=Y.ProcIndexBuffer
    || X.ProcVertexBuffer.Num()!=Y.ProcVertexBuffer.Num()) return false;
   for(int32 V=0;V<X.ProcVertexBuffer.Num();++V)
   {
    const FProcMeshVertex& P=X.ProcVertexBuffer[V]; const FProcMeshVertex& Q=Y.ProcVertexBuffer[V];
    if(P.Position!=Q.Position || P.Normal!=Q.Normal || P.Color!=Q.Color || P.Tangent.TangentX!=Q.Tangent.TangentX
     || P.Tangent.bFlipTangentY!=Q.Tangent.bFlipTangentY || P.UV0!=Q.UV0 || P.UV1!=Q.UV1 || P.UV2!=Q.UV2 || P.UV3!=Q.UV3) return false;
   }
  }
  return true;
 }
 /** 真正调用持久化序列化，旧格式分支模拟未带自定义版本的原组件数据。 */
 FMeshSaveSnapshot SaveSnapshot(ULxCompactProceduralMeshComponent* Mesh,bool bLegacy=false,bool bTransaction=false)
 {
  FMeshSaveSnapshot Saved;
  FMemoryWriter Writer(Saved.Bytes,true); Writer.SetIsTransacting(bTransaction);
  FObjectAndNameAsStringProxyArchive Archive(Writer,false);
  if(bLegacy) Mesh->UProceduralMeshComponent::Serialize(Archive); else Mesh->Serialize(Archive);
  Saved.Versions=Writer.GetCustomVersions();
  return Saved;
 }
 /** 将磁盘数据读到全新组件，返回归档是否正确恢复结束。 */
 bool LoadSnapshot(ULxCompactProceduralMeshComponent* Mesh,const FMeshSaveSnapshot& Saved,bool bLegacy=false,bool bTransaction=false)
 {
  FMemoryReader Reader(Saved.Bytes,true); Reader.SetIsTransacting(bTransaction);
  FCustomVersionContainer Versions=Saved.Versions;
  // 仅移除项目缓存版本，仍保留场景组件等引擎属性所需的历史版本。
  if(bLegacy) Versions.SetVersion(FGuid(0x6DA44785,0xBF324120,0xA19C1B2D,0x05F2773E),-1,TEXT("LxCompactProceduralMesh"));
  Reader.SetCustomVersions(Versions);
  FObjectAndNameAsStringProxyArchive Archive(Reader,false);
  Mesh->Serialize(Archive);
  return !Archive.IsError() && Reader.Tell()==Reader.TotalSize();
 }
}

/** 验证无损恢复、旧格式迁移、材质碰撞、重复缓存、复制与撤销行为。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxCompactMeshRoundTrip,"LxARPG.World.GeneratedMesh.CompactRoundTrip",
 EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FLxCompactMeshRoundTrip::RunTest(const FString& Parameters)
{
 auto* Source=MakeTestMesh(); auto* Restored=NewObject<ULxCompactProceduralMeshComponent>();
 const FMeshSaveSnapshot Legacy=SaveSnapshot(Source,true);
 TestTrue(TEXT("读取未改造的原组件属性格式"),LoadSnapshot(Restored,Legacy,true));
 TestTrue(TEXT("旧格式完整几何保留"),EqualGeometry(Source,Restored));
 TestTrue(TEXT("旧格式标记自动迁移"),Restored->bLoadedLegacyGeometry);
 const FMeshSaveSnapshot Saved=SaveSnapshot(Restored);
 auto* Reloaded=NewObject<ULxCompactProceduralMeshComponent>();
 TestTrue(TEXT("新组件读取紧凑二进制缓存"),LoadSnapshot(Reloaded,Saved));
 TestTrue(TEXT("双精度坐标、法线、切线、颜色和四组纹理坐标无损恢复"),EqualGeometry(Source,Reloaded));
 TestEqual(TEXT("空分区及材质槽编号保留"),Reloaded->GetNumSections(),3);
 TestTrue(TEXT("材质引用保留"),Reloaded->GetMaterial(2)==Source->GetMaterial(2));
 TestEqual(TEXT("碰撞启用方式保留"),Reloaded->GetCollisionEnabled(),Source->GetCollisionEnabled());
 TestTrue(TEXT("碰撞对象保留"),Reloaded->ProcMeshBodySetup==Source->ProcMeshBodySetup);
 TestEqual(TEXT("新格式小于带属性标签的旧格式"),Saved.Bytes.Num()<Legacy.Bytes.Num(),true);
 SaveSnapshot(Reloaded); SaveSnapshot(Reloaded);
 TestEqual(TEXT("加载缓存重复保存无需重新压缩"),Reloaded->CompactCompressionCount,0);
 Reloaded->GetProcMeshSection(2)->ProcVertexBuffer[1].UV3.X+=0.000000001;
 const FMeshSaveSnapshot Edited=SaveSnapshot(Reloaded);
 TestEqual(TEXT("直接修改引擎网格接口也能检测并刷新缓存"),Reloaded->CompactCompressionCount,1);
 TestTrue(TEXT("重新加载编辑后的网格"),LoadSnapshot(Restored,Edited));
 TestTrue(TEXT("编辑后的完整数据仍然精确恢复"),EqualGeometry(Reloaded,Restored));
 auto* Copied=DuplicateObject<ULxCompactProceduralMeshComponent>(Restored,GetTransientPackage());
 TestTrue(TEXT("普通对象复制保留网格"),EqualGeometry(Copied,Restored));
 const FMeshSaveSnapshot Undo=SaveSnapshot(Restored,false,true);
 Restored->ClearAllMeshSections();
 TestTrue(TEXT("撤销归档读取成功"),LoadSnapshot(Restored,Undo,false,true));
 TestTrue(TEXT("撤销仍恢复全部编辑前分区"),EqualGeometry(Restored,Reloaded));
 auto* Empty=NewObject<ULxCompactProceduralMeshComponent>();
 TestTrue(TEXT("空网格缓存有效"),LoadSnapshot(Empty,SaveSnapshot(Empty)));
 return true;
}

/** 使用真正的 PIE 复制归档，验证双精度几何、独立缓冲、碰撞引用和保存缓存均保持正确。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxCompactMeshPIECopy,"LxARPG.World.GeneratedMesh.CompactPIECopy",
 EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FLxCompactMeshPIECopy::RunTest(const FString& Parameters)
{
 auto* Source=MakeTestMesh();
 const FMeshSaveSnapshot Before=SaveSnapshot(Source);
 const int32 Compressions=Source->CompactCompressionCount;
 FObjectDuplicationParameters CopyParams(Source,GetTransientPackage());
 CopyParams.DuplicateMode=EDuplicateMode::PIE;
 CopyParams.PortFlags|=PPF_DuplicateForPIE;
 auto* Copied=Cast<ULxCompactProceduralMeshComponent>(StaticDuplicateObjectEx(CopyParams));
 if(!TestNotNull(TEXT("PIE 复制返回完整组件"),Copied)) return false;
 TestTrue(TEXT("PIE 无损复制位置、法线、切线、颜色、四组纹理坐标、边界和分区状态"),EqualGeometry(Source,Copied));
 TestEqual(TEXT("PIE 保留中间空分区"),Copied->GetNumSections(),3);
 TestTrue(TEXT("PIE 保留材质资产引用"),Copied->GetMaterial(2)==Source->GetMaterial(2));
 TestEqual(TEXT("PIE 保留碰撞启用方式"),Copied->GetCollisionEnabled(),Source->GetCollisionEnabled());
 TestEqual(TEXT("PIE 保留复杂碰撞方式"),Copied->bUseComplexAsSimpleCollision,Source->bUseComplexAsSimpleCollision);
 TestTrue(TEXT("PIE 正确复制并重映射碰撞对象"),Copied->ProcMeshBodySetup && Copied->ProcMeshBodySetup!=Source->ProcMeshBodySetup);
 TestTrue(TEXT("游戏顶点缓冲与编辑器缓冲独立"),Copied->GetProcMeshSection(0)->ProcVertexBuffer.GetData()!=Source->GetProcMeshSection(0)->ProcVertexBuffer.GetData());
 TestTrue(TEXT("游戏索引缓冲与编辑器缓冲独立"),Copied->GetProcMeshSection(0)->ProcIndexBuffer.GetData()!=Source->GetProcMeshSection(0)->ProcIndexBuffer.GetData());
 const FVector OriginalPosition=Source->GetProcMeshSection(0)->ProcVertexBuffer[0].Position;
 // 游戏副本沿内部引擎坐标移动一米，不能写回编辑器源几何。
 Copied->GetProcMeshSection(0)->ProcVertexBuffer[0].Position.X+=100.00;
 TestEqual(TEXT("游戏编辑不会改变源顶点"),Source->GetProcMeshSection(0)->ProcVertexBuffer[0].Position,OriginalPosition);
 TestEqual(TEXT("PIE 不触发源缓存压缩"),Source->CompactCompressionCount,Compressions);
 TestTrue(TEXT("PIE 后磁盘保存内容完全相同"),Before.Bytes==SaveSnapshot(Source).Bytes);
 TestEqual(TEXT("PIE 后重复保存仍复用压缩缓存"),Source->CompactCompressionCount,Compressions);
 // 直接修改尚未保存的高精度纹理坐标，复制必须读取当前数组而非上次保存缓存。
 Source->GetProcMeshSection(2)->ProcVertexBuffer[1].UV3.X+=0.000000001;
 auto* Current=Cast<ULxCompactProceduralMeshComponent>(StaticDuplicateObjectEx(CopyParams));
 TestTrue(TEXT("未保存的修改也完整进入 PIE"),Current && EqualGeometry(Source,Current));
 auto* Empty=NewObject<ULxCompactProceduralMeshComponent>();
 FObjectDuplicationParameters EmptyParams(Empty,GetTransientPackage());
 EmptyParams.DuplicateMode=EDuplicateMode::PIE; EmptyParams.PortFlags|=PPF_DuplicateForPIE;
 auto* EmptyCopy=Cast<ULxCompactProceduralMeshComponent>(StaticDuplicateObjectEx(EmptyParams));
 TestTrue(TEXT("空程序化网格也正确进入 PIE"),EmptyCopy && EmptyCopy->GetNumSections()==0);
 return true;
}

/** 验证损坏的缓存不悄悄产生部分网格，读入失败时保留原来的有效分区。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxCompactMeshIntegrity,"LxARPG.World.GeneratedMesh.CompactIntegrity",
 EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FLxCompactMeshIntegrity::RunTest(const FString& Parameters)
{
 auto* Mesh=MakeTestMesh(); FMeshSaveSnapshot Saved=SaveSnapshot(Mesh);
 // 只损坏缓存头的 CRC，压缩流保持完整，从而单独验证本组件的校验与原子恢复。
 Saved.Bytes[Saved.Bytes.Num()-int32(Mesh->CompactStoredBytes)-4]^=0x5a;
 AddExpectedError(TEXT("网格缓存恢复失败"),EAutomationExpectedErrorFlags::Contains,1);
 TestFalse(TEXT("损坏缓存读取明确失败"),LoadSnapshot(Mesh,Saved));
 TestTrue(TEXT("恢复失败不会清空原有网格"),EqualGeometry(Mesh,MakeTestMesh()));
 return true;
}
#endif
