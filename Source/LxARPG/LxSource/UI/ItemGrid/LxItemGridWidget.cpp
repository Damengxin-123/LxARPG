#include "LxItemGridWidget.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Input/Reply.h"
#include "InputCoreTypes.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "LxARPG/LxSource/Model/Item/DataType/ItemBase/LxItemBase.h"
#include "LxARPG/LxSource/Model/Item/DataType/Slot/LxItemSlotData.h"
#include "LxARPG/LxSource/Systems/LxLocalPlayerSubsystem.h"
#include "LxARPG/LxSource/UI/ItemGrid/LxItemUIData.h"
#include "LxARPG/LxSource/UI/ItemGrid/LxItemDragInfo.h"
#include "LxARPG/LxSource/UI/ItemGrid/LxItemDragIconWidget.h"
#include "LxARPG/LxSource/UI/Manager/LxUIManager.h"

namespace
{
	bool IsShortcutInputActionID(ELxInputActionID InInputActionID)
	{
		return InInputActionID >= ELxInputActionID::Shortcut0
			&& InInputActionID <= ELxInputActionID::Shortcut9;
	}
}

void ULxItemGridWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	InitItemData(ListItemObject);
}

void ULxItemGridWidget::HandleInputValue(ELxInputActionID InInputActionID, FLxInputValue InValue)
{
	if (!InValue.m_blValue || !IsShortcutInputActionID(InInputActionID) || GetSlotType() != ELxItemSlotType::Shortcut)
	{
		return;
	}

	UseItem();
}

ELxItemSlotType ULxItemGridWidget::GetSlotType() const
{
	if (CurrentSlotData)
	{
		return CurrentSlotData->GetSlotType();
	}
	return ELxItemSlotType::None;
}


bool ULxItemGridWidget::UseItem() const
{
	// 仓库格子只负责长期存放和拖拽物品，不响应右键使用。
	if (!CurrentSlotData
		|| CurrentSlotData->GetSlotType() == ELxItemSlotType::Warehouse
		|| CurrentSlotData->GetSlotType() == ELxItemSlotType::TreasureChest)
	{
		return false;
	}
	CurrentSlotData->ItemUse();

	return true;
}

void ULxItemGridWidget::SetItemSlotData(ULxItemSlotData* InSlotData)
{
	SetCurrentSlotDataInternal(InSlotData);
}

bool ULxItemGridWidget::ItemIsVaild() const
{
	return CurrentSlotData != nullptr && CurrentSlotData->IsValid();
}

UTexture2D* ULxItemGridWidget::GetDisplayIcon() const
{
	if (ItemIsVaild())
	{
		// 物品图标从基础物品结构体读取；没有配置时才回退到格子默认图标。
		const TSoftObjectPtr<UTexture2D> ItemIcon = CurrentSlotData->GetItem()->ItemIcon();
		if (!ItemIcon.IsNull())
		{
			return ItemIcon.LoadSynchronous();
		}
	}

	return DefaultIcon.IsNull() ? nullptr : DefaultIcon.LoadSynchronous();
}

FText ULxItemGridWidget::GetItemDisplayName() const
{
	if (ItemIsVaild())
	{
		return CurrentSlotData->GetItem()->ItemDisplayName();
	}
	return FText::GetEmpty();
}

FText ULxItemGridWidget::GetItemDisplayDescription() const
{
	if (ItemIsVaild())
	{
		return CurrentSlotData->GetItem()->ItemDisplayDescription();
	}
	return FText::GetEmpty();
}

void ULxItemGridWidget::SetDefaultIcon(UTexture2D* InDefaultIcon)
{
	DefaultIcon = InDefaultIcon;
	BroadcastGridDataChanged();
}

bool ULxItemGridWidget::GetEquipmentType(FGameplayTag& OutEquipmentType) const
{
	if (!CurrentSlotData || CurrentSlotData->GetSlotType() != ELxItemSlotType::Equipment)
	{
		return false;
	}
	
	OutEquipmentType = CurrentSlotData->GetItemTypeTag();

	return true;
}

FText ULxItemGridWidget::GetItemCount() const
{
	if (ItemIsVaild())
	{
		return CurrentSlotData->GetItem()->ItemCountText().ToFText();
	}
	return FLxString("").ToFText();
}

ELxItemRarityType ULxItemGridWidget::GetItemRarity() const
{
	if (ItemIsVaild())
	{
		return CurrentSlotData->GetItem()->ItemRarity();
	}
	return ELxItemRarityType::None;
}

FReply ULxItemGridWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		return UseItem() ? FReply::Handled() : FReply::Unhandled();
	}

	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && ItemIsVaild() && CurrentSlotData->ItemIsLeave())
	{
		return UWidgetBlueprintLibrary::DetectDragIfPressed(InMouseEvent, this, EKeys::LeftMouseButton).NativeReply;
	}

	// return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	return FReply::Handled();
}

void ULxItemGridWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent,
	UDragDropOperation*& OutOperation)
{
	Super::NativeOnDragDetected(InGeometry, InMouseEvent, OutOperation);

	// 拖拽发起时，判断当前格子是否为空，或者当前格子类型为不可拖拽的格子
	if (!ItemIsVaild() || !CurrentSlotData->ItemIsLeave())
	{
		OutOperation = nullptr;
		return;
	}
	// 创建拖拽事件信息对象
	ULxItemDragInfo* DragOperation = NewObject<ULxItemDragInfo>(this);
	if (DragOperation == nullptr)
	{
		OutOperation = nullptr;
		return;
	}

	// 设置拖拽信息对象参数
	DragOperation->SourceSlot = CurrentSlotData;

	// 创建拖拽时图标
	if (ItemDragIconWidgetClass)
	{
		if (ULxItemDragIconWidget* DragVisual = CreateWidget<ULxItemDragIconWidget>(this, ItemDragIconWidgetClass))
		{
			DragVisual->SetIcon(GetDisplayIcon());
			DragOperation->DefaultDragVisual = DragVisual;
		}
	}

	OutOperation = DragOperation;
}

