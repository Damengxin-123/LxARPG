#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "LxSpecialAttributeTypes.generated.h"

/** 角色属性组件根据双方阵营标签计算出的关系。 */
UENUM(BlueprintType, DisplayName="角色阵营关系")
enum class ELxCharacterFactionRelation : uint8
{
	Friendly UMETA(DisplayName="友方"),
	Neutral UMETA(DisplayName="中立"),
	Hostile UMETA(DisplayName="敌方")
};

/** 角色阵营标签数据，用于描述当前角色认可的我方阵营和敌对阵营。 */
USTRUCT(BlueprintType, DisplayName="角色阵营标签数据")
struct LXARPG_API FLxCharacterFactionData
{
	GENERATED_BODY()

	/** 当前角色所属或认可为我方的阵营标签。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色|属性|阵营", DisplayName="我方标签")
	FGameplayTagContainer FriendlyTags;

	/** 当前角色会视为敌方的阵营标签。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色|属性|阵营", DisplayName="敌对标签")
	FGameplayTagContainer HostileTags;
};

/** 角色状态属性标签变化事件。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLxSpecialAttributeStateTagsChanged, FGameplayTag, StateCategoryTag, const FGameplayTagContainer&, StateTags);

/** 角色生命周期属性变化事件。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLxSpecialAttributeLifecycleChanged, bool, bIsAlive, FGameplayTag, LifecycleStateTag);
