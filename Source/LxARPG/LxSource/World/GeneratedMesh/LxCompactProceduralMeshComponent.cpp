#include "LxCompactProceduralMeshComponent.h"
#include "Serialization/ArchiveProxy.h"
#include "Serialization/CustomVersion.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "UObject/UnrealType.h"
#include "Misc/Compression.h"
#include "Misc/Crc.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
#include <type_traits>

DEFINE_LOG_CATEGORY(LogLxGeneratedMeshSave);

namespace
{
 /** 格式版本与引擎包版本独立；旧关卡没有该版本时继续读取原属性数据。 */
 const FGuid CompactMeshVersionGuid(0x6DA44785,0xBF324120,0xA19C1B2D,0x05F2773E);
 /** 第一版保留全部双精度数据，采用固定字段顺序及 Zlib 无损压缩。 */
 constexpr int32 CompactMeshFormatVersion = 1;
 /** 每个顶点包含九个坐标、四组二维纹理坐标、颜色与切线方向标记。 */
 constexpr int32 PackedVertexBytes = 141;
 /** 注册自定义版本，让旧文件迁移、烹饪与重新加载识别正确格式。 */
 FCustomVersionRegistration CompactMeshVersion(CompactMeshVersionGuid,CompactMeshFormatVersion,TEXT("LxCompactProceduralMesh"));

 /** 只跳过继承的网格分区属性，材质、变换、边界和凸碰撞仍由引擎保存。 */
 class FCompactMeshArchive final : public FArchiveProxy
 {
 public:
  /** 包装当前保存或复制归档，不修改全局反射属性标记。 */
  explicit FCompactMeshArchive(FArchive& Inner) : FArchiveProxy(Inner) {}
  /** 消除逐顶点属性标签，保留其余所有组件属性。 */
  virtual bool ShouldSkipProperty(const FProperty* Property) const override
  {
   return (Property && Property->GetFName()==TEXT("ProcMeshSections")
    && Property->GetOwnerClass()==UProceduralMeshComponent::StaticClass()) || InnerArchive.ShouldSkipProperty(Property);
  }
 };

 /** 通过原生反射属性访问私有分区数组，不反复重建物理或渲染状态。 */
 TArray<FProcMeshSection>& MeshSections(UProceduralMeshComponent* Component)
 {
  static const FArrayProperty* Property=FindFProperty<FArrayProperty>(UProceduralMeshComponent::StaticClass(),TEXT("ProcMeshSections"));
  check(Property);
  return *Property->ContainerPtrToValuePtr<TArray<FProcMeshSection>>(Component);
 }

 /** PIE 在同一进程内复制纯数值缓冲，不压缩、不访问磁盘缓存，也不重建碰撞。 */
 void SerializePIEGeometry(UProceduralMeshComponent* Component,FArchive& Ar)
 {
  TRACE_CPUPROFILER_EVENT_SCOPE(LxGeneratedMesh_CopyPIEGeometry);
  // 只有不含对象引用且可直接复制的顶点布局才能使用此同进程传输方式。
  static_assert(std::is_trivially_copyable_v<FProcMeshVertex>);
  TArray<FProcMeshSection>& Sections=MeshSections(Component);
  int32 Count=Sections.Num(); Ar << Count;
  if(Ar.IsError() || Count<0
   || (Ar.IsLoading() && int64(Count)*int64(sizeof(int32))*2>Ar.TotalSize()-Ar.Tell()))
  { Ar.SetError(); return; }
  if(Ar.IsLoading()) Sections.SetNum(Count);
  for(FProcMeshSection& Section:Sections)
  {
   int32 Vertices=Section.ProcVertexBuffer.Num(),Indices=Section.ProcIndexBuffer.Num();
   Ar << Vertices << Indices << Section.SectionLocalBox << Section.bEnableCollision << Section.bSectionVisible;
   const int64 VertexBytes=int64(Vertices)*int64(sizeof(FProcMeshVertex)),IndexBytes=int64(Indices)*int64(sizeof(uint32));
   if(Ar.IsError() || Vertices<0 || Indices<0
    || (Ar.IsLoading() && VertexBytes+IndexBytes>Ar.TotalSize()-Ar.Tell()))
   { Ar.SetError(); return; }
   if(Ar.IsLoading())
   {
    Section.ProcVertexBuffer.SetNumUninitialized(Vertices);
    Section.ProcIndexBuffer.SetNumUninitialized(Indices);
   }
   // 目标数组独立分配，游戏内修改几何不会影响编辑器源数据；所有双精度字段原样保留。
   if(VertexBytes) Ar.Serialize(Section.ProcVertexBuffer.GetData(),VertexBytes);
   if(IndexBytes) Ar.Serialize(Section.ProcIndexBuffer.GetData(),IndexBytes);
   if(Ar.IsError()) return;
  }
 }