bool ULxItemGridWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent,
	UDragDropOperation* InOperation)
{
	const ULxItemDragInfo* DragOperation = Cast<ULxItemDragInfo>(InOperation);
	// 判断拖拽事件处理参数是否有效 拖拽数据对象 本格子槽位指针  发起方槽位指针
	if (DragOperation == nullptr || CurrentSlotData == nullptr || DragOperation->SourceSlot == nullptr)
	{
		return Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
	}

	// 判断拖拽双方是否是同一个格子
	if (DragOperation->SourceSlot == CurrentSlotData || !CurrentSlotData->ItemIsEnter())
	{
		return false;
	}
	// 调用槽位类型的接口进行堆叠
	switch (CurrentSlotData->ItemEnterToThis(DragOperation->SourceSlot))
	{
		case ELxItemSlotDropResult::Swapped:
		case ELxItemSlotDropResult::StackedAll:
		case ELxItemSlotDropResult::StackedPartial:
		case ELxItemSlotDropResult::EnterSuccess:
			return true;
		default:
			return false;
	}
}

void ULxItemGridWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
	ShowItemTooltip(InMouseEvent.GetScreenSpacePosition());
}

FReply ULxItemGridWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	FVector2D MousePos = UWidgetLayoutLibrary::GetMousePositionOnViewport(GetWorld());
	UpdateItemTooltipPosition(MousePos);
	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

void ULxItemGridWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);
	HideItemTooltip();
}

ULxUIManager* ULxItemGridWidget::GetUIManager() const
{
	const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	if (!LocalPlayer)
	{
		return nullptr;
	}

	const ULxLocalPlayerSubsystem* LocalPlayerSubsystem = ULxLocalPlayerSubsystem::GetFromLocalPlayer(LocalPlayer);
	if (!LocalPlayerSubsystem)
	{
		return nullptr;
	}

	return LocalPlayerSubsystem->GetUIManager();
}

void ULxItemGridWidget::ShowItemTooltip(const FVector2D& InMouseScreenPosition) const
{
	if (!CurrentSlotData || !CurrentSlotData->IsValid() || !CurrentSlotData->GetItem())
	{
		HideItemTooltip();
		return;
	}

	if (ULxUIManager* UIManager = GetUIManager())
	{
		UIManager->ShowItemTooltip(CurrentSlotData->GetItem(), InMouseScreenPosition);
	}
}

void ULxItemGridWidget::UpdateItemTooltipPosition(const FVector2D& InMouseScreenPosition) const
{
	if (!CurrentSlotData || !CurrentSlotData->IsValid() || !CurrentSlotData->GetItem())
	{
		HideItemTooltip();
		return;
	}

	if (ULxUIManager* UIManager = GetUIManager())
	{
		UIManager->UpdateItemTooltipPosition(InMouseScreenPosition);
	}
}

void ULxItemGridWidget::HideItemTooltip() const
{
	if (ULxUIManager* UIManager = GetUIManager())
	{
		UIManager->HideItemTooltip();
	}
}

void ULxItemGridWidget::InitItemData(UObject* ListItemObject)
{
	ULxItemSlotData* NewSlotData = nullptr;

	if (ULxItemUIData* BackpackData = Cast<ULxItemUIData>(ListItemObject))
	{
		NewSlotData = BackpackData->m_pSlotData;
	}

	SetCurrentSlotDataInternal(NewSlotData);
}

void ULxItemGridWidget::SetCurrentSlotDataInternal(ULxItemSlotData* InSlotData)
{

	if (CurrentSlotData)
	{
		CurrentSlotData->OnItemDataChanged.RemoveDynamic(this, &ULxItemGridWidget::HandleCurrentSlotChanged);
	}

	CurrentSlotData = InSlotData;

	if (CurrentSlotData)
	{
		CurrentSlotData->OnItemDataChanged.RemoveDynamic(this, &ULxItemGridWidget::HandleCurrentSlotChanged);
		CurrentSlotData->OnItemDataChanged.AddDynamic(this, &ULxItemGridWidget::HandleCurrentSlotChanged);
	}

	BroadcastGridDataChanged();
}


void ULxItemGridWidget::HandleCurrentSlotChanged(ULxItemBase* InItemData) 
{

	BroadcastGridDataChanged();
}

void ULxItemGridWidget::HandleCurrentItemChanged(ULxItemBase* Item)
{
	BroadcastItemCountChanged();
}

void ULxItemGridWidget::BroadcastGridDataChanged()
{
	FGameplayTag EquipmentType;
	if (!ItemIsVaild() && GetEquipmentType(EquipmentType))
	{
		// 再由蓝图按装备部位决定默认图标。
		OnEmptyEquipmentSlotUpdated(true, EquipmentType);
		return;
	}
	if (!ItemIsVaild())
	{
		// 空非装备槽位显示默认图标。
		OnEmptyEquipmentSlotUpdated(false, EquipmentType);
		return;
	}

	// 槽位或物品对象变化时，蓝图需要同时刷新图标、名称、描述和数量。
	OnItemDisplayUpdated(GetDisplayIcon(), GetItemDisplayName(), GetItemDisplayDescription(), GetItemCount(), GetItemRarity());
}

void ULxItemGridWidget::BroadcastItemCountChanged()
{
	// 只有数量变化时避免重刷整块显示，蓝图只更新数量文本即可。
	OnItemCountUpdated(GetItemCount());
}
