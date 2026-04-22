#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxCharacterComponentBase.h"
#include "LxARPG/LxSource/Model/Attribute/DataType/LxAttributeEnumType.h"
#include "LxARPG/LxSource/Model/Entry/DataType/LxItemEntryData.h"
#include "LxCharacterEntryComponent.generated.h"

class ALxBaseCharacter;
class ULxCharacterBackpackComponent;
class ULxCharacterEquipmentComponent;
class ULxEquipmentLogic;
class ULxEquipmentSlotData;
class ULxItemLogicBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInstantRestoreEntryApplied, ELxCharacterAttributeID, AttributeID, float, RestoreValue);

USTRUCT(BlueprintType)
struct FLxCharacterEntryPackage
{
	GENERATED_BODY()

	/** 当前所有已安装来源（目前主要是装备）提取出的原始词条列表。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FLxItemEntryData> InstalledEntryList;

	/** 直接作用于角色属性计算的词条。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FLxItemEntryData> CharacterAttributeEntryList;

	/** 作用于装备自身效果或装备派生属性的词条。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FLxItemEntryData> EquipmentAttributeEntryList;

	/** 需要交给 Buff 系统处理的词条。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FLxItemEntryData> BuffEntryList;

	/** 使用物品时可触发的即时恢复类词条。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FLxItemEntryData> InstantRestoreEntryList;

	/** 施加状态效果的词条。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FLxItemEntryData> StatusEntryList;

	/** 提供探索或功能性能力的词条。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FLxItemEntryData> UtilityEntryList;

	/** 投掷型消耗品相关词条。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FLxItemEntryData> ThrowConsumableEntryList;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCharacterEntryPackageChanged, const FLxCharacterEntryPackage&, EntryPackage);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable, DisplayName="角色词条组件")
class LXARPG_API ULxCharacterEntryComponent : public ULxCharacterComponentBase
{
	GENERATED_BODY()

public:
	ULxCharacterEntryComponent();

	virtual void BaseComponentInitialize() override;

	/** 获取当前已安装来源解析后的全部词条。 */
	void GetInstalledEntries(TArray<FLxItemEntryData>& OutEntryList) const;

	/** 获取最近一次打包好的分类结果，供其他系统直接消费。 */
	const FLxCharacterEntryPackage& GetEntryPackage() const { return m_EntryPackage; }

	UPROPERTY(BlueprintAssignable, DisplayName="词条即时恢复事件")
	FOnInstantRestoreEntryApplied OnInstantRestoreEntryApplied;

	/** 当已安装词条重新分类后，向属性/Buff/其他组件广播最新打包结果。 */
	UPROPERTY(BlueprintAssignable, DisplayName="词条打包更新事件")
	FOnCharacterEntryPackageChanged OnEntryPackageChanged;

private:
	UFUNCTION()
	void HandleEquipmentDataChange();

	UFUNCTION()
	void HandleBackpackItemUsed(ULxItemLogicBase* UsedItem);

	/** 重新收集所有已安装来源中的词条，并刷新分类缓存。 */
	void RefreshInstalledEntries();

	/** 按逻辑类型将已安装词条打包成统一分发结构。 */
	void BuildEntryPackage();

	/** 从装备列表中提取原始词条；装备组件只提供装备本身，不负责词条解释。 */
	void AppendEquipmentEntries(const ULxCharacterEquipmentComponent& InEquipmentComponent, TArray<FLxItemEntryData>& OutEntryList) const;

	/** 处理一次“使用物品”产生的临时词条效果。 */
	void DispatchUsedItemEntries(const TArray<FLxItemEntryData>& InEntryList);

	UPROPERTY()
	TObjectPtr<ALxBaseCharacter> m_pOwnerCharacter;

	UPROPERTY()
	TArray<FLxItemEntryData> m_vInstalledEntries;

	/** 供其他组件直接消费的分类打包结果。 */
	UPROPERTY()
	FLxCharacterEntryPackage m_EntryPackage;

	bool m_bEntryInitialized = false;
};
