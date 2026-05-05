#include "LxEntryTableConfig.h"

namespace
{
	TMap<ELxAttributeGainEntryID, FLxEntryAttributeGain> GAttributeGainEntryMap;
	TMap<ELxAttributeRecoveryEntryID, FLxEntryAttributeRecovery> GAttributeRecoveryEntryMap;
	TMap<ELxChangeStateEntryID, FLxEntryChangeState> GChangeStateEntryMap;
	TMap<ELxCreateBuffEntryID, FLxEntryCreateBuff> GCreateBuffEntryMap;
	TMap<ELxMultiTargetEntryID, FLxEntryMultiTarget> GMultiTargetEntryMap;
	TMap<ELxDisplayTextEntryID, FLxEntryDisplayText> GDisplayTextEntryMap;
}

namespace LxEntryConfig
{
	void ClearEntryConfig()
	{
		GAttributeGainEntryMap.Empty();
		GAttributeRecoveryEntryMap.Empty();
		GChangeStateEntryMap.Empty();
		GCreateBuffEntryMap.Empty();
		GMultiTargetEntryMap.Empty();
		GDisplayTextEntryMap.Empty();
	}

	void SetAttributeGainEntryData(const FLxEntryAttributeGain& InEntryData)
	{
		if (InEntryData.EntryID != ELxAttributeGainEntryID::None)
		{
			GAttributeGainEntryMap.Add(InEntryData.EntryID, InEntryData);
		}
	}

	void SetAttributeRecoveryEntryData(const FLxEntryAttributeRecovery& InEntryData)
	{
		if (InEntryData.EntryID != ELxAttributeRecoveryEntryID::None)
		{
			GAttributeRecoveryEntryMap.Add(InEntryData.EntryID, InEntryData);
		}
	}

	void SetChangeStateEntryData(const FLxEntryChangeState& InEntryData)
	{
		if (InEntryData.EntryID != ELxChangeStateEntryID::None)
		{
			GChangeStateEntryMap.Add(InEntryData.EntryID, InEntryData);
		}
	}

	void SetCreateBuffEntryData(const FLxEntryCreateBuff& InEntryData)
	{
		if (InEntryData.EntryID != ELxCreateBuffEntryID::None)
		{
			GCreateBuffEntryMap.Add(InEntryData.EntryID, InEntryData);
		}
	}

	void SetMultiTargetEntryData(const FLxEntryMultiTarget& InEntryData)
	{
		if (InEntryData.EntryID != ELxMultiTargetEntryID::None)
		{
			GMultiTargetEntryMap.Add(InEntryData.EntryID, InEntryData);
		}
	}

	void SetDisplayTextEntryData(const FLxEntryDisplayText& InEntryData)
	{
		if (InEntryData.EntryID != ELxDisplayTextEntryID::None)
		{
			GDisplayTextEntryMap.Add(InEntryData.EntryID, InEntryData);
		}
	}

	const TMap<ELxAttributeGainEntryID, FLxEntryAttributeGain>& GetAttributeGainEntryMap()
	{
		return GAttributeGainEntryMap;
	}

	const TMap<ELxAttributeRecoveryEntryID, FLxEntryAttributeRecovery>& GetAttributeRecoveryEntryMap()
	{
		return GAttributeRecoveryEntryMap;
	}

	const TMap<ELxChangeStateEntryID, FLxEntryChangeState>& GetChangeStateEntryMap()
	{
		return GChangeStateEntryMap;
	}

	const TMap<ELxCreateBuffEntryID, FLxEntryCreateBuff>& GetCreateBuffEntryMap()
	{
		return GCreateBuffEntryMap;
	}

	const TMap<ELxMultiTargetEntryID, FLxEntryMultiTarget>& GetMultiTargetEntryMap()
	{
		return GMultiTargetEntryMap;
	}

	const TMap<ELxDisplayTextEntryID, FLxEntryDisplayText>& GetDisplayTextEntryMap()
	{
		return GDisplayTextEntryMap;
	}

	FLxEntryBase* GetEntryData(ELxEntryType InEntryType, uint8 InEntryID)
	{
		switch (InEntryType)
		{
		case ELxEntryType::AttributeGain:
			return GAttributeGainEntryMap.Find(static_cast<ELxAttributeGainEntryID>(InEntryID));
		case ELxEntryType::AttributeRecovery:
			return GAttributeRecoveryEntryMap.Find(static_cast<ELxAttributeRecoveryEntryID>(InEntryID));
		case ELxEntryType::ChangeState:
			return GChangeStateEntryMap.Find(static_cast<ELxChangeStateEntryID>(InEntryID));
		case ELxEntryType::CreateBuff:
			return GCreateBuffEntryMap.Find(static_cast<ELxCreateBuffEntryID>(InEntryID));
		case ELxEntryType::MultiTarget:
			return GMultiTargetEntryMap.Find(static_cast<ELxMultiTargetEntryID>(InEntryID));
		case ELxEntryType::DisplayText:
			return GDisplayTextEntryMap.Find(static_cast<ELxDisplayTextEntryID>(InEntryID));
		default:
			return nullptr;
		}
	}
}
