#include "LxCharacterBackpackComponent.h"

#include "LxARPG/LxSource/Core/Database/LxConstValue.h"
#include "LxARPG/LxSource/Model/Buff/DataType/LxBuffDefineTableConfig.h"
#include "LxARPG/LxSource/Model/Buff/DataType/LxBuffLogic.h"
#include "LxARPG/LxSource/Model/Entry/Logic/LxCharacterEntryComponent.h"
#include "LxARPG/LxSource/Model/Item/DataType/Consumable/LxConsumableDefineTableConfig.h"
#include "LxARPG/LxSource/Model/Item/DataType/Consumable/LxConsumableLogic.h"
#include "LxARPG/LxSource/Model/Item/DataType/Equipment/LxEquipmentDefineTableConfig.h"
#include "LxARPG/LxSource/Model/Item/DataType/Equipment/LxEquipmentLogic.h"
#include "LxARPG/LxSource/Model/Item/DataType/ItemBase/LxItemLogicBase.h"
#include "LxARPG/LxSource/Model/Item/DataType/Material/LxMaterialDefineTableConfig.h"
#include "LxARPG/LxSource/Model/Item/DataType/Material/LxMaterialLogic.h"
#include "LxARPG/LxSource/Model/Item/DataType/Skill/LxSkillDefineTableConfig.h"
#include "LxARPG/LxSource/Model/Item/DataType/Skill/LxSkillLogic.h"
#include "LxARPG/LxSource/Model/Item/DataType/Slot/LxItemSlotData.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"
#include "LxARPG/LxSource/Systems/LxGameInstanceSubsystem.h"
#include "LxARPG/LxSource/Systems/DatabaseSystem/LxGameDataTablesManager.h"

namespace
{
	/**
	 * @brief 获取安全的最大堆叠数量。
	 *
	 * 数据表中如果最大堆叠数量异常小于 1，这里会兜底返回 1，
	 * 避免后续在拆分堆叠和计算槽位数量时出现非法值。
	 *
	 * @param InItemDefine 物品定义数据。
	 * @return 返回可用于逻辑计算的最大堆叠数量。
	 */
	int32 GetSafeStackLimit(const FLxItemDefineBase& InItemDefine)
	{
		return FMath::Max(1, InItemDefine.ItemStackInfo.ItemMaxCount);
	}

	/**
	 * @brief 判断当前物品是否允许在背包中按堆叠方式加入。
	 *
	 * 仅当物品本身配置为可堆叠，且类型不是装备、技能、Buff 时，
	 * 才允许按堆叠逻辑补入已有物品堆。
	 *
	 * @param InItemDefine 物品定义数据。
	 * @return 如果允许堆叠则返回 true，否则返回 false。
	 */
	bool CanAddByStack(const FLxItemDefineBase& InItemDefine)
	{
		return InItemDefine.ItemStackInfo.ItemCanStack
			&& InItemDefine.ItemInfo.ItemType != ELxItemType::Equipment
			&& InItemDefine.ItemInfo.ItemType != ELxItemType::Skill
			&& InItemDefine.ItemInfo.ItemType != ELxItemType::Buff;
	}

	/**
	 * @brief 统计背包中的空槽位数量。
	 *
	 * 用于在真正执行新增物品之前，先判断剩余空槽位是否足够。
	 *
	 * @param InSlots 背包槽位数组。
	 * @return 返回当前空槽位数量。
	 */
	int32 CountEmptyBackpackSlots(const TArray<TObjectPtr<ULxItemSlotData>>& InSlots)
	{
		int32 EmptySlotCount = 0;
		for (const ULxItemSlotData* Slot : InSlots)
		{
			if (Slot != nullptr && Slot->IsEntry())
			{
				++EmptySlotCount;
			}
		}
		return EmptySlotCount;
	}

	/**
	 * @brief 查找一个可用的空背包槽位。
	 *
	 * @param InSlots 背包槽位数组。
	 * @return 返回找到的空槽位；如果不存在则返回 nullptr。
	 */
	ULxItemSlotData* FindEmptyBackpackSlot(const TArray<TObjectPtr<ULxItemSlotData>>& InSlots)
	{
		for (ULxItemSlotData* Slot : InSlots)
		{
			if (Slot != nullptr && Slot->IsEntry())
			{
				return Slot;
			}
		}
		return nullptr;
	}

