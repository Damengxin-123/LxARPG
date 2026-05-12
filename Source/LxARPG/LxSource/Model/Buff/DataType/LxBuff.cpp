#include "LxBuff.h"

ULxBuff::ULxBuff()
{
	m_fBuffInformation = FLxBuffInformation();
}

ULxBuff::~ULxBuff()
{
}

ELxItemUseState ULxBuff::ItemUse()
{
	return ELxItemUseState::ActivateEntry;
}


FLxString ULxBuff::ItemCountText()
{
	if (RemainingDuration < 0.f)
	{
		return FLxString(TEXT("永久"));
	}

	return FLxString(FMath::Max(0, FMath::CeilToInt(RemainingDuration)));
}

void ULxBuff::SetRemainingDuration(float InRemainingDuration)
{
	const int32 OldDisplaySeconds = RemainingDuration < 0.f ? INDEX_NONE : FMath::CeilToInt(RemainingDuration);
	RemainingDuration = InRemainingDuration;
	const int32 NewDisplaySeconds = RemainingDuration < 0.f ? INDEX_NONE : FMath::CeilToInt(RemainingDuration);

	// 复用数量变化事件驱动格子数量文本刷新；Buff 的“数量文本”实际代表剩余时间。
	if (OldDisplaySeconds != NewDisplaySeconds)
	{
		BroadcastItemCountChanged();
	}
}

void ULxBuff::SetItemData(const FLxItemInformationBase* InItemData, FLxItemCount InItemCount)
{
	if (InItemData)
	{
		if (InItemData->ItemType == ELxItemType::Buff)
		{
			m_fBuffInformation = *static_cast<const FLxBuffInformation*>(InItemData);
			m_fBuffInformation.ItemCount = InItemCount;
		}
	}
}

FLxItemInformationBase* ULxBuff::ItemBase()
{
	return &m_fBuffInformation;
}
