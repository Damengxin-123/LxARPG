#pragma once
#include "CoreMinimal.h"
#include "LxUIManagementObject.h"
#include "LxManufacturingUIManager.generated.h"

class ULxEquipmentCraftingWidget;
class ULxItemManufacturingComponent;

/** 主UI管理器的新增制造层，负责打造页面布局、生命周期和鼠标状态。 */
UCLASS(BlueprintType, DisplayName="物品制造UI管理器")
class LXARPG_API ULxManufacturingUIManager : public ULxUIManagementObject
{
    GENERATED_BODY()
public:
    /** 接入主布局管理器的显式分层注册接口，注册时先隐藏页面。 */
    bool RegisterEquipmentPage(ULxEquipmentCraftingWidget* Widget);
    /** 将交互创建的打造页面加入制造层。 */
    bool ShowEquipmentPage(ULxEquipmentCraftingWidget* Widget, ULxItemManufacturingComponent* Component);
    /** 关闭制造层并释放选择引用，由交互管理器统一调用。 */
    void ReleasePage();
    /** 判断制造页面是否可见。 */
    bool HasVisiblePage() const;
    /** 随角色数据刷新当前页面。 */
    virtual void RefreshManagedUI() override;
    /** 判断是否为本层管理的控件。 */
    virtual bool ContainsWidget(const ULxUIBaseObject* Widget) const override;
private:
    /** 当前被制造层管理的页面 */
    UPROPERTY(Transient, VisibleInstanceOnly, Category="装备打造|运行时", DisplayName="当前被制造层管理的页面")
    TObjectPtr<ULxEquipmentCraftingWidget> Page;
};
