#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LxInteractionData.h"
#include "LxInteractionTreeAsset.generated.h"

class UEdGraph;
class ULxInteractionNode;

/** 可复用的静态节点配置；运行时为每个NPC创建独立节点。 */
UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced, meta=(DisplayName="交互树节点配置"))
class LXARPG_API ULxInteractionTreeNodeData : public UObject
{
	GENERATED_BODY()
public:
	/** 节点类型由图菜单决定，避免修改后破坏功能唯一性。 */
	UPROPERTY(VisibleAnywhere, Category="节点", DisplayName="节点类型")
	ELxInteractionActionType Type = ELxInteractionActionType::Dialogue;
	/** 选项中显示的交互提示。 */
	UPROPERTY(EditAnywhere, Category="节点", DisplayName="交互提示文本", meta=(MultiLine=true))
	FText PromptText;
	/** 进入入口、对话或任务节点时显示的NPC发言；任务节点需关闭自动退出才能显示。 */
	UPROPERTY(EditAnywhere, Category="节点", DisplayName="NPC发言文本", meta=(MultiLine=true, EditCondition="Type == ELxInteractionActionType::Entrance || Type == ELxInteractionActionType::Dialogue || Type == ELxInteractionActionType::Quest", EditConditionHides))
	FText NpcDialogueText;
	/** 执行节点后关闭对话并结束交互；未勾选时继续显示子项。 */
	UPROPERTY(EditAnywhere, Category="节点", DisplayName="是否关闭交互对话框", meta=(EditCondition="Type != ELxInteractionActionType::Entrance && Type != ELxInteractionActionType::InteractionExit", EditConditionHides))
	bool bCloseInteractionDialogue = false;
	/** 与同级节点一起显示时的排序值，数值越小越靠前。 */
	UPROPERTY(EditAnywhere, Category="节点", DisplayName="选项顺序")
	int32 OptionOrder = 0;
	/** 节点可见性和执行前的共同限制。 */
	UPROPERTY(EditAnywhere, Category="节点", DisplayName="节点限制")
	FLxInteractionRequirement Requirement;
	/** 每个任务节点独立引用的任务，在右侧功能参数面板编辑。 */
	UPROPERTY(EditAnywhere, Category="节点功能", DisplayName="任务配置", meta=(EditCondition="Type == ELxInteractionActionType::Quest", EditConditionHides))
	FLxQuestInteractionConfig QuestConfig;
	/** 序列化时稳定识别节点，不作为编辑引脚。 */
	UPROPERTY()
	FGuid NodeId;
	/** 已编译的子节点标识，顺序稳定。 */
	UPROPERTY()
	TArray<FGuid> Children;
};

/** 交互树允许使用的功能及节点数量规则，内容由各NPC组件配置。 */
USTRUCT(BlueprintType, DisplayName="交互树功能配置")
struct LXARPG_API FLxInteractionTreeFeatures
{
	GENERATED_BODY()

	/** 是否允许放置并实例化宝箱功能。 */
	UPROPERTY(EditAnywhere, Category="功能", DisplayName="启用宝箱功能")
	bool bEnableTreasureChest = false;

	/** 是否允许放置并实例化仓库功能。 */
	UPROPERTY(EditAnywhere, Category="功能", DisplayName="启用仓库功能")
	bool bEnableWarehouse = false;

	/** 是否允许放置并实例化交易功能。 */
	UPROPERTY(EditAnywhere, Category="功能", DisplayName="启用交易功能")
	bool bEnableTradeContainer = false;

	/** 是否允许放置并实例化机关功能。 */
	UPROPERTY(EditAnywhere, Category="功能", DisplayName="启用机关功能")
	bool bEnableTriggerMechanism = false;

	/** 是否允许放置并实例化物品传递功能。 */
	UPROPERTY(EditAnywhere, Category="功能", DisplayName="启用物品传递功能")
	bool bEnableItemTransfer = false;

	/** 是否允许放置并实例化功能界面功能。 */
	UPROPERTY(EditAnywhere, Category="功能", DisplayName="启用功能界面功能")
	bool bEnableFunctionPage = false;

	/** 是否允许放置并实例化任务功能。 */
	UPROPERTY(EditAnywhere, Category="功能", DisplayName="启用任务功能")
	bool bEnableQuest = false;

	/** 默认每种功能只能放一个；显式加入此列表可允许多个，例如多个任务节点。 */
	UPROPERTY(EditAnywhere, Category="功能", DisplayName="允许多个节点的功能")
	TSet<ELxInteractionActionType> MultipleNodeTypes;

	/** 查询指定功能是否启用。 */
	bool IsEnabled(ELxInteractionActionType Type) const;
};

/** 交互流程与功能配置资产，不保存NPC或玩家的运行状态。 */
UCLASS(BlueprintType, meta=(DisplayName="交互树资产"))
class LXARPG_API ULxInteractionTreeAsset : public UDataAsset
{
	GENERATED_BODY()
public:
	/** 资产管理功能节点放置开关与数量规则。 */
	UPROPERTY(EditAnywhere, Category="交互树", DisplayName="功能配置", meta=(ShowOnlyInnerProperties))
	FLxInteractionTreeFeatures Features;
	/** 静态节点实例，由图形编辑器管理。 */
	UPROPERTY(Instanced)
	TArray<TObjectPtr<ULxInteractionTreeNodeData>> Nodes;
	/** 开始节点连接的入口列表。 */
	UPROPERTY()
	TArray<FGuid> Roots;
#if WITH_EDITORONLY_DATA
	/** 仅供编辑器保存布局与连线，打包时剥离。 */
	UPROPERTY()
	TObjectPtr<UEdGraph> EditorGraph;
#endif
	/** 根据稳定标识查找静态节点。 */
	ULxInteractionTreeNodeData* FindNode(const FGuid& Id) const;
	/** 判断是否为功能节点类型。 */
	static bool IsFunctionType(ELxInteractionActionType Type);
	/** 返回节点类型的简短中文名称。 */
	static FText GetTypeLabel(ELxInteractionActionType Type);
	/** 检查指定类型是否允许新增节点。 */
	bool CanAddNode(ELxInteractionActionType Type) const;
	/** 检查树结构、功能开关与数量，返回中文诊断。 */
	bool ValidateTree(FText& OutError) const;
	/** 创建归属于指定组件的独立运行时树；无效配置拒绝加载。 */
	bool CreateRuntimeTree(UObject* Outer, TArray<ULxInteractionNode*>& OutRoots, FText& OutError) const;
};
