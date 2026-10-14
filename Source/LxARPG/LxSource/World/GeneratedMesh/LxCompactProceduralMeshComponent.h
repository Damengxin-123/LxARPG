#pragma once

#include "CoreMinimal.h"
#include "ProceduralMeshComponent.h"
#include "IO/IoHash.h"
#include "LxCompactProceduralMeshComponent.generated.h"

/** 自动生成网格的构建、缓存及关卡保存分段耗时日志。 */
LXARPG_API DECLARE_LOG_CATEGORY_EXTERN(LogLxGeneratedMeshSave, Log, All);

/** 保留原程序化组件接口与碰撞，磁盘使用无损紧凑缓存，PIE 使用独立的批量内存复制。 */
UCLASS(ClassGroup=(Rendering), meta=(BlueprintSpawnableComponent, DisplayName="紧凑程序化网格", Category="生成优化"))
class LXARPG_API ULxCompactProceduralMeshComponent : public UProceduralMeshComponent
{
 GENERATED_BODY()
public:
 /** 沿用引擎组件初始化与完整网格操作接口。 */
 ULxCompactProceduralMeshComponent(const FObjectInitializer& ObjectInitializer);
 /** 磁盘保存使用紧凑缓存，PIE 批量复制数值缓冲；旧文件、普通复制与撤销保持兼容。 */
 virtual void Serialize(FArchive& Ar) override;
 /** 最近一次保存的未压缩二进制数据大小，不包括材质及碰撞对象。 */
 UPROPERTY(VisibleAnywhere, Transient, Category="生成优化|保存", meta=(DisplayName="网格二进制原始字节数"))
 int64 CompactRawBytes = 0;
 /** 最近一次保存或读取的无损压缩数据大小。 */
 UPROPERTY(VisibleAnywhere, Transient, Category="生成优化|保存", meta=(DisplayName="网格压缩缓存字节数"))
 int64 CompactStoredBytes = 0;
 /** 几何未改变时复用已压缩数据，该计数不会因重复保存增加。 */
 UPROPERTY(VisibleAnywhere, Transient, Category="生成优化|保存", meta=(DisplayName="网格缓存压缩次数"))
 int32 CompactCompressionCount = 0;
 /** 当前实例从旧属性格式读取，下一次保存自动转换为紧凑格式。 */
 UPROPERTY(VisibleAnywhere, Transient, Category="生成优化|保存", meta=(DisplayName="已读取旧格式网格"))
 bool bLoadedLegacyGeometry = false;
private:
 /** 连续保存时复用的压缩缓存，不参加属性序列化或对象引用收集。 */
 TArray<uint8> CompressedGeometry;
 /** 完整无损二进制的摘要，检测直接通过引擎接口或蓝图进行的网格修改。 */
 FIoHash GeometryHash;
 /** 最近缓存的校验值，读取时先验证完整性再替换全部分区。 */
 uint32 GeometryCRC = 0;
 /** 当前压缩缓存是否已经对应一次有效的完整网格快照。 */
 bool bHasCompressedGeometry = false;
 /** 将当前分区打包为固定格式，所有坐标与纹理坐标仍保留双精度。 */
 bool PackGeometry(TArray<uint8>& Bytes);
 /** 校验并一次性恢复全部分区，沿用已序列化的边界与碰撞对象。 */
 bool RestoreGeometry(const TArray<uint8>& Bytes);
};
