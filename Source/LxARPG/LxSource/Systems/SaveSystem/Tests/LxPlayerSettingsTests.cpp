#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "HAL/FileManager.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Paths.h"
#include "LxARPG/LxSource/Model/Input/DataType/LxInputActionConfig.h"
#include "LxARPG/LxSource/Systems/SaveSystem/LxPlayerSettingsSaveData.h"
#include "LxARPG/LxSource/Systems/SaveSystem/LxPlayerSettingsSubsystem.h"
#include "LxARPG/LxSource/Systems/SaveSystem/LxSaveFile.h"
#include "UObject/StrongObjectPtr.h"

/** 用例只替换存档前缀，不必初始化其他会读取角色或地图存档的子系统。 */
struct FLxPlayerSettingsTestAccess
{
	/** 创建不会访问真实设置槽的管理器，磁盘加载由用例显式触发。 */
	static ULxPlayerSettingsSubsystem* Create(UGameInstance* Instance, const FString& Prefix)
	{
		ULxPlayerSettingsSubsystem* Settings = NewObject<ULxPlayerSettingsSubsystem>(Instance);
		Settings->SlotPrefix = Prefix;
		Settings->UserIndex = 0;
		return Settings;
	}
};

namespace
{
	/** 自动恢复全局输入表并清理用例独占的两份玩家设置文件。 */
	struct FPlayerSettingsTestScope
	{
		/** 用例随机槽前缀，确保不接触本机真实玩家设置。 */
		FString Prefix = TEXT("LxPlayerSettingsTest_") + FGuid::NewGuid().ToString(EGuidFormats::Digits);
		/** 测试前的完整数据表缓存，退出时逐项恢复。 */
		TMap<ELxInputActionID, FLxInputActionInfo> OriginalInfo = LxInputActionConfig::GetInputActionInfoMap();

		/** 使用三项互不冲突的数字按键作为固定测试预设。 */
		FPlayerSettingsTestScope()
		{
			LxInputActionConfig::ClearInputActionConfig();
			AddDefault(ELxInputActionID::Jump, EKeys::SpaceBar);
			AddDefault(ELxInputActionID::Sprint, EKeys::LeftShift);
			AddDefault(ELxInputActionID::Backpack, EKeys::B);
		}

		/** 恢复输入表并只删除本用例的两个固定槽。 */
		~FPlayerSettingsTestScope()
		{
			LxInputActionConfig::ClearInputActionConfig();
			for (const TPair<ELxInputActionID, FLxInputActionInfo>& Pair : OriginalInfo)
				LxInputActionConfig::SetInputActionInfo(Pair.Value);
			for (int32 Side = 0; Side < 2; ++Side)
			{
				const FString Slot = FString::Printf(TEXT("%s_%d"), *Prefix, Side);
				UGameplayStatics::DeleteGameInSlot(Slot, 0);
				// 写入失败测试创建的是空目录，不使用递归删除。
				IFileManager::Get().DeleteDirectory(*(FPaths::ProjectSavedDir() / TEXT("SaveGames") / (Slot + TEXT(".sav"))), false, false);
			}
		}

		/** 添加一项可修改的数字输入预设。 */
		static void AddDefault(ELxInputActionID ActionID, const FKey& Key)
		{
			FLxInputActionInfo Info;
			Info.InputActionID = ActionID;
			Info.DisplayName = FText::FromName(LxInputActionConfig::GetInputActionName(ActionID));
			Info.DefaultKey = Key;
			Info.ValueType = EInputActionValueType::Boolean;
			LxInputActionConfig::SetInputActionInfo(Info);
		}
	};

	/** 生成一项用户按键配置。 */
	FLxInputKeySetting MakeKeySetting(ELxInputActionID ActionID, const FKey& Key)
	{
		FLxInputKeySetting Setting;
		Setting.InputActionID = ActionID;
		Setting.LocalSettingKey = Key;
		return Setting;
	}
}