	/**
	 * @brief 根据物品定义创建具体的物品逻辑对象。
	 *
	 * 这里对 ULxItemLogicBase::CreateItemLogicObject 做一层简单封装，
	 * 方便在背包新增逻辑里统一调用不同类型的物品创建流程。
	 *
	 * @tparam ItemLogicType 目标物品逻辑类型。
	 * @param InItemDefine 物品定义数据。
	 * @param InOuter 新对象的外层对象。
	 * @return 返回创建成功的物品逻辑对象；失败返回 nullptr。
	 */
	template<typename ItemLogicType>
	ItemLogicType* CreateItemByDefine(const FLxItemDefineBase* InItemDefine, UObject* InOuter)
	{
		return ULxItemLogicBase::CreateItemLogicObject<ItemLogicType>(InItemDefine, InOuter);
	}
}

ULxCharacterBackpackComponent::ULxCharacterBackpackComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void ULxCharacterBackpackComponent::HandleTrackedItemChanged()
{
	const bool bRemovedInvalidItems = CleanupInvalidItems();
	if (bRemovedInvalidItems)
	{
		RefreshTrackedItemBindings();
		OnDataChange.Broadcast();
	}
}

void ULxCharacterBackpackComponent::HandleTrackedItemUsed(ULxItemLogicBase* UsedItem)
{
	if (UsedItem == nullptr || !UsedItem->ItemIsValid())
	{
		return;
	}

	FLxItemDateBase* ItemData = UsedItem->GetItemDataBase();
	if (ItemData == nullptr)
	{
		return;
	}

	if (ItemData->ItemInfo.ItemType == ELxItemType::Consumable)
	{
		// 背包组件不解释消耗品词条，只负责把“使用行为”往外抛。
		OnItemUsed.Broadcast(UsedItem);
	}
}

void ULxCharacterBackpackComponent::RefreshTrackedItemBindings()
{
	for (ULxItemLogicBase* ItemLogic : m_vItemList)
	{
		if (ItemLogic == nullptr)
		{
			continue;
		}

		ItemLogic->OnItemInfoChanged.RemoveDynamic(this, &ULxCharacterBackpackComponent::HandleTrackedItemChanged);
		ItemLogic->OnItemInfoChanged.AddDynamic(this, &ULxCharacterBackpackComponent::HandleTrackedItemChanged);
		ItemLogic->OnItemUsed.RemoveDynamic(this, &ULxCharacterBackpackComponent::HandleTrackedItemUsed);
		ItemLogic->OnItemUsed.AddDynamic(this, &ULxCharacterBackpackComponent::HandleTrackedItemUsed);
	}
}


bool ULxCharacterBackpackComponent::CleanupInvalidItems()
{
	TArray<TObjectPtr<ULxItemLogicBase>> InvalidItems;

	for (ULxItemLogicBase* ItemLogic : m_vItemList)
	{
		if (ItemLogic == nullptr || !ItemLogic->ItemIsValid())
		{
			InvalidItems.Add(ItemLogic);
		}
	}

	if (InvalidItems.IsEmpty())
	{
		return false;
	}

	for (ULxItemLogicBase* InvalidItem : InvalidItems)
	{
		if (InvalidItem)
		{
			InvalidItem->OnItemInfoChanged.RemoveDynamic(this, &ULxCharacterBackpackComponent::HandleTrackedItemChanged);
			InvalidItem->OnItemUsed.RemoveDynamic(this, &ULxCharacterBackpackComponent::HandleTrackedItemUsed);
		}
	}

	for (ULxItemSlotData* Slot : m_vBackpackSlots)
	{
		if (Slot == nullptr || Slot->ItemDataPtr == nullptr)
		{
			continue;
		}

		if (!Slot->ItemDataPtr->ItemIsValid() || InvalidItems.Contains(Slot->ItemDataPtr))
		{
			Slot->ClearItem();
		}
	}

	m_vItemList.RemoveAll([](ULxItemLogicBase* ItemLogic)
	{
		return ItemLogic == nullptr || !ItemLogic->ItemIsValid();
	});

	return true;
}

