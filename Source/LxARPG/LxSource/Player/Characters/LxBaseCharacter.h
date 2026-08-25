#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "GameFramework/Character.h"
#include "LxCharacterStateEnum.h"
#include "LxARPG/LxSource/Model/Entry/DataType/LxItemEntryData.h"
#include "LxBaseCharacter.generated.h"

class UDataTable;
class ULxCharacterAnimationProcessComponent;
class ULxCharacterAttributeComponent;
class ULxCharacterAnchorPointComponent;
class ULxCharacterBackpackModule;
class ULxCharacterBuffModule;
class ULxCharacterCloseCombatModule;
class ULxCharacterCombatComponent;
class ULxCharacterContentComponent;
class ULxCharacterDataTransferComponent;
class ULxCharacterEffectComponent;
class ULxCharacterEffectProcessModule;
class ULxCharacterEffectCacheModule;
class ULxCharacterEffectTransferModule;
class ULxCharacterEquipmentModule;
class ULxCharacterLifecycleComponent;
class ULxCharacterProfessionModule;
class ULxCharacterTestComponent;
class ULxCharacterBehaviorControlComponent;
class ULxCharacterStateComponent;
class ULxCharacterSpecialAttributeComponent;
class ULxSkillBackpackModule;
class ULxSkillCastModule;

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

	/** 获取统一管理移动、跳跃、朝向和即时状态的角色行为控制组件。 */
	UFUNCTION(BlueprintCallable, Category="组件|角色行为", DisplayName="获取角色行为控制组件")
	ULxCharacterBehaviorControlComponent* GetCharacterBehaviorControlComponent() const { return m_pCharacterBehaviorControlComponent; }

	/** 获取角色动画处理组件。 */
	UFUNCTION(BlueprintCallable, Category="组件|角色动画", DisplayName="获取角色动画处理组件")
	ULxCharacterAnimationProcessComponent* GetCharacterAnimationProcessComponent() const { return m_pCharacterAnimationProcessComponent; }

	/** 获取角色属性组件。 */
	UFUNCTION(BlueprintCallable, Category="组件", DisplayName="获取角色属性组件")
	ULxCharacterAttributeComponent* GetCharacterAttributeComponent() const { return m_pCharacterAttributeComponent; }

	/** 获取统一角色属性组件；保留旧函数名兼容现有调用。 */
	UFUNCTION(BlueprintCallable, Category="组件|旧版兼容", DisplayName="获取角色特殊属性组件（兼容）", meta=(DeprecatedFunction, DeprecationMessage="请改用获取角色属性组件"))
	ULxCharacterAttributeComponent* GetCharacterSpecialAttributeComponent() const { return m_pCharacterAttributeComponent; }

	/** 获取角色背包组件。 */
	ULxCharacterBackpackModule* GetCharacterBackpackComponent() const;

	/** 获取角色 Buff 组件。 */
	ULxCharacterBuffModule* GetCharacterBuffComponent() const;

	/** 获取统一持有角色内容模块的角色内容组件。 */
	UFUNCTION(BlueprintCallable, Category="组件|角色内容", DisplayName="获取角色内容组件")
	ULxCharacterContentComponent* GetCharacterContentComponent() const { return m_pCharacterContentComponent; }

	/** 获取角色状态组件。 */
	UFUNCTION(BlueprintCallable, Category="组件|旧版兼容", DisplayName="获取旧版角色状态组件", meta=(DeprecatedFunction, DeprecationMessage="请改用获取角色特殊属性组件"))
	ULxCharacterStateComponent* GetCharacterStateComponent() const { return m_pCharacterStateComponent; }

	/** 获取角色数据中转组件。 */
	UFUNCTION(BlueprintCallable, Category="组件", DisplayName="获取角色数据中转组件")
	ULxCharacterDataTransferComponent* GetCharacterDataTransferComponent() const { return m_pCharacterDataTransferComponent; }

	/** 获取统一持有效果处理、缓存和传递模块的角色效果组件。 */
	UFUNCTION(BlueprintCallable, Category="组件|角色效果", DisplayName="获取角色效果组件")
	ULxCharacterEffectComponent* GetCharacterEffectComponent() const { return m_pCharacterEffectComponent; }

	/** 获取角色效果处理模块；保留旧函数名以兼容现有调用。 */
	UFUNCTION(BlueprintCallable, Category="组件|角色效果", DisplayName="获取角色效果处理模块")
	ULxCharacterEffectProcessModule* GetCharacterEffectProcessComponent() const;

	/** 获取角色效果缓存模块；保留旧函数名以兼容现有调用。 */
	UFUNCTION(BlueprintCallable, Category="组件|角色效果", DisplayName="获取角色效果缓存模块")
	ULxCharacterEffectCacheModule* GetCharacterEffectCacheComponent() const;

	/** 获取角色效果传递模块；保留旧函数名以兼容现有调用。 */
	UFUNCTION(BlueprintCallable, Category="组件|角色效果", DisplayName="获取角色效果传递模块")
	ULxCharacterEffectTransferModule* GetCharacterEffectTransferComponent() const;

	/** 获取角色生命周期组件。 */
	UFUNCTION(BlueprintCallable, Category="组件|旧版兼容", DisplayName="获取旧版角色生命周期组件", meta=(DeprecatedFunction, DeprecationMessage="请改用获取角色特殊属性组件"))
	ULxCharacterLifecycleComponent* GetCharacterLifecycleComponent() const { return m_pCharacterLifecycleComponent; }

	/** 获取角色装备组件。 */
	ULxCharacterEquipmentModule* GetCharacterEquipmentComponent() const;

	/** 获取统一持有技能释放和近身战斗模块的角色战斗组件。 */
	UFUNCTION(BlueprintCallable, Category="组件|角色战斗", DisplayName="获取角色战斗组件")
	ULxCharacterCombatComponent* GetCharacterCombatComponent() const { return m_pCharacterCombatComponent; }

	/** 获取技能释放模块；保留旧函数名兼容现有调用。 */
	UFUNCTION(BlueprintCallable, Category="组件|角色战斗", DisplayName="获取技能释放模块")
	ULxSkillCastModule* GetSkillCastComponent() const;

	/** 获取角色近身战斗模块；保留旧函数名兼容现有调用。 */
	UFUNCTION(BlueprintCallable, Category="组件|角色战斗", DisplayName="获取角色近身战斗模块")
	ULxCharacterCloseCombatModule* GetCharacterCloseCombatComponent() const;

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
	ULxSkillBackpackModule* GetSkillBackpackComponent() const;

	/** 获取角色职业组件。 */
	ULxCharacterProfessionModule* GetCharacterProfessionComponent() const;

	/** 获取角色测试组件。 */
	UFUNCTION(BlueprintCallable, Category="组件", DisplayName="获取角色测试组件")
	ULxCharacterTestComponent* GetCharacterTestComponent() const { return m_pCharacterTestComponent; }

	/** 获取角色属性配置表查询使用的唯一角色ID标签。 */
	UFUNCTION(BlueprintPure, Category="角色配置|基础属性", DisplayName="获取角色ID标签")
	FGameplayTag GetCharacterIDTag() const;

	/** 获取角色初始化时自动应用的默认词条配置。 */
	UFUNCTION(BlueprintPure, Category="角色配置|默认词条", DisplayName="获取默认词条配置")
	const TArray<FLxEntryQuote>& GetDefaultEntryConfig() const { return DefaultEntryConfig; }


	/** 查询角色初始化后缓存的命名文本。 */
	UFUNCTION(BlueprintPure, Category="角色|命名", DisplayName="查询角色命名文本")
	FText GetCharacterNamingText() const { return CharacterNamingText; }

