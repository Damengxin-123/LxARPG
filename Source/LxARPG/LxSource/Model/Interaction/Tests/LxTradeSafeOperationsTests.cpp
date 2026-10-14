#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "UObject/StrongObjectPtr.h"
#include "LxARPG/LxSource/Model/Content/Logic/LxCharacterContentComponent.h"
#include "LxARPG/LxSource/Model/DataTransfer/LxCharacterDataTransferComponent.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxTradeContainerInteractionComponent.h"
#include "LxARPG/LxSource/Model/Item/DataType/ConstData/LxItemConstData.h"
#include "LxARPG/LxSource/Model/Item/DataType/Equipment/LxEquipment.h"
#include "LxARPG/LxSource/Model/Item/DataType/Slot/LxItemSlotData.h"
#include "LxARPG/LxSource/Model/Item/Logic/LxCharacterBackpackComponent.h"
#include "LxARPG/LxSource/Model/Item/Logic/LxSafeItemOperations.h"
#include "LxARPG/LxSource/Player/Characters/LxPlayerCharacter.h"
#include "LxARPG/LxSource/Systems/LxGameInstanceSubsystem.h"
#include "LxARPG/LxSource/Systems/SaveSystem/LxSaveManager.h"

namespace
{
	/** 真实玩家背包与交易模块组成的隔离环境，全部测试禁止写入存档。 */
	struct FSafeTradeFixture
	{
		/** 游戏实例加载实际物品配置并持有只读存档管理器。 */
		TStrongObjectPtr<UGameInstance> Instance;
		/** 保持测试交易模块存活，不占用用户场景中的商城。 */
		TStrongObjectPtr<ULxTradeContainerInteractionComponent> Trade;
		/** 测试角色所在的权威单机世界。 */
		UWorld* World = nullptr;
		/** 初始化后的真实背包模块。 */
		ULxCharacterBackpackModule* Backpack = nullptr;
		/** 真实数据中转组件供原有买卖入口调用。 */
		ULxCharacterDataTransferComponent* DataTransfer = nullptr;

		/** 初始化独立世界、只读游戏实例和角色数据模块。 */
		FSafeTradeFixture() : Instance(NewObject<UGameInstance>()), Trade(NewObject<ULxTradeContainerInteractionComponent>())
		{
			World = UWorld::CreateWorld(EWorldType::Game, false);
			World->InitializeActorsForPlay(FURL());
			World->SetGameInstance(Instance.Get());
			Instance->Init();
			if (ULxGameInstanceSubsystem* System = Instance->GetSubsystem<ULxGameInstanceSubsystem>())
				if (System->GetSaveManager()) System->GetSaveManager()->SetReadOnly(true);
			ALxPlayerCharacter* Player = World->SpawnActor<ALxPlayerCharacter>();
			if (!Player) return;
			Player->GetCharacterContentComponent()->BaseComponentInitialize();
			DataTransfer = Player->GetCharacterDataTransferComponent();
			DataTransfer->BaseComponentInitialize();
			Backpack = Player->GetCharacterBackpackComponent();
		}

		/** 关闭隔离实例与场景，不保存测试库存。 */
		~FSafeTradeFixture()
		{
			Instance->Shutdown();
			World->SetGameInstance(nullptr);
			World->DestroyWorld(false);
		}

		/** 选择有真实正数售价的普通可堆叠商品。 */
		FGameplayTag FindCommodity() const
		{
			for (const auto& Pair : LxItemConfig::GetConsumableItemMap())
				if (Pair.Value.ItemSellPrice > 0 && Pair.Value.ItemCountMax >= 2) return Pair.Key;
			for (const auto& Pair : LxItemConfig::GetMaterialItemMap())
				if (Pair.Key != LxTag_Item_Material_Currency_Gold && Pair.Value.ItemSellPrice > 0 && Pair.Value.ItemCountMax >= 2) return Pair.Key;
			return FGameplayTag();
		}

		/** 配置并初始化一个商品，保持有限和无限库存业务语义。 */
		ULxItemSlotData* ConfigureTrade(FGameplayTag ItemID, int32 Count, bool bLimited = true)
		{
			FLxTradeContainerInteractionConfig Config;
			FLxTradeItemConfig& Item = Config.TradeItems.AddDefaulted_GetRef();
			Item.ItemIDTag = ItemID;
			Item.ItemCount = Count;
			Item.bLimitedStock = bLimited;
			Trade->ApplyConfig(Config);
			Trade->OnInitializeInteractionFeature();
			return Trade->GetTradeSlotAt(0);
		}

