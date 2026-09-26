#include "LxCharacterSaveComponent.h"

#include "LxGameSaveData.h"
#include "LxARPG/LxSource/Model/Content/Logic/LxCharacterContentComponent.h"
#include "LxARPG/LxSource/Model/Item/Logic/LxCharacterBackpackComponent.h"
#include "LxARPG/LxSource/Model/Item/Logic/LxCharacterEquipmentComponent.h"
#include "LxARPG/LxSource/Model/Profession/Logic/LxCharacterProfessionComponent.h"
#include "LxARPG/LxSource/Model/Quest/Logic/LxCharacterQuestModule.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"

FGameplayTag ULxCharacterSaveComponent::GetSaveID() const
{
	const FGameplayTag ExplicitID = Super::GetSaveID();
	const ALxBaseCharacter* Character = Cast<ALxBaseCharacter>(GetOwner());
	return ExplicitID.IsValid() || !Character ? ExplicitID : Character->GetCharacterIDTag();
}

bool ULxCharacterSaveComponent::CaptureSaveData(ULxGameSaveData* InSaveData) const
{
	const ALxBaseCharacter* Character = Cast<ALxBaseCharacter>(GetOwner());
	ULxCharacterContentComponent* Content = Character ? Character->GetCharacterContentComponent() : nullptr;
	if (!InSaveData || !Character || !Character->HasAuthority() || !GetSaveID().IsValid() || !Content
		|| !Content->GetBackpackModule() || !Content->GetEquipmentModule()
		|| !Content->GetProfessionModule() || !Content->GetQuestModule())
	{
		return false;
	}
	FLxCharacterSaveRecord Record;
	Record.SaveID = GetSaveID();
	Record.BackpackSlotCount = Content->GetBackpackModule()->GetAllItems().Num();
	LxItemSaveData::CaptureSlots(Content->GetBackpackModule()->GetAllItems(), Record.BackpackSlots);
	LxItemSaveData::CaptureSlots(Content->GetEquipmentModule()->GetEquipmentSlots(), Record.EquipmentSlots);
	Record.QuestRecords = Content->GetQuestModule()->GetAllQuestRecords();
	Content->GetProfessionModule()->CaptureProfessionSaveData(Record.Professions);
	InSaveData->Players.Add(Record.SaveID, MoveTemp(Record));
	return true;
}

bool ULxCharacterSaveComponent::RestoreSaveData(const ULxGameSaveData* InSaveData)
{
	const ALxBaseCharacter* Character = Cast<ALxBaseCharacter>(GetOwner());
	ULxCharacterContentComponent* Content = Character ? Character->GetCharacterContentComponent() : nullptr;
	if (!InSaveData || !Character || !Character->HasAuthority() || !GetSaveID().IsValid() || !Content
		|| !Content->GetBackpackModule() || !Content->GetEquipmentModule()
		|| !Content->GetProfessionModule() || !Content->GetQuestModule())
	{
		return false;
	}
	const FLxCharacterSaveRecord* Record = InSaveData->Players.Find(GetSaveID());
	if (!Record)
	{
		return true;
	}
	if (Record->SaveID != GetSaveID())
	{
		return false;
	}
	// 先检查四类数据，避免坏存档导致背包已替换但任务或职业恢复失败。
	if (!Content->GetBackpackModule()->RestoreBackpackSaveData(Record->BackpackSlotCount, Record->BackpackSlots, false)
		|| !Content->GetEquipmentModule()->RestoreEquipmentSaveData(Record->EquipmentSlots, false)
		|| !Content->GetQuestModule()->RestoreQuestSaveData(Record->QuestRecords, false)
		|| !Content->GetProfessionModule()->RestoreProfessionSaveData(Record->Professions, false))
	{
		return false;
	}
	// 职业先恢复，再由装备刷新来源效果，避免装备授予的职业进度被随后覆盖。
	return Content->GetBackpackModule()->RestoreBackpackSaveData(Record->BackpackSlotCount, Record->BackpackSlots)
		&& Content->GetQuestModule()->RestoreQuestSaveData(Record->QuestRecords)
		&& Content->GetProfessionModule()->RestoreProfessionSaveData(Record->Professions)
		&& Content->GetEquipmentModule()->RestoreEquipmentSaveData(Record->EquipmentSlots);
}
