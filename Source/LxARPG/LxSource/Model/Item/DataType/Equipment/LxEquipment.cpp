#include "LxEquipment.h"
#include "LxARPG/LxSource/Model/Item/DataType/ConstData/LxItemConstData.h"
#include "LxARPG/LxSource/Model/Entry/DataType/LxEntryTableConfig.h"

ULxEquipment::ULxEquipment()
{
	m_fEquipmentInformation = FLxEquipmentInformation();
}

ULxEquipment::~ULxEquipment()
{
}

ELxItemUseState ULxEquipment::ItemUse()
{
	return ELxItemUseState::InstallEquipment;
}

FLxString ULxEquipment::ItemCountText()
{
	return m_fEquipmentInformation.ItemCount > 1 ? FLxString(m_fEquipmentInformation.ItemCount) : FLxString();
}

ELxEquipmentType ULxEquipment::EquipmentType() const
{
	return m_fEquipmentInformation.EquipmentType;
}

FLxEquipmentInformation ULxEquipment::EquipmentInformation() const
{
	return m_fEquipmentInformation;
}

void ULxEquipment::SetItemData(const FLxItemInformationBase* InItemData, FLxItemCount InItemCount)
{
	if (InItemData)
	{
		if (InItemData->ItemType == ELxItemType::Equipment)
		{
			m_fEquipmentInformation = *static_cast<const FLxEquipmentInformation*>(InItemData);
			m_fEquipmentInformation.ItemCount = 1;
            m_fEquipmentInformation.ItemCountMax = 1;
            InstanceID = FGuid::NewGuid();
            m_fEquipmentInformation.Enhancement = FMath::Max(0, m_fEquipmentInformation.EnhancementValueOverride) / 100.f;
            // 静态表未填写ID的占位项与旧加载行为一致，不作为实际词条或稀有度计数。
            int32 NormalCount = 0;
            m_fEquipmentInformation.ItemEntryConfigs.RemoveAll([&NormalCount](const FLxItemEntryConfig& Entry)
            {
                return !Entry.EntryQuote.EntryID.IsValid()
                    || (Entry.EntryLogicType != ELxEntryLogicType::Base && ++NormalCount > MaxNormalEntries);
            });
            // 旧分类只用于读取兼容；装备运行时统一为基础与普通两类。
            for (FLxItemEntryConfig& Entry : m_fEquipmentInformation.ItemEntryConfigs)
            {
                if (Entry.EntryLogicType != ELxEntryLogicType::Base) Entry.EntryLogicType = ELxEntryLogicType::Normal;
            }
            m_fEquipmentInformation.ItemRarity = CalculateRarity(HasFixedEntries(), GetNormalEntries().Num());
		}
	}
}

FLxItemInformationBase* ULxEquipment::ItemBase()
{
	return &m_fEquipmentInformation;
}

TArray<FLxEquipmentEntry> ULxEquipment::GetNormalEntries() const
{
    TArray<FLxEquipmentEntry> Result;
    for (const FLxItemEntryConfig& Entry : m_fEquipmentInformation.ItemEntryConfigs)
    {
        if (Entry.EntryLogicType == ELxEntryLogicType::Base) continue;
        FLxEquipmentEntry& Normal = Result.AddDefaulted_GetRef();
        Normal.EntryID = Entry.EntryQuote.EntryID;
        Normal.Proportion = Entry.EntryQuote.EntryProportion;
    }
    return Result;
}

ELxItemRarityType ULxEquipment::CalculateRarity(bool bFixed, int32 Count)
{
    if (bFixed) return Count < MaxNormalEntries * .4f ? ELxItemRarityType::Legend : ELxItemRarityType::Epic;
    if (Count < MaxNormalEntries * .1f) return ELxItemRarityType::Ordinary;
    if (Count < MaxNormalEntries * .4f) return ELxItemRarityType::Advanced;
    if (Count < MaxNormalEntries * .6f) return ELxItemRarityType::Rare;
    return ELxItemRarityType::Mythic;
}

FLxEquipmentSaveRecord ULxEquipment::CaptureEquipmentState() const
{
    FLxEquipmentSaveRecord State;
    State.EquipmentID = m_fEquipmentInformation.ItemIDTag;
    State.Enhancement = GetEnhancement();
    if (!HasFixedEntries()) State.NormalEntries = GetNormalEntries();
    return State;
}

