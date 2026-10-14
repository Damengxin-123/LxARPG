// Fill out your copyright notice in the Description page of Project Settings.


#include "LxPlayerController.h"
#include "LxARPG/LxSource/Systems/LxLocalPlayerSubsystem.h"
#include "LxARPG/LxSource/Model/DataTransfer/LxCharacterDataTransferComponent.h"
#include "LxARPG/LxSource/Model/Input/Logic/LxInputComponent.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxInteractableComponent.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxInteractionNode.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxItemTransferInteractionComponent.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxQuestInteractionComponent.h"
#include "LxARPG/LxSource/Model/PlayerControl/Logic/LxPlayerInteractionModule.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxTradeContainerInteractionComponent.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxTriggerMechanismInteractionComponent.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxTreasureChestInteractionComponent.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxWarehouseInteractionComponent.h"
#include "LxARPG/LxSource/Model/Item/Logic/LxCharacterBackpackComponent.h"
#include "LxARPG/LxSource/Model/Item/Logic/LxSafeItemOperations.h"
#include "LxARPG/LxSource/Model/Item/DataType/Slot/LxItemSlotData.h"
#include "LxARPG/LxSource/Model/Chat/Logic/LxPlayerChatComponent.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"
#include "LxARPG/LxSource/Player/Characters/LxPlayerCharacter.h"
#include "LxARPG/LxSource/Model/SystemOperate/LxPlayerSystemOperateComponent.h"
#include "LxARPG/LxSource/Systems/GameMode/LxARPGGameMode.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	/** 根据对象、资产实例版本和节点序号获取功能，拒绝树切换前发出的过期请求。 */
	template <typename TFeature>
	TFeature* FindInteractionFeature(const ALxPlayerController& PlayerController, AActor* InteractionOwner,
		int32 RuntimeNodeIndex, int32 InteractionTreeRevision,
		ELxInteractionActionType ExpectedType)
	{
		if (!InteractionOwner)
		{
			return nullptr;
		}
		const ALxPlayerCharacter* PlayerCharacter = Cast<ALxPlayerCharacter>(PlayerController.GetPawn());
		ULxPlayerInteractionModule* PlayerInteractionModule = PlayerCharacter
			? PlayerCharacter->GetPlayerInteractionComponent()
			: nullptr;
		ULxInteractableComponent* InteractableComponent = InteractionOwner->FindComponentByClass<ULxInteractableComponent>();
		if (!PlayerInteractionModule || !InteractableComponent
			|| InteractableComponent->GetInteractionTreeRevision() != InteractionTreeRevision
			|| !PlayerInteractionModule->IsInteractableComponentInRange(InteractableComponent))
		{
			return nullptr;
		}

		ULxInteractionNode* InteractionNode = InteractableComponent->FindInteractionNodeByRuntimeIndex(RuntimeNodeIndex);
		if (!InteractionNode || InteractionNode->GetInteractionActionType() != ExpectedType
			|| !InteractionNode->CanProcessActiveInteractionRequest(PlayerInteractionModule))
		{
			return nullptr;
		}

		return Cast<TFeature>(InteractionNode->GetInteractionFeature());
	}

	/** 服务器已解析并验证权限的容器，界面地址不能直接访问其他角色的数据模块。 */
	struct FLxResolvedItemContainer
	{
		/** 当前控制角色自己的背包模块。 */
		ULxCharacterBackpackModule* Backpack = nullptr;
		/** 通过交互权限检查的仓库模块。 */
		ULxWarehouseInteractionComponent* Warehouse = nullptr;
		/** 只作为快速取出来源的宝箱模块。 */
		ULxTreasureChestInteractionComponent* TreasureChest = nullptr;
		/** 只能通过原有购买与出售流程提交的交易模块。 */
		ULxTradeContainerInteractionComponent* Trade = nullptr;

		/** 获取实际来源槽位，界面筛选顺序不会影响数据位置。 */
		ULxItemSlotData* GetSlot(int32 SlotIndex) const
		{
			if (Backpack) return Backpack->GetBackpackSlotAt(SlotIndex);
			if (Warehouse) return Warehouse->GetWarehouseSlotAt(SlotIndex);
			if (TreasureChest) return TreasureChest->GetTreasureChestSlotAt(SlotIndex);
			return Trade ? Trade->GetTradeSlotAt(SlotIndex) : nullptr;
		}

		/** 标识同一数据容器，防止快速存取退化为容器内的重复放置。 */
		const UObject* GetModule() const
		{
			if (Backpack) return Backpack;
			if (Warehouse) return Warehouse;
			if (TreasureChest) return TreasureChest;
			return Trade;
		}

		/** 数据模块托管原实例，提取阶段不减少来源数量。 */
		ULxItemTransferTicket* BeginSafeExtract(int32 SlotIndex, FText& OutFailure) const
		{
			if (Backpack) return Backpack->BeginSafeExtractItem(SlotIndex, OutFailure);
			if (Warehouse) return Warehouse->BeginSafeExtractItem(SlotIndex, OutFailure);
			if (TreasureChest) return TreasureChest->BeginSafeExtractItem(SlotIndex, OutFailure);
			OutFailure = NSLOCTEXT("LxItemOperation", "TradeRequiresPurchase", "交易商品必须通过购买流程获取。");
			return nullptr;
		}

		/** 目标模块验证能否整堆接收，禁止部分转移后把余量交给界面保管。 */
		bool CanAccept(ULxItemTransferTicket* Ticket, FText& OutFailure) const
		{
			if (Backpack) return Backpack->CanAcceptSafeItem(Ticket, OutFailure);
			if (Warehouse) return Warehouse->CanAcceptSafeItem(Ticket, OutFailure);
			OutFailure = NSLOCTEXT("LxItemOperation", "UnsupportedReceiver", "该容器不能直接接收快速存取物品。");
			return false;
		}

		/** 由目标模块安全提交来源提取与目标放置，并负责两边的数据通知。 */
		bool Accept(ULxItemTransferTicket* Ticket, FText& OutFailure) const
		{
			if (Backpack) return Backpack->AcceptSafeItem(Ticket, OutFailure);
			if (Warehouse) return Warehouse->AcceptSafeItem(Ticket, OutFailure);
			OutFailure = NSLOCTEXT("LxItemOperation", "UnsupportedReceiver", "该容器不能直接接收快速存取物品。");
			return false;
		}

		/** 在来源模块内寻找独立空格执行拆分，本次操作不会合并其他堆叠。 */
		bool Split(int32 SlotIndex, int32 Count, FText& OutFailure) const
		{
			if (Backpack) return Backpack->SplitItemStack(SlotIndex, Count, OutFailure);
			if (Warehouse) return Warehouse->SplitItemStack(SlotIndex, Count, OutFailure);
			OutFailure = NSLOCTEXT("LxItemOperation", "UnsupportedSplitContainer", "宝箱和交易商品不能执行拆分。");
			return false;
		}
	};

	/** 从通用容器地址解析服务器模块，同时验证所属角色、交互距离、节点及树版本。 */
	bool ResolveItemContainer(const ALxPlayerController& PlayerController, const FLxItemContainerAddress& Address,
		FLxResolvedItemContainer& OutContainer, FText& OutFailure)
	{
		const ALxBaseCharacter* Character = Cast<ALxBaseCharacter>(PlayerController.GetPawn());
		if (!Character || Character->GetController() != &PlayerController || !Character->HasAuthority())
		{
			OutFailure = NSLOCTEXT("LxItemOperation", "InvalidCharacter", "当前角色不可操作物品。");
			return false;
		}
		if (Address.Kind == ELxItemContainerKind::Backpack)
		{
			if ((Address.Owner && Address.Owner != Character) || !Character->GetCharacterBackpackComponent())
			{
				OutFailure = NSLOCTEXT("LxItemOperation", "InvalidBackpack", "无法访问该背包。");
				return false;
			}
			OutContainer.Backpack = Character->GetCharacterBackpackComponent();
			return true;
		}
		if (Address.Kind == ELxItemContainerKind::Warehouse)
		{
			if (!IsValid(Address.Owner) || Address.Owner->GetWorld() != PlayerController.GetWorld()
				|| !Address.Owner->HasAuthority())
			{
				OutFailure = NSLOCTEXT("LxItemOperation", "InvalidWarehouse", "仓库已失效，无法操作物品。");
				return false;
			}
			OutContainer.Warehouse = FindInteractionFeature<ULxWarehouseInteractionComponent>(PlayerController,
				Address.Owner, Address.RuntimeNodeIndex, Address.InteractionTreeRevision, ELxInteractionActionType::Warehouse);
			if (!OutContainer.Warehouse)
			{
				OutFailure = NSLOCTEXT("LxItemOperation", "WarehousePermission", "仓库交互已变化、超出距离或不允许访问。");
				return false;
			}
			return true;
		}
		if (Address.Kind == ELxItemContainerKind::TreasureChest || Address.Kind == ELxItemContainerKind::Trade)
		{
			if (!IsValid(Address.Owner) || Address.Owner->GetWorld() != PlayerController.GetWorld()
				|| !Address.Owner->HasAuthority())
			{
				OutFailure = NSLOCTEXT("LxItemOperation", "InvalidInteractionContainer", "物品交互对象已失效。");
				return false;
			}
			if (Address.Kind == ELxItemContainerKind::TreasureChest)
			{
				OutContainer.TreasureChest = FindInteractionFeature<ULxTreasureChestInteractionComponent>(PlayerController,
					Address.Owner, Address.RuntimeNodeIndex, Address.InteractionTreeRevision, ELxInteractionActionType::TreasureChest);
				if (OutContainer.TreasureChest) return true;
			}
			else
			{
				OutContainer.Trade = FindInteractionFeature<ULxTradeContainerInteractionComponent>(PlayerController,
					Address.Owner, Address.RuntimeNodeIndex, Address.InteractionTreeRevision, ELxInteractionActionType::TradeContainer);
				if (OutContainer.Trade) return true;
			}
			OutFailure = NSLOCTEXT("LxItemOperation", "InteractionContainerPermission", "物品交互已变化、超出距离或不允许访问。");
			return false;
		}
		OutFailure = NSLOCTEXT("LxItemOperation", "UnsupportedContainer", "该容器不支持物品存取或拆分。");
		return false;
	}
}

