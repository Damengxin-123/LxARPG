#include "LxBackpackWidget.h"
#include "LxARPG/LxSource/UI/ItemOperations/LxItemOperationUIRouter.h"
#include "LxARPG/LxSource/UI/Manager/LxUIManager.h"
#include "GameFramework/PlayerController.h"

#include "LxARPG/LxSource/Model/DataTransfer/LxCharacterDataTransferComponent.h"
#include "LxARPG/LxSource/Model/Item/DataType/ItemBase/LxItemBase.h"
#include "LxARPG/LxSource/Model/Item/DataType/Slot/LxItemSlotData.h"
#include "LxARPG/LxSource/Model/Item/Logic/LxSafeItemOperations.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxTradeContainerInteractionComponent.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"
#include "LxARPG/LxSource/UI/ItemGrid/LxItemGridWidget.h"
#include "LxARPG/LxSource/UI/ItemGrid/LxItemUIData.h"

void ULxBackpackWidget::UpdateUIComponents(ULxCharacterDataTransferComponent* CharacterDataTransferComponent)
{
	BindDataTransferComponent(CharacterDataTransferComponent);
	Super::UpdateUIComponents(CharacterDataTransferComponent);
	UpdatedBackpack();
}

bool ULxBackpackWidget::GetQuickItemContainer_Implementation(FLxItemContainerAddress& OutContainer) const
{
	if (!IsVisible() || !GetIsEnabled() || !m_pCharacterDataTransferComponent || !GetOwningPlayer()
		|| m_pCharacterDataTransferComponent->GetOwner() != GetOwningPlayer()->GetPawn()) return false;
	OutContainer = FLxItemContainerAddress();
	OutContainer.Kind = ELxItemContainerKind::Backpack;
	OutContainer.Owner = m_pCharacterDataTransferComponent->GetOwner();
	return true;
}

ULxItemTransferTicket* ULxBackpackWidget::PrepareQuickItemTransfer_Implementation(ULxItemSlotData* SourceSlot, FText& OutReason)
{
	FLxItemContainerAddress Address;
	if (!GetQuickItemContainer_Implementation(Address))
	{
		OutReason = NSLOCTEXT("LxItemOperations", "BackpackWindowInvalid", "背包窗口已失效");
		return nullptr;
	}
	return m_pCharacterDataTransferComponent->BeginSafeBackpackExtract(SourceSlot, OutReason);
}

bool ULxBackpackWidget::CanReceiveQuickItemTransfer_Implementation(ULxItemTransferTicket* Ticket, FText& OutReason) const
{
	FLxItemContainerAddress Address;
	if (!GetQuickItemContainer_Implementation(Address))
	{
		OutReason = NSLOCTEXT("LxItemOperations", "BackpackWindowInvalid", "背包窗口已失效");
		return false;
	}
    if (Ticket && Ticket->SourceSlot && Ticket->SourceSlot->GetSlotType() == ELxItemSlotType::Transaction)
    {
        // 商品进入背包必须经过交易模块的价格与容量规划，界面不直接搬运商品或扣除金币。
        ULxTradeContainerInteractionComponent* Trade = Ticket->SourceSlot->GetTypedOuter<ULxTradeContainerInteractionComponent>();
        ALxBaseCharacter* Character = Cast<ALxBaseCharacter>(Address.Owner);
        if (!Ticket->IsCompleted() && Trade && Character
            && LxSafeItemOperations::MatchesExpectedRecord(Ticket->SourceSlot, Ticket->ExpectedItem))
            return Trade->CanBuyTradeItem(Character->GetCharacterBackpackComponent(), Ticket->SourceSlot->GetSlotIndex(), OutReason);
        OutReason = NSLOCTEXT("LxItemOperations", "TradeSourceChanged", "商品已变化，请刷新交易界面后重试");
        return false;
    }
    return m_pCharacterDataTransferComponent->CanAcceptSafeBackpackItem(Ticket, OutReason);
}

bool ULxBackpackWidget::ReceiveQuickItemTransfer_Implementation(ULxItemTransferTicket* Ticket,
	const FLxItemOperationRequest& Request, ULxUIBaseObject* SourceWindow, FText& OutReason)
{
	FLxItemOperationRequest Received = Request;
	if (!GetQuickItemContainer_Implementation(Received.Target)) return false;
	ULxUIManager* Layout = GetOwningUIManager();
	return Layout && Layout->GetItemOperationRouter()->SubmitOperation(Received, Ticket, SourceWindow, OutReason);
}

void ULxBackpackWidget::NotifyQuickItemOperationResult_Implementation(const FLxItemOperationResult& Result)
{
	LastItemOperationResult = Result;
	OnItemOperationCompleted(Result);
}

void ULxBackpackWidget::NativeDestruct()
{
	UnbindDataTransferComponent();
	Super::NativeDestruct();
}

void ULxBackpackWidget::UpdatedBackpack()
{
	if (m_pCharacterDataTransferComponent == nullptr)
	{
		m_vItemSlotList.Reset();
		m_vEquipmentSlotList.Reset();
		PendingEquipmentSlotBindingList.Reset();
		OnItemListUpdated(GetItemUIDataList());
		OnEquipmentListUpdated(GetEquipmentUIDataList());
		OnEquipmentSlotBindingRequested();
		return;
	}

	TArray<ULxItemSlotData*> BackpackItems;
	m_pCharacterDataTransferComponent->GetAllBackpackItems(BackpackItems);
	HandleBackpackItemsChanged(BackpackItems);

	TArray<ULxItemSlotData*> EquipmentSlots;
	m_pCharacterDataTransferComponent->GetAllEquipment(EquipmentSlots);
	HandleEquipmentSlotsChanged(EquipmentSlots);
}