bool ULxEquipment::RebuildEquipment(float Enhancement, const TArray<FLxEquipmentEntry>& Entries)
{
    if (!FMath::IsFinite(Enhancement) || Enhancement < 0.f || Entries.Num() > MaxNormalEntries) return false;
    const FLxItemInformationBase* Static = LxItemConfig::GetItemData(m_fEquipmentInformation.ItemIDTag);
    if (!Static || Static->ItemType != ELxItemType::Equipment) return false;
    TArray<FLxItemEntryConfig> Configs;
    for (const FLxItemEntryConfig& Config : Static->ItemEntryConfigs)
    {
        if (Config.EntryLogicType == ELxEntryLogicType::Base && Config.EntryQuote.EntryID.IsValid()) Configs.Add(Config);
    }
    for (const FLxEquipmentEntry& Entry : Entries)
    {
        if (!LxEntryConfig::GetEntryData(Entry.EntryID) || !FMath::IsFinite(Entry.Proportion)
            || Entry.Proportion < 0.f) return false;
        FLxEntryQuote Quote;
        Quote.EntryID = Entry.EntryID;
        Quote.EntryProportion = float(FMath::RoundToDouble(double(Entry.Proportion) * 100.) / 100.);
        Configs.Emplace(Quote, ELxEntryLogicType::Normal);
    }
    const float Previous = GetEnhancement();
    m_fEquipmentInformation.Enhancement = float(FMath::RoundToDouble(double(Enhancement) * 100.) / 100.);
    if (!ULxItemBase::RestoreItemEntries(Configs))
    {
        m_fEquipmentInformation.Enhancement = Previous;
        return false;
    }
    m_fEquipmentInformation.ItemRarity = CalculateRarity(HasFixedEntries(), Entries.Num());
    return true;
}

bool ULxEquipment::RestoreEquipmentState(const FLxEquipmentSaveRecord& State, const FGuid& InInstanceID)
{
    if (State.EquipmentID != m_fEquipmentInformation.ItemIDTag || !InInstanceID.IsValid()
        || (HasFixedEntries() && !State.NormalEntries.IsEmpty())) return false;
    TArray<FLxEquipmentEntry> Entries = State.NormalEntries;
    if (HasFixedEntries())
    {
        Entries.Reset();
        const FLxItemInformationBase* Static = LxItemConfig::GetItemData(State.EquipmentID);
        if (!Static) return false;
        for (const FLxItemEntryConfig& Config : Static->ItemEntryConfigs)
        {
            if (Config.EntryLogicType == ELxEntryLogicType::Base || !Config.EntryQuote.EntryID.IsValid()
                || Entries.Num() >= MaxNormalEntries) continue;
            FLxEquipmentEntry& Entry = Entries.AddDefaulted_GetRef();
            Entry.EntryID = Config.EntryQuote.EntryID;
            Entry.Proportion = Config.EntryQuote.EntryProportion;
        }
    }
    if (!RebuildEquipment(State.Enhancement, Entries)) return false;
    InstanceID = InInstanceID;
    return true;
}

bool ULxEquipment::RestoreItemEntries(const TArray<FLxItemEntryConfig>& InEntries)
{
    if (HasFixedEntries()) return RestoreEquipmentState(CaptureEquipmentState(), InstanceID);
    TArray<FLxEquipmentEntry> Entries;
    for (const FLxItemEntryConfig& Config : InEntries)
    {
        if (Config.EntryLogicType == ELxEntryLogicType::Base) continue;
        FLxEquipmentEntry& Entry = Entries.AddDefaulted_GetRef();
        Entry.EntryID = Config.EntryQuote.EntryID;
        Entry.Proportion = Config.EntryQuote.EntryProportion;
    }
    return RebuildEquipment(GetEnhancement(), Entries);
}

bool ULxEquipment::ApplyCraftingState(float Enhancement, const TArray<FLxEquipmentEntry>& Entries)
{
    if (HasFixedEntries())
    {
        const TArray<FLxEquipmentEntry> Current = GetNormalEntries();
        if (Current.Num() != Entries.Num()) return false;
        for (int32 Index = 0; Index < Current.Num(); ++Index)
            if (Current[Index].EntryID != Entries[Index].EntryID || Current[Index].Proportion != Entries[Index].Proportion) return false;
    }
    return RebuildEquipment(Enhancement, Entries);
}
