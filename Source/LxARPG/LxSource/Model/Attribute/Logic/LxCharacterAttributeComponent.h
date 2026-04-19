#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxCharacterComponentBase.h"
#include "LxARPG/LxSource/Model/Attribute/DataType/LxAttributeData.h"
#include "LxCharacterAttributeComponent.generated.h"

class ALxBaseCharacter;
class ULxEquipmentLogic;
class ULxCharacterBackpackComponent;
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
	 * @brief 响应装备组件数据变化事件。
	 *
	 * 当角色装备数据发生变化时，重新计算装备词条带来的属性加成，
	 * 并在计算完成后广播属性组件自身的数据更新事件。
	 */
	UFUNCTION()
	void HandleEquipmentDataChange();

	UFUNCTION()
	void HandleBackpackInstantRestore(ELxCharacterAttributeID InAttributeID, float InRestoreValue);

	/**
	 * @brief 根据当前所有已装备物品刷新角色最终属性。
	 *
	 * 该函数会先将所有属性的计算值还原为基础值，
	 * 然后遍历全部装备词条并将其叠加到对应属性上。
	 */
	void RefreshCharacterAttributeByEquipment();

	/**
	 * @brief 获取角色当前已装备的全部装备逻辑对象。
	 *
	 * @param OutEquipmentList 输出参数，用于接收当前所有有效装备。
	 */
	void GetAllEquipmentList(TArray<ULxEquipmentLogic*>& OutEquipmentList) const;

	/**
	 * @brief 将单条装备词条应用到目标属性上。
	 *
	 * @param InOutAttributeData 待修改的角色属性数据。
	 * @param InEntryData 需要应用的装备词条数据。
	 */
	static void ApplyEquipmentEntryToAttribute(FLxAttributeData& InOutAttributeData, const FLxItemEntryData& InEntryData);

	UPROPERTY()
	TObjectPtr<ALxBaseCharacter> m_pOwnerCharacter;

	UPROPERTY()
	TMap<ELxCharacterAttributeID, FLxAttributeData> m_mapCharacterAttributeTable;

	bool m_bAttributeInitialized = false;
};