void ULxBackpackWidget::SortingOfItems()
{
	if (m_pCharacterDataTransferComponent)
	{
		// 操作入口也走数据中转组件，排序后的刷新由中转事件回流。
		m_pCharacterDataTransferComponent->SortBackpackItems();
	}
}

TArray<UObject*> ULxBackpackWidget::GetItemUIDataList()
{
	TArray<UObject*> ItemUIDataList;
	for (ULxItemSlotData* ItemSlot : m_vItemSlotList)
	{
		ULxItemUIData* ItemUIData = NewObject<ULxItemUIData>(this);
		ItemUIData->m_pSlotData = ItemSlot;
		ItemUIData->ItemOwnerWindow = this;
		ItemUIDataList.Add(ItemUIData);
	}
	return ItemUIDataList;
}

TArray<UObject*> ULxBackpackWidget::GetEquipmentUIDataList()
{


	TArray<UObject*> EquipmentUIDataList;
	for (ULxItemSlotData* EquipmentSlot : m_vEquipmentSlotList)
	{
		ULxItemUIData* EquipmentUIData = NewObject<ULxItemUIData>(this);
		EquipmentUIData->m_pSlotData = EquipmentSlot;
		EquipmentUIData->ItemOwnerWindow = this;
		EquipmentUIDataList.Add(EquipmentUIData);
	}
	return EquipmentUIDataList;
}

bool ULxBackpackWidget::BindEquipmentSlotToItemGrid(ULxItemGridWidget* ItemGridWidget, FGameplayTag EquipmentTypeTag)
{
	if (ItemGridWidget == nullptr)
	{
		return false;
	}

	for (int32 SlotIndex = 0; SlotIndex < PendingEquipmentSlotBindingList.Num(); ++SlotIndex)
	{
		ULxItemSlotData* SlotData = PendingEquipmentSlotBindingList[SlotIndex];
		if (SlotData == nullptr || SlotData->GetItemTypeTag() != EquipmentTypeTag)
		{
			continue;
		}

		ItemGridWidget->SetItemSlotData(SlotData);
		ItemGridWidget->SetItemOwnerWindow(this);
		ItemGridWidget->SetVisibility(ESlateVisibility::Visible);
		PendingEquipmentSlotBindingList.RemoveAt(SlotIndex);
		return true;
	}

	ItemGridWidget->SetItemSlotData(nullptr);
	ItemGridWidget->SetVisibility(ESlateVisibility::Collapsed);
	return false;
}

void ULxBackpackWidget::SwitchItemType(ELxItemType NewType)
{
	m_vItemSlotList.Reset();

	if (m_pCharacterDataTransferComponent)
	{
		TArray<ULxItemSlotData*> BackpackItems;
		if (NewType == ELxItemType::None)
		{
			// 全部分类保留空槽位，与背包初次刷新使用相同的数据入口。
			m_pCharacterDataTransferComponent->GetAllBackpackItems(BackpackItems);
		}
		else
		{
			m_pCharacterDataTransferComponent->QueryBackpackItemsByFilter(NewType, ELxItemRarityType::None, BackpackItems);
		}
		HandleBackpackItemsChanged(BackpackItems);
		return;
	}

	OnItemListUpdated(GetItemUIDataList());
}

void ULxBackpackWidget::BindDataTransferComponent(ULxCharacterDataTransferComponent* InDataTransferComponent)
{
	if (m_pCharacterDataTransferComponent == InDataTransferComponent)
	{
		return;
	}

	UnbindDataTransferComponent();
	m_pCharacterDataTransferComponent = InDataTransferComponent;

	if (m_pCharacterDataTransferComponent == nullptr)
	{
		return;
	}

	m_pCharacterDataTransferComponent->OnBackpackItemChanged.RemoveAll(this);
	m_pCharacterDataTransferComponent->OnBackpackItemChanged.AddUObject(this, &ULxBackpackWidget::HandleBackpackItemsChanged);
	m_pCharacterDataTransferComponent->OnEquipmentChanged.RemoveAll(this);
	m_pCharacterDataTransferComponent->OnEquipmentChanged.AddUObject(this, &ULxBackpackWidget::HandleEquipmentSlotsChanged);
}

void ULxBackpackWidget::UnbindDataTransferComponent()
{
	if (m_pCharacterDataTransferComponent == nullptr)
	{
		return;
	}

	m_pCharacterDataTransferComponent->OnBackpackItemChanged.RemoveAll(this);
	m_pCharacterDataTransferComponent->OnEquipmentChanged.RemoveAll(this);
	m_pCharacterDataTransferComponent = nullptr;
}

void ULxBackpackWidget::HandleBackpackItemsChanged(const TArray<ULxItemSlotData*>& BackpackItems)
{
	m_vItemSlotList.Reset();
	for (ULxItemSlotData* SlotData : BackpackItems)
	{
		m_vItemSlotList.Add(SlotData);
	}
	OnItemListUpdated(GetItemUIDataList());
}

void ULxBackpackWidget::HandleEquipmentSlotsChanged(const TArray<ULxItemSlotData*>& EquipmentSlots)
{
	m_vEquipmentSlotList.Reset();
	PendingEquipmentSlotBindingList.Reset();
	for (ULxItemSlotData* SlotData : EquipmentSlots)
	{
		m_vEquipmentSlotList.Add(SlotData);
		PendingEquipmentSlotBindingList.Add(SlotData);
	}
	OnEquipmentListUpdated(GetEquipmentUIDataList());
	OnEquipmentSlotBindingRequested();
}
