#include "LxItemGridWidget.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Input/Reply.h"
#include "InputCoreTypes.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "LxARPG/LxSource/Model/Item/DataType/ItemBase/LxItemBase.h"
#include "LxARPG/LxSource/Model/Item/DataType/Skill/LxSkillItem.h"
#include "LxARPG/LxSource/Model/Item/DataType/Slot/LxItemSlotData.h"
#include "LxARPG/LxSource/Model/DataTransfer/LxCharacterDataTransferComponent.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxTradeContainerInteractionComponent.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxTreasureChestInteractionComponent.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxWarehouseInteractionComponent.h"
#include "LxARPG/LxSource/Model/Aim/LxPlayerAimComponent.h"
#include "LxARPG/LxSource/Model/Skill/Logic/Skill/LxSkill.h"
#include "LxARPG/LxSource/Model/Skill/Logic/Skill/LxSkillCastComponent.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"
#include "LxARPG/LxSource/Player/Characters/LxPlayerCharacter.h"
#include "LxARPG/LxSource/Player/Controllers/LxPlayerController.h"
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

	/** 为技能释放构建上下文；玩家角色优先使用准星瞄准结果，其他角色保持原有通用上下文。 */
	FLxSkillCastContext MakeSkillCastContextForWidget(const ULxItemGridWidget* Widget,
		ULxSkillCastComponent* SkillCastComponent, UObject* SourceObject)
	{
		if (const ALxPlayerCharacter* PlayerCharacter = Widget
			? Cast<ALxPlayerCharacter>(Widget->GetOwningPlayerPawn())
			: nullptr)
		{
			if (ULxPlayerAimComponent* PlayerAimComponent = PlayerCharacter->GetPlayerAimComponent())
			{
				return PlayerAimComponent->MakeAimSkillCastContext(SourceObject);
			}
		}

		return SkillCastComponent->MakeSkillCastContext(SourceObject);
	}
}

void ULxItemGridWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	InitItemData(ListItemObject);
}

void ULxItemGridWidget::HandleInputValue(ELxInputActionID InInputActionID, FLxInputValue InValue)
{
	if (!IsShortcutInputActionID(InInputActionID) || GetSlotType() != ELxItemSlotType::Shortcut)
	{
		return;
	}

	if (InValue.m_blValue)
	{
		StartUseItem();
	}
	else
	{
		EndUseItem();
	}
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
		|| CurrentSlotData->GetSlotType() == ELxItemSlotType::TreasureChest
		|| CurrentSlotData->GetSlotType() == ELxItemSlotType::Transaction)
	{
		return false;
	}

	if (TryReleaseSkillItemDirectly())
	{
		return true;
	}

	CurrentSlotData->ItemUse();

	return true;
}

bool ULxItemGridWidget::StartUseItem() const
{
	// 仓库格子只负责长期存放和拖拽物品，不响应使用输入。
	if (!CurrentSlotData
		|| CurrentSlotData->GetSlotType() == ELxItemSlotType::Warehouse
		|| CurrentSlotData->GetSlotType() == ELxItemSlotType::TreasureChest
		|| CurrentSlotData->GetSlotType() == ELxItemSlotType::Transaction)
	{
		return false;
	}

	if (TryStartUseSkillItem())
	{
		return true;
	}

	CurrentSlotData->StartUseItem();
	return true;
}

bool ULxItemGridWidget::EndUseItem() const
{
	// 仓库格子只负责长期存放和拖拽物品，不响应使用输入。
	if (!CurrentSlotData
		|| CurrentSlotData->GetSlotType() == ELxItemSlotType::Warehouse
		|| CurrentSlotData->GetSlotType() == ELxItemSlotType::TreasureChest
		|| CurrentSlotData->GetSlotType() == ELxItemSlotType::Transaction)
	{
		return false;
	}

	if (TryEndUseSkillItem())
	{
		return true;
	}

	CurrentSlotData->EndUseItem();
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
		return StartUseItem() ? FReply::Handled().CaptureMouse(TakeWidget()) : FReply::Unhandled();
	}

	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && ItemIsVaild() && CurrentSlotData->ItemIsLeave())
	{
		return UWidgetBlueprintLibrary::DetectDragIfPressed(InMouseEvent, this, EKeys::LeftMouseButton).NativeReply;
	}

	// return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	return FReply::Handled();
}

