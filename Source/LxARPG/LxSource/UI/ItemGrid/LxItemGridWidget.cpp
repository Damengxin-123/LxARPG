#include "LxItemGridWidget.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Input/Reply.h"
#include "InputCoreTypes.h"
#include "LxARPG/LxSource/Model/Item/DataType/ItemBase/LxItemBase.h"
#include "LxARPG/LxSource/Model/Item/DataType/ItemBase/LxItemLogicBase.h"
#include "LxARPG/LxSource/Model/Item/DataType/Slot/LxItemSlotData.h"
#include "LxARPG/LxSource/Systems/LxLocalPlayerSubsystem.h"
#include "LxARPG/LxSource/UI/ItemGrid/LxItemUIData.h"
#include "LxARPG/LxSource/UI/ItemGrid/LxItemDragInfo.h"
#include "LxARPG/LxSource/UI/ItemGrid/LxItemDragIconWidget.h"
#include "LxARPG/LxSource/UI/Manager/LxUIManager.h"
#include "LxARPG/LxSource/UI/Manager/LxUIFunctionTypes.h"

void ULxItemGridWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	InitItemData(ListItemObject);
}

const FLxItemDateBase& ULxItemGridWidget::GetCurrentItemData() const
{
	static FLxItemDateBase EmptyItemData;

	if (CurrentSlotData == nullptr || !CurrentSlotData->IsValid())
	{
		return EmptyItemData;
	}

	 return *CurrentSlotData->ItemDataPtr->GetItemDataBase();
}

ELxItemSlotType ULxItemGridWidget::GetSlotType() const
{
	if (CurrentSlotData)
	{
		return CurrentSlotData->ItemSlotType;
	}
	return ELxItemSlotType::None;
}

int32 ULxItemGridWidget::GetItemType() const
{
	if (CurrentSlotData)
	{
		switch (CurrentSlotData->ItemSlotType)
		{

		case ELxItemSlotType::Equipment:
			{
				if (ULxEquipmentSlotData* equSlot = Cast<ULxEquipmentSlotData>(CurrentSlotData))
				{
					return static_cast<int32>(equSlot->EquipmentType);
				}
			}
			break;
			default:
			return 0;
		}
	}
	return 0;
}

bool ULxItemGridWidget::UseItem() const
{
	if (!CurrentSlotData || !CurrentSlotData->IsValid())
	{
		return false;
	}

	CurrentSlotData->UseItem();
	return true;
}

bool ULxItemGridWidget::ItemIsVaild() const
{
	return CurrentSlotData != nullptr && CurrentSlotData->IsValid();
}

UTexture2D* ULxItemGridWidget::GetDisplayIcon() const
{
	if (ItemIsVaild())
	{
		const FLxItemDateBase& ItemData = GetCurrentItemData();
		if (!ItemData.ItemShowInfo.ItemIcon.IsNull())
		{
			return ItemData.ItemShowInfo.ItemIcon.LoadSynchronous();
		}
	}

	return DefaultIcon.IsNull() ? nullptr : DefaultIcon.LoadSynchronous();
}

int32 ULxItemGridWidget::GetItemCount() const
{
	if (ItemIsVaild())
	{
		const FLxItemDateBase& ItemData = GetCurrentItemData();
		if (!ItemData.ItemShowInfo.ItemIcon.IsNull())
		{
			return ItemData.ItemCount;
		}
	}
	return 0;
}

FLinearColor ULxItemGridWidget::GetItemRarityColor() const
{
	if (ItemIsVaild())
	{
		const FLxItemDateBase& ItemData = GetCurrentItemData();
		if (!ItemData.ItemShowInfo.ItemIcon.IsNull())
		{
			return ItemData.ItemRarity.RarityColor;
		}
	}
	return FLinearColor::White;
}

FReply ULxItemGridWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		return UseItem() ? FReply::Handled() : FReply::Unhandled();
	}

	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && ItemIsVaild() && CurrentSlotData->CanGetItemData())
	{
		return UWidgetBlueprintLibrary::DetectDragIfPressed(InMouseEvent, this, EKeys::LeftMouseButton).NativeReply;
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void ULxItemGridWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent,
	UDragDropOperation*& OutOperation)
{
	Super::NativeOnDragDetected(InGeometry, InMouseEvent, OutOperation);

	// 拖拽发起时，判断当前格子是否为空，或者当前格子类型为不可拖拽的格子
	if (!ItemIsVaild() || !CurrentSlotData->CanGetItemData())
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
	if (DragOperation->SourceSlot == CurrentSlotData)
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
	UpdateItemTooltipPosition(InMouseEvent.GetScreenSpacePosition());
	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

void ULxItemGridWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);
	HideItemTooltip();
}

ULxCharacterPopupUIFunction* ULxItemGridWidget::GetCharacterPopupUIFunction() const
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

	const ULxUIManager* UIManager = LocalPlayerSubsystem->GetUIManager();
	return UIManager ? UIManager->GetCharacterPopupUIFunction() : nullptr;
}

void ULxItemGridWidget::ShowItemTooltip(const FVector2D& InMouseScreenPosition) const
{
	if (!CurrentSlotData || !CurrentSlotData->IsValid() || !CurrentSlotData->ItemDataPtr)
	{
		HideItemTooltip();
		return;
	}

	if (ULxCharacterPopupUIFunction* PopupFunction = GetCharacterPopupUIFunction())
	{
		PopupFunction->ShowItemTooltip(CurrentSlotData->ItemDataPtr, InMouseScreenPosition);
	}
}

void ULxItemGridWidget::UpdateItemTooltipPosition(const FVector2D& InMouseScreenPosition) const
{
	if (!CurrentSlotData || !CurrentSlotData->IsValid() || !CurrentSlotData->ItemDataPtr)
	{
		HideItemTooltip();
		return;
	}

	if (ULxCharacterPopupUIFunction* PopupFunction = GetCharacterPopupUIFunction())
	{
		PopupFunction->UpdateItemTooltipPosition(InMouseScreenPosition);
	}
}

void ULxItemGridWidget::HideItemTooltip() const
{
	if (ULxCharacterPopupUIFunction* PopupFunction = GetCharacterPopupUIFunction())
	{
		PopupFunction->HideItemTooltip();
	}
}

void ULxItemGridWidget::InitItemData(UObject* ListItemObject)
{
	if (CurrentSlotData)
	{
		CurrentSlotData->OnSlotChanged.RemoveDynamic(this, &ULxItemGridWidget::HandleCurrentSlotChanged);
	}

	CurrentSlotData = nullptr;

	if (ULxItemUIData* BackpackData = Cast<ULxItemUIData>(ListItemObject))
	{
		CurrentSlotData = BackpackData->m_pSlotData;
	}


	if (CurrentSlotData)
	{
		CurrentSlotData->OnSlotChanged.AddDynamic(this, &ULxItemGridWidget::HandleCurrentSlotChanged);
	}
	
}


void ULxItemGridWidget::HandleCurrentSlotChanged() 
{
	BroadcastGridDataChanged();
}

void ULxItemGridWidget::BroadcastGridDataChanged() const
{
	OnItemGridDataChanged.Broadcast();
}
