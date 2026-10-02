#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LxMenuPreviewActor.generated.h"

class USkeletalMeshComponent;
class UCameraComponent;
class UWorldPartitionStreamingSourceComponent;
struct FLxCharacterSaveRecord;

/** 只包含显示组件、镜头和流送源的菜单角色，没有背包、战斗或存档组件。 */
UCLASS(DisplayName="菜单角色展示")
class LXARPG_API ALxMenuPreviewActor : public AActor
{
	GENERATED_BODY()
public:
	/** 创建展示组件，允许在预览世界未开始游戏时更新。 */
	ALxMenuPreviewActor();
	/** 从角色快照设置外观与变换，返回资源是否可用于展示。 */
	bool Configure(const FLxCharacterSaveRecord& Record, const FTransform& FallbackTransform);
	/** 检查目标区域加载及编辑器材质编译是否完成。 */
	bool IsSceneReady() const;
	/** 激活实际镜头与角色后等待渲染资源稳定，再允许撤掉加载遮罩。 */
	bool IsPresentationReady();
	/** 在场景就绪后摆放防遮挡镜头并显示角色。 */
	void Reveal();
	/** 在未启动玩法的世界里单独驱动展示动画。 */
	virtual void Tick(float DeltaSeconds) override;
private:
	/** 当前镜头下资源持续就绪的开始时间。 */
	double PresentationReadySince = 0;
	/** 主体显示，不参与碰撞。 */
	UPROPERTY(VisibleAnywhere, Category="主菜单|展示", DisplayName="展示网格")
	TObjectPtr<USkeletalMeshComponent> Mesh;
	/** 菜单构图镜头。 */
	UPROPERTY(VisibleAnywhere, Category="主菜单|镜头", DisplayName="展示镜头")
	TObjectPtr<UCameraComponent> Camera;
	/** 远处角色位置的区域加载源。 */
	UPROPERTY(VisibleAnywhere, Category="主菜单|场景", DisplayName="场景流送源")
	TObjectPtr<UWorldPartitionStreamingSourceComponent> StreamingSource;
};