ALxPlayerController::ALxPlayerController()
{
	m_pInputComponent = CreateDefaultSubobject<ULxInputComponent>(TEXT("外部输入管理组件"));
	m_pSystemOperateComponent = CreateDefaultSubobject<ULxPlayerSystemOperateComponent>(TEXT("系统操作组件"));
	m_pChatComponent = CreateDefaultSubobject<ULxPlayerChatComponent>(TEXT("玩家聊天组件"));

}


void ALxPlayerController::BeginPlay()
{
	Super::BeginPlay();
	InitializePauseMenuInput();
	if (ULxLocalPlayerSubsystem* LocalPlayerSubsystem = GET_LOCAL_PLAYER_SYSTEM())
	{
		LocalPlayerSubsystem->SetPlayerControllerQuote(this);
		LocalPlayerSubsystem->SetControlledCharacter(m_pCurrentCharacter);
	}
	if (m_pSystemOperateComponent)
	{
		m_pSystemOperateComponent->BaseComponentInitialize();
	}
	if (m_pInputComponent)
	{
		
		m_pInputComponent->BaseComponentInitialize();
	}
}

void ALxPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (m_pInputComponent)
	{
		m_pInputComponent->BaseComponentInitialize();
	}
}

void ALxPlayerController::CreatePlayerCharacter()
{
	if (GetNetMode() == NM_Standalone)
	{
		SpawnAndPossessPlayerCharacter();
		return;
	}

	CreateServerPlayerCharacter();
}

