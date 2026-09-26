#include "LxInteractionSaveComponent.h"

#include "LxGameSaveData.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxInteractableComponent.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxInteractionActionComponentBase.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxInteractionNode.h"

namespace
{
	/** 判断当前功能是否属于需要长期保存的交互对象数据。 */
	bool IsPersistentInteractionFeature(const ULxInteractionActionComponentBase* Feature)
	{
		if (!Feature) return false;
		const ELxInteractionActionType Type = Feature->GetInteractionActionType();
		return Type == ELxInteractionActionType::TriggerMechanism
			|| Type == ELxInteractionActionType::TreasureChest
			|| Type == ELxInteractionActionType::TradeContainer
			|| Type == ELxInteractionActionType::Warehouse;
	}
}

void ULxInteractionSaveComponent::SetInteractableComponent(ULxInteractableComponent* InComponent)
{
	InteractableComponent = InComponent;
}

FGameplayTag ULxInteractionSaveComponent::GetSaveID() const
{
	return IsValid(InteractableComponent) ? InteractableComponent->InteractionIDTag : FGameplayTag();
}

bool ULxInteractionSaveComponent::CaptureSaveData(ULxGameSaveData* InSaveData) const
{
	if (!InSaveData || !IsValid(InteractableComponent) || !GetSaveID().IsValid()) return false;
	FLxInteractionSaveRecord Record;
	Record.InteractionIDTag = GetSaveID();
	for (ULxInteractionActionComponentBase* Feature : InteractableComponent->GetInteractionFeatures())
	{
		if (!IsPersistentInteractionFeature(Feature)) continue;
		FLxInteractionFeatureSaveRecord FeatureRecord;
		if (!Feature->CapturePersistentData(FeatureRecord) || Record.Features.Contains(FeatureRecord.NodeID))
		{
			UE_LOG(LogTemp, Warning, TEXT("交互对象 %s 存档失败：功能节点ID无效或重复。"), *GetNameSafe(GetOwner()));
			return false;
		}
		Record.Features.Add(FeatureRecord.NodeID, MoveTemp(FeatureRecord));
	}
	// 功能模块已清理时禁止以空记录覆盖结束运行前采集的数据。
	if (Record.Features.IsEmpty()) return false;
	InSaveData->Interactions.Add(Record.InteractionIDTag, MoveTemp(Record));
	return true;
}

bool ULxInteractionSaveComponent::RestoreSaveData(const ULxGameSaveData* InSaveData)
{
	if (!InSaveData || !IsValid(InteractableComponent) || !GetSaveID().IsValid()) return false;
	const FLxInteractionSaveRecord* Record = InSaveData->Interactions.Find(GetSaveID());
	if (!Record) return true;
	if (Record->InteractionIDTag != GetSaveID()) return false;
	bool bSucceeded = true;
	for (ULxInteractionActionComponentBase* Feature : InteractableComponent->GetInteractionFeatures())
	{
		if (!IsPersistentInteractionFeature(Feature) || !Feature->GetOwnerInteractionNode()) continue;
		const FGuid NodeID = Feature->GetOwnerInteractionNode()->GetPersistentNodeID();
		const FLxInteractionFeatureSaveRecord* FeatureRecord = Record->Features.Find(NodeID);
		if (FeatureRecord && !Feature->RestorePersistentData(*FeatureRecord))
		{
			UE_LOG(LogTemp, Warning, TEXT("交互对象 %s 的节点 %s 无法恢复存档，保留当前配置。"),
				*GetNameSafe(GetOwner()), *NodeID.ToString());
			bSucceeded = false;
		}
	}
	return bSucceeded;
}