/** 验证首次预设、独立持久化、新增行为回默认、还原默认和成功通知。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxPlayerSettingsPersistenceTest, "LxARPG.Input.PlayerSettings.Persistence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxPlayerSettingsPersistenceTest::RunTest(const FString& Parameters)
{
	FPlayerSettingsTestScope Scope;
	TStrongObjectPtr<UGameInstance> Instance(NewObject<UGameInstance>());
	TStrongObjectPtr<ULxPlayerSettingsSubsystem> Settings(FLxPlayerSettingsTestAccess::Create(Instance.Get(), Scope.Prefix));
	TestTrue(TEXT("首次无档案可加载默认配置"), Settings->EnsureLoaded());
	TestEqual(TEXT("首次跳跃采用配置表预设"), Settings->GetEffectiveKey(ELxInputActionID::Jump), EKeys::SpaceBar);
	TestFalse(TEXT("首次读取不自动创建设置存档"), UGameplayStatics::DoesSaveGameExist(Scope.Prefix + TEXT("_0"), 0));
	int32 Notifications = 0;
	Settings->OnInputSettingsChanged.AddLambda([&Notifications]() { ++Notifications; });
	FText Error;
	TestTrue(TEXT("保存自定义鼠标按键"), Settings->SaveInputSettings({MakeKeySetting(ELxInputActionID::Jump, EKeys::ThumbMouseButton)}, Error));
	TestEqual(TEXT("成功提交发出一次通知"), Notifications, 1);
	TestEqual(TEXT("保存后立即采用用户设置"), Settings->GetEffectiveKey(ELxInputActionID::Jump), EKeys::ThumbMouseButton);
	TestEqual(TEXT("配置表默认按键没有被覆盖"), LxInputActionConfig::GetInputActionInfo(ELxInputActionID::Jump)->DefaultKey, EKeys::SpaceBar);
	TStrongObjectPtr<ULxPlayerSettingsSubsystem> Reloaded(FLxPlayerSettingsTestAccess::Create(Instance.Get(), Scope.Prefix));
	TestTrue(TEXT("新实例重新读取独立设置"), Reloaded->EnsureLoaded());
	TestEqual(TEXT("重启保留自定义按键"), Reloaded->GetEffectiveKey(ELxInputActionID::Jump), EKeys::ThumbMouseButton);
	FPlayerSettingsTestScope::AddDefault(ELxInputActionID::Quest, EKeys::J);
	TestEqual(TEXT("新增输入行为采用新增预设"), Reloaded->GetEffectiveKey(ELxInputActionID::Quest), EKeys::J);
	TestTrue(TEXT("清空覆盖可以还原全部默认"), Reloaded->SaveInputSettings({}, Error));
	TestTrue(TEXT("默认按键不写冗余覆盖记录"), Reloaded->GetKeyOverrides().IsEmpty());
	TStrongObjectPtr<ULxPlayerSettingsSubsystem> Restored(FLxPlayerSettingsTestAccess::Create(Instance.Get(), Scope.Prefix));
	TestTrue(TEXT("重启读取较新空覆盖副本"), Restored->EnsureLoaded());
	TestEqual(TEXT("重启后仍为默认按键"), Restored->GetEffectiveKey(ELxInputActionID::Jump), EKeys::SpaceBar);
	return true;
}

/** 验证新增冲突被拒绝，数据表已经允许的共键行为不会被误判。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxPlayerSettingsValidationTest, "LxARPG.Input.PlayerSettings.Validation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxPlayerSettingsValidationTest::RunTest(const FString& Parameters)
{
	FPlayerSettingsTestScope Scope;
	TStrongObjectPtr<UGameInstance> Instance(NewObject<UGameInstance>());
	TStrongObjectPtr<ULxPlayerSettingsSubsystem> Settings(FLxPlayerSettingsTestAccess::Create(Instance.Get(), Scope.Prefix));
	FText Error;
	TestFalse(TEXT("不能把跳跃改成背包已有按键"), Settings->SaveInputSettings({MakeKeySetting(ELxInputActionID::Jump, EKeys::B)}, Error));
	TestFalse(TEXT("冲突错误包含用户提示"), Error.IsEmpty());
	TestEqual(TEXT("冲突不会改变生效配置"), Settings->GetEffectiveKey(ELxInputActionID::Jump), EKeys::SpaceBar);
	TestFalse(TEXT("视角轴不允许进入用户覆盖"), Settings->SaveInputSettings({MakeKeySetting(ELxInputActionID::LookX, EKeys::V)}, Error));
	TestFalse(TEXT("无效按键不可保存"), Settings->SaveInputSettings({MakeKeySetting(ELxInputActionID::Jump, FKey())}, Error));
	TestFalse(TEXT("同一行为不能出现两次"), Settings->SaveInputSettings({MakeKeySetting(ELxInputActionID::Jump, EKeys::V), MakeKeySetting(ELxInputActionID::Jump, EKeys::N)}, Error));
	FPlayerSettingsTestScope::AddDefault(ELxInputActionID::InteractionInteract, EKeys::E);
	FPlayerSettingsTestScope::AddDefault(ELxInputActionID::InteractionBack, EKeys::E);
	TestTrue(TEXT("默认已有的共键组合允许保存其他修改"), Settings->SaveInputSettings({MakeKeySetting(ELxInputActionID::Jump, EKeys::V)}, Error));
	return true;
}

/** 验证损坏或未知版本文件被保留，仍可回退到上一份完整设置。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxPlayerSettingsCorruptionTest, "LxARPG.Input.PlayerSettings.CorruptionProtection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxPlayerSettingsCorruptionTest::RunTest(const FString& Parameters)
{
	FPlayerSettingsTestScope Scope;
	TStrongObjectPtr<UGameInstance> Instance(NewObject<UGameInstance>());
	TStrongObjectPtr<ULxPlayerSettingsSubsystem> Settings(FLxPlayerSettingsTestAccess::Create(Instance.Get(), Scope.Prefix));
	FText Error;
	TestTrue(TEXT("建立第一份完整设置"), Settings->SaveInputSettings({MakeKeySetting(ELxInputActionID::Jump, EKeys::V)}, Error));
	const FString DamagedSlot = Scope.Prefix + TEXT("_1");
	const TArray<uint8> DamagedBytes = {1, 3, 5, 7};
	TestTrue(TEXT("准备隔离的损坏副本"), UGameplayStatics::SaveDataToSlot(DamagedBytes, DamagedSlot, 0));
	AddExpectedError(TEXT("玩家设置存档"), EAutomationExpectedErrorFlags::Contains, 1);
	TStrongObjectPtr<ULxPlayerSettingsSubsystem> Reloaded(FLxPlayerSettingsTestAccess::Create(Instance.Get(), Scope.Prefix));
	TestTrue(TEXT("损坏另一个副本时仍能加载上次完整设置"), Reloaded->EnsureLoaded());
	TestEqual(TEXT("保留完整副本中的用户按键"), Reloaded->GetEffectiveKey(ELxInputActionID::Jump), EKeys::V);
	TestFalse(TEXT("存在损坏文件时禁止静默覆盖"), Reloaded->SaveInputSettings({}, Error));
	TArray<uint8> RemainingBytes;
	TestTrue(TEXT("损坏文件仍可读取原始字节"), UGameplayStatics::LoadDataFromSlot(RemainingBytes, DamagedSlot, 0));
	TestTrue(TEXT("损坏文件内容保持不变"), RemainingBytes == DamagedBytes);
	UGameplayStatics::DeleteGameInSlot(Scope.Prefix + TEXT("_0"), 0);
	TStrongObjectPtr<ULxPlayerSettingsSaveData> FutureData(NewObject<ULxPlayerSettingsSaveData>());
	FutureData->FormatVersion = 999;
	FutureData->Revision = 1;
	TestTrue(TEXT("准备隔离的未知版本副本"), LxSaveFile::Write(FutureData.Get(), DamagedSlot, 0));
	AddExpectedError(TEXT("玩家设置存档"), EAutomationExpectedErrorFlags::Contains, 1);
	TStrongObjectPtr<ULxPlayerSettingsSubsystem> Future(FLxPlayerSettingsTestAccess::Create(Instance.Get(), Scope.Prefix));
	TestFalse(TEXT("唯一未知版本存档应报告加载失败"), Future->EnsureLoaded());
	TestEqual(TEXT("加载失败保留安全默认按键"), Future->GetEffectiveKey(ELxInputActionID::Jump), EKeys::SpaceBar);
	TestFalse(TEXT("不允许覆盖未知版本"), Future->SaveInputSettings({}, Error));
	return true;
}

/** 验证新副本写入失败时运行按键、通知次数和上次完整文件均保持不变。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxPlayerSettingsWriteFailureTest, "LxARPG.Input.PlayerSettings.WriteFailure",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxPlayerSettingsWriteFailureTest::RunTest(const FString& Parameters)
{
	FPlayerSettingsTestScope Scope;
	TStrongObjectPtr<UGameInstance> Instance(NewObject<UGameInstance>());
	TStrongObjectPtr<ULxPlayerSettingsSubsystem> Settings(FLxPlayerSettingsTestAccess::Create(Instance.Get(), Scope.Prefix));
	FText Error;
	if (!TestTrue(TEXT("保存上次完整设置"), Settings->SaveInputSettings({MakeKeySetting(ELxInputActionID::Jump, EKeys::V)}, Error))) return false;
	TArray<uint8> OriginalBytes;
	UGameplayStatics::LoadDataFromSlot(OriginalBytes, Scope.Prefix + TEXT("_0"), 0);
	const FString BlockedPath = FPaths::ProjectSavedDir() / TEXT("SaveGames") / (Scope.Prefix + TEXT("_1.sav"));
	if (!TestTrue(TEXT("用空目录阻止新副本成为文件"), IFileManager::Get().MakeDirectory(*BlockedPath, false))) return false;
	int32 Notifications = 0;
	Settings->OnInputSettingsChanged.AddLambda([&Notifications]() { ++Notifications; });
	TestFalse(TEXT("无法创建新副本时返回保存失败"), Settings->SaveInputSettings({MakeKeySetting(ELxInputActionID::Jump, EKeys::N)}, Error));
	TestEqual(TEXT("失败不广播设置变化"), Notifications, 0);
	TestEqual(TEXT("失败不改变运行按键"), Settings->GetEffectiveKey(ELxInputActionID::Jump), EKeys::V);
	TArray<uint8> RemainingBytes;
	UGameplayStatics::LoadDataFromSlot(RemainingBytes, Scope.Prefix + TEXT("_0"), 0);
	TestTrue(TEXT("上次完整文件字节保持不变"), RemainingBytes == OriginalBytes);
	return true;
}

#endif
