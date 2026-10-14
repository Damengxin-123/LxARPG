#pragma once
#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"
#include "LxGeneratedMeshRenderComponent.generated.h"

class UProceduralMeshComponent;

/** 自动场景的持久化渲染缓存：原程序化组件保留碰撞，静态网格负责显示和投影。 */
UCLASS(ClassGroup=(Rendering), meta=(DisplayName="生成网格渲染优化"))
class LXARPG_API ULxGeneratedMeshRenderComponent : public UStaticMeshComponent
{
 GENERATED_BODY()
public:
 /** 创建不重复产生碰撞和导航的渲染组件。 */
 ULxGeneratedMeshRenderComponent();
 /** 编辑时使用程序化预览，在保存及运行前构建 Nanite；可关闭以保持程序化渲染。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="生成优化", meta=(DisplayName="启用Nanite渲染优化"))
 bool bEnableNanite = true;
 /** 根据源网格更新缓存；相同几何只刷新材质、可见性和阴影开关。 */
 void Synchronize(UProceduralMeshComponent* Source, bool bFinalize = false);
 /** 比较轻量输入，捕获尚未触发构造回调的最后一次参数或样条修改。 */
 bool IsSourceCurrent() const;
 /** 外部直接修改程序化源后使缓存失效；生成器内部重建也使用此入口。 */
 UFUNCTION(BlueprintCallable, Category="生成优化", meta=(DisplayName="标记源网格已修改"))
 void MarkSourceDirty();
 /** 确认整体变换或连接检查未改变源几何，仅更新输入快照，不触发静态转换。 */
 void AcceptUnchangedSource();
 /** 本次会话收到的源几何更新次数，用于确认未修改对象没有重复生成。 */
 UPROPERTY(Transient, VisibleAnywhere, Category="生成优化", meta=(DisplayName="源几何更新次数"))
 int32 SourceUpdateCount = 0;
 /** 本次会话实际转换网格描述的次数，不包含轻量输入比较。 */
 UPROPERTY(Transient, VisibleAnywhere, Category="生成优化", meta=(DisplayName="网格转换次数"))
 int32 GeometryConversionCount = 0;
 /** 手动完成当前源网格的静态转换，保存和运行前也会自动调用。 */
 UFUNCTION(BlueprintCallable, CallInEditor, Category="生成优化", meta=(DisplayName="完成静态网格构建"))
 void FinalizeMesh();
 /** 保存和烹饪前转换最新程序化形状。 */
 virtual void PreSave(FObjectPreSaveContext SaveContext) override;
 /** 当前编辑预览是否等待转换；用于编辑器诊断并随实例序列化。 */
 UPROPERTY(VisibleAnywhere, Category="生成优化", meta=(DisplayName="等待静态网格构建"))
 bool bPendingMeshBuild = true;
 /** 编辑器加载旧关卡时自动补齐尚未保存的优化缓存。 */
 virtual void OnRegister() override;
 /** 旧关卡或运行时新建组件没有缓存时恢复可用的渲染路径。 */
 virtual void BeginPlay() override;
private:
 /** 源几何对应的生成参数；持久化后重载和进入游戏无需重新生成。 */
 UPROPERTY(VisibleAnywhere, AdvancedDisplay, meta=(DisplayName="源几何输入快照", Category="生成优化"))
 TArray<uint8> SourceInputs;
 /** 应用已有缓存的材质、显示和阴影，不访问源顶点。 */
 void ApplyCachedRendering(UProceduralMeshComponent* Source);
 /** 随关卡保存的网格，实例复制时保持正确的归属关系。 */
 UPROPERTY(Instanced, VisibleAnywhere, AdvancedDisplay, Category="生成优化", meta=(DisplayName="渲染几何缓存"))
 TObjectPtr<UStaticMesh> GeneratedMeshCache;
 /** 包含顶点、索引、法线、颜色与全部纹理坐标的摘要。 */
 UPROPERTY(VisibleAnywhere, AdvancedDisplay, Category="生成优化", meta=(DisplayName="几何缓存摘要"))
 FString CachedGeometryHash;
};
