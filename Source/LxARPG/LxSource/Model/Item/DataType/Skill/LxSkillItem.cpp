#include "LxSkillItem.h"

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
		return ELxItemUseState::Failed;
	}

	if (!SkillObject->TryEndSkillCharge())
	{
		return ELxItemUseState::Failed;
	}

	return ELxItemUseState::CastSkill;
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
	CreateSkillObject();
}

FLxItemInformationBase* ULxSkillItem::ItemBase()
{
	return &SkillItemInformation;
}

void ULxSkillItem::CreateSkillObject()
{
	SkillObject = nullptr;
	if (!SkillItemInformation.SkillClass)
	{
		return;
	}

	SkillObject = NewObject<ULxSkill>(this, SkillItemInformation.SkillClass);
}
