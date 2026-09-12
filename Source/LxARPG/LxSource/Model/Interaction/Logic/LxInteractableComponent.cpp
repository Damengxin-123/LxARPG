#include "LxInteractableComponent.h"

#include "Engine/ActorChannel.h"
#include "Engine/World.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/Actor.h"
#include "LxARPG/LxSource/Model/Interaction/Interface/LxInteractionReceiverInterface.h"
#include "LxFunctionPageInteractionComponent.h"
#include "LxInteractionActionComponentBase.h"
#include "LxInteractionNode.h"
#include "LxItemTransferInteractionComponent.h"
#include "LxQuestInteractionComponent.h"
#include "LxTradeContainerInteractionComponent.h"
#include "LxTreasureChestInteractionComponent.h"
#include "LxTriggerMechanismInteractionComponent.h"
#include "LxWarehouseInteractionComponent.h"
#include "Net/UnrealNetwork.h"

ULxInteractableComponent::ULxInteractableComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void ULxInteractableComponent::BaseComponentInitialize()
{
	Super::BaseComponentInitialize();
	InitializeInteractionFeatures();
}

void ULxInteractableComponent::BeginPlay()
{
	Super::BeginPlay();
	if (AActor* OwnerActor = GetOwner())
	{
		OwnerActor->SetReplicates(true);
	}
	InitializeInteractionFeatures();
	BindInteractionRangeColliders();
	RefreshAutomaticInteractionRange();
}

void ULxInteractableComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	OnInteractableComponentEndPlayNative.Broadcast();
	OnInteractableComponentEndPlayNative.Clear();
	SetInteractionRangeColliders({});
	ShutdownInteractionFeatures();
	Super::EndPlay(EndPlayReason);
}

void ULxInteractableComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ULxInteractableComponent, InteractionFeatures);
}

bool ULxInteractableComponent::ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch,
	FReplicationFlags* RepFlags)
{
	bool bWroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);
	for (ULxInteractionActionComponentBase* InteractionFeature : InteractionFeatures)
	{
		if (InteractionFeature)
		{
			bWroteSomething |= Channel->ReplicateSubobject(InteractionFeature, *Bunch, *RepFlags);
		}
	}
	return bWroteSomething;
}

void ULxInteractableComponent::SetRootInteractionNodes(const TArray<ULxInteractionNode*>& InRootNodes)
{
	RootInteractionNodes.Reset();
	for (ULxInteractionNode* RootNode : InRootNodes)
	{
		if (!RootNode || RootInteractionNodes.Contains(RootNode))
		{
			continue;
		}

		RootNode->SetParentNode(nullptr);
		RootInteractionNodes.Add(RootNode);
	}

	InitializeInteractionFeatures();
	RefreshInteractionOptions();
}

void ULxInteractableComponent::BuildInteractionTree(TArray<ULxInteractionNode*> RootNodes)
{
	SetRootInteractionNodes(RootNodes);
}

void ULxInteractableComponent::AddRootInteractionNode(ULxInteractionNode* InRootNode)
{
	if (!InRootNode || RootInteractionNodes.Contains(InRootNode))
	{
		return;
	}

	InRootNode->SetParentNode(nullptr);
	RootInteractionNodes.Add(InRootNode);
	InitializeInteractionFeatures();
	RefreshInteractionOptions();
}

TArray<ULxInteractionNode*> ULxInteractableComponent::GetRootInteractionNodes() const
{
	TArray<ULxInteractionNode*> Result;
	for (ULxInteractionNode* RootNode : RootInteractionNodes)
	{
		Result.Add(RootNode);
	}
	return Result;
}

