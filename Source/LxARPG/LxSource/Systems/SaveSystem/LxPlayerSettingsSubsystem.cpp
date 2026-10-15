#include "LxPlayerSettingsSubsystem.h"

#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "LxPlayerSettingsSaveData.h"
#include "LxSaveFile.h"
#include "LxARPG/LxSource/Model/Input/DataType/LxInputActionConfig.h"
#include "LxARPG/LxSource/Systems/SettingSystem/LxGameSettings.h"
#include "UObject/StrongObjectPtr.h"

/** 玩家设置读取和提交失败的专用日志分类。 */
DEFINE_LOG_CATEGORY_STATIC(LogLxPlayerSettings, Log, All);

namespace
{
	/** 当前唯一受支持的玩家设置存档格式。 */
	constexpr int32 PlayerSettingsFormatVersion = 1;

	/** 比较完整有序覆盖记录，保存回读时确认数据没有丢失或改变。 */
	bool AreOverridesEqual(const TArray<FLxInputKeySetting>& Left, const TArray<FLxInputKeySetting>& Right)
	{
		if (Left.Num() != Right.Num()) return false;
		for (int32 Index = 0; Index < Left.Num(); ++Index)
		{
			if (Left[Index].InputActionID != Right[Index].InputActionID
				|| Left[Index].LocalSettingKey != Right[Index].LocalSettingKey) return false;
		}
		return true;
	}

	/** 磁盘校验不依赖配置表初始化顺序，运行时查询再检查行为是否允许修改。 */
	bool IsValidSettingsData(const ULxPlayerSettingsSaveData* Data)
	{
		if (!Data || Data->FormatVersion != PlayerSettingsFormatVersion || Data->Revision <= 0) return false;
		const UEnum* ActionEnum = StaticEnum<ELxInputActionID>();
		if (!ActionEnum) return false;
		TSet<ELxInputActionID> SeenActions;
		for (const FLxInputKeySetting& Setting : Data->InputKeyOverrides)
		{
			if (Setting.InputActionID == ELxInputActionID::None
				|| Setting.InputActionID == ELxInputActionID::LookX || Setting.InputActionID == ELxInputActionID::LookY
				|| !ActionEnum->IsValidEnumValue(static_cast<int64>(Setting.InputActionID))
				|| SeenActions.Contains(Setting.InputActionID)
				|| !LxInputActionConfig::IsSupportedUserKey(Setting.LocalSettingKey)) return false;
			SeenActions.Add(Setting.InputActionID);
		}
		return true;
	}
}

void ULxPlayerSettingsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UserIndex = GetDefault<ULxGameSettings>()->SaveUserIndex;
	FString RequestedPrefix;
	if (FParse::Value(FCommandLine::Get(), TEXT("LxPlayerSettingsSlot="), RequestedPrefix))
	{
		// 测试前缀只允许文件名字符，避免命令行参数指向设置目录以外的位置。
		bool bSafePrefix = !RequestedPrefix.IsEmpty();
		for (const TCHAR Character : RequestedPrefix)
		{
			bSafePrefix &= FChar::IsAlnum(Character) || Character == TEXT('_') || Character == TEXT('-');
		}
		if (bSafePrefix) SlotPrefix = RequestedPrefix;
		else
		{
			bLoadAttempted = true;
			bWriteBlocked = true;
			LastError = FText::FromString(TEXT("玩家设置存档参数无效，只允许字母、数字、下划线与连字符。"));
			UE_LOG(LogLxPlayerSettings, Error, TEXT("%s"), *LastError.ToString());
			return;
		}
	}
	EnsureLoaded();
}

void ULxPlayerSettingsSubsystem::Deinitialize()
{
	OnInputSettingsChanged.Clear();
	Super::Deinitialize();
}

FString ULxPlayerSettingsSubsystem::GetSlot(int32 Side) const
{
	return FString::Printf(TEXT("%s_%d"), *SlotPrefix, Side);
}

