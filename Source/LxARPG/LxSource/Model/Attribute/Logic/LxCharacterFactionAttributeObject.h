#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Model/Attribute/DataType/LxSpecialAttributeTypes.h"
#include "LxCharacterSpecialAttributeObject.h"
#include "LxCharacterFactionAttributeObject.generated.h"

/** 角色阵营属性对象，保存阵营标签并计算角色关系。 */
UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced, DisplayName="角色阵营属性")
class LXARPG_API ULxCharacterFactionAttributeObject : public ULxCharacterSpecialAttributeObject
{
	GENERATED_BODY()

public:
	/** 注册阵营数据网络复制字段。 */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** 根据目标阵营标签判断阵营关系。 */
	ELxCharacterFactionRelation GetFactionRelation(const FGameplayTagContainer& InTargetFactionTags) const;

	/** 获取当前角色的我方阵营标签。 */
	const FGameplayTagContainer& GetFriendlyTags() const { return CharacterFaction.FriendlyTags; }

	/** 判断当前是否尚未配置任何我方或敌对阵营标签。 */
	bool IsFactionConfigurationEmpty() const
	{
		return CharacterFaction.FriendlyTags.IsEmpty() && CharacterFaction.HostileTags.IsEmpty();
	}

	/** 设置完整角色阵营数据，供角色类型写入合理的默认阵营。 */
	UFUNCTION(BlueprintCallable, Category="角色|属性|阵营", DisplayName="设置角色阵营")
	void SetCharacterFactionData(const FLxCharacterFactionData& InCharacterFaction);

private:
	/** 当前角色用于判断友方、敌对方和中立方的阵营标签配置。 */
	UPROPERTY(EditAnywhere, Replicated, Category="角色|属性|阵营", DisplayName="角色阵营")
	FLxCharacterFactionData CharacterFaction;
};