void ULxInteractableComponent::InitializeInteractionFeatures()
{
	const AActor* OwnerActor = GetOwner();
	const bool bCanCreateRuntimeFeatures = OwnerActor == nullptr || OwnerActor->HasAuthority();
	if (bCanCreateRuntimeFeatures)
	{
		ShutdownInteractionFeatures();
	}
	RuntimeNodeIndex.Reset();

	int32 NextRuntimeNodeIndex = 0;
	TSet<ULxInteractionNode*> VisitedNodes;
	for (ULxInteractionNode* RootNode : RootInteractionNodes)
	{
		BuildInteractionFeaturesRecursive(RootNode, nullptr, NextRuntimeNodeIndex, VisitedNodes);
	}

	if (!bCanCreateRuntimeFeatures)
	{
		BindReplicatedFeaturesToNodes();
	}

	if (AActor* MutableOwnerActor = GetOwner(); MutableOwnerActor && MutableOwnerActor->HasAuthority())
	{
		MutableOwnerActor->ForceNetUpdate();
	}
}

TArray<ULxInteractionActionComponentBase*> ULxInteractableComponent::GetInteractionFeatures() const
{
	TArray<ULxInteractionActionComponentBase*> Result;
	for (ULxInteractionActionComponentBase* InteractionFeature : InteractionFeatures)
	{
		Result.Add(InteractionFeature);
	}
	return Result;
}

ULxInteractionNode* ULxInteractableComponent::FindInteractionNodeByRuntimeIndex(int32 InRuntimeNodeIndex) const
{
	const TObjectPtr<ULxInteractionNode>* FoundNode = RuntimeNodeIndex.Find(InRuntimeNodeIndex);
	return FoundNode ? FoundNode->Get() : nullptr;
}

bool ULxInteractableComponent::OwnsInteractionNode(const ULxInteractionNode* InteractionNode) const
{
	return InteractionNode && RuntimeNodeIndex.FindRef(InteractionNode->GetRuntimeNodeIndex()) == InteractionNode;
}

bool ULxInteractableComponent::ExecuteInteractionNode(ULxInteractionNode* InteractionNode,
	ULxPlayerInteractionModule* PlayerInteractionComponent, bool& bShouldOpenFunctionUI)
{
	bShouldOpenFunctionUI = false;
	if (!OwnsInteractionNode(InteractionNode)
		|| !InteractionNode->IsFunctionNode()
		|| !InteractionNode->IsNodeInteractable(PlayerInteractionComponent))
	{
		return false;
	}

	ULxInteractionActionComponentBase* InteractionFeature = InteractionNode->GetInteractionFeature();
	if (!InteractionFeature || !InteractionFeature->ExecuteInteraction(PlayerInteractionComponent))
	{
		return false;
	}

	bShouldOpenFunctionUI = InteractionFeature->ShouldOpenFunctionUI();
	return true;
}

void ULxInteractableComponent::RefreshInteractionOptions()
{
	OnInteractableOptionsChanged.Broadcast();
	OnDataChange.Broadcast();
}

void ULxInteractableComponent::NotifyItemTransferCompleted()
{
	OnItemTransferCompleted.Broadcast();
}

void ULxInteractableComponent::NotifyTreasureChestCompleted()
{
	OnTreasureChestCompleted.Broadcast();
}

void ULxInteractableComponent::NotifyMechanismStateChanged(ELxMechanismState NewState)
{
	OnMechanismStateChanged.Broadcast(NewState);
}

void ULxInteractableComponent::SetInteractionRangeColliders(const TArray<UPrimitiveComponent*>& InColliders)
{
	UnbindInteractionRangeColliders();
	InteractionRangeColliders.Reset();
	for (UPrimitiveComponent* Collider : InColliders)
	{
		if (IsValid(Collider))
		{
			InteractionRangeColliders.AddUnique(Collider);
		}
	}
	BindInteractionRangeColliders();
	RefreshAutomaticInteractionRange();
}

void ULxInteractableComponent::BindInteractionRangeColliders()
{
	// 构造脚本在编辑器中只记录列表，游戏开始后再绑定，避免编辑器预览触发交互。
	if (IsTemplate() || !GetWorld() || !GetWorld()->IsGameWorld())
	{
		return;
	}
	for (UPrimitiveComponent* Collider : InteractionRangeColliders)
	{
		if (IsValid(Collider))
		{
			Collider->OnComponentBeginOverlap.AddUniqueDynamic(this, &ULxInteractableComponent::HandleAutomaticRangeBeginOverlap);
			Collider->OnComponentEndOverlap.AddUniqueDynamic(this, &ULxInteractableComponent::HandleAutomaticRangeEndOverlap);
		}
	}
}