void ALxPlayerController::RequestItemOperation(const FLxItemOperationRequest& Request)
{
	if (!IsLocalController() || IsPauseMenuOpen() || UGameplayStatics::IsGamePaused(this) || !Request.RequestId.IsValid())
	{
		FLxItemOperationResult Result;
		Result.RequestId = Request.RequestId;
		Result.Kind = Request.Kind;
		if (!Request.RequestId.IsValid())
		{
			Result.Message = NSLOCTEXT("LxItemOperation", "InvalidRequestId", "物品操作缺少有效的请求标识。");
		}
		else if (!IsLocalController())
		{
			Result.Message = NSLOCTEXT("LxItemOperation", "LocalControllerRequired", "只能从本地玩家界面发起物品操作。");
		}
		else
		{
			Result.Message = NSLOCTEXT("LxItemOperation", "LocalUnavailable", "游戏暂停期间无法执行物品操作。");
		}
		ClientCompleteItemOperation_Implementation(Result);
		return;
	}
	ServerExecuteItemOperation(Request);
}

void ALxPlayerController::ServerExecuteItemOperation_Implementation(const FLxItemOperationRequest& Request)
{
	if (const FLxItemOperationResult* ExistingResult = CompletedItemOperations.Find(Request.RequestId))
	{
		// 同一请求返回已完成结果；即使物品后来回到原槽位，也不再次执行。
		ClientCompleteItemOperation(*ExistingResult);
		return;
	}
	if (ExecutingItemOperations.Contains(Request.RequestId))
	{
		// 同步数据通知也可能重发当前标识；等待原请求完成，不能用忙碌失败覆盖其最终结果。
		return;
	}

	FLxItemOperationResult Result;
	Result.RequestId = Request.RequestId;
	Result.Kind = Request.Kind;
	if (!HasAuthority() || !GetWorld() || !Request.RequestId.IsValid()
		|| UGameplayStatics::IsGamePaused(this) || IsPauseMenuOpen())
	{
		Result.Message = NSLOCTEXT("LxItemOperation", "ServerUnavailable", "物品操作请求无效或游戏已暂停。");
		CompleteItemOperation(Result);
		return;
	}
	if (Request.Kind != ELxItemOperationKind::QuickTransfer && Request.Kind != ELxItemOperationKind::SplitStack)
	{
		Result.Message = NSLOCTEXT("LxItemOperation", "UnsupportedOperation", "不支持该物品操作。");
		CompleteItemOperation(Result);
		return;
	}
	ExecutingItemOperations.Add(Request.RequestId);

	FLxResolvedItemContainer SourceContainer;
	if (!ResolveItemContainer(*this, Request.Source, SourceContainer, Result.Message))
	{
		CompleteItemOperation(Result);
		return;
	}
	ULxItemSlotData* SourceSlot = SourceContainer.GetSlot(Request.SourceSlotIndex);
	if (!LxSafeItemOperations::MatchesExpectedRecord(SourceSlot, Request.ExpectedItem))
	{
		Result.Message = NSLOCTEXT("LxItemOperation", "SourceChanged", "来源物品已变化，请等待界面刷新后重试。");
		CompleteItemOperation(Result);
		return;
	}

	if (Request.Kind == ELxItemOperationKind::SplitStack)
	{
		Result.bSucceeded = SourceContainer.Split(Request.SourceSlotIndex, Request.Count, Result.Message);
		if (Result.bSucceeded)
		{
			Result.Message = NSLOCTEXT("LxItemOperation", "SplitSucceeded", "物品拆分成功。");
		}
		CompleteItemOperation(Result);
		return;
	}

	if (Request.Count != 0 && Request.Count != Request.ExpectedItem.ItemCount)
	{
		Result.Message = NSLOCTEXT("LxItemOperation", "WholeStackRequired", "快速存取必须移动这一格中的完整物品。");
		CompleteItemOperation(Result);
		return;
	}
	FLxResolvedItemContainer TargetContainer;
	if (!ResolveItemContainer(*this, Request.Target, TargetContainer, Result.Message))
	{
		CompleteItemOperation(Result);
		return;
	}
	if (SourceContainer.GetModule() == TargetContainer.GetModule())
	{
		Result.Message = NSLOCTEXT("LxItemOperation", "SameContainer", "快速存取的目标不能是来源容器。");
		CompleteItemOperation(Result);
		return;
	}
	if (TargetContainer.TreasureChest || (SourceContainer.TreasureChest && !TargetContainer.Backpack))
	{
		Result.Message = NSLOCTEXT("LxItemOperation", "ChestWithdrawOnly", "宝箱物品只能快速取出到背包。");
		CompleteItemOperation(Result);
		return;
	}
	if (SourceContainer.Trade || TargetContainer.Trade)
	{
		const ALxBaseCharacter* CurrentCharacter = Cast<ALxBaseCharacter>(GetPawn());
		ULxCharacterDataTransferComponent* DataTransfer = CurrentCharacter ? CurrentCharacter->GetCharacterDataTransferComponent() : nullptr;
		if (DataTransfer && SourceContainer.Trade && TargetContainer.Backpack)
		{
			// 商品不走普通物品转移，沿用交易模块原有整堆购买规则与金币事务。
			Result.bSucceeded = SourceContainer.Trade->BuyTradeSlot(SourceSlot, DataTransfer);
			Result.Message = Result.bSucceeded
				? NSLOCTEXT("LxItemOperation", "QuickPurchaseSucceeded", "物品购买成功。")
				: NSLOCTEXT("LxItemOperation", "QuickPurchaseFailed", "购买失败，请检查金币、背包空间或商品状态；物品与金币已保留。");
		}
		else if (DataTransfer && SourceContainer.Backpack && TargetContainer.Trade)
		{
			// 出售仍由交易模块按当前收购比例执行，界面不直接清槽或增加金币。
			Result.bSucceeded = TargetContainer.Trade->SellBackpackSlot(SourceSlot, DataTransfer);
			Result.Message = Result.bSucceeded
				? NSLOCTEXT("LxItemOperation", "QuickSaleSucceeded", "物品出售成功。")
				: NSLOCTEXT("LxItemOperation", "QuickSaleFailed", "出售失败，请检查物品状态或金币容量；物品与金币已保留。");
		}
		else
		{
			Result.Message = NSLOCTEXT("LxItemOperation", "UnsupportedTradeRoute", "交易快捷操作只支持商品买入背包和背包物品出售。");
		}
		CompleteItemOperation(Result);
		return;
	}

	ULxItemTransferTicket* Ticket = SourceContainer.BeginSafeExtract(Request.SourceSlotIndex, Result.Message);
	if (!Ticket)
	{
		CompleteItemOperation(Result);
		return;
	}
	if (!TargetContainer.CanAccept(Ticket, Result.Message) || !TargetContainer.Accept(Ticket, Result.Message))
	{
		// 提取凭据尚未提交时来源始终保留原实例，取消只释放临时托管状态。
		LxSafeItemOperations::Cancel(Ticket);
		CompleteItemOperation(Result);
		return;
	}
	Result.bSucceeded = true;
	Result.Message = NSLOCTEXT("LxItemOperation", "TransferSucceeded", "物品快速存取成功。");
	CompleteItemOperation(Result);
}

