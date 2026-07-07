#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "GameFramework/Character.h"
#include "LxCharacterStateEnum.h"
#include "LxBaseCharacter.generated.h"

class UDataTable;
class ULxCharacterAttributeComponent;
class ULxCharacterAnchorPointComponent;
class ULxCharacterBackpackComponent;
class ULxCharacterBuffComponent;
class ULxCharacterCloseCombatComponent;
class ULxCharacterDataTransferComponent;
class ULxCharacterEffectProcessComponent;
class ULxCharacterEffectCacheComponent;
class ULxCharacterEffectTransferComponent;
class ULxCharacterEquipmentComponent;
class ULxCharacterLifecycleComponent;
class ULxCharacterProfessionComponent;
class ULxCharacterTestComponent;
class ULxCharacterMoveComponent;
class ULxCharacterStateComponent;
class ULxSkillBackpackComponent;
class ULxSkillCastComponent;

/** 角色状态变化事件。 */
/** 角色命名数据表行，用于通过单位命名标签查询角色显示名称。 */
USTRUCT(BlueprintType, DisplayName="角色命名表行")
struct LXARPG_API FLxCharacterNamingRow : public FTableRowBase
{
	GENERATED_BODY()

public:
	/** 命名ID标签，用于匹配角色命名配置。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色命名", DisplayName="命名ID标签", meta=(Categories="单位命名"))
	FGameplayTag NamingIDTag;

	/** 查询到角色命名后实际用于显示的文本。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色命名", DisplayName="命名文本")
	FText NamingText;
};
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCharacterStateChange, const ELxCharacterState, State);

/**
 * 角色基础类。
 * 负责持有角色移动、属性、背包、装备、Buff、效果处理、效果传递和数据中转等核心组件。
 */
UCLASS(Blueprintable, DisplayName="基础角色")
class LXARPG_API ALxBaseCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	/** 创建角色基础对象，并初始化默认组件。 */
	ALxBaseCharacter();

	/** 初始化角色运行时信息。 */
	virtual void InitialCharacterInformation();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** 设置角色当前状态。 */
	virtual void SetCharacterState(const ELxCharacterState InState);

	/** 获取角色当前状态。 */
	virtual const ELxCharacterState GetCurrentState();

	UFUNCTION(Server, Reliable)
	void ServerSetCharacterState(ELxCharacterState InState);

	UFUNCTION(Server, Unreliable)
	void ServerSetCharacterRotation(FRotator InRotation);

	/** 角色状态变化事件。 */
	UPROPERTY(BlueprintAssignable, Category="角色|状态", DisplayName="角色状态变化事件")
	FOnCharacterStateChange OnCharacterStateChange;

protected:
	/** 角色进入游戏时触发。 */
	virtual void BeginPlay() override;

public:
	/** 角色每帧更新。 */
	virtual void Tick(float DeltaTime) override;

	/** 绑定角色输入组件。 */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	/** 获取角色移动组件。 */
	UFUNCTION(BlueprintCallable, Category="组件", DisplayName="获取角色移动组件")
	ULxCharacterMoveComponent* GetCharacterMoveComponent() const { return m_pCharacterMoveComponent; }

	/** 获取角色属性组件。 */
	UFUNCTION(BlueprintCallable, Category="组件", DisplayName="获取角色属性组件")
	ULxCharacterAttributeComponent* GetCharacterAttributeComponent() const { return m_pCharacterAttributeComponent; }

	/** 获取角色背包组件。 */
	UFUNCTION(BlueprintCallable, Category="组件", DisplayName="获取角色背包组件")
	ULxCharacterBackpackComponent* GetCharacterBackpackComponent() const { return m_pCharacterBackpackComponent; }

	/** 获取角色 Buff 组件。 */
	UFUNCTION(BlueprintCallable, Category="组件", DisplayName="获取角色Buff组件")
	ULxCharacterBuffComponent* GetCharacterBuffComponent() const { return m_pCharacterBuffComponent; }

	/** 获取角色状态组件。 */
	UFUNCTION(BlueprintCallable, Category="组件", DisplayName="获取角色状态组件")
	ULxCharacterStateComponent* GetCharacterStateComponent() const { return m_pCharacterStateComponent; }

	/** 获取角色数据中转组件。 */
	UFUNCTION(BlueprintCallable, Category="组件", DisplayName="获取角色数据中转组件")
	ULxCharacterDataTransferComponent* GetCharacterDataTransferComponent() const { return m_pCharacterDataTransferComponent; }

	/** 获取角色效果处理组件。 */
	UFUNCTION(BlueprintCallable, Category="组件", DisplayName="获取角色效果处理组件")
	ULxCharacterEffectProcessComponent* GetCharacterEffectProcessComponent() const { return m_pCharacterEffectProcessComponent; }