void ULxCharacterBackpackComponent::BaseComponentInitialize()
{
	Super::BaseComponentInitialize();
	m_pOwnerCharacter = GetCharacterOwner();
	InitializeBackpack();
}

bool ULxCharacterBackpackComponent::AddItemByRowID(ELxItemType InItemType, FName InItemID, int32 InItemCount)
{
	// 基础参数校验，物品类型、物品 ID 和数量任一非法时直接返回失败。
	if (InItemType == ELxItemType::None || InItemID.IsNone() || InItemCount <= 0)
	{
		return false;
	}

	// 如果背包槽位尚未初始化，则先初始化背包基础数据。
	if (m_vBackpackSlots.IsEmpty())
	{
		InitializeBackpack();
	}

	// 获取游戏实例子系统，用于访问全局数据表管理器。
	ULxGameInstanceSubsystem* GameInstanceSubsystem = ULxGameInstanceSubsystem::GetInstance(GetWorld());
	if (GameInstanceSubsystem == nullptr)
	{
		return false;
	}

	// 获取数据表管理器，后续会根据物品类型从不同配置表中查询定义数据。
	const ULxGameDataTablesManager* GameDataTablesManager = GameInstanceSubsystem->GetGameDataManager();
	if (GameDataTablesManager == nullptr)
	{
		return false;
	}

	const FLxItemDefineBase* ItemDefine = nullptr;
	TFunction<ULxItemLogicBase*(const FLxItemDefineBase*)> CreateItemLogic;

	// 根据物品类型，选择对应的数据表配置并绑定对应的逻辑对象创建方式。
	switch (InItemType)
	{
	case ELxItemType::Equipment:
		if (GameDataTablesManager->m_pEquipmentDefineTableConfig == nullptr)
		{
			return false;
		}
		ItemDefine = GameDataTablesManager->m_pEquipmentDefineTableConfig->GetEquipmentDefine(InItemID);
		CreateItemLogic = [this](const FLxItemDefineBase* InDefine)
		{
			return CreateItemByDefine<ULxEquipmentLogic>(InDefine, this);
		};
		break;

	case ELxItemType::Consumable:
		if (GameDataTablesManager->m_pConsumableDefineTableConfig == nullptr)
		{
			return false;
		}
		ItemDefine = GameDataTablesManager->m_pConsumableDefineTableConfig->GetConsumableDefine(InItemID);
		CreateItemLogic = [this](const FLxItemDefineBase* InDefine)
		{
			return CreateItemByDefine<ULxConsumableLogic>(InDefine, this);
		};
		break;

	case ELxItemType::Material:
		if (GameDataTablesManager->m_pMaterialDefineTableConfig == nullptr)
		{
			return false;
		}
		ItemDefine = GameDataTablesManager->m_pMaterialDefineTableConfig->GetMaterialDefine(InItemID);
		CreateItemLogic = [this](const FLxItemDefineBase* InDefine)
		{
			return CreateItemByDefine<ULxMaterialLogic>(InDefine, this);
		};
		break;

	case ELxItemType::Skill:
		if (GameDataTablesManager->m_pSkillDefineTableConfig == nullptr)
		{
			return false;
		}
		ItemDefine = GameDataTablesManager->m_pSkillDefineTableConfig->GetSkillDefine(InItemID);
		CreateItemLogic = [this](const FLxItemDefineBase* InDefine)
		{
			return CreateItemByDefine<ULxSkillLogic>(InDefine, this);
		};
		break;

	case ELxItemType::Buff:
		if (GameDataTablesManager->m_pBuffDefineTableConfig == nullptr)
		{
			return false;
		}
		ItemDefine = GameDataTablesManager->m_pBuffDefineTableConfig->GetBuffDefine(InItemID);
		CreateItemLogic = [this](const FLxItemDefineBase* InDefine)
		{
			return CreateItemByDefine<ULxBuffLogic>(InDefine, this);
		};
		break;

	default:
		return false;
	}

	if (ItemDefine == nullptr || ItemDefine->ItemInfo.ItemID.IsNone())
	{
		return false;
	}

	// 计算该物品是否允许堆叠，以及单堆的最大数量。
	const bool bCanStack = CanAddByStack(*ItemDefine);
	const int32 StackLimit = GetSafeStackLimit(*ItemDefine);
	int32 RemainingCount = InItemCount;
	int32 AvailableStackSpace = 0;

	// 先扫描已有物品列表，统计同类型同 ID 物品还剩余多少可用堆叠空间。
	if (bCanStack)
	{
		for (ULxItemLogicBase* ItemLogic : m_vItemList)
		{
			if (ItemLogic == nullptr)
			{
				continue;
			}

			FLxItemDateBase* ItemData = ItemLogic->GetItemDataBase();
			if (ItemData == nullptr)
			{
				continue;
			}

			if (ItemData->ItemInfo.ItemType != InItemType || ItemData->ItemInfo.ItemID != InItemID)
			{
				continue;
			}

			const int32 CurrentSpace = StackLimit - ItemData->ItemCount;
			if (CurrentSpace <= 0)
			{
				continue;
			}

			// 记录当前物品堆还能容纳的数量，供后续统一预估所需空槽位。
			AvailableStackSpace += CurrentSpace;
		}
	}

	// 扣除已有堆叠可容纳的空间后，计算还需要新建多少个槽位。
	const int32 RemainingCountAfterStacking = FMath::Max(0, RemainingCount - AvailableStackSpace);
	const int32 NeedNewSlotCount = bCanStack
		? FMath::DivideAndRoundUp(RemainingCountAfterStacking, StackLimit)
		: RemainingCountAfterStacking;

	// 如果空槽位不足，则直接返回失败，避免出现只添加一部分的情况。
	if (NeedNewSlotCount > CountEmptyBackpackSlots(m_vBackpackSlots))
	{
		return false;
	}

	// 在确认空槽位足够后，优先把数量补入已有的同类堆叠中。
	if (bCanStack)
	{
		for (ULxItemLogicBase* ItemLogic : m_vItemList)
		{
			if (RemainingCount <= 0)
			{
				break;
			}

			if (ItemLogic == nullptr)
			{
				continue;
			}

			FLxItemDateBase* ItemData = ItemLogic->GetItemDataBase();
			if (ItemData == nullptr)
			{
				continue;
			}

			if (ItemData->ItemInfo.ItemType != InItemType || ItemData->ItemInfo.ItemID != InItemID)
			{
				continue;
			}

			const int32 CurrentSpace = StackLimit - ItemData->ItemCount;
			if (CurrentSpace <= 0)
			{
				continue;
			}

			// 按当前堆剩余空间与待加入数量的较小值进行补堆。
			const int32 AddCount = FMath::Min(CurrentSpace, RemainingCount);
			ItemData->ItemCount += AddCount;
			RemainingCount -= AddCount;

			// 数量发生变化后，通知监听该物品的界面或逻辑刷新。
			ItemLogic->OnItemInfoChanged.Broadcast();
		}
	}

	// 若仍有剩余数量，则创建新的物品对象并分配到空槽位中。
	while (RemainingCount > 0)
	{
		// 查找一个可用空槽位，用于放置本次新建的物品对象。
		ULxItemSlotData* EmptySlot = FindEmptyBackpackSlot(m_vBackpackSlots);
		if (EmptySlot == nullptr)
		{
			return false;
		}

		// 根据前面绑定的创建函数，生成对应类型的物品逻辑对象。
		ULxItemLogicBase* NewItemLogic = CreateItemLogic(ItemDefine);
		if (NewItemLogic == nullptr)
		{
			return false;
		}

		// 获取新物品的运行时数据缓存，用于写入当前这份实例的数量。
		FLxItemDateBase* NewItemData = NewItemLogic->GetItemDataBase();
		if (NewItemData == nullptr)
		{
			return false;
		}

		// 可堆叠物品按最大堆叠数拆分数量，不可堆叠物品固定为 1。
		NewItemData->ItemCount = bCanStack
			? FMath::Min(StackLimit, RemainingCount)
			: 1;

		// 将新建物品放入空槽位；如果槽位设置失败则终止。
		if (!EmptySlot->SetItem(NewItemLogic))
		{
			return false;
		}

		// 维护背包内部物品列表，并扣减剩余待添加数量。
		m_vItemList.Add(NewItemLogic);
		RemainingCount -= NewItemData->ItemCount;

		// 通知该物品实例的数量与状态已经完成初始化。
		NewItemLogic->OnItemInfoChanged.Broadcast();
	}

	RefreshTrackedItemBindings();

	// 物品新增流程完成后，统一广播背包数据变化事件。
	OnDataChange.Broadcast();
	return true;
}