protected:
	/** 角色属性配置表查询使用的唯一ID标签，由具体角色类型配置。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色配置|基础属性", DisplayName="角色ID标签", meta=(Categories="角色"))
	FGameplayTag CharacterIDTag;

	/** 当前角色使用的命名ID标签，初始化时会用它从名称数据表查询显示文本。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色配置|命名", DisplayName="名称ID标签", meta=(Categories="单位命名"))
	FGameplayTag CharacterNameIDTag;

	/** 当前角色使用的名称数据表，行结构使用 FLxCharacterNamingRow。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色配置|命名", DisplayName="名称数据表", meta=(RequiredAssetDataTags="RowStructure=/Script/LxARPG.LxCharacterNamingRow"))
	TObjectPtr<UDataTable> CharacterNamingTable = nullptr;

	/** 角色初始化时自动应用的词条引用列表，可混合配置属性、Buff、状态、技能和职业等词条。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色配置|默认词条", DisplayName="默认词条配置")
	TArray<FLxEntryQuote> DefaultEntryConfig;

	/** 角色行为控制组件，统一管理移动、跳跃、朝向、即时行为状态和动画运动信号。 */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="组件|角色行为", DisplayName="角色行为控制组件")
	TObjectPtr<ULxCharacterBehaviorControlComponent> m_pCharacterBehaviorControlComponent;

	/** 角色动画处理组件，用于将运动信号转换为动画播放信号。 */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="组件|角色动画", DisplayName="角色动画处理组件")
	TObjectPtr<ULxCharacterAnimationProcessComponent> m_pCharacterAnimationProcessComponent;

	/** 角色属性组件，用于维护角色属性表和属性词条缓存。 */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="组件", DisplayName="角色属性组件")
	TObjectPtr<ULxCharacterAttributeComponent> m_pCharacterAttributeComponent;

	/** 旧特殊属性组件成员名兼容别名，不会创建第二个挂载组件。 */
	UPROPERTY(Transient)
	TObjectPtr<ULxCharacterAttributeComponent> m_pCharacterSpecialAttributeComponent;

	/** 角色内容组件，统一持有背包、装备、技能背包、Buff 和职业模块。 */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="组件|角色内容", DisplayName="角色内容组件")
	TObjectPtr<ULxCharacterContentComponent> m_pCharacterContentComponent;

	/** 角色状态组件，用于按分类保存角色当前拥有的状态标签。 */
	UPROPERTY(Transient, BlueprintReadOnly, Category="组件|旧版兼容", DisplayName="旧版角色状态组件")
	TObjectPtr<ULxCharacterStateComponent> m_pCharacterStateComponent;

	/** 角色数据中转组件，用于统一转发属性、背包、装备、Buff 和效果数据。 */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="组件", DisplayName="角色数据中转组件")
	TObjectPtr<ULxCharacterDataTransferComponent> m_pCharacterDataTransferComponent;

	/** 角色效果组件，统一持有效果处理、缓存和传递模块。 */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="组件|角色效果", DisplayName="角色效果组件")
	TObjectPtr<ULxCharacterEffectComponent> m_pCharacterEffectComponent;

	/** 旧效果处理组件成员名兼容别名，仅供已保存的蓝图组件事件绑定查找，不会创建第二个挂载组件。 */
	UPROPERTY(Transient)
	TObjectPtr<ULxCharacterEffectComponent> m_pCharacterEffectProcessComponent;

	/** 角色生命周期组件，用于管理存活、死亡等生命周期状态。 */
	UPROPERTY(Transient, BlueprintReadOnly, Category="组件|旧版兼容", DisplayName="旧版角色生命周期组件")
	TObjectPtr<ULxCharacterLifecycleComponent> m_pCharacterLifecycleComponent;



	/** 角色战斗组件，统一持有技能释放和近身战斗模块。 */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="组件|角色战斗", DisplayName="角色战斗组件")
	TObjectPtr<ULxCharacterCombatComponent> m_pCharacterCombatComponent;

	/** 旧技能释放组件成员名兼容别名，不会创建第二个挂载组件。 */
	UPROPERTY(Transient)
	TObjectPtr<ULxCharacterCombatComponent> m_pSkillCastComponent;

	/** 旧近身战斗组件成员名兼容别名，不会创建第二个挂载组件。 */
	UPROPERTY(Transient)
	TObjectPtr<ULxCharacterCombatComponent> m_pCharacterCloseCombatComponent;

	/** 技能单元创建和持续更新时使用的释放点，可在角色蓝图视口中调整。 */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="组件|角色锚点", DisplayName="技能释放锚点")
	TObjectPtr<ULxCharacterAnchorPointComponent> m_pSkillReleaseAnchorPoint;

	/** 光环效果单元固定依附的角色点位，可在角色蓝图视口中调整。 */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="组件|角色锚点", DisplayName="光环效果锚点")
	TObjectPtr<ULxCharacterAnchorPointComponent> m_pAuraEffectAnchorPoint;

	/** 角色测试组件，用于集中提供物品、技能、职业等测试接口。 */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="组件", DisplayName="角色测试组件")
	TObjectPtr<ULxCharacterTestComponent> m_pCharacterTestComponent;

	/** 角色当前状态。 */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, ReplicatedUsing=OnRep_CharacterState, Category="角色|状态", DisplayName="当前角色状态")
	ELxCharacterState m_nCharacterState = ELxCharacterState::Idle;

		/** 初始化并缓存角色命名文本。 */
	void InitializeCharacterNamingText();

	/** 在服务端创建并应用当前角色配置的全部默认词条。 */
	void ApplyDefaultEntryConfig();
UFUNCTION()
	void OnRep_CharacterState();

	/** 初始化后缓存的角色命名文本，查询接口会直接返回该值。 */
	UPROPERTY(Transient)
	FText CharacterNamingText;
	/** 标记角色是否已经完成初始化，避免重复初始化。 */
	bool IsInitialized = false;
};