FReply ULxItemGridWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		return EndUseItem() ? FReply::Handled().ReleaseMouseCapture() : FReply::Unhandled();
	}

	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
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

	if (TryHandleTradeDrop(DragOperation->SourceSlot))
	{
		return true;
	}

	if (TryHandleServerSlotDrop(DragOperation->SourceSlot))
	{
		return true;
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
		UIManager->ShowItemTooltipWithValue(CurrentSlotData->GetItem(), CurrentSlotData->GetItemValue(), true, InMouseScreenPosition);
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

bool ULxItemGridWidget::TryHandleTradeDrop(ULxItemSlotData* SourceSlot)
{
	if (CurrentSlotData == nullptr || SourceSlot == nullptr)
	{
		return false;
	}

	if (SourceSlot->GetSlotIndex() == INDEX_NONE || CurrentSlotData->GetSlotIndex() == INDEX_NONE)
	{
		return false;
	}

	ALxPlayerController* PlayerController = Cast<ALxPlayerController>(GetOwningPlayer());
	if (PlayerController == nullptr)
	{
		return false;
	}

	if (SourceSlot->GetSlotType() == ELxItemSlotType::Transaction && CurrentSlotData->GetSlotType() == ELxItemSlotType::Backpack)
	{
		if (ULxTradeContainerInteractionComponent* TradeComponent = Cast<ULxTradeContainerInteractionComponent>(SourceSlot->GetOuter()))
		{
			if (TradeComponent->GetOwner() == nullptr)
			{
				return false;
			}

			PlayerController->ServerBuyTradeSlotToBackpackSlot(
				TradeComponent->GetOwner(),
				SourceSlot->GetSlotIndex(),
				CurrentSlotData->GetSlotIndex());
			return true;
		}
	}

	if (CurrentSlotData->GetSlotType() == ELxItemSlotType::Transaction && SourceSlot->GetSlotType() == ELxItemSlotType::Backpack)
	{
		if (ULxTradeContainerInteractionComponent* TradeComponent = Cast<ULxTradeContainerInteractionComponent>(CurrentSlotData->GetOuter()))
		{
			if (TradeComponent->GetOwner() == nullptr)
			{
				return false;
			}

			PlayerController->ServerSellBackpackSlot(TradeComponent->GetOwner(), SourceSlot->GetSlotIndex());
			return true;
		}
	}

	return false;
}

bool ULxItemGridWidget::TryHandleServerSlotDrop(ULxItemSlotData* SourceSlot)
{
	if (CurrentSlotData == nullptr || SourceSlot == nullptr)
	{
		return false;
	}

	const bool bSourceBackpack = SourceSlot->GetSlotType() == ELxItemSlotType::Backpack;
	const bool bSourceWarehouse = SourceSlot->GetSlotType() == ELxItemSlotType::Warehouse;
	const bool bSourceTreasureChest = SourceSlot->GetSlotType() == ELxItemSlotType::TreasureChest;
	const bool bTargetBackpack = CurrentSlotData->GetSlotType() == ELxItemSlotType::Backpack;
	const bool bTargetWarehouse = CurrentSlotData->GetSlotType() == ELxItemSlotType::Warehouse;
	const bool bMoveToWarehouse = bSourceBackpack && bTargetWarehouse;
	const bool bMoveToBackpack = bSourceWarehouse && bTargetBackpack;
	const bool bMoveTreasureChestToBackpack = bSourceTreasureChest && bTargetBackpack;
	const bool bMoveWithinBackpack = bSourceBackpack && bTargetBackpack;
	const bool bMoveWithinWarehouse = bSourceWarehouse && bTargetWarehouse;
	if (!bMoveToWarehouse && !bMoveToBackpack && !bMoveTreasureChestToBackpack && !bMoveWithinBackpack && !bMoveWithinWarehouse)
	{
		return false;
	}

	if (SourceSlot->GetSlotIndex() == INDEX_NONE || CurrentSlotData->GetSlotIndex() == INDEX_NONE)
	{
		return false;
	}

	ALxPlayerController* PlayerController = Cast<ALxPlayerController>(GetOwningPlayer());
	if (PlayerController == nullptr)
	{
		return false;
	}

	if (bMoveWithinBackpack)
	{
		PlayerController->ServerMoveBackpackSlot(SourceSlot->GetSlotIndex(), CurrentSlotData->GetSlotIndex());
		return true;
	}

	if (bMoveTreasureChestToBackpack)
	{
		ULxTreasureChestInteractionComponent* TreasureChestComponent = Cast<ULxTreasureChestInteractionComponent>(SourceSlot->GetOuter());
		if (TreasureChestComponent == nullptr || TreasureChestComponent->GetOwner() == nullptr)
		{
			return false;
		}

		PlayerController->ServerMoveTreasureChestSlotToBackpack(
			TreasureChestComponent->GetOwner(),
			SourceSlot->GetSlotIndex(),
			CurrentSlotData->GetSlotIndex());
		return true;
	}

	ULxWarehouseInteractionComponent* WarehouseComponent = nullptr;
	if (bSourceWarehouse)
	{
		WarehouseComponent = Cast<ULxWarehouseInteractionComponent>(SourceSlot->GetOuter());
	}
	else if (bTargetWarehouse)
	{
		WarehouseComponent = Cast<ULxWarehouseInteractionComponent>(CurrentSlotData->GetOuter());
	}

	if (WarehouseComponent == nullptr || WarehouseComponent->GetOwner() == nullptr || PlayerController == nullptr)
	{
		return false;
	}

	if (bMoveWithinWarehouse)
	{
		PlayerController->ServerMoveWarehouseSlot(
			WarehouseComponent->GetOwner(),
			SourceSlot->GetSlotIndex(),
			CurrentSlotData->GetSlotIndex());
		return true;
	}

	PlayerController->ServerMoveItemBetweenBackpackAndWarehouse(
		WarehouseComponent->GetOwner(),
		SourceSlot->GetSlotIndex(),
		CurrentSlotData->GetSlotIndex(),
		bMoveToWarehouse);
	return true;
}

ULxCharacterDataTransferComponent* ULxItemGridWidget::GetCharacterDataTransferComponentForTrade() const
{
	if (m_pCharacterDataTransferComponent)
	{
		return m_pCharacterDataTransferComponent;
	}

	const ALxBaseCharacter* OwnerCharacter = Cast<ALxBaseCharacter>(GetOwningPlayerPawn());
	return OwnerCharacter ? OwnerCharacter->GetCharacterDataTransferComponent() : nullptr;
}

ULxSkillCastComponent* ULxItemGridWidget::GetSkillCastComponent() const
{
	const ALxBaseCharacter* OwnerCharacter = Cast<ALxBaseCharacter>(GetOwningPlayerPawn());
	return OwnerCharacter ? OwnerCharacter->GetSkillCastComponent() : nullptr;
}

ULxSkillItem* ULxItemGridWidget::GetCurrentSkillItem() const
{
	if (!CurrentSlotData || !CurrentSlotData->IsValid())
	{
		return nullptr;
	}

	return Cast<ULxSkillItem>(CurrentSlotData->GetItem());
}

bool ULxItemGridWidget::TryReleaseSkillItemDirectly() const
{
	ULxSkillItem* SkillItem = GetCurrentSkillItem();
	ULxSkillCastComponent* SkillCastComponent = GetSkillCastComponent();
	if (!SkillItem || !SkillCastComponent)
	{
		return false;
	}

	ULxSkill* Skill = SkillItem->GetOrCreateSkillObject();
	if (!Skill)
	{
		return false;
	}

	return SkillCastComponent->HandleSkillReleaseInput(
		Skill,
		Skill->GetDirectReleaseInputState(),
		MakeSkillCastContextForWidget(this, SkillCastComponent, SkillItem));
}

bool ULxItemGridWidget::TryStartUseSkillItem() const
{
	ULxSkillItem* SkillItem = GetCurrentSkillItem();
	ULxSkillCastComponent* SkillCastComponent = GetSkillCastComponent();
	if (!SkillItem || !SkillCastComponent)
	{
		return false;
	}

	ULxSkill* Skill = SkillItem->GetOrCreateSkillObject();
	if (!Skill)
	{
		return false;
	}

	return SkillCastComponent->HandleSkillReleaseInput(
		Skill,
		ELxSkillReleaseInputState::Start,
		MakeSkillCastContextForWidget(this, SkillCastComponent, SkillItem));
}

bool ULxItemGridWidget::TryEndUseSkillItem() const
{
	ULxSkillItem* SkillItem = GetCurrentSkillItem();
	ULxSkillCastComponent* SkillCastComponent = GetSkillCastComponent();
	if (!SkillItem || !SkillCastComponent)
	{
		return false;
	}

	ULxSkill* Skill = SkillItem->GetOrCreateSkillObject();
	if (!Skill)
	{
		return false;
	}

	return SkillCastComponent->HandleSkillReleaseInput(
		Skill,
		ELxSkillReleaseInputState::End,
		MakeSkillCastContextForWidget(this, SkillCastComponent, SkillItem));
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
	if (CurrentSlotData && CurrentSlotData->GetSlotType() == ELxItemSlotType::Transaction)
	{
		OnTradeRequirementUpdated(CurrentSlotData->CanTrade());
	}

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