bool ULxCharacterBackpackComponent::RemoveItemAt(ELxItemType InItemType, FName InItemID, int32 InItemCount)
{
	for (auto& item : m_vItemList)
	{
		if (item->GetItemDataBase()->ItemInfo.ItemType == InItemType && item->GetItemDataBase()->ItemInfo.ItemID == InItemID)
		{
			if (item->GetItemDataBase()->ItemCount >= InItemCount)
			{
				// 此处需要进行物品数量的减少，和物品的移除
				return true;
			}
			else
			{
				// 此处需要进行物品数量的减少，和物品的移除
				return true;
			}
		}
	}
	return false;
}

bool ULxCharacterBackpackComponent::CheckHaveItem(ELxItemType InItemType, FName InItemID, int32 InItemCount) const
{
	for (auto& item : m_vItemList)
	{
		if (item->GetItemDataBase()->ItemInfo.ItemType == InItemType && item->GetItemDataBase()->ItemInfo.ItemID == InItemID)
		{
			if (item->GetItemDataBase()->ItemCount >= InItemCount)
			{
				// 物品数量满足
				return true;
			}
			else
			{
				// 当前不太满足，需要继续遍历剩余物品
				InItemCount -= item->GetItemDataBase()->ItemCount;
				continue;
			}
		}
	}
	return InItemCount <= 0;
}

