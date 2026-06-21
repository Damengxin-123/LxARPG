#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
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
	void SetGrantSkillEntryData(const FLxEntryGrantSkill& InEntryData);

	/**
	 * 根据词条标签 ID 查询词条数据。
	 *
	 * 词条标签 ID 本身包含词条类型层级，查询结果中的 EntryType 用于区分运行时词条对象。
	 */
	const FLxEntryBase* GetEntryData(FGameplayTag InEntryID);
}
