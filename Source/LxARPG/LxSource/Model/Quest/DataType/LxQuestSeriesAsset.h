#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "LxQuestSeriesAsset.generated.h"

class UEdGraph;
class ULxQuestSeriesAsset;

/** 单个任务节点的静态配置，运行时通过任务ID标签从所属任务系列中查询。 */
USTRUCT(BlueprintType, DisplayName="任务节点定义")
struct LXARPG_API FLxQuestNodeDefinition
{
	GENERATED_BODY()

	/** 供NPC配置、运行时查询和进度存档使用的任务ID标签。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="任务|标识", DisplayName="任务ID", meta=(Categories="任务"))
	FGameplayTag QuestId;

#if WITH_EDITORONLY_DATA
	/** 仅用于稳定关联编辑器画布节点，策划和运行时都不需要使用。 */
	UPROPERTY()
	FGuid EditorNodeId;

	/** 兼容首版任务系列资产保存的旧节点ID，加载后会迁移到编辑器内部ID。 */
	UPROPERTY(meta=(DeprecatedProperty, DeprecationMessage="已由编辑器内部节点ID替代"))
	FGuid NodeId_DEPRECATED;
#endif

	/** 面向开发人员的简短名称，用于编辑器列表和调试输出。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="任务|标识", DisplayName="开发名称")
	FName DeveloperName = NAME_None;

	/** 玩家界面中显示的任务名称。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="任务|显示", DisplayName="可视化名称")
	FText DisplayName;

	/** 玩家界面中显示的任务描述。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="任务|显示", DisplayName="可视化描述", meta=(MultiLine="true"))
	FText DisplayDescription;

	/** 当前节点承载的任务内容，后续可替换为专用任务配置结构。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="任务|内容", DisplayName="任务内容", meta=(MultiLine="true"))
	FText QuestContent;

	/** 用于任务类型筛选、交互条件和运行时查询的标签集合。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="任务|标识", DisplayName="任务标签")
	FGameplayTagContainer QuestTags;
};

/** 两个任务节点之间从上级到下级的有向依赖关系。 */
USTRUCT(BlueprintType, DisplayName="任务节点关系")
struct LXARPG_API FLxQuestNodeLink
{
	GENERATED_BODY()

	/** 上级任务ID，对应关系图中的输出端。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="任务|关系", DisplayName="上级任务ID")
	FGameplayTag FromQuestId;

	/** 下级任务ID，对应关系图中的输入端。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="任务|关系", DisplayName="下级任务ID")
	FGameplayTag ToQuestId;

	/** 判断两条任务关系是否完全相同。 */
	bool operator==(const FLxQuestNodeLink& Other) const
	{
		return FromQuestId == Other.FromQuestId && ToQuestId == Other.ToQuestId;
	}
};

/** 全局任务系列索引表使用的行数据。 */
USTRUCT(BlueprintType, DisplayName="任务系列索引")
struct LXARPG_API FLxQuestSeriesRegistryRow : public FTableRowBase
{
	GENERATED_BODY()

	/** 对外查询任务系列资产时使用的标签ID。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="任务|系列", DisplayName="任务系列ID", meta=(Categories="任务"))
	FGameplayTag QuestSeriesId;

	/** 对应的完整任务系列资产，管理器可按需异步加载。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="任务|系列", DisplayName="任务系列资产")
	TSoftObjectPtr<ULxQuestSeriesAsset> QuestSeriesAsset;
};

/** 将同一系列的任务节点、关系和编辑器图作为整体保存的资产。 */
UCLASS(BlueprintType, meta=(DisplayName="任务系列资产"))
class LXARPG_API ULxQuestSeriesAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** 使用任务系列标签作为资产管理器中的稳定资产名称。 */
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	/** 根据任务ID标签复制取得任务节点静态配置。 */
	UFUNCTION(BlueprintPure, Category="任务|系列", DisplayName="获取任务节点")
	bool GetQuestNode(const FGameplayTag& QuestId, FLxQuestNodeDefinition& OutQuestNode) const;