void ULxInteractableComponent::UnbindInteractionRangeColliders()
{
	for (UPrimitiveComponent* Collider : InteractionRangeColliders)
	{
		if (IsValid(Collider))
		{
			Collider->OnComponentBeginOverlap.RemoveDynamic(this, &ULxInteractableComponent::HandleAutomaticRangeBeginOverlap);
			Collider->OnComponentEndOverlap.RemoveDynamic(this, &ULxInteractableComponent::HandleAutomaticRangeEndOverlap);
		}
	}
}

void ULxInteractableComponent::RefreshAutomaticInteractionRange()
{
	TSet<TWeakObjectPtr<AActor>> CurrentActors;
	if (!IsTemplate() && GetWorld() && GetWorld()->IsGameWorld())
	{
		for (UPrimitiveComponent* Collider : InteractionRangeColliders)
		{
			if (!IsValid(Collider) || !Collider->IsRegistered() || !Collider->GetGenerateOverlapEvents()
				|| !Collider->IsQueryCollisionEnabled())
			{
				continue;
			}
			TArray<AActor*> OverlappingActors;
			Collider->GetOverlappingActors(OverlappingActors);
			for (AActor* Actor : OverlappingActors)
			{
				if (IsValid(Actor) && Actor->GetClass()->ImplementsInterface(ULxInteractionReceiverInterface::StaticClass()))
				{
					CurrentActors.Add(Actor);
				}
			}
		}
	}

	// 先更新缓存再通知蓝图，重复设置或重入不会重复报告同一次进入。
	const TSet<TWeakObjectPtr<AActor>> PreviousActors = MoveTemp(AutomaticRangeActors);
	AutomaticRangeActors = CurrentActors;
	for (const TWeakObjectPtr<AActor>& Actor : PreviousActors)
	{
		if (Actor.IsValid() && !AutomaticRangeActors.Contains(Actor))
		{
			HandleInteractionRangeEndOverlap(Actor.Get());
		}
	}
	for (const TWeakObjectPtr<AActor>& Actor : CurrentActors)
	{
		if (Actor.IsValid() && !PreviousActors.Contains(Actor) && AutomaticRangeActors.Contains(Actor))
		{
			HandleInteractionRangeBeginOverlap(Actor.Get());
		}
	}
}

void ULxInteractableComponent::HandleAutomaticRangeBeginOverlap(UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	RefreshAutomaticInteractionRange();
}

void ULxInteractableComponent::HandleAutomaticRangeEndOverlap(UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	RefreshAutomaticInteractionRange();
}

void ULxInteractableComponent::HandleInteractionRangeBeginOverlap(AActor* OtherActor)
{
	if (!OtherActor || !OtherActor->GetClass()->ImplementsInterface(ULxInteractionReceiverInterface::StaticClass()))
	{
		return;
	}

	ILxInteractionReceiverInterface::Execute_ReceiveInteractableComponent(OtherActor, this);
}

void ULxInteractableComponent::HandleInteractionRangeEndOverlap(AActor* OtherActor)
{
	if (!OtherActor || !OtherActor->GetClass()->ImplementsInterface(ULxInteractionReceiverInterface::StaticClass()))
	{
		return;
	}

	ILxInteractionReceiverInterface::Execute_RemoveInteractableComponent(OtherActor, this);
}

void ULxInteractableComponent::ShutdownInteractionFeatures()
{
	for (ULxInteractionActionComponentBase* InteractionFeature : InteractionFeatures)
	{
		if (InteractionFeature)
		{
			InteractionFeature->ShutdownInteractionFeature();
		}
	}

	InteractionFeatures.Reset();
	for (const TPair<int32, TObjectPtr<ULxInteractionNode>>& NodePair : RuntimeNodeIndex)
	{
		if (NodePair.Value)
		{
			NodePair.Value->SetInteractionFeature(nullptr);
		}
	}
}

