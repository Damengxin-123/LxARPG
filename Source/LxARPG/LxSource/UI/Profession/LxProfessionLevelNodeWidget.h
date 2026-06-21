#pragma once

#include "CoreMinimal.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "LxARPG/LxSource/Core/Database/LxUIBaseObject.h"
#include "LxProfessionLevelNodeWidget.generated.h"

class ULxProfessionLevelNodeUIData;

/** 职业等级节点控件，用于在 ListView 中显示单个职业等级的信息。 */
UCLASS(BlueprintType, Blueprintable, DisplayName="职业等级节点控件")
class LXARPG_API ULxProfessionLevelNodeWidget : public ULxUIBaseObject, public IUserObjectListEntry
{
	GENERATED_BODY()

public:
	/** ListView 设置列表项对象时刷新职业等级节点显示数据。 */
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;

	/** 获取当前绑定的职业等级节点数据。 */
	UFUNCTION(BlueprintPure, Category="职业UI|等级节点", DisplayName="获取职业等级节点数据")
	ULxProfessionLevelNodeUIData* GetProfessionLevelNodeData() const { return ProfessionLevelNodeData; }

protected:
	/** 蓝图刷新职业等级节点显示时调用。 */
	UFUNCTION(BlueprintImplementableEvent, Category="职业UI|等级节点", DisplayName="职业等级节点显示更新")
	void OnProfessionLevelNodeUpdated(ULxProfessionLevelNodeUIData* InProfessionLevelNodeData);

private:
	/** 当前绑定的职业等级节点数据。 */
	UPROPERTY(Transient)
	TObjectPtr<ULxProfessionLevelNodeUIData> ProfessionLevelNodeData = nullptr;
};
