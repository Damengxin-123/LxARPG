#include "LxEquipmentCraftingTypes.h"

FLxEquipmentCraftQuote LxEquipmentCrafting::CalculateQuote(ELxEquipmentCraftOperation Operation,
    float Enhancement, int32 EntryCount, const FLxEquipmentCraftService& Service)
{
    FLxEquipmentCraftQuote Quote;
    if (!FMath::IsFinite(Enhancement) || Enhancement < 0.f || EntryCount < 0
        || !FMath::IsFinite(Service.SuccessMultiplier) || Service.SuccessMultiplier < 0.f
        || !FMath::IsFinite(Service.GoldMultiplier) || Service.GoldMultiplier < 0.f)
    {
        Quote.Reason = FText::FromString(TEXT("打造配置无效")); return Quote;
    }
    double BaseCost = 5.;
    switch (Operation)
    {
    case ELxEquipmentCraftOperation::Enhance:
        Quote.SuccessChance = FMath::Clamp(FMath::Max(.01, FMath::Pow(100., -double(Enhancement))) * Service.SuccessMultiplier, 0., 1.);
        BaseCost = 1. + 49. * Enhancement;
        break;
    case ELxEquipmentCraftOperation::AddEntry:
        Quote.SuccessChance = FMath::Clamp(FMath::Max(.01, FMath::Pow(100., -EntryCount / 10.)) * Service.SuccessMultiplier, 0., 1.);
        BaseCost = 1. + 4.9 * EntryCount;
        break;
    case ELxEquipmentCraftOperation::AdjustEntries: Quote.SuccessChance = .5f; break;
    case ELxEquipmentCraftOperation::RemoveEntry: Quote.SuccessChance = 1.f - FMath::Pow(.5f, EntryCount); break;
    default: Quote.Reason = FText::FromString(TEXT("未知打造操作")); return Quote;
    }
    const double Cost = BaseCost * Service.GoldMultiplier;
    if (!FMath::IsFinite(Cost) || Cost > MAX_int32)
    {
        Quote.Reason = FText::FromString(TEXT("金币费用超出有效范围")); return Quote;
    }
    Quote.GoldCost = FMath::CeilToInt(Cost - 1.e-6);
    Quote.bAvailable = true;
    return Quote;
}

float LxEquipmentCrafting::RollEnhancement(float Current, bool bSuccess, FRandomStream& Random)
{
    if (!bSuccess && Current <= .3f) return Current;
    const float Delta = Random.RandRange(1, 10) / 100.f;
    return FMath::RoundToFloat(FMath::Max(0.f, Current + (bSuccess ? Delta : -Delta)) * 100.f) / 100.f;
}

float LxEquipmentCrafting::RollProportion(float Current, FRandomStream& Random)
{
    const bool bIncrease = Random.RandRange(0, 1) == 1;
    const float Delta = Random.RandRange(0, 20) / 100.f;
    return float(FMath::RoundToDouble(FMath::Max(0., double(Current) + (bIncrease ? Delta : -Delta)) * 100.) / 100.);
}

int32 LxEquipmentCrafting::RollRemoval(int32 Count, FRandomStream& Random)
{
    for (int32 Index = 0; Index < Count; ++Index) if (Random.FRand() < .5f) return Index;
    return INDEX_NONE;
}
