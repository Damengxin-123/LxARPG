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

	SkillObject->ReleaseSkillDirectly();
	return ELxItemUseState::CastSkill;
}

FLxString ULxSkillItem::ItemCountText()
{
	return SkillItemInformation.ItemCount > 1 ? FLxString(SkillItemInformation.ItemCount) : FLxString();
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