void ULxInteractableComponent::BuildInteractionFeaturesRecursive(ULxInteractionNode* InteractionNode,
	ULxInteractionNode* ParentNode, int32& InOutRuntimeNodeIndex, TSet<ULxInteractionNode*>& VisitedNodes)
{
	if (!InteractionNode || VisitedNodes.Contains(InteractionNode))
	{
		return;
	}

	VisitedNodes.Add(InteractionNode);
	InteractionNode->SetParentNode(ParentNode);
	InteractionNode->SetRuntimeNodeIndex(InOutRuntimeNodeIndex);
	RuntimeNodeIndex.Add(InOutRuntimeNodeIndex, InteractionNode);
	++InOutRuntimeNodeIndex;

	const AActor* OwnerActor = GetOwner();
	if (InteractionNode->IsFunctionNode() && (OwnerActor == nullptr || OwnerActor->HasAuthority()))
	{
		if (ULxInteractionActionComponentBase* InteractionFeature = CreateInteractionFeatureForNode(InteractionNode))
		{
			InteractionFeatures.Add(InteractionFeature);
			InteractionNode->SetInteractionFeature(InteractionFeature);
			InteractionFeature->InitializeInteractionFeature(this, InteractionNode,
				InteractionNode->GetRuntimeNodeIndex());
		}
	}

	for (ULxInteractionNode* ChildNode : InteractionNode->GetChildNodes())
	{
		BuildInteractionFeaturesRecursive(ChildNode, InteractionNode, InOutRuntimeNodeIndex, VisitedNodes);
	}
}

ULxInteractionActionComponentBase* ULxInteractableComponent::CreateInteractionFeatureForNode(
	ULxInteractionNode* InteractionNode)
{
	if (!InteractionNode || !InteractionNode->IsFunctionNode()
		|| !IsInteractionFeatureEnabled(InteractionNode->GetInteractionActionType()))
	{
		return nullptr;
	}

	TSubclassOf<ULxInteractionActionComponentBase> FeatureClass;
	switch (InteractionNode->GetInteractionActionType())
	{
	case ELxInteractionActionType::TriggerMechanism:
		FeatureClass = ULxTriggerMechanismInteractionComponent::StaticClass();
		break;
	case ELxInteractionActionType::ItemTransfer:
		FeatureClass = ULxItemTransferInteractionComponent::StaticClass();
		break;
	case ELxInteractionActionType::TreasureChest:
		FeatureClass = ULxTreasureChestInteractionComponent::StaticClass();
		break;
	case ELxInteractionActionType::Warehouse:
		FeatureClass = ULxWarehouseInteractionComponent::StaticClass();
		break;
	case ELxInteractionActionType::TradeContainer:
		FeatureClass = ULxTradeContainerInteractionComponent::StaticClass();
		break;
	case ELxInteractionActionType::FunctionPage:
		FeatureClass = ULxFunctionPageInteractionComponent::StaticClass();
		break;
	case ELxInteractionActionType::Quest:
		FeatureClass = ULxQuestInteractionComponent::StaticClass();
		break;
	case ELxInteractionActionType::Entrance:
	case ELxInteractionActionType::Dialogue:
	case ELxInteractionActionType::InteractionExit:
	default:
		UE_LOG(LogTemp, Error, TEXT("功能交互节点使用了不支持的交互类型：%d。"),
			static_cast<int32>(InteractionNode->GetInteractionActionType()));
		break;
	}

	ULxInteractionActionComponentBase* InteractionFeature = nullptr;
	if (FeatureClass)
	{
		const FName BaseFeatureName(*FString::Printf(TEXT("交互功能模块_%d"),
			InteractionNode->GetRuntimeNodeIndex()));
		const FName UniqueFeatureName = MakeUniqueObjectName(this, FeatureClass, BaseFeatureName);
		InteractionFeature = NewObject<ULxInteractionActionComponentBase>(this, FeatureClass, UniqueFeatureName);
	}

	ApplyFeatureConfigToFeature(InteractionFeature, InteractionNode);
	return InteractionFeature;
}

