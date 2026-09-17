#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxUIBaseObject.h"
#include "LxARPG/LxSource/Model/Quest/DataType/LxQuestRuntimeData.h"
#include "LxARPG/LxSource/Model/Quest/DataType/LxQuestSeriesAsset.h"
#include "LxQuestWidgetBase.generated.h"

/** 任务详情显示快照，包含静态内容与当前角色进度。 */
USTRUCT(BlueprintType, DisplayName="任务详细信息")
struct LXARPG_API FLxQuestDetailViewData
{
	GENERATED_BODY()
	/** 任务系列、任务标识和当前状态。 */
	UPROPERTY(BlueprintReadOnly, Category="任务UI", DisplayName="任务进度")
	FLxQuestRuntimeRecord Progress;
	/** 任务名称、描述、内容及奖励。 */
	UPROPERTY(BlueprintReadOnly, Category="任务UI", DisplayName="任务内容")
	FLxQuestNodeDefinition Definition;
	/** 静态配置是否成功取得，蓝图可据此显示数据缺失提示。 */
	UPROPERTY(BlueprintReadOnly, Category="任务UI", DisplayName="任务配置有效")
	bool bHasDefinition = false;
};

/** 两种任务界面共用的角色绑定与任务快照读取逻辑。 */
UCLASS(Abstract, BlueprintType, DisplayName="任务界面基类")
class LXARPG_API ULxQuestWidgetBase : public ULxUIBaseObject
{
	GENERATED_BODY()
public:
	/** 切换角色时解除旧角色监听并刷新任务列表。 */
	virtual void UpdateUIComponents(ULxCharacterDataTransferComponent* Component) override;
	/** 控件重新加入界面后恢复监听。 */
	virtual void NativeConstruct() override;
	/** 控件移除时解除任务监听。 */
	virtual void NativeDestruct() override;
protected:
	/** 读取角色已接取的任务，按可提交、进行中、已完成稳定排序。 */
	TArray<FLxQuestDetailViewData> ReadQuestDetails() const;
	/** 子类将最新任务数据发送给蓝图。 */
	virtual void RefreshQuestDisplay() {}
private:
	/** 绑定当前角色，防止重复订阅。 */
	void BindQuestEvents();
	/** 解除当前角色事件。 */
	void UnbindQuestEvents();
	/** 本地进度修改或网络复制后更新界面。 */
	UFUNCTION()
	void HandleQuestProgressChanged();
};
