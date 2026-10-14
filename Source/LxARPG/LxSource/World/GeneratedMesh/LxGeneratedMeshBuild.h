#pragma once

#include "CoreMinimal.h"
#include "LxGeneratedStaticMesh.h"

class UStaticMesh;
class UStaticMeshComponent;
class UWorld;
class AActor;
struct FMeshDescription;

/** 自动场景网格共用的构建策略，编辑器生成 Nanite，打包后的动态生成保留普通网格回退。 */
namespace LxGeneratedMeshBuild
{
 /** 构建版本参与形状缓存，修改构建策略时递增以重新生成旧关卡缓存。 */
 inline constexpr double CacheVersion = 3.0;
 /** 标记轻量编辑状态；以固定间隔合并请求，连续输入也能显示新形状。 */
 LXARPG_API void RequestPreview(AActor* Actor);
 /** 返回该对象是否使用简化编辑几何；游戏世界始终使用完整几何。 */
 LXARPG_API bool IsPreview(const AActor* Actor);
 /** 保存或运行前退出预览并取消尚未提交的刷新请求。 */
 LXARPG_API void EndPreview(AActor* Actor);
 /** 配置 Nanite 与完整回退网格，避免不支持 Nanite 的平台损失几何细节。 */
 LXARPG_API void ConfigureNanite(UStaticMesh* Mesh, bool bEnableNanite);
 /** 编辑时构建快速预览；显式完成时才生成 Nanite，保留法线、颜色及纹理坐标。 */
 LXARPG_API bool Build(UStaticMesh* Mesh, const FMeshDescription& Description, bool bEnableNanite, bool bFinalize = false);
 /** 将待构建预览升级为完整网格；已完成时直接复用。 */
 LXARPG_API bool Finalize(UStaticMesh* Mesh);
 /** 在运行或保存前提交当前生成器参数，并完成世界中待构建网格。 */
 LXARPG_API void PrepareWorld(UWorld* World);
 /** 仅记录生成器输入与样条控制点；可排除世界变换及预览标记，以复用移动中的完整本地几何。 */
 LXARPG_API TArray<uint8> CaptureInputs(const AActor* Actor, bool bIncludeWorldTransform = true, bool bIncludePreviewState = true);
 /** 材质不支持 Nanite 时仅让当前组件回退，恢复兼容材质后重新启用。 */
 LXARPG_API void RefreshMaterialCompatibility(UStaticMeshComponent* Component);
}
