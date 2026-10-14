#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"
#include "LxARPG/LxSource/Systems/LxGameInstanceSubsystem.h"
#include "LxARPG/LxSource/Systems/SaveSystem/LxSaveManager.h"
#include "LxARPG/LxSource/Model/Content/Logic/LxCharacterContentComponent.h"
#include "LxARPG/LxSource/Model/Item/Crafting/LxItemManufacturingComponent.h"
#include "LxARPG/LxSource/Model/Item/DataType/ConstData/LxItemConstData.h"
#include "LxARPG/LxSource/Model/Item/DataType/Equipment/LxEquipment.h"
#include "LxARPG/LxSource/Model/Item/DataType/Slot/LxItemSlotData.h"
#include "LxARPG/LxSource/Model/Item/Logic/LxCharacterBackpackComponent.h"
#include "LxARPG/LxSource/Model/Item/Logic/LxSafeItemOperations.h"
#include "LxARPG/LxSource/Model/Interaction/DataType/LxInteractionTreeAsset.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxInteractableComponent.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxInteractionNode.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxFunctionPageInteractionComponent.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxTreasureChestInteractionComponent.h"
#include "LxARPG/LxSource/Model/PlayerControl/Logic/LxPlayerControlComponent.h"
#include "LxARPG/LxSource/Model/PlayerControl/Logic/LxPlayerInteractionModule.h"
#include "LxARPG/LxSource/Player/Characters/LxPlayerCharacter.h"
#include "LxARPG/LxSource/UI/Backpack/LxBackpackWidget.h"
#include "LxARPG/LxSource/UI/Crafting/LxEquipmentCraftingWidget.h"
#include "LxARPG/LxSource/UI/ItemGrid/LxItemUIData.h"
#include "LxARPG/LxSource/UI/TreasureChest/LxTreasureChestWidget.h"

namespace
{
	/** 用只读配置和真实角色模块检查交接，禁止保存到玩家存档。 */
	struct FChestCraftingQuickFixture
	{
		/** 保持真实物品表所属游戏实例存活。 */
		TStrongObjectPtr<UGameInstance> Instance;
		/** 隔离的权威测试世界。 */
		UWorld* World = nullptr;
		/** 提供背包和制造模块的真实角色。 */
		ALxPlayerCharacter* Player = nullptr;
		/** 业务窗口构建后的可见 Slate 状态。 */
		TArray<TSharedRef<SWidget>> Windows;

		/** 加载配置，创建角色并初始化业务模块。 */
		FChestCraftingQuickFixture() : Instance(NewObject<UGameInstance>())
		{
			World = UWorld::CreateWorld(EWorldType::Game, false);
			World->InitializeActorsForPlay(FURL());
			World->SetGameInstance(Instance.Get());
			Instance->Init();
			if (ULxGameInstanceSubsystem* System = Instance->GetSubsystem<ULxGameInstanceSubsystem>())
				if (System->GetSaveManager()) System->GetSaveManager()->SetReadOnly(true);
			Player = World->SpawnActor<ALxPlayerCharacter>();
			Player->GetCharacterContentComponent()->BaseComponentInitialize();
			Player->GetPlayerControlComponent()->BaseComponentInitialize();
		}
		/** 先释放业务窗口和角色，再关闭只读配置实例。 */
		~FChestCraftingQuickFixture()
		{
			Windows.Reset();
			World->DestroyWorld(false);
			Instance->Shutdown();
		}
		/** 为真实模块建立与节点一致的交互树，类型由单一根节点声明。 */
		ULxInteractableComponent* MakeProvider(ELxInteractionActionType Type, FGameplayTag EquipmentID = FGameplayTag())
		{
			AActor* Owner = World->SpawnActor<AActor>();
			ULxInteractableComponent* Provider = NewObject<ULxInteractableComponent>(Owner);
			Provider->FeatureConfig.bEnableTreasureChest = Type == ELxInteractionActionType::TreasureChest;
			Provider->FeatureConfig.TreasureChestConfig.ItemList = {FLxItemQuote(EquipmentID, 1)};
			Provider->FeatureConfig.bEnableFunctionPage = Type == ELxInteractionActionType::FunctionPage;
			Provider->FeatureConfig.FunctionPageConfig.PageWidgetClass = ULxEquipmentCraftingWidget::StaticClass();
			ULxInteractionTreeAsset* Tree = NewObject<ULxInteractionTreeAsset>(Provider);
			Tree->Features.bEnableTreasureChest = Type == ELxInteractionActionType::TreasureChest;
			Tree->Features.bEnableFunctionPage = Type == ELxInteractionActionType::FunctionPage;
			ULxInteractionTreeNodeData* Node = NewObject<ULxInteractionTreeNodeData>(Tree);
			Node->NodeId = FGuid::NewGuid();
			Node->Type = Type;
			Tree->Nodes.Add(Node);
			Tree->Roots.Add(Node->NodeId);
			if (!Provider->SetInteractionTreeAsset(Tree)) return nullptr;
			Player->GetPlayerInteractionComponent()->AddInteractableComponent(Provider);
			return Provider;
		}
		/** 使用真实 Slate 构建窗口，使可见状态与运行时相同。 */
		template<typename T> T* MakeWindow()
		{
			T* Window = NewObject<T>(World);
			Window->Initialize();
			Windows.Add(Window->TakeWidget());
			Window->SetVisibility(ESlateVisibility::Visible);
			return Window;
		}
	};
}

