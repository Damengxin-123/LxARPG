#include "LxInteractableComponent.h"

#include "Engine/ActorChannel.h"
#include "GameFramework/Actor.h"
#include "LxARPG/LxSource/Model/Interaction/Interface/LxInteractionReceiverInterface.h"
#include "LxFunctionPageInteractionComponent.h"
#include "LxInteractionActionComponentBase.h"
#include "LxInteractionNode.h"
#include "LxItemTransferInteractionComponent.h"
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
}

void ULxInteractableComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
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

TArray<ULxInteractionNode*> ULxInteractableComponent::GetValidRootInteractionNodes() const
{
	TArray<ULxInteractionNode*> Result;
	for (ULxInteractionNode* RootNode : RootInteractionNodes)
	{
		if (RootNode && RootNode->IsNodeValid())
		{
			Result.Add(RootNode);
		}
	}
	return Result;
}

bool ULxInteractableComponent::HasValidInteraction() const
{
	return GetValidRootInteractionNodes().Num() > 0;
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

ULxInteractionActionComponentBase* ULxInteractableComponent::FindInteractionFeatureByType(
	ELxInteractionActionType InteractionType) const
{
	for (ULxInteractionActionComponentBase* InteractionFeature : InteractionFeatures)
	{
		if (InteractionFeature && InteractionFeature->GetInteractionActionType() == InteractionType)
		{
			return InteractionFeature;
		}
	}
	return nullptr;
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

	ULxInteractionActionComponentBase* InteractionFeature = InteractionNode->GetActionComponent();
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
			NodePair.Value->SetActionComponent(nullptr);
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
			InteractionNode->SetActionComponent(InteractionFeature);
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
	if (!InteractionNode || !InteractionNode->IsFunctionNode())
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

	ApplyNodeConfigToFeature(InteractionNode, InteractionFeature);
	return InteractionFeature;
}

void ULxInteractableComponent::ApplyNodeConfigToFeature(ULxInteractionNode* InteractionNode,
	ULxInteractionActionComponentBase* InteractionFeature) const
{
	if (!InteractionNode || !InteractionFeature)
	{
		return;
	}

	const FLxInteractionFeatureNodeConfig& FeatureConfig = InteractionNode->GetFeatureConfig();
	if (ULxTreasureChestInteractionComponent* TreasureChestFeature = Cast<ULxTreasureChestInteractionComponent>(InteractionFeature))
	{
		TreasureChestFeature->ApplyConfig(FeatureConfig.TreasureChestConfig);
	}
	else if (ULxWarehouseInteractionComponent* WarehouseFeature = Cast<ULxWarehouseInteractionComponent>(InteractionFeature))
	{
		WarehouseFeature->ApplyConfig(FeatureConfig.WarehouseConfig);
	}
	else if (ULxTradeContainerInteractionComponent* TradeFeature = Cast<ULxTradeContainerInteractionComponent>(InteractionFeature))
	{
		TradeFeature->ApplyConfig(FeatureConfig.TradeContainerConfig);
	}
	else if (ULxTriggerMechanismInteractionComponent* MechanismFeature = Cast<ULxTriggerMechanismInteractionComponent>(InteractionFeature))
	{
		MechanismFeature->ApplyConfig(FeatureConfig.TriggerMechanismConfig);
	}
	else if (ULxItemTransferInteractionComponent* ItemTransferFeature = Cast<ULxItemTransferInteractionComponent>(InteractionFeature))
	{
		ItemTransferFeature->ApplyConfig(FeatureConfig.ItemTransferConfig);
	}
	else if (ULxFunctionPageInteractionComponent* FunctionPageFeature = Cast<ULxFunctionPageInteractionComponent>(InteractionFeature))
	{
		FunctionPageFeature->ApplyConfig(FeatureConfig.FunctionPageConfig);
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

		InteractionNode->SetActionComponent(InteractionFeature);
		ApplyNodeConfigToFeature(InteractionNode, InteractionFeature);
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
