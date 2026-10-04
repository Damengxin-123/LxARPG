#if WITH_DEV_AUTOMATION_TESTS

#include "Editor.h"
#include "Components/MeshComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/FileManager.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "Tests/AutomationEditorCommon.h"
#include "UObject/StrongObjectPtr.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxInteractableComponent.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxInteractionNode.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxTradeContainerInteractionComponent.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxTreasureChestInteractionComponent.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxTriggerMechanismInteractionComponent.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxWarehouseInteractionComponent.h"
#include "LxARPG/LxSource/Model/Item/DataType/ItemBase/LxItemBase.h"
#include "LxARPG/LxSource/Model/Item/DataType/Slot/LxItemSlotData.h"
#include "LxARPG/LxSource/Systems/LxGameInstanceSubsystem.h"
#include "LxARPG/LxSource/Systems/MainMenu/LxMainMenuSettings.h"
#include "LxARPG/LxSource/Systems/MainMenu/LxMainMenuSubsystem.h"
#include "LxARPG/LxSource/Systems/SaveSystem/LxGameSaveData.h"
#include "LxARPG/LxSource/Systems/SaveSystem/LxInteractionSaveComponent.h"
#include "LxARPG/LxSource/Systems/SaveSystem/LxSaveFile.h"
#include "LxARPG/LxSource/Systems/SaveSystem/LxSaveManager.h"
#include "LxARPG/LxSource/Systems/SaveSystem/LxSaveProfileStore.h"
#include "LxARPG/LxSource/Systems/SettingSystem/LxGameSettings.h"

namespace
{
	/** 保存真实地图功能的身份和期望结果，不跨 PIE 持有运行对象。 */
	struct FLxInteractionPIEExpectation
	{
		/** 当前用例负责验证的功能类型。 */
		ELxInteractionActionType Type = ELxInteractionActionType::Warehouse;
		/** 首次运行从真实地图组件读取的稳定对象标识。 */
		FGameplayTag ObjectID;
		/** 首次运行从真实交互树读取的稳定节点标识。 */
		FGuid NodeID;
		/** 修改或清空的容器槽位。 */
		int32 SlotIndex = INDEX_NONE;
		/** 仓库放入物品或商人售罄商品的配置标识。 */
		FGameplayTag ItemID;
		/** 仓库或无限商品恢复后的数量，宝箱和有限商品清空后的数量为零。 */
		int32 ItemCount = 0;
		/** 地图商人完全没有有限商品时，验证无限商品不写库存并按配置恢复。 */
		bool bOnlyUnlimitedTrade = false;
		/** 真实存在的无限商品槽位，任何一个都不能进入地图库存快照。 */
		TArray<int32> UnlimitedTradeSlots;
		/** 保证机关状态与地图初始状态不同，防止恢复断言空跑。 */
		ELxMechanismState MechanismState = ELxMechanismState::Opened;
		/** 修改机关前的可移动网格体姿态，用于确认真实门扇确实响应了状态。 */
		TMap<FName, FTransform> InitialMeshTransforms;
		/** 首次修改并等待蓝图更新后得到的姿态，下一局必须自动恢复一致。 */
		TMap<FName, FTransform> SavedMeshTransforms;
	};

	/** 两次完整 PIE 共享测试身份和期望，所有磁盘访问局限在随机测试前缀。 */
	struct FLxInteractionPIEContext
	{
		/** 接收自动化断言和错误的测试实例。 */
		FAutomationTestBase* Test = nullptr;
		/** 命令行提供、经过随机标识校验的独占前缀。 */
		FString Prefix;
		/** 隔离旧单槽读取前保存的内存设置。 */
		FString OriginalLegacySlot;
		/** 隔离平台用户索引前保存的内存设置。 */
		int32 OriginalUserIndex = 0;
		/** 第一局选择的角色档，第二局必须继续同一组合。 */
		FGuid CharacterID;
		/** 第一局选择的地图档，磁盘检查只读取此地图。 */
		FGuid WorldID;
		/** 四类真实地图对象及其修改结果。 */
		TArray<FLxInteractionPIEExpectation> Expectations;
		/** 确认第二次运行已经创建新的游戏实例。 */
		TWeakObjectPtr<UGameInstance> FirstGameInstance;
		/** 第一阶段失败时后续阶段只负责安全结束，不再修改游戏。 */
		bool bFailed = false;