 /** 显式记录三个双精度分量，避免缓存格式依赖结构体填充或引擎坐标版本。 */
 void SerializeVector(FArchive& Ar,FVector& Value) { Ar << Value.X << Value.Y << Value.Z; }
 /** 显式记录两个双精度分量，保留原始纹理坐标。 */
 void SerializeUV(FArchive& Ar,FVector2D& Value) { Ar << Value.X << Value.Y; }
 /** 固定顺序记录顶点全部字段，不进行精度缩减或颜色空间转换。 */
 void SerializeVertex(FArchive& Ar,FProcMeshVertex& Vertex)
 {
  SerializeVector(Ar,Vertex.Position); SerializeVector(Ar,Vertex.Normal); SerializeVector(Ar,Vertex.Tangent.TangentX);
  uint8 Flip=Vertex.Tangent.bFlipTangentY?1:0;
  Ar << Flip << Vertex.Color.R << Vertex.Color.G << Vertex.Color.B << Vertex.Color.A;
  if(Ar.IsLoading()) Vertex.Tangent.bFlipTangentY=Flip!=0;
  SerializeUV(Ar,Vertex.UV0); SerializeUV(Ar,Vertex.UV1); SerializeUV(Ar,Vertex.UV2); SerializeUV(Ar,Vertex.UV3);
 }
}

ULxCompactProceduralMeshComponent::ULxCompactProceduralMeshComponent(const FObjectInitializer& ObjectInitializer)
 : Super(ObjectInitializer) {}

bool ULxCompactProceduralMeshComponent::PackGeometry(TArray<uint8>& Bytes)
{
 TRACE_CPUPROFILER_EVENT_SCOPE(LxGeneratedMesh_PackBinary);
 FMemoryWriter Writer(Bytes,true);
 TArray<FProcMeshSection>& Sections=MeshSections(this);
 int32 Count=Sections.Num(); Writer << Count;
 for(FProcMeshSection& Section:Sections)
 {
  int32 Vertices=Section.ProcVertexBuffer.Num(),Indices=Section.ProcIndexBuffer.Num();
  Writer << Vertices << Indices;
  uint8 Collision=Section.bEnableCollision?1:0,Visible=Section.bSectionVisible?1:0,Valid=Section.SectionLocalBox.IsValid;
  Writer << Collision << Visible << Valid;
  SerializeVector(Writer,Section.SectionLocalBox.Min); SerializeVector(Writer,Section.SectionLocalBox.Max);
  for(FProcMeshVertex& Vertex:Section.ProcVertexBuffer) SerializeVertex(Writer,Vertex);
  // 索引按原来的三角形绕序直接保存，归档负责跨字节序转换。
  for(uint32& Index:Section.ProcIndexBuffer) Writer << Index;
 }
 return !Writer.IsError();
}

