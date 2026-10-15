#pragma once

#include "CoreMinimal.h"
#include "LxSaveComponentBase.h"
#include "LxShortcutSaveData.h"
#include "LxCharacterSaveComponent.generated.h"

class ULxItemSlotData;

/** 玩家存档适配组件，持有角色快捷栏引用并转换玩家持久化数据。 */
UCLASS(ClassGroup=("存档"), meta=(BlueprintSpawnableComponent), BlueprintType, DisplayName="玩家存档组件")
class LXARPG_API ULxCharacterSaveComponent : public ULxSaveComponentBase
{
	GENERATED_BODY()

public:
	/** 项目提供十个快捷键，对应索引零至九。 */
	static constexpr int32 ShortcutSlotCount = 10;

	/** 获取角色持有的快捷槽；重建界面不会丢失物品引用，非法索引返回空。 */
	UFUNCTION(BlueprintCallable, Category="角色|快捷栏", DisplayName="获取玩家快捷栏槽位")
	ULxItemSlotData* GetShortcutSlot(int32 ShortcutIndex);

	/** 延迟生成期间绑定选中档案的记录标签，开始运行后禁止改写身份。 */
	void SetSessionIdentity(const FGameplayTag& InID)
	{
		if (InID.IsValid() && !HasBegunPlay()) SaveID = InID;
	}
	/** 优先使用手动存档ID，未配置时使用角色已有的标签ID。 */
	virtual FGameplayTag GetSaveID() const override;

	/** 将背包、装备、任务和职业属性汇总到管理模块持有的存档对象。 */
	virtual bool CaptureSaveData(ULxGameSaveData* InSaveData) const override;

	/** 查询本角色记录并整体恢复内容模块，未找到记录时保留新角色默认数据。 */
	virtual bool RestoreSaveData(const ULxGameSaveData* InSaveData) override;

	/** 标记该组件使用玩家记录索引，以便与世界交互对象的ID分开管理。 */
	virtual bool IsPlayerSaveComponent() const override { return true; }

private:
	/** 根据物品实例当前所在容器导出快捷格索引，不保存运行时对象。 */
	void CaptureShortcutSaveData(TArray<FLxShortcutSaveRecord>& OutRecords) const;
	/** 在替换背包前校验快捷栏记录结构，防止坏档部分恢复。 */
	static bool ValidateShortcutSaveData(const TArray<FLxShortcutSaveRecord>& Records);
	/** 内容模块恢复后重新绑定现有实例，缺失物品安全清空。 */
	void RestoreShortcutSaveData(const TArray<FLxShortcutSaveRecord>& Records);
	/** 快捷槽由角色持有，界面只负责展示和使用同一份引用。 */
	UPROPERTY(Transient, VisibleAnywhere, Category="角色|快捷栏", DisplayName="玩家快捷栏槽位")
	TArray<TObjectPtr<ULxItemSlotData>> ShortcutSlots;
};
