#pragma once

#include "CoreMinimal.h"
#include "LxEquipmentCraftingTypes.generated.h"

/** 物品制造中的装备打造子功能。 */
UENUM(BlueprintType, DisplayName="装备打造操作")
enum class ELxEquipmentCraftOperation : uint8
{
    Enhance UMETA(DisplayName="强化装备"),
    AdjustEntries UMETA(DisplayName="调整词条"),
    AddEntry UMETA(DisplayName="增加词条"),
    RemoveEntry UMETA(DisplayName="移除词条")
};

/** NPC提供的装备打造服务倍率。 */
USTRUCT(BlueprintType, DisplayName="装备打造服务")
struct LXARPG_API FLxEquipmentCraftService
{
    GENERATED_BODY()
    /** 最终强化和新增成功率为基础成功率乘以此值，最高100%。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="交互|装备打造", DisplayName="成功率倍率", meta=(ClampMin="0.0", Delta="0.01"))
    float SuccessMultiplier = 1.f;
    /** 所有打造操作的金币倍率，结算向上取整；0表示免费。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="交互|装备打造", DisplayName="金币消耗倍率", meta=(ClampMin="0.0", Delta="0.01"))
    float GoldMultiplier = 1.f;
};

/** 服务端与界面共用的操作报价。 */
USTRUCT(BlueprintType, DisplayName="装备打造报价")
struct LXARPG_API FLxEquipmentCraftQuote
{
    GENERATED_BODY()
    /** 当前所有需求（包括金币）是否满足。 */
    UPROPERTY(BlueprintReadOnly, Category="装备打造", DisplayName="可以执行")
    bool bAvailable = false;
    /** 已应用NPC倍率并向上取整的金币费用。 */
    UPROPERTY(BlueprintReadOnly, Category="装备打造", DisplayName="金币费用")
    int32 GoldCost = 0;
    /** 最终成功率；调整表示每条提高概率，移除表示至少移除一条的概率。 */
    UPROPERTY(BlueprintReadOnly, Category="装备打造", DisplayName="成功率")
    float SuccessChance = 0.f;
    /** 不可执行时的中文原因。 */
    UPROPERTY(BlueprintReadOnly, Category="装备打造", DisplayName="操作说明")
    FText Reason;
};

/** 一次已完成或被拒绝的装备打造结果。 */
USTRUCT(BlueprintType, DisplayName="装备打造结果")
struct LXARPG_API FLxEquipmentCraftResult
{
    GENERATED_BODY()
    /** 本次操作类型。 */
    UPROPERTY(BlueprintReadOnly, Category="装备打造", DisplayName="操作")
    ELxEquipmentCraftOperation Operation = ELxEquipmentCraftOperation::Enhance;
    /** 是否完成实际尝试；金币不足等拒绝操作不会扣费。 */
    UPROPERTY(BlueprintReadOnly, Category="装备打造", DisplayName="已执行")
    bool bExecuted = false;
    /** 强化、新增和移除是否成功；调整完成即为真。 */
    UPROPERTY(BlueprintReadOnly, Category="装备打造", DisplayName="成功")
    bool bSuccess = false;
    /** 实际扣除的金币。 */
    UPROPERTY(BlueprintReadOnly, Category="装备打造", DisplayName="消耗金币")
    int32 GoldSpent = 0;
    /** 强化度本次变化，负值表示失败扣减。 */
    UPROPERTY(BlueprintReadOnly, Category="装备打造", DisplayName="强化度变化")
    float EnhancementDelta = 0.f;
    /** 实际发生变化的普通词条数量。 */
    UPROPERTY(BlueprintReadOnly, Category="装备打造", DisplayName="变化词条数")
    int32 ChangedEntries = 0;
    /** 可直接展示给玩家的操作结果。 */
    UPROPERTY(BlueprintReadOnly, Category="装备打造", DisplayName="结果说明")
    FText Message;
};

/** 可独立验证的装备打造数值规则，随机逻辑使用可注入种子的随机流。 */
namespace LxEquipmentCrafting
{
    /** 根据强化度、普通词条数和服务倍率计算未检查资金的报价。 */
    LXARPG_API FLxEquipmentCraftQuote CalculateQuote(ELxEquipmentCraftOperation Operation, float Enhancement,
        int32 EntryCount, const FLxEquipmentCraftService& Service);
    /** 强化成功增加1%至10%；失败仅在当前强化度超过30%时扣减。 */
    LXARPG_API float RollEnhancement(float Current, bool bSuccess, FRandomStream& Random);
    /** 每条独立以50%概率加减0至20个百分点，最低0%，允许超过100%。 */
    LXARPG_API float RollProportion(float Current, FRandomStream& Random);
    /** 从前到后独立投掷50%，命中一条即停止，可能全部未命中。 */
    LXARPG_API int32 RollRemoval(int32 Count, FRandomStream& Random);
}