void ALxPlayerController::CompleteItemOperation(const FLxItemOperationResult& InResult)
{
	FLxItemOperationResult Result = InResult;
	ExecutingItemOperations.Remove(Result.RequestId);
	if (!Result.bSucceeded && Result.Message.IsEmpty())
	{
		Result.Message = NSLOCTEXT("LxItemOperation", "OperationFailed", "物品操作失败，原物品已保留。");
	}
	if (Result.RequestId.IsValid() && !CompletedItemOperations.Contains(Result.RequestId))
	{
		/** 每个控制器仅缓存近期请求，避免长期游戏产生无界增长。 */
		constexpr int32 MaximumRememberedRequests = 256;
		if (CompletedItemOperationOrder.Num() >= MaximumRememberedRequests)
		{
			CompletedItemOperations.Remove(CompletedItemOperationOrder[0]);
			CompletedItemOperationOrder.RemoveAt(0);
		}
		CompletedItemOperationOrder.Add(Result.RequestId);
		CompletedItemOperations.Add(Result.RequestId, Result);
	}
	ClientCompleteItemOperation(Result);
}

void ALxPlayerController::ClientCompleteItemOperation_Implementation(const FLxItemOperationResult& Result)
{
	OnItemOperationCompleted.Broadcast(Result);
}

