#pragma once

#include "CoreMinimal.h"
#include "LxAICharacter.h"
#include "LxARPG/LxSource/UI/WorldSpace/TrainingDummy/LxTrainingDummyDamageRecord.h"
#include "LxTrainingDummyCharacter.generated.h"

class UMaterialInterface;
class UStaticMeshComponent;
class UWidgetComponent;
class ULxTrainingDummyDamageWidget;

/** 无自动行为的伤害测试木桩，使用正常怪物阵营、角色属性和最终承伤流程。 */
UCLASS(Blueprintable, DisplayName="伤害测试木桩", meta=(Category="角色|木桩"))
class LXARPG_API ALxTrainingDummyCharacter : public ALxAICharacter
{
	GENERATED_BODY()

public:
	/** 创建简化人形上半身、木桩立柱、底座和左侧场景界面。 */
	ALxTrainingDummyCharacter();

	/** 初始化高生命属性、怪物阵营和承伤记录，保持角色站定。 */
	virtual void InitialCharacterInformation() override;

	/** 在编辑器预览中应用蓝图配置的木材和金属材质。 */
	virtual void OnConstruction(const FTransform& Transform) override;

	/** 仅刷新场景界面朝向，不执行任何移动、攻击或AI行为。 */
	virtual void Tick(float DeltaSeconds) override;

	/** 解除最终承伤事件监听。 */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 注册最近十次承伤记录的网络同步字段。 */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** 获取最近十次实际承伤，最近的攻击位于索引零。 */
	UFUNCTION(BlueprintPure, Category="木桩|伤害测试", DisplayName="获取木桩最近伤害")
	const TArray<FLxTrainingDummyDamageRecord>& GetRecentDamageHistory() const { return RecentDamageHistory; }

	/** 在权威端清空伤害记录，以开始下一轮测试。 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="木桩|伤害测试", DisplayName="清空木桩伤害记录")
	void ClearDamageHistory();

public:
	/** 木质头部、躯干、手臂、立柱和底座共用的材质。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="木桩|外形", DisplayName="木材材质")
	TObjectPtr<UMaterialInterface> WoodMaterial;

	/** 底座箍环和胸部靶心共用的克制金属材质。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="木桩|外形", DisplayName="金属材质")
	TObjectPtr<UMaterialInterface> MetalMaterial;

	/** 实现十行布局的场景UI蓝图类型。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="木桩|场景UI", DisplayName="伤害列表界面类型")
	TSubclassOf<ULxTrainingDummyDamageWidget> DamageHistoryWidgetClass;

protected:
	/** 场景伤害列表距角色左手侧的水平距离，单位为米。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="木桩|场景UI", DisplayName="界面左侧距离（米）", meta=(ClampMin="0.50", Units="m", Delta="0.01"))
	float DamagePanelLeftOffsetMeters = 1.10f;

	/** 场景伤害列表相对角色中心的高度，单位为米。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="木桩|场景UI", DisplayName="界面高度偏移（米）", meta=(Units="m", Delta="0.01"))
	float DamagePanelHeightOffsetMeters = 0.30f;

	/** 挂在角色本地左侧的世界空间界面组件。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="木桩|场景UI", DisplayName="伤害列表场景组件")
	TObjectPtr<UWidgetComponent> DamageHistoryWidgetComponent;

	/** 接收已经通过护甲、护盾和生命值计算的最终实际承伤。 */
	UFUNCTION(Category="木桩|伤害测试", DisplayName="记录木桩实际承伤")
	void HandleTrainingDummyDamage(const FLxDamageReceiveResult& DamageReceiveResult, AActor* AttackerActor);

private:
	/** 应用模型材质、界面类型和以米配置的界面位置。 */
	void RefreshPresentation();

	/** 推送当前伤害历史到场景界面。 */
	void RefreshDamageWidget();

	/** 为缺少木桩配置表的环境补齐持久生命上限，保证属性重算后仍为999999。 */
	void InitializeTrainingAttributes();

	/** 客户端收到最新承伤历史后刷新自身场景界面。 */
	UFUNCTION(Category="木桩|场景UI|网络", DisplayName="同步木桩伤害记录")
	void OnRep_RecentDamageHistory();

	/** 木质外形组件集合，基础形状无需额外导入模型即可显示。 */
	UPROPERTY(VisibleAnywhere, Category="木桩|外形", DisplayName="木质外形组件")
	TArray<TObjectPtr<UStaticMeshComponent>> WoodMeshComponents;

	/** 金属靶心和箍环组件集合。 */
	UPROPERTY(VisibleAnywhere, Category="木桩|外形", DisplayName="金属外形组件")
	TArray<TObjectPtr<UStaticMeshComponent>> MetalMeshComponents;

	/** 最近十次承伤的复制数据，按最新在前排列。 */
	UPROPERTY(ReplicatedUsing=OnRep_RecentDamageHistory, BlueprintReadOnly, Category="木桩|伤害测试", DisplayName="最近十次伤害", meta=(AllowPrivateAccess="true"))
	TArray<FLxTrainingDummyDamageRecord> RecentDamageHistory;

	/** 当前测试轮内已收到的有效攻击序号。 */
	int32 ReceivedAttackCount = 0;

	/** 防止重复初始化时补满生命或重置测试进度。 */
	bool bTrainingDummyInitialized = false;
};
