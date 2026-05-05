#pragma once

#include "CoreMinimal.h"
#include "LxItemEntryData.h"

namespace LxEntryConfig
{
	void ClearEntryConfig();

	void SetAttributeGainEntryData(const FLxEntryAttributeGain& InEntryData);
	void SetAttributeRecoveryEntryData(const FLxEntryAttributeRecovery& InEntryData);
	void SetChangeStateEntryData(const FLxEntryChangeState& InEntryData);
	void SetCreateBuffEntryData(const FLxEntryCreateBuff& InEntryData);
	void SetMultiTargetEntryData(const FLxEntryMultiTarget& InEntryData);
	void SetDisplayTextEntryData(const FLxEntryDisplayText& InEntryData);

	const TMap<ELxAttributeGainEntryID, FLxEntryAttributeGain>& GetAttributeGainEntryMap();
	const TMap<ELxAttributeRecoveryEntryID, FLxEntryAttributeRecovery>& GetAttributeRecoveryEntryMap();
	const TMap<ELxChangeStateEntryID, FLxEntryChangeState>& GetChangeStateEntryMap();
	const TMap<ELxCreateBuffEntryID, FLxEntryCreateBuff>& GetCreateBuffEntryMap();
	const TMap<ELxMultiTargetEntryID, FLxEntryMultiTarget>& GetMultiTargetEntryMap();
	const TMap<ELxDisplayTextEntryID, FLxEntryDisplayText>& GetDisplayTextEntryMap();

	/**
	 * 根据词条类型和 uint8 ID 查询词条数据。
	 *
	 * 调用方传入统一的 uint8 ID 后，函数会根据词条类型转换到对应的词条 ID 枚举并查询对应表。
	 */
	FLxEntryBase* GetEntryData(ELxEntryType InEntryType, uint8 InEntryID);
}