void ALxPlayerController::ServerMoveItemBetweenBackpackAndWarehouse_Implementation(AActor* WarehouseOwner,
	int32 RuntimeNodeIndex, int32 InteractionTreeRevision, int32 SourceSlotIndex, int32 TargetSlotIndex, bool bMoveToWarehouse)
{
	ULxWarehouseInteractionComponent* WarehouseComponent = FindInteractionFeature<ULxWarehouseInteractionComponent>(
		*this, WarehouseOwner, RuntimeNodeIndex, InteractionTreeRevision, ELxInteractionActionType::Warehouse);
	const ALxBaseCharacter* CurrentCharacter = Cast<ALxBaseCharacter>(GetPawn());
	ULxCharacterBackpackModule* BackpackComponent = CurrentCharacter ? CurrentCharacter->GetCharacterBackpackComponent() : nullptr;
	if (WarehouseComponent == nullptr || BackpackComponent == nullptr)
	{
		return;
	}

	if (bMoveToWarehouse)
	{
		WarehouseComponent->MoveBackpackSlotToWarehouse(BackpackComponent, SourceSlotIndex, TargetSlotIndex);
		return;
	}

	WarehouseComponent->MoveWarehouseSlotToBackpack(BackpackComponent, SourceSlotIndex, TargetSlotIndex);
}