bool ULxCompactProceduralMeshComponent::RestoreGeometry(const TArray<uint8>& Bytes)
{
 TRACE_CPUPROFILER_EVENT_SCOPE(LxGeneratedMesh_RestoreBinary);
 FMemoryReader Reader(Bytes,true);
 if(Bytes.Num()<4) return false;
 int32 Count=0; Reader << Count;
 if(Count<0 || Count>65536 || int64(Count)*59>Reader.TotalSize()-Reader.Tell()) return false;
 TArray<FProcMeshSection> Restored; Restored.SetNum(Count);
 for(FProcMeshSection& Section:Restored)
 {
  if(Reader.TotalSize()-Reader.Tell()<59) return false;
  int32 Vertices=0,Indices=0; uint8 Collision=0,Visible=0,Valid=0;
  Reader << Vertices << Indices << Collision << Visible << Valid;
  SerializeVector(Reader,Section.SectionLocalBox.Min); SerializeVector(Reader,Section.SectionLocalBox.Max);
  const int64 Required=int64(Vertices)*PackedVertexBytes+int64(Indices)*sizeof(uint32);
  if(Vertices<0 || Indices<0 || Indices%3!=0 || Required>Reader.TotalSize()-Reader.Tell()
   || Collision>1 || Visible>1 || Valid>1) return false;
  Section.bEnableCollision=Collision!=0; Section.bSectionVisible=Visible!=0; Section.SectionLocalBox.IsValid=Valid;
  Section.ProcVertexBuffer.SetNum(Vertices); Section.ProcIndexBuffer.SetNum(Indices);
  for(FProcMeshVertex& Vertex:Section.ProcVertexBuffer) SerializeVertex(Reader,Vertex);
  for(uint32& Index:Section.ProcIndexBuffer) { Reader << Index; if(Index>=uint32(Vertices)) return false; }
 }
 if(Reader.IsError() || Reader.Tell()!=Reader.TotalSize()) return false;
 // Serialize 在注册组件之前恢复全部分区；原边界、BodySetup 和凸碰撞照常加载。
 MeshSections(this)=MoveTemp(Restored);
 return true;
}

