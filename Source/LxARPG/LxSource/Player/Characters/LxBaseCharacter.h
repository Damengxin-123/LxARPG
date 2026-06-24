#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "LxCharacterStateEnum.h"
#include "LxBaseCharacter.generated.h"

class UDataTable;
class ULxCharacterAttributeComponent;
class ULxCharacterBackpackComponent;
class ULxCharacterBuffComponent;
class ULxCharacterDataTransferComponent;
class ULxCharacterDamageComponent;
class ULxCharacterEquipmentComponent;
class ULxCharacterLifecycleComponent;
class ULxCharacterProfessionComponent;
class ULxCharacterTestComponent;
class ULxCharacterMoveComponent;
class ULxCharacterStateComponent;
class ULxSkillBackpackComponent;
class ULxSkillCastComponent;

/** 角色状态变化事件。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCharacterStateChange, const ELxCharacterState, State);

/**
 * 角色基础类。
 *
 * 负责持有角色移动、属性、背包、装备、Buff 和数据中转等核心组件，
 * 并提供初始化、状态切换和组件访问接口。
 */
UCLASS(Blueprintable, DisplayName="基础角色")
class LXARPG_API ALxBaseCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	/** 创建角色基础对象，并初始化默认组件。 */
	ALxBaseCharacter();

	/**
	 * 初始化角色运行时信息。
	 *
	 * 会初始化角色身上的核心组件，并同步初始状态数据。
	 */
	virtual void InitialCharacterInformation();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/**
	 * 设置角色当前状态。
	 *
	 * @param InState 要切换到的新角色状态。
	 */
	virtual void SetCharacterState(const ELxCharacterState InState);

	/**
	 * 获取角色当前状态。
	 *
	 * @return 当前角色状态枚举值。
	 */
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
	/**
	 * 角色每帧更新。
	 *
	 * @param DeltaTime 当前帧与上一帧之间的时间差。
	 */
	virtual void Tick(float DeltaTime) override;

	/**
	 * 绑定角色输入组件。
	 *
	 * @param PlayerInputComponent 当前角色可用的输入组件。
	 */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	/**
	 * 获取角色移动组件。
	 *
	 * @return 角色移动组件指针；未初始化时返回 nullptr。
	 */
	UFUNCTION(BlueprintCallable, Category="组件", DisplayName="获取角色移动组件")
	ULxCharacterMoveComponent* GetCharacterMoveComponent() const { return m_pCharacterMoveComponent; }

	/**
	 * 获取角色属性组件。
	 *
	 * @return 角色属性组件指针；未初始化时返回 nullptr。
	 */
	UFUNCTION(BlueprintCallable, Category="组件", DisplayName="获取角色属性组件")
	ULxCharacterAttributeComponent* GetCharacterAttributeComponent() const { return m_pCharacterAttributeComponent; }

	/**
	 * 获取角色背包组件。
	 *
	 * @return 角色背包组件指针；未初始化时返回 nullptr。
	 */
	UFUNCTION(BlueprintCallable, Category="组件", DisplayName="获取角色背包组件")
	ULxCharacterBackpackComponent* GetCharacterBackpackComponent() const { return m_pCharacterBackpackComponent; }

	/**
	 * 获取角色 Buff 组件。
	 *
	 * @return 角色 Buff 组件指针；未初始化时返回 nullptr。
	 */
	UFUNCTION(BlueprintCallable, Category="组件", DisplayName="获取角色Buff组件")
	ULxCharacterBuffComponent* GetCharacterBuffComponent() const { return m_pCharacterBuffComponent; }

	/**
	 * 获取角色状态组件。
	 *
	 * @return 角色状态组件指针；未初始化时返回 nullptr。
	 */
	UFUNCTION(BlueprintCallable, Category="组件", DisplayName="获取角色状态组件")
	ULxCharacterStateComponent* GetCharacterStateComponent() const { return m_pCharacterStateComponent; }

	/**
	 * 获取角色数据中转组件。
	 *
	 * @return 角色数据中转组件指针；未初始化时返回 nullptr。
	 */
	UFUNCTION(BlueprintCallable, Category="组件", DisplayName="获取角色数据中转组件")
	ULxCharacterDataTransferComponent* GetCharacterDataTransferComponent() const { return m_pCharacterDataTransferComponent; }

	/** 获取角色伤害计算组件。 */
	UFUNCTION(BlueprintCallable, Category="组件", DisplayName="获取角色伤害计算组件")
	ULxCharacterDamageComponent* GetCharacterDamageComponent() const { return m_pCharacterDamageComponent; }

	/** 获取角色生命周期组件。 */
	UFUNCTION(BlueprintCallable, Category="组件", DisplayName="获取角色生命周期组件")
	ULxCharacterLifecycleComponent* GetCharacterLifecycleComponent() const { return m_pCharacterLifecycleComponent; }

	/**
	 * 获取角色装备组件。
	 *
	 * @return 角色装备组件指针；未初始化时返回 nullptr。
	 */
	UFUNCTION(BlueprintCallable, Category="组件", DisplayName="获取角色装备组件")
	ULxCharacterEquipmentComponent* GetCharacterEquipmentComponent() const { return m_pCharacterEquipmentComponent; }

	/** 获取角色技能释放组件。 */
	UFUNCTION(BlueprintCallable, Category="组件", DisplayName="获取技能释放组件")
	ULxSkillCastComponent* GetSkillCastComponent() const { return m_pSkillCastComponent; }

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
protected:
	/** 当前角色使用的基础属性数值表，行结构使用 FLxAttributeValueConfig，所有基础角色派生类型都需要手动配置。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|属性", DisplayName="基础属性数值表", meta=(RequiredAssetDataTags="RowStructure=/Script/LxARPG.LxAttributeValueConfig"))
	TObjectPtr<UDataTable> CharacterAttributeValueTable = nullptr;

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

	/** 角色数据中转组件，用于统一对外转发属性、背包、装备和 Buff 数据。 */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="组件", DisplayName="角色数据中转组件")
	TObjectPtr<ULxCharacterDataTransferComponent> m_pCharacterDataTransferComponent;

	/** 角色伤害计算组件，用于处理伤害输出和伤害接收流程。 */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="组件", DisplayName="角色伤害计算组件")
	TObjectPtr<ULxCharacterDamageComponent> m_pCharacterDamageComponent;

	/** 角色生命周期组件，用于管理存活、死亡等生命周期状态。 */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="组件", DisplayName="角色生命周期组件")
	TObjectPtr<ULxCharacterLifecycleComponent> m_pCharacterLifecycleComponent;

	/** 角色装备组件，用于管理装备槽位和已装备物品。 */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="组件", DisplayName="角色装备组件")
	TObjectPtr<ULxCharacterEquipmentComponent> m_pCharacterEquipmentComponent;

	/** 技能释放组件，用于统一处理玩家和 AI 的技能释放、蓄力和释放上下文。 */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="组件", DisplayName="技能释放组件")
	TObjectPtr<ULxSkillCastComponent> m_pSkillCastComponent;

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

	UFUNCTION()
	void OnRep_CharacterState();

	/** 标记角色是否已经完成初始化，避免重复初始化。 */
	bool IsInitialized = false;
};