		/** 创建完整保存记录，用于真实背包的整体恢复。 */
		FLxItemSlotSaveRecord MakeSaved(int32 SlotIndex, FGameplayTag ItemID, int32 Count)
		{
			FLxItemSlotSaveRecord Record;
			Record.SlotIndex = SlotIndex;
			Record.SlotTag = LxTag_Item;
			ULxItemBase* Item = ULxItemBase::CreateItemObject(Backpack, FLxItemQuote(ItemID, Count));
			LxItemSaveData::CaptureItem(Item, Record.Item);
			return Record;
		}

		/** 将商品价格调整为较小确定数值，避免真实配置价格影响容量测试。 */
		void SetPurchasePrice(ULxItemSlotData* Stock, int32 Price)
		{
			const int64 Base = static_cast<int64>(Stock->GetItem()->ItemInformation().ItemSellPrice) * Stock->GetItem()->ItemCount();
			Trade->SetTradeItemValueRate(static_cast<float>(Price) / static_cast<float>(Base));
		}
	};
}

/** 购买先完整规划金币与商品，失败保留对象且通知回调只观察最终状态。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxTradeAtomicPurchaseTest, "LxARPG.Items.SafeOperations.TradeAtomicPurchase",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxTradeAtomicPurchaseTest::RunTest(const FString& Parameters)
{
	FSafeTradeFixture Fixture;
	if (!TestNotNull(TEXT("真实背包模块"), Fixture.Backpack)) return false;
	const FGameplayTag CommodityID = Fixture.FindCommodity();
	if (!TestTrue(TEXT("存在有价可堆叠商品"), CommodityID.IsValid())) return false;
	ULxItemSlotData* Stock = Fixture.ConfigureTrade(CommodityID, 2);
	if (!TestTrue(TEXT("真实有限商品完整创建"), Stock && Stock->GetItem())) return false;
	Fixture.SetPurchasePrice(Stock, 4);
	const int32 Price = Stock->GetItemValue();
	if (!TestEqual(TEXT("确定购买价"), Price, 4)) return false;
	ULxItemBase* OriginalStock = Stock->GetItem();
	FLxItemSaveRecord ExpectedStock;
	LxItemSaveData::CaptureItem(OriginalStock, ExpectedStock);

	if (!TestTrue(TEXT("准备未能释放空格的金币背包"), Fixture.Backpack->RestoreBackpackSaveData(1,
		{Fixture.MakeSaved(0, LxTag_Item_Material_Currency_Gold, Price + 1)}))) return false;
	ULxItemSlotData* GoldSlot = Fixture.Backpack->GetBackpackSlotAt(0);
	if (!TestNotNull(TEXT("满包金币槽位存在"), GoldSlot)) return false;
	ULxItemBase* OriginalGold = GoldSlot->GetItem();
	if (!TestNotNull(TEXT("满包金币实例存在"), OriginalGold)) return false;
	TestFalse(TEXT("满包预检拒绝购买"), Fixture.Trade->CanBuyTradeSlot(Stock, Fixture.DataTransfer));
	TestFalse(TEXT("满包执行不能先扣金币"), Fixture.Trade->BuyTradeSlot(Stock, Fixture.DataTransfer));
	TestTrue(TEXT("失败金币保留原实例"), GoldSlot->GetItem() == OriginalGold);
	TestEqual(TEXT("失败金币余额不变"), OriginalGold->ItemCount(), Price + 1);
	TestTrue(TEXT("失败商品完整记录不变"), LxSafeItemOperations::MatchesExpectedRecord(Stock, ExpectedStock));

	if (!TestTrue(TEXT("准备金币不足但有空格的背包"), Fixture.Backpack->RestoreBackpackSaveData(2,
		{Fixture.MakeSaved(0, LxTag_Item_Material_Currency_Gold, Price - 1)}))) return false;
	GoldSlot = Fixture.Backpack->GetBackpackSlotAt(0);
	if (!TestNotNull(TEXT("不足余额金币槽位存在"), GoldSlot)) return false;
	OriginalGold = GoldSlot->GetItem();
	if (!TestNotNull(TEXT("不足余额金币实例存在"), OriginalGold)) return false;
	TestFalse(TEXT("金币不足拒绝购买"), Fixture.Trade->BuyTradeSlot(Stock, Fixture.DataTransfer));
	TestTrue(TEXT("金币不足保留金币对象"), GoldSlot->GetItem() == OriginalGold);
	TestEqual(TEXT("金币不足不扣任何数量"), OriginalGold->ItemCount(), Price - 1);
	TestTrue(TEXT("金币不足保留商品对象"), Stock->GetItem() == OriginalStock);

	if (!TestTrue(TEXT("支付恰好可释放原金币格"), Fixture.Backpack->RestoreBackpackSaveData(1,
		{Fixture.MakeSaved(0, LxTag_Item_Material_Currency_Gold, Price)}))) return false;
	GoldSlot = Fixture.Backpack->GetBackpackSlotAt(0);
	if (!TestNotNull(TEXT("完整付款金币槽位存在"), GoldSlot)) return false;
	OriginalGold = GoldSlot->GetItem();
	if (!TestNotNull(TEXT("完整付款金币实例存在"), OriginalGold)) return false;
	TStrongObjectPtr<ULxItemSlotData> GoldShortcut(NewObject<ULxItemSlotData>(Fixture.Instance.Get()));
	GoldShortcut->InitItemSlot(ELxItemSlotType::Shortcut, LxTag_Item, OriginalGold);
	int32 DirectGoldEvents = 0;
	int32 ShortcutEvents = 0;
	const FDelegateHandle DirectGoldHandle = OriginalGold->OnItemCountChanged.AddLambda([&](ULxItemBase*) { ++DirectGoldEvents; });
	const FDelegateHandle ShortcutHandle = GoldShortcut->OnItemDataChanged.AddLambda([&](ULxItemBase*) { ++ShortcutEvents; });
	TestTrue(TEXT("预检计入付款后释放的空格"), Fixture.Trade->CanBuyTradeSlot(Stock, Fixture.DataTransfer));
	int32 BackpackChanges = 0;
	bool bFinalOnly = true;
	bool bReentryDenied = false;
	const FDelegateHandle ChangedHandle = Fixture.Backpack->OnDataChange.AddLambda([&]() { ++BackpackChanges; });
	const FDelegateHandle SlotHandle = GoldSlot->OnItemDataChanged.AddLambda([&](ULxItemBase*)
	{
		bFinalOnly &= GoldSlot->GetItem() == OriginalStock && Stock->GetItem() == nullptr;
		bReentryDenied = !Fixture.Trade->SellBackpackSlot(GoldSlot, Fixture.DataTransfer)
			&& !Fixture.Backpack->AddItemList({FLxItemQuote(LxTag_Item_Material_Currency_Gold, 1)});
	});
	TestTrue(TEXT("购买将商品放到支付后空格"), Fixture.Trade->BuyTradeSlot(Stock, Fixture.DataTransfer));
	GoldSlot->OnItemDataChanged.Remove(SlotHandle);
	OriginalGold->OnItemCountChanged.Remove(DirectGoldHandle);
	GoldShortcut->OnItemDataChanged.Remove(ShortcutHandle);
	Fixture.Backpack->OnDataChange.Remove(ChangedHandle);
	TestTrue(TEXT("所有回调只看到商品与金币最终状态"), bFinalOnly);
	TestTrue(TEXT("回调不能重入出售或金币添加"), bReentryDenied);
	TestEqual(TEXT("背包只发布一次完整交易状态"), BackpackChanges, 1);
	TestEqual(TEXT("耗尽金币对象归零"), OriginalGold->ItemCount(), 0);
	TestEqual(TEXT("金币直接数量监听发布一次"), DirectGoldEvents, 1);
	TestEqual(TEXT("金币快捷引用收到一次更新"), ShortcutEvents, 1);
	TestFalse(TEXT("已耗尽金币的快捷引用失效"), GoldShortcut->IsValid());
	TestTrue(TEXT("有限库存转移原商品对象"), GoldSlot->GetItem() == OriginalStock);
	TestTrue(TEXT("商品对象归属迁移到背包"), OriginalStock->GetOuter() == Fixture.Backpack);
	TestNull(TEXT("有限库存售罄"), Stock->GetItem());
	return true;
}

/** 出售收入必须全量容纳，任何失败均保留原物品、金币对象与完整数量。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxTradeAtomicSaleTest, "LxARPG.Items.SafeOperations.TradeAtomicSale",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxTradeAtomicSaleTest::RunTest(const FString& Parameters)
{
	FSafeTradeFixture Fixture;
	if (!TestNotNull(TEXT("真实背包模块"), Fixture.Backpack)) return false;
	const FGameplayTag CommodityID = Fixture.FindCommodity();
	if (!TestTrue(TEXT("存在可出售商品"), CommodityID.IsValid())) return false;
	Fixture.ConfigureTrade(CommodityID, 2);
	const FLxItemInformationBase* GoldConfig = LxItemConfig::GetItemData(LxTag_Item_Material_Currency_Gold);
	if (!TestTrue(TEXT("金币真实堆叠配置有效"), GoldConfig && GoldConfig->ItemCountMax >= 5)) return false;
	if (!TestTrue(TEXT("准备商品和已满金币的真实背包"), Fixture.Backpack->RestoreBackpackSaveData(2,
		{Fixture.MakeSaved(0, CommodityID, 2), Fixture.MakeSaved(1, LxTag_Item_Material_Currency_Gold, GoldConfig->ItemCountMax)}))) return false;
	ULxItemSlotData* Source = Fixture.Backpack->GetBackpackSlotAt(0);
	ULxItemSlotData* GoldSlot = Fixture.Backpack->GetBackpackSlotAt(1);
	if (!TestNotNull(TEXT("待出售来源格存在"), Source) || !TestNotNull(TEXT("满金币格存在"), GoldSlot)) return false;
	ULxItemBase* OriginalSource = Source->GetItem();
	ULxItemBase* OriginalGold = GoldSlot->GetItem();
	if (!TestNotNull(TEXT("待出售原实例存在"), OriginalSource) || !TestNotNull(TEXT("满金币原实例存在"), OriginalGold)) return false;
	// 隔离来源格只接受商品类别，收入不能利用该空格；不向恢复接口传入非法槽位记录。
	Source->InitItemSlot(ELxItemSlotType::Backpack, CommodityID, OriginalSource);
	FLxItemSaveRecord ExpectedSource;
	LxItemSaveData::CaptureItem(OriginalSource, ExpectedSource);
	Fixture.Trade->SetPurchaseValueRate(static_cast<float>(MAX_int32));
	TestFalse(TEXT("超大收入无法全部放置时预检拒绝"), Fixture.Trade->CanSellBackpackSlot(Source, Fixture.DataTransfer));
	TestFalse(TEXT("出售收入不足空间时不清除物品"), Fixture.Trade->SellBackpackSlot(Source, Fixture.DataTransfer));
	TestTrue(TEXT("失败来源完整记录保留"), LxSafeItemOperations::MatchesExpectedRecord(Source, ExpectedSource));
	TestTrue(TEXT("失败来源对象保留"), Source->GetItem() == OriginalSource);
	TestTrue(TEXT("失败金币对象保留"), GoldSlot->GetItem() == OriginalGold);
	TestEqual(TEXT("失败金币未部分添加"), OriginalGold->ItemCount(), GoldConfig->ItemCountMax);

	if (!TestTrue(TEXT("恢复可出售商品与金币"), Fixture.Backpack->RestoreBackpackSaveData(2,
		{Fixture.MakeSaved(0, CommodityID, 2), Fixture.MakeSaved(1, LxTag_Item_Material_Currency_Gold, 1)}))) return false;
	Source = Fixture.Backpack->GetBackpackSlotAt(0);
	GoldSlot = Fixture.Backpack->GetBackpackSlotAt(1);
	if (!TestNotNull(TEXT("成功出售来源格存在"), Source) || !TestNotNull(TEXT("成功出售金币格存在"), GoldSlot)) return false;
	OriginalSource = Source->GetItem();
	OriginalGold = GoldSlot->GetItem();
	if (!TestNotNull(TEXT("成功出售原实例存在"), OriginalSource) || !TestNotNull(TEXT("成功出售金币原实例存在"), OriginalGold)) return false;
	TStrongObjectPtr<ULxItemSlotData> SoldShortcut(NewObject<ULxItemSlotData>(Fixture.Instance.Get()));
	SoldShortcut->InitItemSlot(ELxItemSlotType::Shortcut, LxTag_Item, OriginalSource);
	int32 SoldItemEvents = 0;
	const FDelegateHandle SoldHandle = OriginalSource->OnItemCountChanged.AddLambda([&](ULxItemBase*) { ++SoldItemEvents; });

	const int32 UnitPrice = OriginalSource->ItemInformation().ItemSellPrice;
	Fixture.Trade->SetPurchaseValueRate(3.0f / (static_cast<float>(UnitPrice) * static_cast<float>(OriginalSource->ItemCount())));
	const int32 SalePrice = Fixture.Trade->GetBackpackSlotSellPrice(Source);
	TestEqual(TEXT("确定整组收购价"), SalePrice, 3);
	int32 Changes = 0;
	bool bFinalOnly = true;
	bool bNestedRejected = false;
	const FDelegateHandle ChangeHandle = Fixture.Backpack->OnDataChange.AddLambda([&]() { ++Changes; });
	const FDelegateHandle SlotHandle = Source->OnItemDataChanged.AddLambda([&](ULxItemBase*)
	{
		bFinalOnly &= Source->GetItem() == nullptr && GoldSlot->GetItem() == OriginalGold
			&& OriginalGold->ItemCount() == 1 + SalePrice;
		bNestedRejected = !Fixture.Trade->BuyTradeSlot(Fixture.Trade->GetTradeSlotAt(0), Fixture.DataTransfer);
	});
	TestTrue(TEXT("出售成功统一提交商品和金币"), Fixture.Trade->SellBackpackSlot(Source, Fixture.DataTransfer));
	Source->OnItemDataChanged.Remove(SlotHandle);
	OriginalSource->OnItemCountChanged.Remove(SoldHandle);
	Fixture.Backpack->OnDataChange.Remove(ChangeHandle);
	TestTrue(TEXT("出售通知只读到最终金币余额"), bFinalOnly);
	TestTrue(TEXT("出售回调不能重入购买"), bNestedRejected);
	TestEqual(TEXT("出售只发布一个背包批次"), Changes, 1);
	TestTrue(TEXT("正常金币堆叠保留原实例"), GoldSlot->GetItem() == OriginalGold);
	TestEqual(TEXT("出售增加准确整组收入"), OriginalGold->ItemCount(), 1 + SalePrice);
	TestNull(TEXT("只有成功出售才移除来源"), Source->GetItem());
	TestEqual(TEXT("售出实例数量归零"), OriginalSource->ItemCount(), 0);
	TestEqual(TEXT("售出实例数量监听发布一次"), SoldItemEvents, 1);
	TestFalse(TEXT("售出物品快捷引用失效"), SoldShortcut->IsValid());
	TestEqual(TEXT("超大价格在转整数前饱和"),
		ULxTradeContainerInteractionComponent::CalculateStackTotalPrice(MAX_int32, MAX_int32, 1000.0f), MAX_int32);
	return true;
}

/** 有限装备保持原对象，无限商品克隆完整状态并分配独立装备身份。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxTradeEquipmentIdentityTest, "LxARPG.Items.SafeOperations.TradeEquipmentIdentity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxTradeEquipmentIdentityTest::RunTest(const FString& Parameters)
{
	FSafeTradeFixture Fixture;
	if (!TestNotNull(TEXT("真实背包模块"), Fixture.Backpack)) return false;
	const auto& Configs = LxItemConfig::GetEquipmentItemMap();
	if (!TestTrue(TEXT("存在装备配置"), !Configs.IsEmpty())) return false;
	const FGameplayTag ID = Configs.CreateConstIterator().Key();
	ULxItemSlotData* Stock = Fixture.ConfigureTrade(ID, 1);
	ULxEquipment* Original = Stock ? Cast<ULxEquipment>(Stock->GetItem()) : nullptr;
	if (!TestNotNull(TEXT("有限装备商品"), Original)) return false;
	TestTrue(TEXT("装备商品包含强化状态"), Original->ApplyCraftingState(0.37f, Original->GetNormalEntries()));
	const FGuid OriginalID = Original->GetInstanceID();
	Fixture.Trade->SetTradeItemValueRate(0.0f);
	if (!TestTrue(TEXT("空背包"), Fixture.Backpack->RestoreBackpackSaveData(2, {}))) return false;
	if (!TestTrue(TEXT("购买有限装备"), Fixture.Trade->BuyTradeSlot(Stock, Fixture.DataTransfer))) return false;
	ULxItemSlotData* LimitedDestination = Fixture.Backpack->GetBackpackSlotAt(0);
	if (!TestNotNull(TEXT("有限装备购买目标格存在"), LimitedDestination)) return false;
	TestTrue(TEXT("有限装备为同一对象"), LimitedDestination->GetItem() == Original);
	TestEqual(TEXT("有限装备身份不变"), Original->GetInstanceID(), OriginalID);
	TestEqual(TEXT("有限装备强化不变"), Original->GetEnhancement(), 0.37f);

	/** 使用独立商城模块重新建立无限商品，避免重置原有限模块的库存含义。 */
	Fixture.Trade.Reset(NewObject<ULxTradeContainerInteractionComponent>());
	Stock = Fixture.ConfigureTrade(ID, 1, false);
	if (!TestNotNull(TEXT("无限商品模板格存在"), Stock)) return false;
	ULxEquipment* Template = Cast<ULxEquipment>(Stock->GetItem());
	if (!TestNotNull(TEXT("无限装备商品模板"), Template)) return false;
	TestTrue(TEXT("修改无限模板强化"), Template->ApplyCraftingState(0.61f, Template->GetNormalEntries()));
	Fixture.Trade->SetTradeItemValueRate(0.0f);
	if (!TestTrue(TEXT("恢复空背包供无限商品购买"), Fixture.Backpack->RestoreBackpackSaveData(2, {}))) return false;
	FLxItemSaveRecord TemplateRecord;
	LxItemSaveData::CaptureItem(Template, TemplateRecord);
	if (!TestTrue(TEXT("无限装备第一次购买"), Fixture.Trade->BuyTradeSlot(Stock, Fixture.DataTransfer))) return false;
	if (!TestTrue(TEXT("无限装备第二次购买"), Fixture.Trade->BuyTradeSlot(Stock, Fixture.DataTransfer))) return false;
	ULxItemSlotData* FirstDestination = Fixture.Backpack->GetBackpackSlotAt(0);
	ULxItemSlotData* SecondDestination = Fixture.Backpack->GetBackpackSlotAt(1);
	if (!TestNotNull(TEXT("无限装备第一目标格存在"), FirstDestination)
		|| !TestNotNull(TEXT("无限装备第二目标格存在"), SecondDestination)) return false;
	ULxEquipment* First = Cast<ULxEquipment>(FirstDestination->GetItem());
	ULxEquipment* Second = Cast<ULxEquipment>(SecondDestination->GetItem());
	if (!TestTrue(TEXT("无限商品生成两个独立完整对象"), First && Second && First != Second && First != Template && Second != Template)) return false;
	TestTrue(TEXT("每件装备具有不同实例身份"), First->GetInstanceID() != Second->GetInstanceID()
		&& First->GetInstanceID() != Template->GetInstanceID() && Second->GetInstanceID() != Template->GetInstanceID());
	TestEqual(TEXT("第一件保留强化"), First->GetEnhancement(), 0.61f);
	TestEqual(TEXT("第二件保留强化"), Second->GetEnhancement(), 0.61f);
	FLxItemSaveRecord FirstRecord;
	LxItemSaveData::CaptureItem(First, FirstRecord);
	FirstRecord.EquipmentInstanceID = TemplateRecord.EquipmentInstanceID;
	TestTrue(TEXT("完整装备属性仅身份不同"), FLxItemSaveRecord::StaticStruct()->CompareScriptStruct(&FirstRecord, &TemplateRecord, 0));
	TestTrue(TEXT("无限商品模板与库存保持完整"), Stock->GetItem() == Template
		&& LxSafeItemOperations::MatchesExpectedRecord(Stock, TemplateRecord));
	return true;
}

#endif
