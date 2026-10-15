#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "LxShortcutSaveData.generated.h"

/** 快捷栏只索引角色已有物品，不额外创建物品实例。 */
UENUM(BlueprintType, DisplayName="快捷栏物品来源")
enum class ELxShortcutItemSource : uint8
{
	None UMETA(DisplayName="空快捷格"),
	Backpack UMETA(DisplayName="角色背包"),
	Equipment UMETA(DisplayName="穿戴装备"),
	SkillBackpack UMETA(DisplayName="技能背包")
};

/** 一个快捷格对应的来源槽位；技能通过稳定标签恢复。 */
USTRUCT(BlueprintType, DisplayName="快捷栏索引存档")
struct LXARPG_API FLxShortcutSaveRecord
{
	GENERATED_BODY()

	/** 物品所在的角色容器，空格使用无来源。 */
	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category="存档|快捷栏", DisplayName="物品来源")
	ELxShortcutItemSource Source = ELxShortcutItemSource::None;

	/** 背包或装备的来源槽位，保存时按实际物品引用重新查询。 */
	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category="存档|快捷栏", DisplayName="来源槽位索引")
	int32 SlotIndex = INDEX_NONE;

	/** 校验来源物品并索引技能，防止已失效的快捷格绑定其他物品。 */
	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category="存档|快捷栏", DisplayName="物品ID")
	FGameplayTag ItemIDTag;
};