	/** 获取角色效果缓存组件。 */
	UFUNCTION(BlueprintCallable, Category="组件", DisplayName="获取角色效果缓存组件")
	ULxCharacterEffectCacheComponent* GetCharacterEffectCacheComponent() const { return m_pCharacterEffectCacheComponent; }

	/** 获取角色效果传递组件。 */
	UFUNCTION(BlueprintCallable, Category="组件", DisplayName="获取角色效果传递组件")
	ULxCharacterEffectTransferComponent* GetCharacterEffectTransferComponent() const { return m_pCharacterEffectTransferComponent; }

	/** 获取角色生命周期组件。 */
	UFUNCTION(BlueprintCallable, Category="组件", DisplayName="获取角色生命周期组件")
	ULxCharacterLifecycleComponent* GetCharacterLifecycleComponent() const { return m_pCharacterLifecycleComponent; }

	/** 获取角色装备组件。 */
	UFUNCTION(BlueprintCallable, Category="组件", DisplayName="获取角色装备组件")
	ULxCharacterEquipmentComponent* GetCharacterEquipmentComponent() const { return m_pCharacterEquipmentComponent; }

	/** 获取角色技能释放组件。 */
	UFUNCTION(BlueprintCallable, Category="组件", DisplayName="获取技能释放组件")
	ULxSkillCastComponent* GetSkillCastComponent() const { return m_pSkillCastComponent; }

	/** 获取角色近身战斗组件。 */
	UFUNCTION(BlueprintCallable, Category="组件", DisplayName="获取角色近身战斗组件")
	ULxCharacterCloseCombatComponent* GetCharacterCloseCombatComponent() const { return m_pCharacterCloseCombatComponent; }

	/** 获取角色蓝图中可调整的技能释放锚点。 */
	UFUNCTION(BlueprintPure, Category="角色|锚点", DisplayName="获取技能释放锚点")
	ULxCharacterAnchorPointComponent* GetSkillReleaseAnchorPoint() const { return m_pSkillReleaseAnchorPoint; }

	/** 获取角色蓝图中可调整的光环效果锚点。 */
	UFUNCTION(BlueprintPure, Category="角色|锚点", DisplayName="获取光环效果锚点")
	ULxCharacterAnchorPointComponent* GetAuraEffectAnchorPoint() const { return m_pAuraEffectAnchorPoint; }

	/** 获取当前技能释放锚点的世界变换。 */
	UFUNCTION(BlueprintPure, Category="角色|锚点", DisplayName="获取技能释放锚点变换")
	FTransform GetSkillReleaseAnchorTransform() const;

	/** 获取角色技能背包组件。 */
	UFUNCTION(BlueprintCallable, Category="组件", DisplayName="获取技能背包组件")
	ULxSkillBackpackComponent* GetSkillBackpackComponent() const { return m_pSkillBackpackComponent; }

	/** 获取角色职业组件。 */
	UFUNCTION(BlueprintCallable, Category="组件", DisplayName="获取角色职业组件")
	ULxCharacterProfessionComponent* GetCharacterProfessionComponent() const { return m_pCharacterProfessionComponent; }

	/** 获取角色测试组件。 */
	UFUNCTION(BlueprintCallable, Category="组件", DisplayName="获取角色测试组件")
	ULxCharacterTestComponent* GetCharacterTestComponent() const { return m_pCharacterTestComponent; }

	/** 获取当前角色使用的基础属性数值表。 */
	UFUNCTION(BlueprintPure, Category="角色|属性", DisplayName="获取基础属性数值表")
	UDataTable* GetCharacterAttributeValueTable() const { return CharacterAttributeValueTable; }