		/** 将失败写入统一状态，让后续命令仍然执行退出和文件清理。 */
		bool Require(bool bCondition, const TCHAR* Message)
		{
			if (!Test->TestTrue(Message, bCondition)) bFailed = true;
			return bCondition;
		}
	};

	/** 按功能类型查找真实地图模块；恢复时必须同时命中原对象和原节点。 */
	ULxInteractionActionComponentBase* FindMapFeature(UWorld* World, const FLxInteractionPIEExpectation& Expected,
		ULxInteractableComponent*& OutProvider)
	{
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			TArray<ULxInteractableComponent*> Providers;
			It->GetComponents(Providers);
			for (ULxInteractableComponent* Provider : Providers)
			{
				if (!Provider || !Provider->HasBegunPlay()
					|| (Expected.ObjectID.IsValid() && Provider->InteractionIDTag != Expected.ObjectID)) continue;
				for (ULxInteractionActionComponentBase* Feature : Provider->GetInteractionFeatures())
				{
					if (!Feature || Feature->GetInteractionActionType() != Expected.Type || !Feature->GetOwnerInteractionNode()) continue;
					if (Expected.NodeID.IsValid() && Feature->GetOwnerInteractionNode()->GetPersistentNodeID() != Expected.NodeID) continue;
					OutProvider = Provider;
					return Feature;
				}
			}
		}
		OutProvider = nullptr;
		return nullptr;
	}

	/** 检查所有已加载的持久对象都有独立标识，测试不会自动补标识。 */
	bool ValidateMapIdentities(UWorld* World, FLxInteractionPIEContext& Context)
	{
		TSet<FGameplayTag> IDs;
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			TArray<ULxInteractionSaveComponent*> Adapters;
			It->GetComponents(Adapters);
			for (ULxInteractionSaveComponent* Adapter : Adapters)
			{
				const FGameplayTag ID = Adapter->GetSaveID();
				if (!ID.IsValid() || IDs.Contains(ID))
				{
					Context.Test->AddError(FString::Printf(TEXT("真实地图对象 %s 的交互存档标识为空或重复：%s"),
						*It->GetPathName(), *ID.ToString()));
					Context.bFailed = true;
					return false;
				}
				IDs.Add(ID);
			}
		}
		return true;
	}

	/** 从选中地图的实际磁盘文件验证结果，禁止使用管理器内存快照替代。 */
	bool VerifyMapOnDisk(FLxInteractionPIEContext& Context)
	{
		TStrongObjectPtr<ULxSaveProfileStore> Store(NewObject<ULxSaveProfileStore>());
		if (!Context.Require(Store->Initialize(Context.Prefix, 0, TEXT("")), TEXT("新建仓库重新读取测试目录"))) return false;
		const FLxSaveProfile* Map = Store->GetCatalog()->Worlds.FindByPredicate(
			[&Context](const FLxSaveProfile& Profile) { return Profile.ID == Context.WorldID; });
		if (!Context.Require(Map != nullptr, TEXT("磁盘目录包含选中的地图档"))) return false;
		TStrongObjectPtr<ULxGameSaveData> Data(Cast<ULxGameSaveData>(LxSaveFile::Read(Map->Slot, 0)));
		if (!Context.Require(Data.Get() != nullptr, TEXT("读取选中地图的实际磁盘快照"))) return false;
		if (!Context.Require(Data->Players.IsEmpty(), TEXT("交互记录所在地图文件不包含角色数据"))) return false;
		for (const FLxInteractionPIEExpectation& Expected : Context.Expectations)
		{
			const FLxInteractionSaveRecord* Object = Data->Interactions.Find(Expected.ObjectID);
			if (!Context.Require(Object != nullptr, TEXT("真实对象已由正式保存入口写入地图"))) return false;
			const FLxInteractionFeatureSaveRecord* Feature = Object->Features.Find(Expected.NodeID);
			if (!Context.Require(Feature && Feature->InteractionType == Expected.Type, TEXT("地图快照包含正确的交互功能节点"))) return false;
			if (Expected.Type == ELxInteractionActionType::TriggerMechanism)
			{
				if (!Context.Require(Feature->MechanismState == Expected.MechanismState, TEXT("机关变更已经写入磁盘"))) return false;
				continue;
			}
			if (Expected.Type == ELxInteractionActionType::TradeContainer)
			{
				for (const FLxInteractionItemSaveRecord& SavedSlot : Feature->ItemSlots)
					if (!Context.Require(!Expected.UnlimitedTradeSlots.Contains(SavedSlot.SlotIndex), TEXT("无限商品没有写入地图有限库存"))) return false;
				if (Expected.bOnlyUnlimitedTrade)
				{
					if (!Context.Require(Feature->ItemSlots.IsEmpty(), TEXT("全无限商品商人的地图库存快照为空"))) return false;
					continue;
				}
			}
			const FLxInteractionItemSaveRecord* Slot = Feature->ItemSlots.FindByPredicate(
				[&Expected](const FLxInteractionItemSaveRecord& Item) { return Item.SlotIndex == Expected.SlotIndex; });
			if (!Context.Require(Slot && Slot->ItemCount == Expected.ItemCount, TEXT("容器修改后的数量已经写入磁盘"))) return false;
			if (Expected.Type != ELxInteractionActionType::TreasureChest
				&& !Context.Require(Slot->ItemIDTag == Expected.ItemID, TEXT("仓库物品与售罄商品的标识已经写入磁盘"))) return false;
		}
		return true;
	}

	/** 每次从真实主菜单进入正式玩法，再修改或验证地图对象。 */
	class FLxInteractionPIEFlowCommand : public IAutomationLatentCommand
	{
	public:
		/** 保存跨运行状态，第二轮只检查由实际生命周期自动恢复的结果。 */
		FLxInteractionPIEFlowCommand(TSharedRef<FLxInteractionPIEContext> InContext, bool bInRestore)
			: Context(InContext), bRestore(bInRestore) {}

		/** 分帧等待地图与菜单就绪，超时后仍交由后续命令结束 PIE。 */
		virtual bool Update() override
		{
			if (Context->bFailed) return true;
			if (StartedAt == 0) StartedAt = FPlatformTime::Seconds();
			if (FPlatformTime::Seconds() - StartedAt > 180)
			{
				Context->Require(false, TEXT("真实交互地图的 PIE 启动、流送或恢复超时"));
				return true;
			}
			UWorld* World = GEditor ? GEditor->PlayWorld.Get() : nullptr;
			if (!World || !World->GetGameInstance()) return false;
			ULxMainMenuSubsystem* Menu = World->GetGameInstance()->GetSubsystem<ULxMainMenuSubsystem>();
			if (!Menu || Menu->IsBusy()) return false;
			if (!bEntered)
			{
				if (!Menu->CanEnterGame()) return false;
				if (!Context->Require(Menu->GetSelectedCharacter() != nullptr, TEXT("真实主菜单具有选中的角色档"))) return true;
				if (bRestore)
				{
					if (!Context->Require(Context->FirstGameInstance.Get() != World->GetGameInstance(), TEXT("第二轮 PIE 使用全新的游戏实例"))
						|| !Context->Require(Menu->GetSelectedCharacter()->ID == Context->CharacterID, TEXT("重新启动保持选中的角色档"))
						|| !Context->Require(Menu->GetSelectedWorldID() == Context->WorldID, TEXT("重新启动保持选中的地图档"))) return true;
				}
				else
				{
					Context->CharacterID = Menu->GetSelectedCharacter()->ID;
					Context->WorldID = Menu->GetSelectedWorldID();
					Context->FirstGameInstance = World->GetGameInstance();
				}
				bEntered = true;
				Menu->EnterGame();
				return false;
			}
			if (Menu->IsEnteringGame()) return false;
			if (!Context->Require(Menu->HasSession() && World->HasBegunPlay(), TEXT("真实主菜单成功开始正式会话"))) return true;
			// 世界分区可能晚于玩家就绪，四种实际对象均出现后才检查身份与数据。
			for (const FLxInteractionPIEExpectation& Expected : Context->Expectations)
			{
				ULxInteractableComponent* Provider = nullptr;
				if (!FindMapFeature(World, Expected, Provider)) return false;
			}
			if (!ValidateMapIdentities(World, *Context)) return true;
			ULxGameInstanceSubsystem* Global = World->GetGameInstance()->GetSubsystem<ULxGameInstanceSubsystem>();
			if (!Context->Require(Global && Global->GetSaveManager() && !Global->GetSaveManager()->IsReadOnly(),
				TEXT("正式会话已启用可写的地图存档管理器"))) return true;
			if (bRestore)
			{
				if (VisualSettleStartedAt == 0) VisualSettleStartedAt = FPlatformTime::Seconds();
				if (FPlatformTime::Seconds() - VisualSettleStartedAt < .25) return false;
				VerifyRestoredFeatures(World);
				if (!Context->bFailed) CheckMechanismVisuals(World, true);
				if (!Context->bFailed) Context->Test->AddInfo(TEXT("真实总关卡退出并重新进入后，仓库、机关及门扇姿态、宝箱和商人库存规则均验证通过。"));
				return true;
			}
			if (!bChangedFeatures)
			{
				if (!ChangeFeatures(World)) return true;
				bChangedFeatures = true;
				VisualSettleStartedAt = FPlatformTime::Seconds();
				return false;
			}
			if (FPlatformTime::Seconds() - VisualSettleStartedAt < .25) return false;
			if (!CheckMechanismVisuals(World, false)) return true;
			if (!Context->Require(Global->RequestSaveGame(), TEXT("通过正式会话入口保存角色和选中地图"))) return true;
			if (!VerifyMapOnDisk(*Context)) return true;
			ChangeWarehouseBeforeShutdown(World);
			return true;
		}

	private:
		/** 手动保存后再改仓库，仅靠随后停止 PIE 的正式清理流程采集并落盘。 */
		void ChangeWarehouseBeforeShutdown(UWorld* World)
		{
			for (FLxInteractionPIEExpectation& Expected : Context->Expectations)
			{
				if (Expected.Type != ELxInteractionActionType::Warehouse) continue;
				ULxInteractableComponent* Provider = nullptr;
				ULxWarehouseInteractionComponent* Warehouse = Cast<ULxWarehouseInteractionComponent>(FindMapFeature(World, Expected, Provider));
				if (!Context->Require(Warehouse != nullptr, TEXT("退出前仍能找到真实仓库"))) return;
				ULxItemSlotData* Slot = Warehouse->GetWarehouseSlotAt(Expected.SlotIndex);
				if (!Context->Require(Slot != nullptr, TEXT("退出前仓库槽位仍有效"))) return;
				Expected.ItemCount = 13;
				ULxItemBase* Item = ULxItemBase::CreateItemObject(Warehouse, FLxItemQuote(Expected.ItemID, Expected.ItemCount));
				Context->Require(Item && Slot->SetItem(Item), TEXT("退出前再次修改仓库数量且不再手动保存"));
			}
		}

		/** 按稳定组件名采集真实机关的可移动网格体相对姿态。 */
		TMap<FName, FTransform> GetMechanismMeshTransforms(AActor* Owner) const
		{
			TMap<FName, FTransform> Result;
			TArray<UMeshComponent*> Meshes;
			Owner->GetComponents(Meshes);
			for (UMeshComponent* Mesh : Meshes)
				if (Mesh && Mesh->Mobility == EComponentMobility::Movable)
					Result.Add(Mesh->GetFName(), Mesh->GetRelativeTransform());
			return Result;
		}

		/** 第一轮确认门扇实际转动，第二轮确认蓝图已将恢复状态应用到相同组件。 */
		bool CheckMechanismVisuals(UWorld* World, bool bCheckRestored)
		{
			for (FLxInteractionPIEExpectation& Expected : Context->Expectations)
			{
				if (Expected.Type != ELxInteractionActionType::TriggerMechanism) continue;
				ULxInteractableComponent* Provider = nullptr;
				if (!Context->Require(FindMapFeature(World, Expected, Provider) != nullptr, TEXT("找到需要验证门扇姿态的真实机关"))) return false;
				const TMap<FName, FTransform> Current = GetMechanismMeshTransforms(Provider->GetOwner());
				if (!Context->Require(!Current.IsEmpty(), TEXT("真实木门具有可移动网格体"))) return false;
				if (!bCheckRestored)
				{
					bool bAnyMeshChanged = false;
					for (const auto& Pair : Current)
					{
						const FTransform* Initial = Expected.InitialMeshTransforms.Find(Pair.Key);
						bAnyMeshChanged |= Initial && !Initial->Equals(Pair.Value, .001);
					}
					if (!Context->Require(bAnyMeshChanged, TEXT("修改机关状态后真实木门网格体确实改变姿态"))) return false;
					Expected.SavedMeshTransforms = Current;
					continue;
				}
				if (!Context->Require(Current.Num() == Expected.SavedMeshTransforms.Num(), TEXT("重新进入后木门可移动网格体数量一致"))) return false;
				for (const auto& Pair : Expected.SavedMeshTransforms)
				{
					const FTransform* Restored = Current.Find(Pair.Key);
					if (!Context->Require(Restored && Restored->Equals(Pair.Value, .001), TEXT("重新进入后木门网格体姿态与已保存机关状态一致"))) return false;
				}
			}
			return true;
		}

		/** 修改真实组件数据，保留对象原有身份、交互树与注册流程。 */
		bool ChangeFeatures(UWorld* World)
		{
			for (FLxInteractionPIEExpectation& Expected : Context->Expectations)
			{
				ULxInteractableComponent* Provider = nullptr;
				ULxInteractionActionComponentBase* Feature = FindMapFeature(World, Expected, Provider);
				if (!Context->Require(Feature && Provider, TEXT("找到真实地图交互功能"))) return false;
				Expected.ObjectID = Provider->InteractionIDTag;
				Expected.NodeID = Feature->GetOwnerInteractionNode()->GetPersistentNodeID();
				if (!Context->Require(Expected.ObjectID.IsValid() && Expected.NodeID.IsValid(), TEXT("地图对象和功能节点具有稳定标识"))) return false;
				if (!Context->Require(Provider->GetOwner()->FindComponentByClass<ULxInteractionSaveComponent>() != nullptr,
					TEXT("真实对象已经自动挂载存档组件"))) return false;
				if (ULxWarehouseInteractionComponent* Warehouse = Cast<ULxWarehouseInteractionComponent>(Feature))
				{
					Expected.SlotIndex = FMath::Min(3, Warehouse->GetWarehouseItemSlots().Num() - 1);
					ULxItemSlotData* Slot = Warehouse->GetWarehouseSlotAt(Expected.SlotIndex);
					if (!Context->Require(Slot != nullptr, TEXT("真实仓库具有可存放物品的槽位"))) return false;
					Expected.ItemID = FGameplayTag::RequestGameplayTag(TEXT("物品.材料.货币.金币"));
					Expected.ItemCount = 11;
					ULxItemBase* Item = ULxItemBase::CreateItemObject(Warehouse, FLxItemQuote(Expected.ItemID, Expected.ItemCount));
					if (!Context->Require(Item != nullptr, TEXT("从游戏预设创建真实仓库物品"))) return false;
					if (!Context->Require(Slot->SetItem(Item), TEXT("成功向真实仓库槽位放入物品"))) return false;
				}
				else if (ULxTriggerMechanismInteractionComponent* Mechanism = Cast<ULxTriggerMechanismInteractionComponent>(Feature))
				{
					Expected.InitialMeshTransforms = GetMechanismMeshTransforms(Provider->GetOwner());
					Expected.MechanismState = Mechanism->GetMechanismState() == ELxMechanismState::Opened
						? ELxMechanismState::Closed : ELxMechanismState::Opened;
					Mechanism->SetMechanismState(Expected.MechanismState);
				}
				else if (ULxTreasureChestInteractionComponent* Chest = Cast<ULxTreasureChestInteractionComponent>(Feature))
				{
					const auto& Slots = Chest->GetTreasureChestItemSlots();
					for (int32 Index = 0; Index < Slots.Num(); ++Index)
					{
						if (Slots[Index] && Slots[Index]->GetItem())
						{
							Expected.SlotIndex = Index;
							Slots[Index]->ClearItem();
							break;
						}
					}
					if (!Context->Require(Expected.SlotIndex != INDEX_NONE, TEXT("真实宝箱含有可取走的预设物品"))) return false;
				}
				else if (ULxTradeContainerInteractionComponent* Trade = Cast<ULxTradeContainerInteractionComponent>(Feature))
				{
					TArray<ULxItemSlotData*> Slots;
					Trade->GetTradeItemSlotList(Slots);
					int32 LimitedSlotCount = 0;
					for (int32 Index = 0; Index < Slots.Num(); ++Index)
					{
						ULxItemSlotData* Slot = Slots[Index];
						if (!Slot) continue;
						if (Trade->IsTradeSlotLimitedStock(Slot))
						{
							++LimitedSlotCount;
							if (Slot->GetItem() && Expected.SlotIndex == INDEX_NONE)
							{
								Expected.SlotIndex = Index;
								Expected.ItemID = Slot->GetItem()->ItemIDTag();
							}
						}
						else if (Slot->GetItem()) Expected.UnlimitedTradeSlots.Add(Index);
					}
					Context->Test->AddInfo(FString::Printf(TEXT("真实商人 %s：有限商品槽位 %d 个，有物品的无限商品槽位 %d 个。"),
						*Provider->GetOwner()->GetName(), LimitedSlotCount, Expected.UnlimitedTradeSlots.Num()));
					if (LimitedSlotCount > 0)
					{
						if (!Context->Require(Expected.SlotIndex != INDEX_NONE, TEXT("真实有限商品配置具有可售罄库存"))) return false;
					}
					else
					{
						if (!Context->Require(!Expected.UnlimitedTradeSlots.IsEmpty(), TEXT("真实商人至少具有一种可验证的无限商品"))) return false;
						Expected.bOnlyUnlimitedTrade = true;
						Expected.SlotIndex = Expected.UnlimitedTradeSlots[0];
						ULxItemBase* Item = Slots[Expected.SlotIndex]->GetItem();
						Expected.ItemID = Item->ItemIDTag();
						Expected.ItemCount = Item->ItemCount();
						Context->Test->AddInfo(TEXT("真实地图商人未配置有限商品，本用例验证无限商品不写库存并按配置恢复；有限商品剩余数量及售罄恢复由 LxARPG.Save.InteractionContainers 覆盖。"));
					}
					// 仅扰动本轮运行对象；无限商品清空后必须在下轮按原预设重建，不能持久化此空槽。
					Slots[Expected.SlotIndex]->ClearItem();
				}
			}
			return true;
		}

		/** 只读取第二个世界中的运行值，不直接调用存档恢复方法。 */
		void VerifyRestoredFeatures(UWorld* World)
		{
			for (const FLxInteractionPIEExpectation& Expected : Context->Expectations)
			{
				ULxInteractableComponent* Provider = nullptr;
				ULxInteractionActionComponentBase* Feature = FindMapFeature(World, Expected, Provider);
				if (!Context->Require(Feature != nullptr, TEXT("重新加载找到同一地图对象和功能节点"))) return;
				if (ULxWarehouseInteractionComponent* Warehouse = Cast<ULxWarehouseInteractionComponent>(Feature))
				{
					ULxItemSlotData* Slot = Warehouse->GetWarehouseSlotAt(Expected.SlotIndex);
					if (!Context->Require(Slot && Slot->GetItem(), TEXT("重新进入后仓库原槽位保留物品"))) return;
					Context->Require(Slot->GetItem()->ItemIDTag() == Expected.ItemID, TEXT("重新进入后仓库物品标识正确"));
					Context->Require(Slot->GetItem()->ItemCount() == Expected.ItemCount, TEXT("重新进入后仓库物品数量正确"));
				}
				else if (ULxTriggerMechanismInteractionComponent* Mechanism = Cast<ULxTriggerMechanismInteractionComponent>(Feature))
				{
					Context->Require(Mechanism->GetMechanismState() == Expected.MechanismState, TEXT("重新进入后机关保持修改后的状态"));
				}
				else if (ULxTreasureChestInteractionComponent* Chest = Cast<ULxTreasureChestInteractionComponent>(Feature))
				{
					ULxItemSlotData* Slot = Chest->GetTreasureChestSlotAt(Expected.SlotIndex);
					Context->Require(Slot && !Slot->GetItem(), TEXT("重新进入后已取走的宝箱物品没有重新生成"));
				}
				else if (ULxTradeContainerInteractionComponent* Trade = Cast<ULxTradeContainerInteractionComponent>(Feature))
				{
					ULxItemSlotData* Slot = Trade->GetTradeSlotAt(Expected.SlotIndex);
					if (Expected.bOnlyUnlimitedTrade)
					{
						if (!Context->Require(Slot && Slot->GetItem(), TEXT("重新进入后无限商品按配置重新生成"))) return;
						Context->Require(!Trade->IsTradeSlotLimitedStock(Slot), TEXT("重新进入后无限商品的库存类型保持配置"));
						Context->Require(Slot->GetItem()->ItemIDTag() == Expected.ItemID, TEXT("无限商品仍使用原配置物品标识"));
						Context->Require(Slot->GetItem()->ItemCount() == Expected.ItemCount, TEXT("无限商品仍使用原配置数量"));
					}
					else Context->Require(Slot && !Slot->GetItem(), TEXT("重新进入后有限数量商品保持售罄"));
				}
			}
		}

		/** 两轮共享的隔离档案和期望结果。 */
		TSharedRef<FLxInteractionPIEContext> Context;
		/** 当前轮是否只验证重新进入后的结果。 */
		bool bRestore = false;
		/** 是否已经通过主菜单请求进入正式玩法。 */
		bool bEntered = false;
		/** 第一轮只修改一次数据，随后继续分帧等待蓝图更新。 */
		bool bChangedFeatures = false;
		/** 两轮均等待相同时长，让真实木门蓝图完成姿态更新。 */
		double VisualSettleStartedAt = 0;
		/** 防止场景流送、主菜单或正式启动无限等待。 */
		double StartedAt = 0;
	};

	/** 等待世界和游戏实例完全销毁，最后一轮只清理本测试的随机文件。 */
	class FLxInteractionPIEEndCommand : public IAutomationLatentCommand
	{
	public:
		/** 区分中间退出和最终清理，确保下一轮可以读到上一轮的磁盘内容。 */
		FLxInteractionPIEEndCommand(TSharedRef<FLxInteractionPIEContext> InContext, bool bInCleanup)
			: Context(InContext), bCleanup(bInCleanup) {}

		/** 退出超时则同步结束 PIE，恢复临时修改的设置后删除本用例文件。 */
		virtual bool Update() override
		{
			if (GEditor && GEditor->PlayWorld)
			{
				if (StartedAt == 0) StartedAt = FPlatformTime::Seconds();
				if (FPlatformTime::Seconds() - StartedAt <= 30) return false;
				Context->Require(false, TEXT("交互存档测试退出 PIE 超时，已同步结束运行"));
				GEditor->EndPlayMap();
				if (GEditor->PlayWorld) return false;
			}
			if (!bCleanup && !Context->bFailed)
			{
				if (VerifyMapOnDisk(*Context))
					Context->Test->AddInfo(TEXT("直接停止 PIE 已将手动保存之后新增的仓库数量 13 自动写入选中地图档。"));
			}
			if (bCleanup)
			{
				ULxGameSettings* Settings = GetMutableDefault<ULxGameSettings>();
				Settings->SaveSlotName = Context->OriginalLegacySlot;
				Settings->SaveUserIndex = Context->OriginalUserIndex;
				TArray<FString> Files;
				IFileManager::Get().FindFiles(Files, *(FPaths::ProjectSavedDir() / TEXT("SaveGames") / (Context->Prefix + TEXT("*.sav"))), true, false);
				for (const FString& File : Files)
					Context->Require(UGameplayStatics::DeleteGameInSlot(FPaths::GetBaseFilename(File), 0), TEXT("清理本次交互测试的独占存档文件"));
			}
			return true;
		}
	private:
		/** 退出时使用的测试身份与原始设置。 */
		TSharedRef<FLxInteractionPIEContext> Context;
		/** 最后一轮结束后才恢复设置并清理磁盘。 */
		bool bCleanup = false;
		/** 退出请求开始的实际时间。 */
		double StartedAt = 0;
	};
}

