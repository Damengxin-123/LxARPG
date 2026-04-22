#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxCharacterComponentBase.h"
#include "LxARPG/LxSource/Model/Attribute/DataType/LxAttributeData.h"
#include "LxCharacterAttributeComponent.generated.h"

class ALxBaseCharacter;
class ULxCharacterEntryComponent;
struct FLxCharacterEntryPackage;
struct FLxItemEntryData;


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable, DisplayName="角色属性组件")
class LXARPG_API ULxCharacterAttributeComponent : public ULxCharacterComponentBase
{
	GENERATED_BODY()

public:
	/**
	 * @brief 定义角色的种族类型。
	 * 使用此组件的角色，必须要设置此变量，否则无法正确获取此种族的基础属性值和属性种类定义
	 * 该属性用于指定角色所属的种族，可以在编辑器中设置或在蓝图中读写。默认值为 None，表示未选择任何种族。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="角色种族")
	ELxCharacterRaceType CharacterRaceType = ELxCharacterRaceType::None;

	
	ULxCharacterAttributeComponent();

	/**
	 * @brief 初始化角色属性组件。
	 *
	 * 负责缓存所属角色并准备角色属性表的运行时数据。
	 */
	virtual void BaseComponentInitialize() override;

	/**
	 * @brief 根据属性 ID 获取可写的属性数据。
	 *
	 * @param InAttributeID 要查询的属性 ID。
	 * @return 若属性存在则返回对应属性数据指针，否则返回 nullptr。
	 */
	FLxAttributeData* GetCharacterAttributeByID(const ELxCharacterAttributeID InAttributeID);

	/**
	 * @brief 根据属性 ID 获取只读的属性数据。
	 *
	 * @param InAttributeID 要查询的属性 ID。
	 * @return 若属性存在则返回只读属性数据指针，否则返回 nullptr。
	 */
	const FLxAttributeData* GetCharacterAttributeByID(const ELxCharacterAttributeID InAttributeID) const;

	/**
	 * @brief 获取角色全部属性表的可写引用。
	 *
	 * @return 返回角色属性表指针，便于外部直接修改属性内容。
	 */
	TMap<ELxCharacterAttributeID, FLxAttributeData>* GetCharacterAttributeTable();

	/**
	 * @brief 获取角色全部属性表的只读引用。
	 *
	 * @return 返回角色属性表只读指针，用于遍历或查询属性。
	 */
	const TMap<ELxCharacterAttributeID, FLxAttributeData>* GetCharacterAttributeTable() const;

	/**
	 * @brief 设置指定属性的完整数据。
	 *
	 * @param InAttributeID 目标属性 ID。
	 * @param InAttributeData 要写入的新属性数据。
	 * @return 设置成功返回 true，属性不存在或写入失败返回 false。
	 */
	bool SetCharacterAttribute(const ELxCharacterAttributeID InAttributeID, const FLxAttributeData& InAttributeData);

	/**
	 * @brief 设置指定属性的当前值。
	 *
	 * @param InAttributeID 目标属性 ID。
	 * @param InNewValue 新的属性当前值。
	 * @return 设置成功返回 true，属性不存在或更新失败返回 false。
	 */
	bool SetCharacterAttributeCurrentValue(const ELxCharacterAttributeID InAttributeID, float InNewValue);

	bool RestoreCharacterAttributeCurrentValue(const ELxCharacterAttributeID InAttributeID, float InRestoreValue);

	

private:
	/**
	 * @brief 响应词条组件数据变化事件。
	 *
	 * 当已安装词条发生变化时，重新计算词条带来的属性加成，
	 * 并在计算完成后广播属性组件自身的数据更新事件。
	 */
	UFUNCTION()
	void HandleEntryDataChange();

	/**
	 * @brief 处理立即恢复属性词条的应用。
	 *
	 * 该函数在有立即恢复属性的词条被应用时调用，用于更新角色指定属性的当前值。
	 * 它会调用 `RestoreCharacterAttributeCurrentValue` 函数来实际执行属性值的恢复操作。
	 *
	 * @param InAttributeID 需要恢复的属性 ID。
	 * @param InRestoreValue 要恢复到该属性上的数值。
	 */
	UFUNCTION()
	void HandleInstantRestoreEntryApplied(ELxCharacterAttributeID InAttributeID, float InRestoreValue);

	/**
	 * @brief 处理词条包变更事件。
	 *
	 * 该函数在词条组件的词条包发生变化时被调用，用于更新角色属性相关的词条列表。
	 * 它接收一个 `FLxCharacterEntryPackage` 参数，该参数包含了最新的词条数据。
	 *
	 * @param InEntryPackage 包含最新词条数据的词条包。
	 */
	UFUNCTION()
	void HandleEntryPackageChanged(const FLxCharacterEntryPackage& InEntryPackage);

	/**
	 * @brief 根据当前所有已安装词条刷新角色最终属性。
	 *
	 * 该函数会先将所有属性的计算值还原为基础值，
	 * 然后遍历全部已安装词条并将其叠加到对应属性上。
	 */
	void RefreshCharacterAttributeByEntries();

	/**
	 * @brief 将单条词条应用到目标属性上。
	 *
	 * @param InOutAttributeData 待修改的角色属性数据。
	 * @param InEntryData 需要应用的词条数据。
	 */
	static void ApplyEntryToAttribute(FLxAttributeData& InOutAttributeData, const FLxItemEntryData& InEntryData);

	UPROPERTY()
	TObjectPtr<ALxBaseCharacter> m_pOwnerCharacter;

	UPROPERTY()
	TMap<ELxCharacterAttributeID, FLxAttributeData> m_mapCharacterAttributeTable;

	/** 由词条组件打包后下发的“作用于角色属性”的词条缓存。 */
	UPROPERTY()
	TArray<FLxItemEntryData> m_vCharacterAttributeEntries;

	bool m_bAttributeInitialized = false;
};