void ALxPlayerController::ServerMoveBackpackSlot_Implementation(int32 SourceSlotIndex, int32 TargetSlotIndex)
{
	const ALxBaseCharacter* CurrentCharacter = Cast<ALxBaseCharacter>(GetPawn());
	ULxCharacterBackpackModule* BackpackComponent = CurrentCharacter ? CurrentCharacter->GetCharacterBackpackComponent() : nullptr;
	if (BackpackComponent)
	{
		BackpackComponent->MoveBackpackSlot(SourceSlotIndex, TargetSlotIndex);
	}
}

void ALxPlayerController::ServerMoveWarehouseSlot_Implementation(AActor* WarehouseOwner,
	int32 RuntimeNodeIndex, int32 InteractionTreeRevision, int32 SourceSlotIndex, int32 TargetSlotIndex)
{
	if (ULxWarehouseInteractionComponent* WarehouseComponent = FindInteractionFeature<ULxWarehouseInteractionComponent>(
		*this, WarehouseOwner, RuntimeNodeIndex, InteractionTreeRevision, ELxInteractionActionType::Warehouse))
	{
		WarehouseComponent->MoveWarehouseSlot(SourceSlotIndex, TargetSlotIndex);
	}
}

void ALxPlayerController::ServerMoveTreasureChestSlotToBackpack_Implementation(AActor* TreasureChestOwner,
	int32 RuntimeNodeIndex, int32 InteractionTreeRevision, int32 TreasureChestSlotIndex, int32 BackpackSlotIndex)
{
	ULxTreasureChestInteractionComponent* TreasureChestComponent = FindInteractionFeature<ULxTreasureChestInteractionComponent>(
		*this, TreasureChestOwner, RuntimeNodeIndex, InteractionTreeRevision, ELxInteractionActionType::TreasureChest);
	const ALxBaseCharacter* CurrentCharacter = Cast<ALxBaseCharacter>(GetPawn());
	ULxCharacterBackpackModule* BackpackComponent = CurrentCharacter ? CurrentCharacter->GetCharacterBackpackComponent() : nullptr;
	if (TreasureChestComponent && BackpackComponent)
	{
		TreasureChestComponent->MoveTreasureChestSlotToBackpack(BackpackComponent, TreasureChestSlotIndex, BackpackSlotIndex);
	}
}

