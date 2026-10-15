#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "LxARPG/LxSource/Model/Input/DataType/LxInputData.h"
#include "LxPlayerSettingsSubsystem.generated.h"

/** 玩家输入设置成功写入并生效后通知所有输入接收端重新建立映射。 */
DECLARE_MULTICAST_DELEGATE(FLxInputSettingsChanged);

/** 游戏实例范围的玩家设置管理器，角色切换和地图旅行均共用同一份设置。 */
UCLASS(BlueprintType, DisplayName="玩家设置管理器")
class LXARPG_API ULxPlayerSettingsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/** 原生设置变化通知，界面基类可以转发为蓝图事件。 */
	FLxInputSettingsChanged OnInputSettingsChanged;

	/** 游戏实例创建时检查本地设置，首次使用保留数据表默认配置。 */
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	/** 游戏实例关闭时释放设置监听关系。 */
	virtual void Deinitialize() override;

	/** 保证本地设置已经读取；损坏或不兼容时返回失败，原文件保持不变。 */
	UFUNCTION(BlueprintCallable, Category="玩家设置|存档", DisplayName="加载玩家设置")
	bool EnsureLoaded();

	/** 获取当前生效的按键；没有用户覆盖或行为不可修改时采用数据表默认值。 */
	UFUNCTION(BlueprintPure, Category="玩家设置|输入", DisplayName="获取生效输入按键")
	FKey GetEffectiveKey(ELxInputActionID InputActionID) const;

	/** 获取只读的本地覆盖记录，不允许调用者修改数据表预设。 */
	const TArray<FLxInputKeySetting>& GetKeyOverrides() const { return KeyOverrides; }

	/** 验证并保存按键配置；完整写入且回读验证后才更新运行配置并通知监听者。 */
	UFUNCTION(BlueprintCallable, Category="玩家设置|输入", DisplayName="保存用户按键设置")
	bool SaveInputSettings(const TArray<FLxInputKeySetting>& InputSettings, FText& OutError);

	/** 获取最近的读取或保存错误，可直接交给设置界面显示。 */
	UFUNCTION(BlueprintPure, Category="玩家设置|存档", DisplayName="获取玩家设置错误")
	FText GetLastError() const { return LastError; }

private:
	/** 自动化用例通过随机前缀隔离磁盘文件，不访问真实玩家设置。 */
	friend struct FLxPlayerSettingsTestAccess;

	/** 生成指定副本的固定平台存档槽名。 */
	FString GetSlot(int32 Side) const;

	/** 保存前检查用户输入并剔除与配置表预设相同的冗余覆盖。 */
	bool NormalizeInputSettings(const TArray<FLxInputKeySetting>& InputSettings,
		TArray<FLxInputKeySetting>& OutOverrides, FText& OutError) const;

	/** 已成功加载或提交的运行时覆盖记录。 */
	UPROPERTY(Transient, VisibleAnywhere, Category="玩家设置|输入", DisplayName="当前用户按键配置")
	TArray<FLxInputKeySetting> KeyOverrides;

	/** 平台槽名前缀，与角色和地图存档槽相互独立。 */
	FString SlotPrefix = TEXT("LxARPG_PlayerSettings");
	/** 当前平台玩家索引，沿用项目的存档用户设置。 */
	int32 UserIndex = 0;
	/** 当前已提交副本，尚无存档时为 INDEX_NONE。 */
	int32 ActiveSide = INDEX_NONE;
	/** 当前已提交修订号，只在写入并校验成功后递增。 */
	int64 ActiveRevision = 0;
	/** 防止多个输入接收端在同次游戏实例中重复访问磁盘。 */
	bool bLoadAttempted = false;
	/** 至少有一个有效设置副本，或本地尚不存在设置文件。 */
	bool bLoadSucceeded = false;
	/** 存在损坏或未知版本文件时禁止自动覆盖，以保留排查和恢复机会。 */
	bool bWriteBlocked = false;
	/** 最近读取或保存失败的中文提示。 */
	FText LastError;
};
