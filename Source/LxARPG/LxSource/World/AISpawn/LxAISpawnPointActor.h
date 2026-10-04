#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "LxAISpawnPointActor.generated.h"

class ALxAICharacter;
class UArrowComponent;
class USceneComponent;
class USphereComponent;
class UTextRenderComponent;
class ULxAISpawnPointSaveComponent;
struct FLxAISpawnPointSaveRecord;

/** 按列表顺序参与概率判定的一种怪物，首领使用独立角色类。 */
USTRUCT(BlueprintType, meta=(DisplayName="刷怪类型配置"))
struct LXARPG_API FLxAISpawnMonsterType
{
	GENERATED_BODY()

	/** 普通怪物的 AI 角色类型；未配置或抽象类型不参与创建。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="刷怪类型", meta=(DisplayName="普通AI角色"))
	TSubclassOf<ALxAICharacter> NormalCharacterClass;

	/** 升级成功后使用的首领角色类；未配置时始终创建普通角色。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="刷怪类型", meta=(DisplayName="Boss体型AI角色"))
	TSubclassOf<ALxAICharacter> BossCharacterClass;

	/** 此项在前面的类型均未命中时参与判定，100 表示必定命中。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="刷怪类型", meta=(DisplayName="创建概率（%）", ClampMin="0.0", ClampMax="100.0"))
	float SpawnProbabilityPercent = 100.0f;

	/** 类型被选中后独立判定是否升级为首领，默认 5%。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="刷怪类型", meta=(DisplayName="Boss升级概率（%）", ClampMin="0.0", ClampMax="100.0"))
	float BossUpgradeProbabilityPercent = 5.0f;
};

/** 放置在场景中的固定刷怪点，负责数量维护、归属注入和按标签保存怪物组成。 */
UCLASS(BlueprintType, Blueprintable, ClassGroup=("AI"), meta=(DisplayName="固定刷怪点"))
class LXARPG_API ALxAISpawnPointActor : public AActor
{
	GENERATED_BODY()

public:
	/** 创建醒目的范围、箭头、文字标记和专用存档组件。 */
	ALxAISpawnPointActor();
	/** 场景参数变化后刷新标记，支持未选中时观察刷怪范围。 */
	virtual void OnConstruction(const FTransform& Transform) override;
	/** 优先恢复存档，再开始周期检查；客户端不创建怪物。 */
	virtual void BeginPlay() override;
	/** 缓存数量后停止定时器并清理本点创建的角色。 */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 获取用于存档精确查询的刷怪点标签。 */
	UFUNCTION(BlueprintPure, Category="刷怪点", meta=(DisplayName="获取刷怪点ID"))
	FGameplayTag GetSpawnPointId() const { return SpawnPointId; }
	/** 获取用于巡逻和逃跑的中心位置。 */
	UFUNCTION(BlueprintPure, Category="刷怪点", meta=(DisplayName="获取刷怪点中心"))
	FVector GetWorldCenter() const { return GetActorLocation(); }
	/** 获取不受 Actor 缩放影响的活动范围半径。 */
	UFUNCTION(BlueprintPure, Category="刷怪点", meta=(DisplayName="获取活动半径（厘米）"))
	float GetRangeRadiusCentimeters() const;
	/** 只计算仍存活且仍归属本点的怪物，不计死亡动画期间的尸体。 */
	UFUNCTION(BlueprintPure, Category="刷怪点", meta=(DisplayName="获取当前怪物数量"))
	int32 GetCurrentMonsterCount() const;
	/** 返回当前活怪列表，供蓝图调试与管理使用。 */
	UFUNCTION(BlueprintPure, Category="刷怪点", meta=(DisplayName="获取当前怪物"))
	TArray<ALxAICharacter*> GetLivingMonsters() const;
	/** 执行一次数量检查；低于下限补到下限，区间内以 50% 概率增加一只。 */
	UFUNCTION(BlueprintCallable, Category="刷怪点", meta=(DisplayName="检查并补充怪物"))
	void CheckPopulation();
	/** 只导出实际角色类和存活数量，不导出位置或具体角色属性。 */
	bool CapturePopulation(FLxAISpawnPointSaveRecord& OutRecord) const;
	/** 按存档中的实际类型重建角色；失败时回滚新角色并保留旧群体。 */
	bool RestorePopulation(const FLxAISpawnPointSaveRecord& Record);
	/** 计算本轮待补数量；显式随机值便于验证上下限与 50% 边界。 */
	static int32 CalculateSpawnCount(int32 CurrentCount, int32 BaseCount, int32 Variation, float ChanceRoll);
	/** 等价于逐项概率判定、整轮未命中则重试；合并空轮以避免低概率或全零概率死循环。 */
	static int32 SelectMonsterTypeIndex(const TArray<FLxAISpawnMonsterType>& Types, float ChanceRoll);