void ULxCharacterBackpackComponent::SortingOfItems()
{
	CleanupInvalidItems();

	// 使用一个简单的冒泡排序
	for (int i = 0;i < m_vItemList.Num() - 1; i++)
	{
		for (int j = 0; j < m_vItemList.Num() - 1; j ++)
		{
			if (m_vItemList[j] < m_vItemList[j + 1])
			{
				std::swap(m_vItemList[i], m_vItemList[j]);
			}
		}
	}
	// 先清空所有槽位
	for (auto& slot : m_vBackpackSlots)
	{
		slot->ClearItem();
	}
	// 将排序后的物品放入槽位中
	for (int i = 0;i < m_vItemList.Num() - 1; i++)
	{
		m_vBackpackSlots[i]->SetItem(m_vItemList[i]);
	}
}

TArray<TObjectPtr<ULxItemSlotData>>& ULxCharacterBackpackComponent::GetAllItems()
{
	return m_vBackpackSlots;
}

TArray<TObjectPtr<ULxItemSlotData>>& ULxCharacterBackpackComponent::QueryItemsOnItemType(ELxItemType InItemType)
{
	m_vFilteringCache.Empty();
	// 遍历所有槽位，将符合条件的物品类型，放入缓存中，
	for (auto& slot : m_vFilteringCache)
	{
		if (slot->ItemDataPtr && slot->ItemDataPtr->GetItemDataBase()->ItemInfo.ItemType == InItemType)
		{
			m_vFilteringCache.Add(slot);
		}
	}
	return m_vFilteringCache;
}

void ULxCharacterBackpackComponent::InitializeBackpack()
{
	// 初始化变量
	
	m_vFilteringCache.Empty();
	m_vBackpackSlots.Empty();
	m_vItemList.Empty();

	// 此处应当查询存档系统，获取此角色的物品信息

	// 初始化物品栏槽位
	for (int32 i = 0; i < BackpackSlotCount; ++i)
	{
		ULxItemSlotData* NewSlot = NewObject<ULxItemSlotData>(this);
		NewSlot->ItemSlotType = ELxItemSlotType::Backpack;
		NewSlot->ID = i;
		m_vBackpackSlots.Add(NewSlot);
	}
	// 此处应当查询存档系统，获取此角色的背包物品存放情况，然后用于初始化所有的槽位
	
	
}
