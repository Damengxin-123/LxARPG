#include "LxSkillItem.h"
#include "LxARPG/LxSource/Model/Skill/Logic/Skill/LxSkillCastComponent.h"

FLxSkillItemInformation::FLxSkillItemInformation()
{
	ItemType = ELxItemType::Skill;
	ItemIDTag = LxTag_Item_Skill;
	ItemCount = 1;
	ItemCountMax = 1;
}

ULxSkillItem::ULxSkillItem()
{
	SkillItemInformation = FLxSkillItemInformation();
}

ULxSkillItem::~ULxSkillItem()
{
}

ELxItemUseState ULxSkillItem::ItemUse()
{
	if (!ItemIsValid())
	{
		return ELxItemUseState::Failed;
	}

	if (!SkillObject)
	{
		CreateSkillObject();
	}

	if (!SkillObject)
	{
		return ELxItemUseState::Failed;
	}

	if (!SkillObject->TryReleaseSkillDirectly())
	{
		return ELxItemUseState::Failed;
	}

	return ELxItemUseState::CastSkill;
}

ELxItemUseState ULxSkillItem::ItemUseStart()
{
	if (!ItemIsValid())
	{
		return ELxItemUseState::Failed;
	}

	if (!SkillObject)
	{
		CreateSkillObject();
	}

	if (!SkillObject)
	{
		return ELxItemUseState::Failed;
	}

	if (SkillObject->CanSkillCharge())
	{
		if (!SkillObject->TryStartSkillCharge())
		{
			return ELxItemUseState::Failed;
		}

		return ELxItemUseState::CastSkill;
	}

	if (!SkillObject->TryReleaseSkillDirectly())
	{
		return ELxItemUseState::Failed;
	}

	return ELxItemUseState::CastSkill;
}

ELxItemUseState ULxSkillItem::ItemUseEnd()
{
	if (!ItemIsValid() || !SkillObject)
	{
		return ELxItemUseState::Failed;
	}

	if (!SkillObject->CanSkillCharge())
	{
		ULxSkillCastModule* Module = SkillObject->ResolveSkillCastModule();
		return SkillObject->IsSustainedReleaseSkill() && Module
			&& Module->EndUseSkillItem(this, SkillObject->GetSkillCastContext())
			? ELxItemUseState::CastSkill : ELxItemUseState::Failed;
	}

	if (!SkillObject->TryEndSkillCharge())
	{
		return ELxItemUseState::Failed;
	}

	return ELxItemUseState::CastSkill;
}

void ULxSkillItem::InvalidateSkillItem()
{
	SkillItemInformation.ItemCount = 0;
	BroadcastItemCountChanged();
}

FLxString ULxSkillItem::ItemCountText()
{
	return SkillItemInformation.ItemCount > 1 ? FLxString(SkillItemInformation.ItemCount) : FLxString();
}

ULxSkill* ULxSkillItem::GetOrCreateSkillObject()
{
	if (!SkillObject)
	{
		CreateSkillObject();
	}

	return SkillObject;
}

void ULxSkillItem::SetItemData(const FLxItemInformationBase* InItemData, FLxItemCount InItemCount)
{
	if (!InItemData || InItemData->ItemType != ELxItemType::Skill)
	{
		return;
	}

	SkillItemInformation = *static_cast<const FLxSkillItemInformation*>(InItemData);
	SkillItemInformation.ItemCount = InItemCount;
	SkillObject = nullptr;
}

FLxItemInformationBase* ULxSkillItem::ItemBase()
{
	return &SkillItemInformation;
}

void ULxSkillItem::CreateSkillObject()
{
	SkillObject = nullptr;
	if (SkillItemInformation.SkillFlow)
	{
		SkillObject = NewObject<ULxSkill>(this);
		SkillObject->FlowAsset = SkillItemInformation.SkillFlow;
		return;
	}
}