bool ULxPlayerSettingsSubsystem::EnsureLoaded()
{
	if (bLoadAttempted) return bLoadSucceeded;
	bLoadAttempted = true;
	bLoadSucceeded = true;
	LastError = FText::GetEmpty();
	bool bFoundExistingFile = false;
	for (int32 Side = 0; Side < 2; ++Side)
	{
		const FString Slot = GetSlot(Side);
		if (!UGameplayStatics::DoesSaveGameExist(Slot, UserIndex)) continue;
		bFoundExistingFile = true;
		TStrongObjectPtr<ULxPlayerSettingsSaveData> Data(Cast<ULxPlayerSettingsSaveData>(LxSaveFile::Read(Slot, UserIndex)));
		if (!IsValidSettingsData(Data.Get()))
		{
			bWriteBlocked = true;
			LastError = FText::FromString(FString::Printf(TEXT("玩家设置存档“%s”损坏或版本不兼容，原文件已保留，请备份并移除该文件后重启游戏。"), *Slot));
			UE_LOG(LogLxPlayerSettings, Error, TEXT("%s"), *LastError.ToString());
			continue;
		}
		if (ActiveSide == INDEX_NONE || Data->Revision > ActiveRevision)
		{
			ActiveSide = Side;
			ActiveRevision = Data->Revision;
			KeyOverrides = Data->InputKeyOverrides;
		}
	}
	bLoadSucceeded = !bFoundExistingFile || ActiveSide != INDEX_NONE;
	return bLoadSucceeded;
}

FKey ULxPlayerSettingsSubsystem::GetEffectiveKey(ELxInputActionID InputActionID) const
{
	const FLxInputActionInfo* DefaultInfo = LxInputActionConfig::GetInputActionInfo(InputActionID);
	if (!DefaultInfo) return FKey();
	if (LxInputActionConfig::IsUserRemappableAction(InputActionID))
	{
		for (const FLxInputKeySetting& Setting : KeyOverrides)
		{
			if (Setting.InputActionID == InputActionID) return Setting.LocalSettingKey;
		}
	}
	return DefaultInfo->DefaultKey;
}

bool ULxPlayerSettingsSubsystem::NormalizeInputSettings(const TArray<FLxInputKeySetting>& InputSettings,
	TArray<FLxInputKeySetting>& OutOverrides, FText& OutError) const
{
	TSet<ELxInputActionID> SeenActions;
	for (const FLxInputKeySetting& Setting : InputSettings)
	{
		const FLxInputActionInfo* DefaultInfo = LxInputActionConfig::GetInputActionInfo(Setting.InputActionID);
		if (!DefaultInfo || !LxInputActionConfig::IsUserRemappableAction(Setting.InputActionID))
		{
			OutError = FText::FromString(TEXT("按键配置包含不存在或不允许修改的输入行为。"));
			return false;
		}
		if (SeenActions.Contains(Setting.InputActionID))
		{
			OutError = FText::FromString(TEXT("同一输入行为只能保存一个按键配置。"));
			return false;
		}
		if (!LxInputActionConfig::IsSupportedUserKey(Setting.LocalSettingKey))
		{
			OutError = FText::FromString(TEXT("请选择有效的键盘按键、鼠标按钮或单方向滚轮。"));
			return false;
		}
		SeenActions.Add(Setting.InputActionID);
		if (Setting.LocalSettingKey != DefaultInfo->DefaultKey) OutOverrides.Add(Setting);
	}
	OutOverrides.Sort([](const FLxInputKeySetting& Left, const FLxInputKeySetting& Right)
	{
		return static_cast<uint8>(Left.InputActionID) < static_cast<uint8>(Right.InputActionID);
	});
	// 默认表中本来共用按键的行为对允许继续共用，用户新增的行为冲突则拒绝。
	TArray<const FLxInputActionInfo*> Defaults;
	TArray<FKey> EffectiveKeys;
	for (const TPair<ELxInputActionID, FLxInputActionInfo>& Pair : LxInputActionConfig::GetInputActionInfoMap())
	{
		Defaults.Add(&Pair.Value);
		FKey Key = Pair.Value.DefaultKey;
		for (const FLxInputKeySetting& Override : OutOverrides)
		{
			if (Override.InputActionID == Pair.Key) { Key = Override.LocalSettingKey; break; }
		}
		EffectiveKeys.Add(Key);
	}
	for (int32 Left = 0; Left < Defaults.Num(); ++Left)
	{
		if (!EffectiveKeys[Left].IsValid()) continue;
		for (int32 Right = Left + 1; Right < Defaults.Num(); ++Right)
		{
			if (EffectiveKeys[Left] == EffectiveKeys[Right] && Defaults[Left]->DefaultKey != Defaults[Right]->DefaultKey)
			{
				OutError = FText::Format(FText::FromString(TEXT("按键“{0}”同时分配给“{1}”与“{2}”，请更换其中一项。")),
					EffectiveKeys[Left].GetDisplayName(), Defaults[Left]->DisplayName, Defaults[Right]->DisplayName);
				return false;
			}
		}
	}
	return true;
}

