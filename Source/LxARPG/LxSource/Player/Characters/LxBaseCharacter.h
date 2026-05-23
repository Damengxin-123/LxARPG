#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "LxCharacterStateEnum.h"
#include "LxBaseCharacter.generated.h"

class ULxCharacterAttributeComponent;
class ULxCharacterBackpackComponent;
class ULxCharacterBuffComponent;
class ULxCharacterDataTransferComponent;
class ULxCharacterEquipmentComponent;
class ULxCharacterMoveComponent;

/** 瑙掕壊鐘舵€佸彉鍖栦簨浠躲€?*/
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCharacterStateChange, const ELxCharacterState, State);

/**
 * 瑙掕壊鍩虹绫汇€? *
 * 璐熻矗鎸佹湁瑙掕壊绉诲姩銆佸睘鎬с€佽儗鍖呫€佽澶囥€丅uff 鍜屾暟鎹腑杞瓑鏍稿績缁勪欢锛? * 骞舵彁渚涘垵濮嬪寲銆佺姸鎬佸垏鎹㈠拰缁勪欢璁块棶鎺ュ彛銆? */
UCLASS(Blueprintable)
class LXARPG_API ALxBaseCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	/** 鍒涘缓瑙掕壊鍩虹瀵硅薄锛屽苟鍒濆鍖栭粯璁ょ粍浠躲€?*/
	ALxBaseCharacter();

	/**
	 * 鍒濆鍖栬鑹茶繍琛屾椂淇℃伅銆?	 *
	 * 浼氬垵濮嬪寲瑙掕壊韬笂鐨勬牳蹇冪粍浠讹紝骞跺悓姝ュ垵濮嬬姸鎬佹暟鎹€?	 */
	virtual void InitialCharacterInformation();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/**
	 * 璁剧疆瑙掕壊褰撳墠鐘舵€併€?	 *
	 * @param InState 瑕佸垏鎹㈠埌鐨勬柊瑙掕壊鐘舵€併€?	 */
	virtual void SetCharacterState(const ELxCharacterState InState);

	/**
	 * 鑾峰彇瑙掕壊褰撳墠鐘舵€併€?	 *
	 * @return 褰撳墠瑙掕壊鐘舵€佹灇涓惧€笺€?	 */
	virtual const ELxCharacterState GetCurrentState();

	UFUNCTION(Server, Reliable)
	void ServerSetCharacterState(ELxCharacterState InState);

	UFUNCTION(Server, Unreliable)
	void ServerSetCharacterRotation(FRotator InRotation);

	/** 瑙掕壊鐘舵€佸彉鍖栦簨浠躲€?*/
	UPROPERTY(BlueprintAssignable)
	FOnCharacterStateChange OnCharacterStateChange;

protected:
	/** 瑙掕壊杩涘叆娓告垙鏃惰Е鍙戙€?*/
	virtual void BeginPlay() override;