	/** 判断当前任务系列是否包含指定任务ID。 */
	UFUNCTION(BlueprintPure, Category="任务|系列", DisplayName="是否包含任务节点")
	bool ContainsQuestNode(const FGameplayTag& QuestId) const;

	/** 判断指定任务ID是否为当前任务系列ID的严格子标签。 */
	UFUNCTION(BlueprintPure, Category="任务|系列", DisplayName="任务ID是否属于当前系列")
	bool IsQuestIdInSeries(const FGameplayTag& QuestId) const;

	/** 获取指定任务节点直接连接的全部下级任务ID。 */
	UFUNCTION(BlueprintPure, Category="任务|系列", DisplayName="获取直接下级任务节点")
	TArray<FGameplayTag> GetDirectSuccessorQuestIds(const FGameplayTag& QuestId) const;

	/** 获取指定任务节点直接依赖的全部上级任务ID。 */
	UFUNCTION(BlueprintPure, Category="任务|系列", DisplayName="获取直接上级任务节点")
	TArray<FGameplayTag> GetDirectPredecessorQuestIds(const FGameplayTag& QuestId) const;

	/** 返回任务节点的只读地址，仅允许在任务系列资产保持加载时短期使用。 */
	const FLxQuestNodeDefinition* FindQuestNode(const FGameplayTag& QuestId) const;

	/** 获取任务系列ID。 */
	const FGameplayTag& GetQuestSeriesId() const { return QuestSeriesId; }

	/** 获取当前系列包含的全部任务节点。 */
	const TArray<FLxQuestNodeDefinition>& GetQuestNodes() const { return QuestNodes; }

	/** 获取当前系列编译后的全部节点关系。 */
	const TArray<FLxQuestNodeLink>& GetQuestLinks() const { return QuestLinks; }

#if WITH_EDITOR
	/** 修复旧资产或复制节点产生的内部ID，返回是否修改了资产。 */
	bool RepairEditorData();

	/** 根据编辑器内部ID查找节点，保证修改任务标签时画布节点仍能稳定关联。 */
	const FLxQuestNodeDefinition* FindQuestNodeByEditorId(const FGuid& EditorNodeId) const;

	/** 判断当前任务系列是否包含指定编辑器内部节点ID。 */
	bool ContainsEditorNode(const FGuid& EditorNodeId) const;

	/** 设置任务系列ID，供专用资产编辑器在事务中修改。 */
	void SetQuestSeriesId(const FGameplayTag& InQuestSeriesId);

	/** 获取仅用于编辑器显示的任务关系图。 */
	UEdGraph* GetEditorGraph() const;

	/** 设置仅用于编辑器显示的任务关系图。 */
	void SetEditorGraph(UEdGraph* InEditorGraph);

	/** 编辑器属性变化后修复内部节点ID、重复任务标签和失效关系。 */
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	/** 编辑器工具可修改任务节点和关系，以保持表格与关系图同步。 */
	TArray<FLxQuestNodeDefinition>& EditQuestNodes() { return QuestNodes; }

	/** 编辑器工具可修改编译后的任务节点关系。 */
	TArray<FLxQuestNodeLink>& EditQuestLinks() { return QuestLinks; }

private:
	/** 对外索引整个任务系列数据资产时使用的标签ID。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="任务|系列", DisplayName="任务系列ID", meta=(AllowPrivateAccess="true", Categories="任务"))
	FGameplayTag QuestSeriesId;

	/** 当前系列包含的全部任务节点静态配置。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="任务|节点", DisplayName="任务节点列表", meta=(AllowPrivateAccess="true"))
	TArray<FLxQuestNodeDefinition> QuestNodes;

	/** 由编辑器关系图生成的运行时节点依赖关系。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="任务|关系", DisplayName="任务节点关系", meta=(AllowPrivateAccess="true"))
	TArray<FLxQuestNodeLink> QuestLinks;

#if WITH_EDITORONLY_DATA
	/** 仅用于保存节点位置和可视化连线，打包后不参与运行时查询。 */
	UPROPERTY()
	TObjectPtr<UEdGraph> EditorGraph;
#endif
};