/** 验证宝箱只取出、失败无损、原装备实例保留，以及完成事件只看到最终背包。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxChestQuickExtractionTest, "LxARPG.UI.ItemOperations.ChestQuickExtraction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxChestQuickExtractionTest::RunTest(const FString& Parameters)
{
	FChestCraftingQuickFixture Fixture;
	if (!TestFalse(TEXT("装备配置已加载"), LxItemConfig::GetEquipmentItemMap().IsEmpty())) return false;
	const FGameplayTag EquipmentID = LxItemConfig::GetEquipmentItemMap().CreateConstIterator().Key();
	ULxInteractableComponent* Provider = Fixture.MakeProvider(ELxInteractionActionType::TreasureChest, EquipmentID);
	if (!TestNotNull(TEXT("宝箱交互树"), Provider)) return false;
	ULxTreasureChestInteractionComponent* Chest = Cast<ULxTreasureChestInteractionComponent>(Provider->GetRootInteractionNodes()[0]->GetInteractionFeature());
	ULxTreasureChestWidget* Page = Fixture.MakeWindow<ULxTreasureChestWidget>();
	Page->SetTreasureChestComponent(Chest);
	ULxItemSlotData* SourceSlot = Chest ? Chest->GetTreasureChestSlotAt(0) : nullptr;
	ULxEquipment* Item = SourceSlot ? Cast<ULxEquipment>(SourceSlot->GetItem()) : nullptr;
	if (!TestNotNull(TEXT("宝箱持有真实装备实例"), Item)) return false;
	TestTrue(TEXT("准备宝箱装备强化状态"), Item->ApplyCraftingState(.47f, Item->GetNormalEntries()));
	const FGuid InstanceID = Item->GetInstanceID();
	FText Reason;
	FLxItemContainerAddress Address;
	TestTrue(TEXT("宝箱提供快速取出地址"), Page->GetQuickItemContainer_Implementation(Address));
	TestEqual(TEXT("地址为宝箱模块"), Address.Kind, ELxItemContainerKind::TreasureChest);
	const TArray<UObject*> UIItems = Page->GetTreasureChestItemUIDataList();
	const ULxItemUIData* UIData = UIItems.IsEmpty() ? nullptr : Cast<ULxItemUIData>(UIItems[0]);
	TestTrue(TEXT("宝箱格子提供来源窗口"), UIData && UIData->ItemOwnerWindow.Get() == Page);
	ULxItemTransferTicket* Ticket = Page->PrepareQuickItemTransfer_Implementation(SourceSlot, Reason);
	if (!TestNotNull(TEXT("UI委托模块创建安全凭证"), Ticket)) return false;
	TestFalse(TEXT("宝箱拒绝快速存入"), Page->CanReceiveQuickItemTransfer_Implementation(Ticket, Reason));
	TestFalse(TEXT("宝箱格子不能拆分"), LxSafeItemOperations::CanSplitItem(SourceSlot, Reason));
	ULxCharacterBackpackModule* Backpack = Fixture.Player->GetCharacterBackpackComponent();
	for (ULxItemSlotData* Slot : Backpack->GetAllItems())
	{
		ULxItemBase* Gold = ULxItemBase::CreateItemObject(Backpack, FLxItemQuote(LxTag_Item_Material_Currency_Gold, 1));
		Slot->SetItem(Gold);
	}
	TestFalse(TEXT("背包无空格时快速取出失败"), Backpack->AcceptSafeItem(Ticket, Reason));
	TestTrue(TEXT("容量失败保留宝箱原实例"), SourceSlot->GetItem() == Item);
	TestEqual(TEXT("失败保留装备强化"), Item->GetEnhancement(), .47f);
	TestFalse(TEXT("失败不完成宝箱"), Chest->GetInteractionState() == ELxInteractionDataState::Finished);
	ULxItemSlotData* TargetSlot = Backpack->GetBackpackSlotAt(0);
	TargetSlot->ClearItem();
	int32 InventoryNotifications = 0;
	int32 Completed = 0;
	bool bCompletedAfterCommit = false;
	Chest->OnTreasureChestSlotListChanged.AddLambda([&](const TArray<ULxItemSlotData*>&) { ++InventoryNotifications; });
	Chest->OnItemAcquireCompleted.AddLambda([&]() { ++Completed; bCompletedAfterCommit = TargetSlot->GetItem() == Item && !SourceSlot->GetItem(); });
	TestTrue(TEXT("空格可用时完整取出"), Backpack->AcceptSafeItem(Ticket, Reason));
	TestTrue(TEXT("目标保存同一个装备对象"), TargetSlot->GetItem() == Item);
	TestTrue(TEXT("成功才清空宝箱来源"), !SourceSlot->GetItem());
	TestEqual(TEXT("装备实例ID保留"), Item->GetInstanceID(), InstanceID);
	TestEqual(TEXT("装备强化保留"), Item->GetEnhancement(), .47f);
	TestEqual(TEXT("宝箱库存只发布一次完整结果"), InventoryNotifications, 1);
	TestEqual(TEXT("宝箱只完成一次"), Completed, 1);
	TestTrue(TEXT("完成回调观察到完整接收"), bCompletedAfterCommit);
	TestEqual(TEXT("装备归属迁入背包"), Item->GetOuter(), static_cast<UObject*>(Backpack));
	return true;
}

/** 验证打造快捷链接使用原装备，不移动、不扣费，并保留失败之前的选择。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxCraftQuickEquipmentLinkTest, "LxARPG.UI.ItemOperations.CraftingQuickEquipmentLink",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxCraftQuickEquipmentLinkTest::RunTest(const FString& Parameters)
{
	FChestCraftingQuickFixture Fixture;
	if (!TestFalse(TEXT("装备配置已加载"), LxItemConfig::GetEquipmentItemMap().IsEmpty())) return false;
	const FGameplayTag EquipmentID = LxItemConfig::GetEquipmentItemMap().CreateConstIterator().Key();
	ULxCharacterBackpackModule* Backpack = Fixture.Player->GetCharacterBackpackComponent();
	TestTrue(TEXT("准备两件装备及金币"), Backpack->AddItemList({FLxItemQuote(EquipmentID, 1), FLxItemQuote(EquipmentID, 1), FLxItemQuote(LxTag_Item_Material_Currency_Gold, 30)}));
	ULxItemSlotData* SourceSlot = Backpack->GetBackpackSlotAt(0);
	ULxItemSlotData* SecondSlot = Backpack->GetBackpackSlotAt(1);
	ULxEquipment* Item = Cast<ULxEquipment>(SourceSlot->GetItem());
	if (!TestNotNull(TEXT("真实装备实例"), Item)) return false;
	ULxInteractableComponent* Provider = Fixture.MakeProvider(ELxInteractionActionType::FunctionPage);
	if (!TestNotNull(TEXT("打造NPC交互树"), Provider)) return false;
	ULxFunctionPageInteractionComponent* Service = Cast<ULxFunctionPageInteractionComponent>(Provider->GetRootInteractionNodes()[0]->GetInteractionFeature());
	ULxItemManufacturingComponent* Manufacturing = Fixture.Player->GetItemManufacturingComponent();
	TestTrue(TEXT("开启制造服务"), Manufacturing->BeginEquipmentService(Service));
	ULxEquipmentCraftingWidget* Page = Fixture.MakeWindow<ULxEquipmentCraftingWidget>();
	Page->SetManufacturingComponent(Manufacturing);
	ULxBackpackWidget* SourceWindow = Fixture.MakeWindow<ULxBackpackWidget>();
	const int32 Gold = Manufacturing->GetGoldCount();
	const FGuid ID = Item->GetInstanceID();
	FText Reason;
	ULxItemTransferTicket* Ticket = Backpack->BeginSafeExtractItem(0, Reason);
	if (!TestNotNull(TEXT("准备完整装备凭证"), Ticket)) return false;
	TestTrue(TEXT("打造只读预检查装备"), Page->CanReceiveQuickItemTransfer_Implementation(Ticket, Reason));
	TestTrue(TEXT("预检查不改变选择"), !Manufacturing->GetSelectedEquipment());
	FLxItemOperationRequest Request;
	Request.RequestId = FGuid::NewGuid();
	Request.Source.Kind = ELxItemContainerKind::Backpack;
	Request.Source.Owner = Fixture.Player;
	Request.SourceSlotIndex = 0;
	Request.Count = Ticket->Count;
	Request.ExpectedItem = Ticket->ExpectedItem;
	TestTrue(TEXT("快捷链接原装备"), Page->ReceiveQuickItemTransfer_Implementation(Ticket, Request, SourceWindow, Reason));
	TestTrue(TEXT("制造模块保存原装备引用"), Manufacturing->GetSelectedEquipment() == Item);
	TestTrue(TEXT("装备仍在背包原槽"), SourceSlot->GetItem() == Item);
	TestEqual(TEXT("快捷链接不消耗金币"), Manufacturing->GetGoldCount(), Gold);
	TestEqual(TEXT("链接保留装备身份"), Manufacturing->GetSelectedEquipment()->GetInstanceID(), ID);
	TestTrue(TEXT("成功后终结提取凭证"), Ticket->IsCompleted());
	const FStructProperty* SourceResultProperty = FindFProperty<FStructProperty>(SourceWindow->GetClass(), TEXT("LastItemOperationResult"));
	const FLxItemOperationResult* SourceResult = SourceResultProperty
		? SourceResultProperty->ContainerPtrToValuePtr<FLxItemOperationResult>(SourceWindow) : nullptr;
	TestTrue(TEXT("来源窗口得到统一成功结果"), SourceResult && SourceResult->bSucceeded && SourceResult->RequestId == Request.RequestId);
	TestTrue(TEXT("打造窗口得到统一成功结果"), Page->LastItemOperationResult.bSucceeded);
	ULxItemSlotData* GoldSlot = nullptr;
	for (ULxItemSlotData* Slot : Backpack->GetAllItems())
		if (Slot && Slot->GetItem() && Slot->GetItem()->ItemIDTag() == LxTag_Item_Material_Currency_Gold) GoldSlot = Slot;
	ULxItemTransferTicket* GoldTicket = GoldSlot ? Backpack->BeginSafeExtractItem(GoldSlot->GetSlotIndex(), Reason) : nullptr;
	if (TestNotNull(TEXT("准备非装备凭证"), GoldTicket))
	{
		TestFalse(TEXT("打造拒绝金币等非装备物品"), Page->CanReceiveQuickItemTransfer_Implementation(GoldTicket, Reason));
		GoldTicket->Cancel();
	}
	Ticket = Backpack->BeginSafeExtractItem(1, Reason);
	FLxItemSaveRecord Before;
	LxItemSaveData::CaptureItem(SecondSlot->GetItem(), Before);
	Page->SetIsEnabled(false);
	TestFalse(TEXT("禁用打造窗口拒绝快捷链接"), Page->CanReceiveQuickItemTransfer_Implementation(Ticket, Reason));
	TestTrue(TEXT("失败保持先前选择"), Manufacturing->GetSelectedEquipment() == Item);
	TestTrue(TEXT("失败保持第二件装备"), LxSafeItemOperations::MatchesExpectedRecord(SecondSlot, Before));
	TestEqual(TEXT("失败保持金币"), Manufacturing->GetGoldCount(), Gold);
	Page->SetIsEnabled(true);
	Manufacturing->EndEquipmentService();
	TestFalse(TEXT("服务关闭后不能继续链接"), Page->CanReceiveQuickItemTransfer_Implementation(Ticket, Reason));
	FLxItemContainerAddress Address;
	TestFalse(TEXT("打造格子不作为可提取容器"), Page->GetQuickItemContainer_Implementation(Address));
	Ticket->Cancel();
	Page->SetManufacturingComponent(nullptr);
	return true;
}

#endif