public:
	/**
	 * 瑙掕壊姣忓抚鏇存柊銆?	 *
	 * @param DeltaTime 褰撳墠甯т笌涓婁竴甯т箣闂寸殑鏃堕棿宸€?	 */
	virtual void Tick(float DeltaTime) override;

	/**
	 * 缁戝畾瑙掕壊杈撳叆缁勪欢銆?	 *
	 * @param PlayerInputComponent 褰撳墠瑙掕壊鍙敤鐨勮緭鍏ョ粍浠躲€?	 */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	/**
	 * 鑾峰彇瑙掕壊绉诲姩缁勪欢銆?	 *
	 * @return 瑙掕壊绉诲姩缁勪欢鎸囬拡锛涙湭鍒濆鍖栨椂杩斿洖 nullptr銆?	 */
	UFUNCTION(BlueprintCallable, Category="组件", DisplayName="获取角色移动组件")
	ULxCharacterMoveComponent* GetCharacterMoveComponent() const { return m_pCharacterMoveComponent; }

	/**
	 * 鑾峰彇瑙掕壊灞炴€х粍浠躲€?	 *
	 * @return 瑙掕壊灞炴€х粍浠舵寚閽堬紱鏈垵濮嬪寲鏃惰繑鍥?nullptr銆?	 */
	UFUNCTION(BlueprintCallable, Category="组件", DisplayName="获取角色属性组件")
	ULxCharacterAttributeComponent* GetCharacterAttributeComponent() const { return m_pCharacterAttributeComponent; }

	/**
	 * 鑾峰彇瑙掕壊鑳屽寘缁勪欢銆?	 *
	 * @return 瑙掕壊鑳屽寘缁勪欢鎸囬拡锛涙湭鍒濆鍖栨椂杩斿洖 nullptr銆?	 */
	UFUNCTION(BlueprintCallable, Category="组件", DisplayName="获取角色背包组件")
	ULxCharacterBackpackComponent* GetCharacterBackpackComponent() const { return m_pCharacterBackpackComponent; }

	/**
	 * 鑾峰彇瑙掕壊 Buff 缁勪欢銆?	 *
	 * @return 瑙掕壊 Buff 缁勪欢鎸囬拡锛涙湭鍒濆鍖栨椂杩斿洖 nullptr銆?	 */
	UFUNCTION(BlueprintCallable, Category="组件", DisplayName="获取角色Buff组件")
	ULxCharacterBuffComponent* GetCharacterBuffComponent() const { return m_pCharacterBuffComponent; }

	/**
	 * 鑾峰彇瑙掕壊鏁版嵁涓浆缁勪欢銆?	 *
	 * @return 瑙掕壊鏁版嵁涓浆缁勪欢鎸囬拡锛涙湭鍒濆鍖栨椂杩斿洖 nullptr銆?	 */
	UFUNCTION(BlueprintCallable, Category="组件", DisplayName="获取角色数据中转组件")
	ULxCharacterDataTransferComponent* GetCharacterDataTransferComponent() const { return m_pCharacterDataTransferComponent; }

	/**
	 * 鑾峰彇瑙掕壊瑁呭缁勪欢銆?	 *
	 * @return 瑙掕壊瑁呭缁勪欢鎸囬拡锛涙湭鍒濆鍖栨椂杩斿洖 nullptr銆?	 */
	UFUNCTION(BlueprintCallable, Category="组件", DisplayName="获取角色装备组件")
	ULxCharacterEquipmentComponent* GetCharacterEquipmentComponent() const { return m_pCharacterEquipmentComponent; }


protected:
	/** 瑙掕壊绉诲姩缁勪欢锛岀敤浜庣鐞嗗拰鎺у埗瑙掕壊绉诲姩琛屼负銆?*/
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="组件", DisplayName="角色移动组件")
	TObjectPtr<ULxCharacterMoveComponent> m_pCharacterMoveComponent;

	/** 瑙掕壊灞炴€х粍浠讹紝鐢ㄤ簬缁存姢瑙掕壊灞炴€ц〃鍜屽睘鎬ц瘝鏉＄紦瀛樸€?*/
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="组件", DisplayName="角色属性组件")
	TObjectPtr<ULxCharacterAttributeComponent> m_pCharacterAttributeComponent;

	/** 瑙掕壊鑳屽寘缁勪欢锛岀敤浜庣鐞嗚儗鍖呮Ы浣嶅拰鑳屽寘鐗╁搧銆?*/
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="组件", DisplayName="角色背包组件")
	TObjectPtr<ULxCharacterBackpackComponent> m_pCharacterBackpackComponent;

	/** 瑙掕壊 Buff 缁勪欢锛岀敤浜庣鐞嗚繍琛屾椂 Buff銆?*/
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="组件", DisplayName="角色Buff组件")
	TObjectPtr<ULxCharacterBuffComponent> m_pCharacterBuffComponent;

	/** 瑙掕壊鏁版嵁涓浆缁勪欢锛岀敤浜庣粺涓€瀵瑰杞彂灞炴€с€佽儗鍖呫€佽澶囧拰 Buff 鏁版嵁銆?*/
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="组件", DisplayName="角色数据中转组件")
	TObjectPtr<ULxCharacterDataTransferComponent> m_pCharacterDataTransferComponent;

	/** 瑙掕壊瑁呭缁勪欢锛岀敤浜庣鐞嗚澶囨Ы浣嶅拰宸茶澶囩墿鍝併€?*/
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="组件", DisplayName="角色装备组件")
	TObjectPtr<ULxCharacterEquipmentComponent> m_pCharacterEquipmentComponent;


	/** 瑙掕壊褰撳墠鐘舵€併€?*/
	UPROPERTY(BlueprintReadOnly, EditAnywhere, ReplicatedUsing=OnRep_CharacterState, Category="Character State", DisplayName="Current Character State")
	ELxCharacterState m_nCharacterState = ELxCharacterState::Idle;

	UFUNCTION()
	void OnRep_CharacterState();

	/** 鏍囪瑙掕壊鏄惁宸茬粡瀹屾垚鍒濆鍖栵紝閬垮厤閲嶅鍒濆鍖栥€?*/
	bool IsInitialized = false;
};