void ALxPlayerController::ServerBuyTradeSlot_Implementation(AActor* TradeOwner, int32 RuntimeNodeIndex, int32 InteractionTreeRevision,
	int32 TradeSlotIndex)
{
	ULxTradeContainerInteractionComponent* TradeComponent = FindInteractionFeature<ULxTradeContainerInteractionComponent>(
		*this, TradeOwner, RuntimeNodeIndex, InteractionTreeRevision, ELxInteractionActionType::TradeContainer);
	const ALxBaseCharacter* CurrentCharacter = Cast<ALxBaseCharacter>(GetPawn());
	ULxCharacterDataTransferComponent* DataTransferComponent = CurrentCharacter ? CurrentCharacter->GetCharacterDataTransferComponent() : nullptr;
	if (TradeComponent && DataTransferComponent)
	{
		TradeComponent->BuyTradeSlot(TradeComponent->GetTradeSlotAt(TradeSlotIndex), DataTransferComponent);
	}
}

void ALxPlayerController::ServerBuyTradeSlotToBackpackSlot_Implementation(AActor* TradeOwner,
	int32 RuntimeNodeIndex, int32 InteractionTreeRevision, int32 TradeSlotIndex, int32 BackpackSlotIndex)
{
	ULxTradeContainerInteractionComponent* TradeComponent = FindInteractionFeature<ULxTradeContainerInteractionComponent>(
		*this, TradeOwner, RuntimeNodeIndex, InteractionTreeRevision, ELxInteractionActionType::TradeContainer);
	const ALxBaseCharacter* CurrentCharacter = Cast<ALxBaseCharacter>(GetPawn());
	ULxCharacterDataTransferComponent* DataTransferComponent = CurrentCharacter ? CurrentCharacter->GetCharacterDataTransferComponent() : nullptr;
	ULxCharacterBackpackModule* BackpackComponent = CurrentCharacter ? CurrentCharacter->GetCharacterBackpackComponent() : nullptr;
	if (TradeComponent && DataTransferComponent && BackpackComponent)
	{
		TradeComponent->BuyTradeSlotToBackpackSlot(
			TradeComponent->GetTradeSlotAt(TradeSlotIndex),
			BackpackComponent->GetBackpackSlotAt(BackpackSlotIndex),
			DataTransferComponent);
	}
}

void ALxPlayerController::ServerSellBackpackSlot_Implementation(AActor* TradeOwner, int32 RuntimeNodeIndex, int32 InteractionTreeRevision,
	int32 BackpackSlotIndex)
{
	ULxTradeContainerInteractionComponent* TradeComponent = FindInteractionFeature<ULxTradeContainerInteractionComponent>(
		*this, TradeOwner, RuntimeNodeIndex, InteractionTreeRevision, ELxInteractionActionType::TradeContainer);
	const ALxBaseCharacter* CurrentCharacter = Cast<ALxBaseCharacter>(GetPawn());
	ULxCharacterDataTransferComponent* DataTransferComponent = CurrentCharacter ? CurrentCharacter->GetCharacterDataTransferComponent() : nullptr;
	ULxCharacterBackpackModule* BackpackComponent = CurrentCharacter ? CurrentCharacter->GetCharacterBackpackComponent() : nullptr;
	if (TradeComponent && DataTransferComponent && BackpackComponent)
	{
		TradeComponent->SellBackpackSlot(BackpackComponent->GetBackpackSlotAt(BackpackSlotIndex), DataTransferComponent);
	}
}

void ALxPlayerController::ServerExecuteItemTransfer_Implementation(AActor* ItemTransferOwner, int32 RuntimeNodeIndex, int32 InteractionTreeRevision)
{
	ULxItemTransferInteractionComponent* ItemTransferComponent = FindInteractionFeature<ULxItemTransferInteractionComponent>(
		*this, ItemTransferOwner, RuntimeNodeIndex, InteractionTreeRevision, ELxInteractionActionType::ItemTransfer);
	const ALxPlayerCharacter* PlayerCharacter = Cast<ALxPlayerCharacter>(GetPawn());
	ULxPlayerInteractionModule* PlayerInteractionComponent = PlayerCharacter ? PlayerCharacter->GetPlayerInteractionComponent() : nullptr;
	if (ItemTransferComponent && PlayerInteractionComponent)
	{
		ItemTransferComponent->ExecuteInteraction(PlayerInteractionComponent);
	}
}