bool ULxPlayerSettingsSubsystem::SaveInputSettings(const TArray<FLxInputKeySetting>& InputSettings, FText& OutError)
{
	OutError = FText::GetEmpty();
	EnsureLoaded();
	if (bWriteBlocked || !bLoadSucceeded)
	{
		OutError = LastError;
		return false;
	}
	TArray<FLxInputKeySetting> NextOverrides;
	if (!NormalizeInputSettings(InputSettings, NextOverrides, OutError))
	{
		LastError = OutError;
		return false;
	}
	if (ActiveRevision == MAX_int64)
	{
		OutError = LastError = FText::FromString(TEXT("玩家设置存档修订号已达到上限，原文件保持不变。"));
		return false;
	}
	const int32 NextSide = ActiveSide == 0 ? 1 : 0;
	const FString NextSlot = GetSlot(NextSide);
	if (ActiveSide != INDEX_NONE)
	{
		TStrongObjectPtr<ULxPlayerSettingsSaveData> Active(Cast<ULxPlayerSettingsSaveData>(LxSaveFile::Read(GetSlot(ActiveSide), UserIndex)));
		if (!IsValidSettingsData(Active.Get()) || Active->Revision != ActiveRevision
			|| !AreOverridesEqual(Active->InputKeyOverrides, KeyOverrides))
		{
			bWriteBlocked = true;
			OutError = LastError = FText::FromString(TEXT("上次完整玩家设置存档已发生外部变更，原文件已保留，请重启游戏后处理。"));
			return false;
		}
	}
	// 保存期间磁盘内容可能被外部替换；损坏文件始终不能作为可覆盖的旧副本。
	if (UGameplayStatics::DoesSaveGameExist(NextSlot, UserIndex))
	{
		TStrongObjectPtr<ULxPlayerSettingsSaveData> Existing(Cast<ULxPlayerSettingsSaveData>(LxSaveFile::Read(NextSlot, UserIndex)));
		if (!IsValidSettingsData(Existing.Get()) || Existing->Revision > ActiveRevision)
		{
			bWriteBlocked = true;
			OutError = LastError = FText::FromString(TEXT("磁盘中的玩家设置已损坏或发生外部变更，原文件已保留，请重启游戏后处理。"));
			return false;
		}
	}
	TStrongObjectPtr<ULxPlayerSettingsSaveData> Next(NewObject<ULxPlayerSettingsSaveData>());
	Next->FormatVersion = PlayerSettingsFormatVersion;
	Next->Revision = ActiveRevision + 1;
	Next->InputKeyOverrides = MoveTemp(NextOverrides);
	if (!LxSaveFile::Write(Next.Get(), NextSlot, UserIndex))
	{
		OutError = LastError = FText::FromString(TEXT("玩家设置写入失败，当前按键配置和上次完整存档保持不变。"));
		return false;
	}
	TStrongObjectPtr<ULxPlayerSettingsSaveData> Verified(Cast<ULxPlayerSettingsSaveData>(LxSaveFile::Read(NextSlot, UserIndex)));
	if (!IsValidSettingsData(Verified.Get()) || Verified->Revision != Next->Revision
		|| !AreOverridesEqual(Verified->InputKeyOverrides, Next->InputKeyOverrides))
	{
		bWriteBlocked = true;
		OutError = LastError = FText::FromString(TEXT("玩家设置写入后的校验失败，当前按键配置和上次完整存档保持不变。"));
		return false;
	}
	ActiveSide = NextSide;
	ActiveRevision = Next->Revision;
	KeyOverrides = MoveTemp(Next->InputKeyOverrides);
	LastError = FText::GetEmpty();
	OnInputSettingsChanged.Broadcast();
	return true;
}
