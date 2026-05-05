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
#include "LxARPG/LxSource/UI/Manager/LxUIFunctionTypes.h"

void ULxItemGridWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	InitItemData(ListItemObject);
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
	if (!CurrentSlotData)
	{
		return false;
	}

	const bool bHadUsableItem = CurrentSlotData->IsValid();
	const ULxShortcutItemSlotData* ShortcutSlot = Cast<ULxShortcutItemSlotData>(CurrentSlotData);
	const bool bHadUsableSource = ShortcutSlot && ShortcutSlot->GetSourceSlot() && ShortcutSlot->GetSourceSlot()->IsValid();

	CurrentSlotData->UseItem();
	return bHadUsableItem || bHadUsableSource;
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
	if (ItemIsVaild() && CurrentSlotData->ItemDataPtr)
	{
		// 物品图标从基础物品结构体读取；没有配置时才回退到格子默认图标。
		const TSoftObjectPtr<UTexture2D> ItemIcon = CurrentSlotData->ItemDataPtr->ItemIcon();
		if (!ItemIcon.IsNull())
		{
			return ItemIcon.LoadSynchronous();
		}
	}

	return DefaultIcon.IsNull() ? nullptr : DefaultIcon.LoadSynchronous();
}

FText ULxItemGridWidget::GetItemDisplayName() const
{
	if (ItemIsVaild() && CurrentSlotData->ItemDataPtr)
	{
		return CurrentSlotData->ItemDataPtr->ItemDisplayName();
	}
	return FText::GetEmpty();
}

FText ULxItemGridWidget::GetItemDisplayDescription() const
{
	if (ItemIsVaild() && CurrentSlotData->ItemDataPtr)
	{
		return CurrentSlotData->ItemDataPtr->ItemDisplayDescription();
	}
	return FText::GetEmpty();
}

void ULxItemGridWidget::SetDefaultIcon(UTexture2D* InDefaultIcon)
{
	DefaultIcon = InDefaultIcon;
	BroadcastGridDataChanged();
}

bool ULxItemGridWidget::GetEquipmentType(ELxEquipmentType& OutEquipmentType) const
{
	if (!CurrentSlotData || CurrentSlotData->ItemSlotType != ELxItemSlotType::Equipment)
	{
		return false;
	}

	const ULxEquipmentSlotData* EquipmentSlotData = Cast<ULxEquipmentSlotData>(CurrentSlotData);
	if (!EquipmentSlotData)
	{
		return false;
	}

	OutEquipmentType = EquipmentSlotData->EquipmentType;
	return true;
}

FText ULxItemGridWidget::GetItemCount() const
{
	if (ItemIsVaild())
	{
		return CurrentSlotData->ItemDataPtr->ItemCountText().ToFText();
	}
	return FLxString("").ToFText();
}

ELxItemRarityType ULxItemGridWidget::GetItemRarity() const
{
	if (ItemIsVaild())
	{
		return CurrentSlotData->ItemDataPtr->ItemRarity();
	}
	return ELxItemRarityType::None;
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

	// return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	return FReply::Handled();
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
	FVector2D MousePos = UWidgetLayoutLibrary::GetMousePositionOnViewport(GetWorld());
	UpdateItemTooltipPosition(MousePos);
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
	ULxItemSlotData* NewSlotData = nullptr;

	if (ULxItemUIData* BackpackData = Cast<ULxItemUIData>(ListItemObject))
	{
		NewSlotData = BackpackData->m_pSlotData;
	}

	SetCurrentSlotDataInternal(NewSlotData);
}

void ULxItemGridWidget::SetCurrentSlotDataInternal(ULxItemSlotData* InSlotData)
{
	if (CurrentItemData)
	{
		CurrentItemData->OnItemCountChanged.RemoveDynamic(this, &ULxItemGridWidget::HandleCurrentItemChanged);
		CurrentItemData = nullptr;
	}

	if (CurrentSlotData)
	{
		CurrentSlotData->OnSlotChanged.RemoveDynamic(this, &ULxItemGridWidget::HandleCurrentSlotChanged);
	}

	CurrentSlotData = InSlotData;

	if (CurrentSlotData)
	{
		CurrentSlotData->OnSlotChanged.RemoveDynamic(this, &ULxItemGridWidget::HandleCurrentSlotChanged);
		CurrentSlotData->OnSlotChanged.AddDynamic(this, &ULxItemGridWidget::HandleCurrentSlotChanged);
	}

	RebindCurrentItemChanged();
	BroadcastGridDataChanged();
}


void ULxItemGridWidget::HandleCurrentSlotChanged() 
{
	RebindCurrentItemChanged();
	BroadcastGridDataChanged();
}

void ULxItemGridWidget::HandleCurrentItemChanged(ULxItemBase* Item, int32 OldCount, int32 NewCount)
{
	BroadcastItemCountChanged();
}

void ULxItemGridWidget::BroadcastGridDataChanged()
{
	ELxEquipmentType EquipmentType = ELxEquipmentType::Weapon;
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
void ULxItemGridWidget::RebindCurrentItemChanged()
{
	if (CurrentItemData)
	{
		CurrentItemData->OnItemCountChanged.RemoveDynamic(this, &ULxItemGridWidget::HandleCurrentItemChanged);
		CurrentItemData = nullptr;
	}

	if (CurrentSlotData && CurrentSlotData->ItemDataPtr)
	{
		CurrentItemData = CurrentSlotData->ItemDataPtr;
		CurrentItemData->OnItemCountChanged.RemoveDynamic(this, &ULxItemGridWidget::HandleCurrentItemChanged);
		CurrentItemData->OnItemCountChanged.AddDynamic(this, &ULxItemGridWidget::HandleCurrentItemChanged);
	}
}