	/** 全场景唯一的稳定标签；更改此标签相当于使用另一份刷怪记录。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="刷怪点|标识", meta=(DisplayName="刷怪点标签ID", Categories="AI.刷怪点"))
	FGameplayTag SpawnPointId;
	/** 数量区间的中心值。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="刷怪点|数量", meta=(DisplayName="基准数量", ClampMin="0", ClampMax="4096"))
	int32 BaseMonsterCount = 5;
	/** 上下限分别为基准数减去和加上此值，下限不小于零。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="刷怪点|数量", meta=(DisplayName="数量波动范围", ClampMin="0", ClampMax="4096"))
	int32 MonsterCountVariation = 1;
	/** 每次检查之间的秒数；运行时修改后在下一轮调度时生效。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="刷怪点|数量", meta=(DisplayName="检查间隔（秒）", ClampMin="0.1", Units="s"))
	float CheckIntervalSeconds = 10.0f;
	/** 从上到下依次判定；靠前的 100% 项会阻止后续项出现。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="刷怪点|怪物", meta=(DisplayName="怪物类型列表", TitleProperty="NormalCharacterClass"))
	TArray<FLxAISpawnMonsterType> MonsterTypes;
	/** 怪物生成、刷怪点巡逻和返回刷怪点共用的范围，单位为米。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="刷怪点|范围", meta=(DisplayName="活动半径（米）", ClampMin="0.0", Units="m"))
	float RangeRadiusMeters = 10.0f;
	/** 开启后仅在可达导航区域生成；导航尚未准备好时留待下次检查。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="刷怪点|范围", meta=(DisplayName="在导航网格上生成"))
	bool bSpawnOnNavigation = true;
	/** 默认在运行时隐藏标记；关闭后保留范围、箭头和文字。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="刷怪点|标记", meta=(DisplayName="运行时隐藏标记"))
	bool bHideMarkerInGame = true;
	/** 标记颜色同时应用于范围、箭头和标题。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="刷怪点|标记", meta=(DisplayName="标记颜色"))
	FColor MarkerColor = FColor(255, 40, 140);
	/** 本点独立的存档组件，始终以刷怪点标签索引。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="刷怪点|存档", meta=(DisplayName="刷怪点存档组件"))
	TObjectPtr<ULxAISpawnPointSaveComponent> SaveComponent;

private:
	/** 刷新编辑态与运行态标记设置。 */
	void RefreshMarker();
	/** 一次性定时器每轮重新读取间隔。 */
	void ScheduleNextCheck();
	/** 创建单只角色，在构造脚本和 AI 启动前注入刷怪点。 */
	ALxAICharacter* SpawnMonster(TSubclassOf<ALxAICharacter> CharacterClass);
	/** 检查对象存活、世界与归属。 */
	bool IsLivingMember(const ALxAICharacter* Character) const;
	/** 删除本次事务创建或点位结束时的怪物以及对应控制器。 */
	void DestroyMonsters(const TArray<TWeakObjectPtr<ALxAICharacter>>& Characters);
	/** 不参与碰撞的根节点。 */
	UPROPERTY(VisibleAnywhere, Category="刷怪点|标记", meta=(DisplayName="场景根组件"))
	TObjectPtr<USceneComponent> SceneRoot;
	/** 不需要选中也能看到的球形边界。 */
	UPROPERTY(VisibleAnywhere, Category="刷怪点|标记", meta=(DisplayName="活动范围标记"))
	TObjectPtr<USphereComponent> RangeMarker;
	/** 指示点位朝向的醒目箭头。 */
	UPROPERTY(VisibleAnywhere, Category="刷怪点|标记", meta=(DisplayName="刷怪点箭头"))
	TObjectPtr<UArrowComponent> DirectionMarker;
	/** 显示点位用途和标签的场景文字。 */
	UPROPERTY(VisibleAnywhere, Category="刷怪点|标记", meta=(DisplayName="刷怪点文字"))
	TObjectPtr<UTextRenderComponent> LabelMarker;
	/** 由本点创建的角色弱引用，不延长已销毁角色生命周期。 */
	TArray<TWeakObjectPtr<ALxAICharacter>> SpawnedMonsters;
	/** 数量检查定时器句柄。 */
	FTimerHandle PopulationTimer;
	/** 防止蓝图初始化回调或读档递归触发另一轮创建。 */
	bool bUpdatingPopulation = false;
	/** 恢复或注册失败时禁止普通补怪覆盖旧存档组成。 */
	bool bPopulationReady = false;
	/** 已恢复的空记录也必须等到下一周期才能参与补怪。 */
	bool bRestoredPopulation = false;
};
