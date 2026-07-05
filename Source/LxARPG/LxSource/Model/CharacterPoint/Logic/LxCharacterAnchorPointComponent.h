#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "LxCharacterAnchorPointComponent.generated.h"

/** 角色锚点用途类型，用于区分角色身上的固定功能点位。 */
UENUM(BlueprintType, DisplayName="角色锚点类型")
enum class ELxCharacterAnchorPointType : uint8
{
	/** 技能单元创建和更新时使用的释放起点。 */
	SkillRelease UMETA(DisplayName="技能释放锚点"),

	/** 光环效果后续生成和定位时使用的点位。 */
	AuraEffect UMETA(DisplayName="光环效果锚点")
};

/**
 * 可附加在角色组件树中的功能锚点。
 * 作为场景组件使用，可在角色蓝图视口中直接调整相对位置和旋转。
 */
UCLASS(BlueprintType, ClassGroup=(LxCharacter), meta=(BlueprintSpawnableComponent, DisplayName="角色锚点组件"))
class LXARPG_API ULxCharacterAnchorPointComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	/** 创建角色锚点组件。 */
	ULxCharacterAnchorPointComponent();

	/** 获取当前锚点的用途类型。 */
	UFUNCTION(BlueprintPure, Category="角色|锚点", DisplayName="获取锚点类型")
	ELxCharacterAnchorPointType GetAnchorPointType() const { return AnchorPointType; }

	/** 设置当前锚点的用途类型。 */
	UFUNCTION(BlueprintCallable, Category="角色|锚点", DisplayName="设置锚点类型")
	void SetAnchorPointType(ELxCharacterAnchorPointType InAnchorPointType) { AnchorPointType = InAnchorPointType; }

protected:
	/** 当前锚点承担的功能用途。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="角色|锚点", DisplayName="锚点类型")
	ELxCharacterAnchorPointType AnchorPointType = ELxCharacterAnchorPointType::SkillRelease;
};