	/** 查询角色初始化后缓存的命名文本。 */
	UFUNCTION(BlueprintPure, Category="角色|命名", DisplayName="查询角色命名文本")
	FText GetCharacterNamingText() const { return CharacterNamingText; }
protected:
	/** 当前角色使用的基础属性数值表，行结构使用 FLxAttributeValueConfig。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|属性", DisplayName="基础属性数值表", meta=(RequiredAssetDataTags="RowStructure=/Script/LxARPG.LxAttributeValueConfig"))
	TObjectPtr<UDataTable> CharacterAttributeValueTable = nullptr;

	
	/** 当前角色使用的命名ID标签，初始化时会用它从名称数据表查询显示文本。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|命名", DisplayName="名称ID标签", meta=(Categories="单位命名"))
	FGameplayTag CharacterNameIDTag;

	/** 当前角色使用的名称数据表，行结构使用 FLxCharacterNamingRow。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|命名", DisplayName="名称数据表", meta=(RequiredAssetDataTags="RowStructure=/Script/LxARPG.LxCharacterNamingRow"))
	TObjectPtr<UDataTable> CharacterNamingTable = nullptr;
/** 角色移动组件，用于管理和控制角色移动行为。 */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="组件", DisplayName="角色移动组件")
	TObjectPtr<ULxCharacterMoveComponent> m_pCharacterMoveComponent;

	/** 角色属性组件，用于维护角色属性表和属性词条缓存。 */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="组件", DisplayName="角色属性组件")
	TObjectPtr<ULxCharacterAttributeComponent> m_pCharacterAttributeComponent;

	/** 角色背包组件，用于管理背包槽位和背包物品。 */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="组件", DisplayName="角色背包组件")
	TObjectPtr<ULxCharacterBackpackComponent> m_pCharacterBackpackComponent;

	/** 角色 Buff 组件，用于管理运行时 Buff。 */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="组件", DisplayName="角色Buff组件")
	TObjectPtr<ULxCharacterBuffComponent> m_pCharacterBuffComponent;

	/** 角色状态组件，用于按分类保存角色当前拥有的状态标签。 */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="组件", DisplayName="角色状态组件")
	TObjectPtr<ULxCharacterStateComponent> m_pCharacterStateComponent;

	/** 角色数据中转组件，用于统一转发属性、背包、装备、Buff 和效果数据。 */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="组件", DisplayName="角色数据中转组件")
	TObjectPtr<ULxCharacterDataTransferComponent> m_pCharacterDataTransferComponent;

	/** 角色效果处理组件，用于解析词条、计算伤害并生成最终效果包。 */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="组件", DisplayName="角色效果处理组件")
	TObjectPtr<ULxCharacterEffectProcessComponent> m_pCharacterEffectProcessComponent;

	/** 角色效果缓存组件，用于缓存可撤回的持续效果包并刷新已接入的角色增益模块。 */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="组件", DisplayName="角色效果缓存组件")
	TObjectPtr<ULxCharacterEffectCacheComponent> m_pCharacterEffectCacheComponent;

	/** 角色效果传递组件，用于处理角色之间的效果包发送和接收。 */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="组件", DisplayName="角色效果传递组件")
	TObjectPtr<ULxCharacterEffectTransferComponent> m_pCharacterEffectTransferComponent;

	/** 角色生命周期组件，用于管理存活、死亡等生命周期状态。 */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="组件", DisplayName="角色生命周期组件")
	TObjectPtr<ULxCharacterLifecycleComponent> m_pCharacterLifecycleComponent;

	/** 角色装备组件，用于管理装备槽位和已装备物品。 */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="组件", DisplayName="角色装备组件")
	TObjectPtr<ULxCharacterEquipmentComponent> m_pCharacterEquipmentComponent;

	/** 技能释放组件，用于统一处理玩家和 AI 的技能释放、蓄力和释放上下文。 */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="组件", DisplayName="技能释放组件")
	TObjectPtr<ULxSkillCastComponent> m_pSkillCastComponent;

	/** 角色近身战斗组件，用于执行近战攻击、格挡及武器盾牌碰撞判定。 */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="组件", DisplayName="角色近身战斗组件")
	TObjectPtr<ULxCharacterCloseCombatComponent> m_pCharacterCloseCombatComponent;

	/** 技能单元创建和持续更新时使用的释放点，可在角色蓝图视口中调整。 */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="组件|角色锚点", DisplayName="技能释放锚点")
	TObjectPtr<ULxCharacterAnchorPointComponent> m_pSkillReleaseAnchorPoint;

	/** 光环效果单元固定依附的角色点位，可在角色蓝图视口中调整。 */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="组件|角色锚点", DisplayName="光环效果锚点")
	TObjectPtr<ULxCharacterAnchorPointComponent> m_pAuraEffectAnchorPoint;

	/** 技能背包组件，用于保存角色已拥有的技能物品和技能背包槽位。 */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="组件", DisplayName="技能背包组件")
	TObjectPtr<ULxSkillBackpackComponent> m_pSkillBackpackComponent;

	/** 角色职业组件，用于管理角色已学习职业、职业等级经验和职业效果。 */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="组件", DisplayName="角色职业组件")
	TObjectPtr<ULxCharacterProfessionComponent> m_pCharacterProfessionComponent;

	/** 角色测试组件，用于集中提供物品、技能、职业等测试接口。 */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="组件", DisplayName="角色测试组件")
	TObjectPtr<ULxCharacterTestComponent> m_pCharacterTestComponent;

	/** 角色当前状态。 */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, ReplicatedUsing=OnRep_CharacterState, Category="角色|状态", DisplayName="当前角色状态")
	ELxCharacterState m_nCharacterState = ELxCharacterState::Idle;

		/** 初始化并缓存角色命名文本。 */
	void InitializeCharacterNamingText();
UFUNCTION()
	void OnRep_CharacterState();

	/** 初始化后缓存的角色命名文本，查询接口会直接返回该值。 */
	UPROPERTY(Transient)
	FText CharacterNamingText;
	/** 标记角色是否已经完成初始化，避免重复初始化。 */
	bool IsInitialized = false;
};
