#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Model/Entry/DataType/LxEntry.h"
#include "LxCharacterEntryPackage.generated.h"

/**
 * 角色词条来源。
 *
 * 用于标记一组词条来自哪个角色模块，方便数据中转组件把词条分发给属性、Buff、装备等系统。
 */
UENUM(BlueprintType)
enum class ELxCharacterEntrySource : uint8
{
	None UMETA(DisplayName="无"),
	Backpack UMETA(DisplayName="背包"),
	Equipment UMETA(DisplayName="装备"),
	Buff UMETA(DisplayName="Buff"),
	Skill UMETA(DisplayName="技能"),
	Profession UMETA(DisplayName="职业"),
	Other UMETA(DisplayName="其他"),
};

/**
 * 角色词条包。
 *
 * 数据中转组件接收其他模块的词条变化后，通过该结构体统一携带词条来源和词条列表。
 */
USTRUCT(BlueprintType)
struct LXARPG_API FLxCharacterEntryPackage
{
	GENERATED_BODY()

	/** 当前词条包的来源模块。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色词条包", DisplayName="词条来源")
	ELxCharacterEntrySource EntrySource = ELxCharacterEntrySource::None;

	/** 原始词条列表，用于保留来源模块传入的完整词条集合。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色词条包", DisplayName="词条列表")
	TArray<TObjectPtr<ULxEntryObjectBase>> EntryList;

	/** 需要转发给角色属性组件的属性增益词条列表。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色词条包", DisplayName="属性增益词条列表")
	TArray<TObjectPtr<ULxEntryObjectBase>> CharacterAttributeEntryList;

	/** 需要转发给角色属性组件的属性回复词条列表。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色词条包", DisplayName="属性回复词条列表")
	TArray<TObjectPtr<ULxEntryObjectBase>> AttributeRecoveryEntryList;

	/** 需要转发给 Buff 组件的 Buff 词条列表。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色词条包", DisplayName="Buff词条列表")
	TArray<TObjectPtr<ULxEntryObjectBase>> BuffEntryList;

	/** 需要转发给装备组件的装备属性词条列表。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色词条包", DisplayName="装备属性词条列表")
	TArray<TObjectPtr<ULxEntryObjectBase>> EquipmentAttributeEntryList;
};