void ALxPlayerController::ServerTriggerMechanism_Implementation(AActor* MechanismOwner, int32 RuntimeNodeIndex, int32 InteractionTreeRevision)
{
	ULxTriggerMechanismInteractionComponent* TriggerMechanismComponent = FindInteractionFeature<ULxTriggerMechanismInteractionComponent>(
		*this, MechanismOwner, RuntimeNodeIndex, InteractionTreeRevision, ELxInteractionActionType::TriggerMechanism);
	const ALxPlayerCharacter* PlayerCharacter = Cast<ALxPlayerCharacter>(GetPawn());
	ULxPlayerInteractionModule* PlayerInteractionComponent = PlayerCharacter ? PlayerCharacter->GetPlayerInteractionComponent() : nullptr;
	if (TriggerMechanismComponent && PlayerInteractionComponent)
	{
		TriggerMechanismComponent->TriggerMechanism(PlayerInteractionComponent);
	}
}

void ALxPlayerController::ServerExecuteQuestInteraction_Implementation(
	AActor* QuestOwner, int32 RuntimeNodeIndex, int32 InteractionTreeRevision)
{
	ULxQuestInteractionComponent* QuestInteractionComponent =
		FindInteractionFeature<ULxQuestInteractionComponent>(
			*this, QuestOwner, RuntimeNodeIndex, InteractionTreeRevision, ELxInteractionActionType::Quest);
	const ALxPlayerCharacter* PlayerCharacter = Cast<ALxPlayerCharacter>(GetPawn());
	ULxPlayerInteractionModule* PlayerInteractionComponent = PlayerCharacter
		? PlayerCharacter->GetPlayerInteractionComponent()
		: nullptr;
	if (QuestInteractionComponent && PlayerInteractionComponent)
	{
		QuestInteractionComponent->ExecuteInteraction(PlayerInteractionComponent);
	}
}

void ALxPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	SyncControlledCharacter(InPawn);
}

void ALxPlayerController::SetPawn(APawn* InPawn)
{
	Super::SetPawn(InPawn);

	SyncControlledCharacter(InPawn);
}

void ALxPlayerController::OnUnPossess()
{
	Super::OnUnPossess();
	m_pCurrentCharacter = nullptr;
	if (ULxLocalPlayerSubsystem* LocalPlayerSubsystem = GET_LOCAL_PLAYER_SYSTEM())
	{
		LocalPlayerSubsystem->SetControlledCharacter(nullptr);
	}
}

void ALxPlayerController::SyncControlledCharacter(APawn* InPawn)
{
	m_pCurrentCharacter =  Cast<ALxBaseCharacter>(InPawn);
	if (m_pCurrentCharacter)
	{
		m_pCurrentCharacter->InitialCharacterInformation();
	}
	if (ULxLocalPlayerSubsystem* LocalPlayerSubsystem = GET_LOCAL_PLAYER_SYSTEM())
	{
		LocalPlayerSubsystem->SetControlledCharacter(m_pCurrentCharacter);
	}
}

void ALxPlayerController::SpawnAndPossessPlayerCharacter()
{
	if (ALxARPGGameMode* GameMode = GetWorld() ? Cast<ALxARPGGameMode>(GetWorld()->GetAuthGameMode()) : nullptr)
	{
		if (APawn* NewPawn = GameMode->SpawnPlayerCharacter(this))
		{
			Possess(NewPawn);
		}
	}
}

void ALxPlayerController::ShowCursorFun()
{
	if (IsPauseMenuOpen()) return;
	// 显示鼠标
	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);

	SetInputMode(InputMode);
	bShowMouseCursor = true;
}

void ALxPlayerController::HideCursorFun()
{
	if (IsPauseMenuOpen()) return;
	// 隐藏鼠标
	FInputModeGameOnly Mode;
	SetInputMode(Mode);

	bShowMouseCursor = false;
}

void ALxPlayerController::CreateServerPlayerCharacter_Implementation()
{
	SpawnAndPossessPlayerCharacter();
}
