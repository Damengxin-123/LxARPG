#include "LxManufacturingUIManager.h"
#include "LxARPG/LxSource/UI/Crafting/LxEquipmentCraftingWidget.h"
#include "LxARPG/LxSource/Model/Item/Crafting/LxItemManufacturingComponent.h"
#include "LxARPG/LxSource/UI/Manager/LxUIManager.h"

bool ULxManufacturingUIManager::RegisterEquipmentPage(ULxEquipmentCraftingWidget* Widget)
{
    if (!Widget || !OwningUIManager) return false;
    if (Page == Widget) return true;
    ReleasePage();
    Page = Widget;
    Page->SetOwningUIManager(OwningUIManager);
    RefreshWidgetData(Page);
    Page->SetVisibility(ESlateVisibility::Collapsed);
    return OwningUIManager->MountRoleWindow(Page);
}

bool ULxManufacturingUIManager::ShowEquipmentPage(ULxEquipmentCraftingWidget* Widget, ULxItemManufacturingComponent* Component)
{
    if (!Component || !RegisterEquipmentPage(Widget)) return false;
    Page->SetManufacturingComponent(Component);
    Page->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    if (!OwningUIManager->MountRoleWindow(Page)) { ReleasePage(); return false; }
    OwningUIManager->ActivateRoleWindow(Page);
    return true;
}

void ULxManufacturingUIManager::ReleasePage()
{
    ULxEquipmentCraftingWidget* Previous = Page;
    Page = nullptr;
    if (!Previous) return;
    if (ULxItemManufacturingComponent* Component = Previous->GetManufacturingComponent()) Component->EndEquipmentService();
    Previous->SetManufacturingComponent(nullptr);
    Previous->SetVisibility(ESlateVisibility::Collapsed);
    if (OwningUIManager) OwningUIManager->UnmountRoleWindow(Previous);
    Previous->RemoveFromParent();
}

bool ULxManufacturingUIManager::HasVisiblePage() const { return IsWidgetVisible(Page); }
bool ULxManufacturingUIManager::ContainsWidget(const ULxUIBaseObject* Widget) const { return Widget && Widget == Page; }
void ULxManufacturingUIManager::RefreshManagedUI() { if (Page) { RefreshWidgetData(Page); Page->RefreshCrafting(); } }
