#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "LxARPG/LxSource/Model/Input/DataType/LxInputReceiveInterface.h"
#include "LxPlayerControlModuleBase.generated.h"

class AActor;
class ALxPlayerCharacter;
class ULxPlayerControlComponent;

/** 玩家操控 UObject 模块基类，提供角色、世界和输入监听访问。 */
UCLASS(Abstract, BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced, DisplayName="玩家操控模块基类")
class LXARPG_API ULxPlayerControlModuleBase : public UObject, public ILxInputReceiveInterface
{
	GENERATED_BODY()

public:
	/** 绑定玩家操控组件并初始化模块。 */
	virtual void InitializeModule(ULxPlayerControlComponent* InOwnerComponent);

	/** 释放输入监听和运行期引用。 */
	virtual void ShutdownModule();

	/** 获取统一玩家操控组件。 */
	UFUNCTION(BlueprintPure, Category="玩家|操控", DisplayName="获取玩家操控组件")
	ULxPlayerControlComponent* GetPlayerControlComponent() const { return OwnerComponent; }

	/** 获取模块所属玩家角色。 */
	UFUNCTION(BlueprintPure, Category="玩家|操控", DisplayName="获取玩家操控模块所属角色")
	ALxPlayerCharacter* GetPlayerCharacter() const;

	/** 获取模块所属 Actor。 */
	AActor* GetOwner() const;

	/** 获取模块使用的世界。 */
	virtual UWorld* GetWorld() const override;

	/** 默认不注册额外监听，具体模块可按需重写。 */
	virtual void InitMonitorRegistration() override {}

protected:
	/** 注册一个输入行为监听。 */
	void RegisterInputActionReceive(ELxInputActionID InInputActionID);

	/** 取消全部输入行为监听。 */
	void UnregisterAllInputActionReceives();

	/** 通知统一玩家操控组件模块数据已经变化。 */
	void BroadcastModuleDataChanged() const;

	/** 当前模块所属的玩家操控组件。 */
	UPROPERTY(Transient, BlueprintReadOnly, Category="玩家|操控", DisplayName="所属玩家操控组件")
	TObjectPtr<ULxPlayerControlComponent> OwnerComponent;

private:
	/** 当前模块已经注册的输入行为。 */
	TSet<ELxInputActionID> RegisteredInputActionIDs;
};