void ULxCompactProceduralMeshComponent::Serialize(FArchive& Ar)
{
 // 导出标记、引用收集及最终写入均登记同一版本，保证包头版本表与实际负载一致。
 const int32 StoredVersion=Ar.IsLoading()?Ar.CustomVer(CompactMeshVersionGuid):-1;
 Ar.UsingCustomVersion(CompactMeshVersionGuid);
 // 仅加速同进程 PIE；普通复制、磁盘保存、撤销和引用收集沿用原有路径。
 const bool bPIEGeometry=(Ar.GetPortFlags() & PPF_DuplicateForPIE)
  && (Ar.IsSaving() || Ar.IsLoading()) && !Ar.IsTransacting() && !Ar.IsObjectReferenceCollector()
  && !Ar.IsCountingMemory() && !Ar.IsTextFormat() && !Ar.ForceByteSwapping();
 if(bPIEGeometry)
 {
  FCompactMeshArchive PIEArchive(Ar);
  // 材质、组件属性和 BodySetup 仍由引擎序列化，保留正常的对象引用重映射。
  Super::Serialize(PIEArchive);
  if(PIEArchive.IsError()) { Ar.SetError(); return; }
  SerializePIEGeometry(this,Ar);
  return;
 }
 // 分区只含数值数据，无对象引用；保存导出标记与垃圾回收不必逐顶点遍历。
 // 材质、BodySetup 与其余反射属性继续经过原引擎的完整引用收集路径。
 if(Ar.IsObjectReferenceCollector() && !Ar.IsTransacting()
  && !(Ar.GetPortFlags() & (PPF_Duplicate | PPF_DuplicateForPIE)))
 {
  FCompactMeshArchive ReferenceArchive(Ar);
  Super::Serialize(ReferenceArchive);
  if(ReferenceArchive.IsError()) Ar.SetError();
  return;
 }
 const bool bDiskGeometry=Ar.IsPersistent() && !Ar.IsTransacting() && !Ar.IsObjectReferenceCollector()
  && !Ar.IsCountingMemory() && !Ar.IsTextFormat() && (Ar.IsSaving() || Ar.IsLoading())
  && !(Ar.GetPortFlags() & (PPF_Duplicate | PPF_DuplicateForPIE));
 if(!bDiskGeometry) { Super::Serialize(Ar); return; }
 if(Ar.IsLoading() && StoredVersion<0)
 {
  Super::Serialize(Ar);
  bLoadedLegacyGeometry=!MeshSections(this).IsEmpty();
  return;
 }
 const double Start=FPlatformTime::Seconds();
 FCompactMeshArchive CompactArchive(Ar);
 Super::Serialize(CompactArchive);
 const double PropertySeconds=FPlatformTime::Seconds()-Start;
 if(CompactArchive.IsError()) { Ar.SetError(); return; }
 int32 Format=CompactMeshFormatVersion,StoredBytes=0; int64 RawBytes=0; uint32 CRC=0;
 double PackingSeconds=0,HashSeconds=0,CompressionSeconds=0;
 if(Ar.IsSaving())
 {
  TArray<uint8> Raw;
  const double PackingStart=FPlatformTime::Seconds();
  if(!PackGeometry(Raw)) { Ar.SetError(); return; }
  PackingSeconds=FPlatformTime::Seconds()-PackingStart;
  const double HashStart=FPlatformTime::Seconds();
  const FIoHash Hash=FIoHash::HashBuffer(Raw.GetData(),Raw.Num());
  HashSeconds=FPlatformTime::Seconds()-HashStart;
  if(!bHasCompressedGeometry || GeometryHash!=Hash)
  {
   const double CompressionStart=FPlatformTime::Seconds();
   int32 Capacity=FCompression::CompressMemoryBound(NAME_Zlib,Raw.Num(),COMPRESS_BiasSpeed);
   CompressedGeometry.SetNumUninitialized(Capacity);
   if(!FCompression::CompressMemory(NAME_Zlib,CompressedGeometry.GetData(),Capacity,Raw.GetData(),Raw.Num(),COMPRESS_BiasSpeed))
   { bHasCompressedGeometry=false; Ar.SetError(); return; }
   CompressedGeometry.SetNum(Capacity,EAllowShrinking::No);
   GeometryHash=Hash; GeometryCRC=FCrc::MemCrc32(Raw.GetData(),Raw.Num()); bHasCompressedGeometry=true;
   ++CompactCompressionCount;
   CompressionSeconds=FPlatformTime::Seconds()-CompressionStart;
  }
  RawBytes=Raw.Num(); StoredBytes=CompressedGeometry.Num(); CRC=GeometryCRC;
 }
 Ar << Format << RawBytes << StoredBytes << CRC;
 if(Ar.IsError() || Format!=CompactMeshFormatVersion || RawBytes<4 || RawBytes>MAX_int32
  || StoredBytes<=0 || StoredBytes>MAX_int32 || (Ar.IsLoading() && int64(StoredBytes)>Ar.TotalSize()-Ar.Tell()))
 { Ar.SetError(); UE_LOG(LogLxGeneratedMeshSave,Error,TEXT("网格缓存头部无效：%s"),*GetPathName()); return; }
 if(Ar.IsLoading()) CompressedGeometry.SetNumUninitialized(StoredBytes);
 const double TransferStart=FPlatformTime::Seconds();
 Ar.Serialize(CompressedGeometry.GetData(),StoredBytes);
 const double TransferSeconds=FPlatformTime::Seconds()-TransferStart;
 if(Ar.IsError()) return;
 if(Ar.IsLoading())
 {
  TArray<uint8> Raw; Raw.SetNumUninitialized(int32(RawBytes));
  if(!FCompression::UncompressMemory(NAME_Zlib,Raw.GetData(),RawBytes,CompressedGeometry.GetData(),StoredBytes)
   || FCrc::MemCrc32(Raw.GetData(),Raw.Num())!=CRC || !RestoreGeometry(Raw))
  { bHasCompressedGeometry=false; Ar.SetError(); UE_LOG(LogLxGeneratedMeshSave,Error,TEXT("网格缓存恢复失败：%s"),*GetPathName()); return; }
  GeometryHash=FIoHash::HashBuffer(Raw.GetData(),Raw.Num()); GeometryCRC=CRC; bHasCompressedGeometry=true;
 }
 CompactRawBytes=RawBytes; CompactStoredBytes=StoredBytes;
 if(Ar.IsSaving() && RawBytes>=1024*1024)
  UE_LOG(LogLxGeneratedMeshSave,Display,TEXT("网格缓存保存 %s 原始=%.2fMB 压缩=%.2fMB 属性=%.2f秒 打包=%.2f秒 摘要=%.2f秒 压缩阶段=%.2f秒 缓冲写入=%.2f秒 合计=%.2f秒"),
   *GetPathName(),double(RawBytes)/1000000.0,double(StoredBytes)/1000000.0,PropertySeconds,PackingSeconds,HashSeconds,CompressionSeconds,TransferSeconds,FPlatformTime::Seconds()-Start);
}