bool ULxInteractableComponent::IsInteractionFeatureEnabled(ELxInteractionActionType InteractionType) const
{
	switch (InteractionType)
	{
	case ELxInteractionActionType::TreasureChest:
		return bEnableTreasureChest;
	case ELxInteractionActionType::Warehouse:
		return bEnableWarehouse;
	case ELxInteractionActionType::TradeContainer:
		return bEnableTradeContainer;
	case ELxInteractionActionType::TriggerMechanism:
		return bEnableTriggerMechanism;
	case ELxInteractionActionType::ItemTransfer:
		return bEnableItemTransfer;
	case ELxInteractionActionType::FunctionPage:
		return bEnableFunctionPage;
	case ELxInteractionActionType::Quest:
		return bEnableQuestInteraction;
	default:
		return false;
	}
}

void ULxInteractableComponent::ApplyFeatureConfigToFeature(
	ULxInteractionActionComponentBase* InteractionFeature, const ULxInteractionNode* InteractionNode) const
{
	if (!InteractionFeature)
	{
		return;
	}
	if (ULxTreasureChestInteractionComponent* TreasureChestFeature = Cast<ULxTreasureChestInteractionComponent>(InteractionFeature))
	{
		TreasureChestFeature->ApplyConfig(TreasureChestConfig);
	}
	else if (ULxWarehouseInteractionComponent* WarehouseFeature = Cast<ULxWarehouseInteractionComponent>(InteractionFeature))
	{
		WarehouseFeature->ApplyConfig(WarehouseConfig);
	}
	else if (ULxTradeContainerInteractionComponent* TradeFeature = Cast<ULxTradeContainerInteractionComponent>(InteractionFeature))
	{
		TradeFeature->ApplyConfig(TradeContainerConfig);
	}
	else if (ULxTriggerMechanismInteractionComponent* MechanismFeature = Cast<ULxTriggerMechanismInteractionComponent>(InteractionFeature))
	{
		MechanismFeature->ApplyConfig(TriggerMechanismConfig);
	}
	else if (ULxItemTransferInteractionComponent* ItemTransferFeature = Cast<ULxItemTransferInteractionComponent>(InteractionFeature))
	{
		ItemTransferFeature->ApplyConfig(ItemTransferConfig);
	}
	else if (ULxFunctionPageInteractionComponent* FunctionPageFeature = Cast<ULxFunctionPageInteractionComponent>(InteractionFeature))
	{
		FunctionPageFeature->ApplyConfig(FunctionPageConfig);
	}
	else if (ULxQuestInteractionComponent* QuestFeature = Cast<ULxQuestInteractionComponent>(InteractionFeature))
	{
		QuestFeature->ApplyConfig(InteractionNode
			? InteractionNode->GetQuestInteractionConfig()
			: FLxQuestInteractionConfig());
	}
}

void ULxInteractableComponent::BindReplicatedFeaturesToNodes()
{
	for (ULxInteractionActionComponentBase* InteractionFeature : InteractionFeatures)
	{
		if (!InteractionFeature)
		{
			continue;
		}

		ULxInteractionNode* InteractionNode = FindInteractionNodeByRuntimeIndex(InteractionFeature->GetRuntimeNodeIndex());
		if (!InteractionNode)
		{
			continue;
		}

		InteractionNode->SetInteractionFeature(InteractionFeature);
		ApplyFeatureConfigToFeature(InteractionFeature, InteractionNode);
		if (InteractionFeature->GetOwnerInteractionNode() != InteractionNode)
		{
			InteractionFeature->InitializeInteractionFeature(this, InteractionNode,
				InteractionNode->GetRuntimeNodeIndex());
		}
	}
}

void ULxInteractableComponent::OnRep_InteractionFeatures()
{
	BindReplicatedFeaturesToNodes();
	RefreshInteractionOptions();
}