/** 在真实总关卡中覆盖两次完整运行和实际磁盘地图档，不补写地图对象的身份。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxInteractionMapPIEPersistenceTest,
	"LxARPG.Save.InteractionMapPIEPersistence", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/** 拒绝真实存档前缀，依次执行进入、修改、保存、退出、重新进入和自动恢复检查。 */
bool FLxInteractionMapPIEPersistenceTest::RunTest(const FString& Parameters)
{
	FString Prefix;
	FGuid UniqueID;
	if (!FParse::Value(FCommandLine::Get(), TEXT("LxMenuSavePrefix="), Prefix)
		|| !Prefix.StartsWith(TEXT("LxMenuSmoke_"))
		|| !FGuid::ParseExact(Prefix.RightChop(12), EGuidFormats::Digits, UniqueID) || !UniqueID.IsValid()
		|| !FParse::Param(FCommandLine::Get(), TEXT("LxMenuIgnoreLegacy")))
	{
		AddError(TEXT("真实地图存档测试要求 LxMenuSavePrefix=LxMenuSmoke_<32位随机GUID> 和 LxMenuIgnoreLegacy，以隔离玩家存档。"));
		return false;
	}
	if (!GEditor || GEditor->PlayWorld)
	{
		AddError(TEXT("真实地图存档测试需要空闲编辑器，不能在已有 PIE 会话中执行。"));
		return false;
	}
	TArray<FString> ExistingFiles;
	IFileManager::Get().FindFiles(ExistingFiles, *(FPaths::ProjectSavedDir() / TEXT("SaveGames") / (Prefix + TEXT("*.sav"))), true, false);
	if (!ExistingFiles.IsEmpty())
	{
		AddError(TEXT("测试前缀已经存在存档，请使用新的随机 GUID；现有文件不会被修改或清理。"));
		return false;
	}
	// 仅忽略项目已有水体笔刷的跨数据层引用问题，不忽略存档注册或恢复错误。
	AddExpectedError(TEXT("Actor /Game/项目内容/关卡/总关卡.城镇-地形_WaterBrushManager 引用另一组运行时数据层中的一个Actor /Game/项目内容/关卡/总关卡.城镇-地形"),
		EAutomationExpectedErrorFlags::Contains, 0, false);
	FAutomationEditorCommonUtils::LoadMap(GetDefault<ULxMainMenuSettings>()->DefaultLevel.ToSoftObjectPath().GetLongPackageName());
	TSharedRef<FLxInteractionPIEContext> Context = MakeShared<FLxInteractionPIEContext>();
	Context->Test = this;
	Context->Prefix = Prefix;
	ULxGameSettings* Settings = GetMutableDefault<ULxGameSettings>();
	Context->OriginalLegacySlot = Settings->SaveSlotName;
	Context->OriginalUserIndex = Settings->SaveUserIndex;
	// 游戏实例早于主菜单初始化旧存档管理器，因此同时隔离该只在内存生效的旧槽位设置。
	Settings->SaveSlotName = Prefix + TEXT("_Legacy");
	Settings->SaveUserIndex = 0;
	for (const ELxInteractionActionType Type : {ELxInteractionActionType::Warehouse, ELxInteractionActionType::TriggerMechanism,
		ELxInteractionActionType::TreasureChest, ELxInteractionActionType::TradeContainer})
		Context->Expectations.AddDefaulted_GetRef().Type = Type;
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FLxInteractionPIEFlowCommand(Context, false));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	ADD_LATENT_AUTOMATION_COMMAND(FLxInteractionPIEEndCommand(Context, false));
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FLxInteractionPIEFlowCommand(Context, true));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	ADD_LATENT_AUTOMATION_COMMAND(FLxInteractionPIEEndCommand(Context, true));
	return true;
}

#endif
