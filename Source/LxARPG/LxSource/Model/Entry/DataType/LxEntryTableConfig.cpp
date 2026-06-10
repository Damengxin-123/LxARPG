#include "LxEntryTableConfig.h"

namespace
{
	TMap<FGameplayTag, TSharedPtr<FLxEntryBase>> GEntryMap;

	bool HasValidEntryID(const FLxEntryBase& InEntryData)
	{
		return InEntryData.EntryID.IsValid();
	}

	template <typename EntryDataType>
	void SetEntryData(const EntryDataType& InEntryData)
	{
		if (HasValidEntryID(InEntryData))
		{
			GEntryMap.Add(InEntryData.EntryID, MakeShared<EntryDataType>(InEntryData));
		}
	}
}

namespace LxEntryConfig
{
	void ClearEntryConfig()
	{
		GEntryMap.Empty();
	}

	void SetAttributeGainEntryData(const FLxEntryAttributeGain& InEntryData)
	{
		SetEntryData(InEntryData);
	}

	void SetAttributeRecoveryEntryData(const FLxEntryAttributeRecovery& InEntryData)
	{
		SetEntryData(InEntryData);
	}

	void SetChangeStateEntryData(const FLxEntryChangeState& InEntryData)
	{
		SetEntryData(InEntryData);
	}

	void SetCreateBuffEntryData(const FLxEntryCreateBuff& InEntryData)
	{
		SetEntryData(InEntryData);
	}

	void SetMultiTargetEntryData(const FLxEntryMultiTarget& InEntryData)
	{
		SetEntryData(InEntryData);
	}

	void SetDisplayTextEntryData(const FLxEntryDisplayText& InEntryData)
	{
		SetEntryData(InEntryData);
	}

	const FLxEntryBase* GetEntryData(FGameplayTag InEntryID)
	{
		if (!InEntryID.IsValid())
		{
			return nullptr;
		}

		if (const TSharedPtr<FLxEntryBase>* EntryData = GEntryMap.Find(InEntryID))
		{
			return EntryData->Get();
		}

		return nullptr;
	}
}
