// Fill out your copyright notice in the Description page of Project Settings.


#include "LxString.h"

#include "LxLogeLevelEnum.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "LxARPG/LxSource/Model/Chat/Logic/LxPlayerChatComponent.h"
#include "LxARPG/LxSource/Player/Controllers/LxPlayerController.h"

namespace
{
	/** 获取聊天调试信息写入的日志文件路径。 */
	FString GetDebugChatLogFilePath()
	{
		return FPaths::Combine(FPaths::ProjectLogDir(), TEXT("LxChatDebug.log"));
	}

	/** 将聊天调试信息追加写入项目日志目录。 */
	void AppendDebugChatMessageToLogFile(const FString& InMessage)
	{
		if (InMessage.IsEmpty())
		{
			return;
		}

		const FString LogDirectory = FPaths::ProjectLogDir();
		IFileManager::Get().MakeDirectory(*LogDirectory, true);

		const FString LogLine = FString::Printf(TEXT("[%s] %s%s"),
			*FDateTime::Now().ToString(TEXT("%Y-%m-%d %H:%M:%S")),
			*InMessage,
			LINE_TERMINATOR);
		FFileHelper::SaveStringToFile(LogLine, *GetDebugChatLogFilePath(), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM, &IFileManager::Get(), FILEWRITE_Append);
	}
}

FString FLxString::NameToString(const FName& InName)
{
	return InName.ToString();
}

FString FLxString::CharToString(const char* InChar)
{
	return UTF8_TO_TCHAR(InChar);
}

FName FLxString::StringToName(const FString& InString)
{
	return FName(*InString);
}

FText FLxString::StringToText(const FString& InString)
{
	return FText::FromString(InString);
}

FString FLxString::TextToString(const FText& InText)
{
	return InText.ToString();
}

FText FLxString::NameToText(const FName& InName)
{
	return FText::FromName(InName);
}

FName FLxString::TextToName(const FText& InText)
{
	return FName(*InText.ToString());
}

FLxString FLxString::DoubleToIntStr(double Value)
{
	int v = Value;
	return FLxString(v);
}

FLxString FLxString::DoubleToAccurateStr(double Value, uint8 Digit)
{
	
	FNumberFormattingOptions Options;
	Options.SetMaximumFractionalDigits(Digit);
	Options.SetMinimumFractionalDigits(Digit);

	FText Text = FText::AsNumber(Value, &Options);

	return Text;
}

FName FLxString::IntToName(uint32 InID)
{
	return FName(*FString::Printf(TEXT("%d"), InID));
}

FLxString::FLxString()
{
}

FLxString::FLxString(const char* Value)
{
	operator<<(Value);
}

FLxString::FLxString(const wchar_t* Value)
{
	operator<<(Value);
}

FLxString::FLxString(const FString& Value)
{
	operator<<(Value);
}

FLxString::FLxString(const FLxString& Value)
{
	operator<<(Value);
}

FLxString::FLxString(const FName& Value)
{
	operator<<(Value);
}

FLxString::FLxString(const FText& Value)
{
	operator<<(Value);
}

FLxString::FLxString(const int32& Value)
{
	operator<<(Value);
}

FLxString::FLxString(const int64& Value)
{
	operator<<(Value);
}

FLxString::FLxString(const float& Value)
{
	operator<<(Value);
}

FLxString::FLxString(const double& Value)
{
	operator<<(Value);
}

FLxString::FLxString(const bool& Value)
{
	operator<<(Value);
}

FLxString& FLxString::Arg(const char* Value)
{
	ReplaceFlag(Value);
	return *this;
}

FLxString& FLxString::Arg(const wchar_t* Value)
{
	ReplaceFlag(Value);
	return *this;
}

FLxString& FLxString::Arg(const FString& Value)
{
	ReplaceFlag(Value);
	return *this;
}

FLxString& FLxString::Arg(const FLxString& Value)
{
	ReplaceFlag(Value);
	return *this;
}

FLxString& FLxString::Arg(const FName& Value)
{
	ReplaceFlag(Value);
	return *this;
}

FLxString& FLxString::Arg(const FText& Value)
{
	ReplaceFlag(Value);
	return *this;
}

FLxString& FLxString::Arg(const int32& Value)
{
	ReplaceFlag(Value);
	return *this;
}

FLxString& FLxString::Arg(const int64& Value)
{
	ReplaceFlag(Value);
	return *this;
}

FLxString& FLxString::Arg(const float& Value)
{
	ReplaceFlag(Value);
	return *this;
}

FLxString& FLxString::Arg(const double& Value)
{
	ReplaceFlag(Value);
	return *this;
}

FLxString& FLxString::Arg(const bool& Value)
{
	ReplaceFlag(Value);
	return *this;
}

FLxString& FLxString::operator<<(const FLxString& Value)
{
	InternalString += Value.InternalString;
	return *this;
}

FLxString& FLxString::operator<<(const char* Value)
{
	InternalString += UTF8_TO_TCHAR(Value);
	return *this;
}

/* ---------- << ---------- */

FLxString& FLxString::operator<<(const FString& Value)
{
	InternalString += Value;
	return *this;
}

FLxString& FLxString::operator<<(const FName& Value)
{
	InternalString += Value.ToString();
	return *this;
}

FLxString& FLxString::operator<<(const FText& Value)
{
	InternalString += Value.ToString();
	return *this;
}

FLxString& FLxString::operator<<(const wchar_t* Value)
{
	InternalString += Value;
	return *this;
}

FLxString& FLxString::operator<<(int32 Value)
{
	InternalString += FString::FromInt(Value);
	return *this;
}

FLxString& FLxString::operator<<(int64 Value)
{
	InternalString += FString::Printf(TEXT("%lld"), Value);
	return *this;
}

FLxString& FLxString::operator<<(float Value)
{
	InternalString += FString::SanitizeFloat(Value);
	return *this;
}

FLxString& FLxString::operator<<(double Value)
{
	InternalString += FString::SanitizeFloat(Value);
	return *this;
}

FLxString& FLxString::operator<<(bool Value)
{
	InternalString += Value ? TEXT("true") : TEXT("false");
	return *this;
}
/* ---------- + ---------- */

#define LXSTRING_PLUS_IMPL(Type) \
FLxString FLxString::operator+(const Type& Value) \
{ \
	operator<<(Value);\
	return *this; \
}
LXSTRING_PLUS_IMPL(FString)
LXSTRING_PLUS_IMPL(FLxString)
LXSTRING_PLUS_IMPL(FName)
LXSTRING_PLUS_IMPL(FText)
LXSTRING_PLUS_IMPL(int32)
LXSTRING_PLUS_IMPL(int64)
LXSTRING_PLUS_IMPL(float)
LXSTRING_PLUS_IMPL(double)
LXSTRING_PLUS_IMPL(bool)

#undef LXSTRING_PLUS_IMPL


/* ---------- 输出 ---------- */

FString FLxString::ToFString() const
{
	return InternalString;
}

FName FLxString::ToFName() const
{
	return FName(*InternalString);
}

FText FLxString::ToFText() const
{
	return FText::FromString(InternalString);
}

const TCHAR* FLxString::ToChar() const
{
	return *InternalString;
}

int FLxString::ToInt() const
{
	int Value = 0;
	LexFromString(Value, *InternalString);
	return Value;
}


void FLxString::Clear()
{
	InternalString.Reset();
}
bool FLxString::IsEmpty() const
{
	return InternalString.IsEmpty();
}

bool FLxString::Contains(const FLxString& Value) const
{
	return InternalString.Contains(Value.ToFString());
}

void FLxString::LogeToConsole(ELxLogeLevelType Level)
{
	switch (Level)
	{
	case ELxLogeLevelType::Debug:
		UE_LOG(LogTemp, Log, TEXT("LxLog : %s"), *InternalString);
		break;
	case ELxLogeLevelType::Warn:
		UE_LOG(LogTemp, Warning, TEXT("LxWarning : %s"), *InternalString);
		break;
	case ELxLogeLevelType::Error:
		UE_LOG(LogTemp, Error, TEXT("LxError : %s"), *InternalString);
		break;
	};
}


void FLxString::LogeToScreenLog(ELxLogeLevelType Level)
{
	if (!GEngine)
	{
		return;
	}
	switch (Level)
	{
		case ELxLogeLevelType::Debug:
			GEngine->AddOnScreenDebugMessage(
			-1,								// Key，-1 表示每次新建一条
			5.0f,							// 显示时间（秒）
			FColor::Green,					// 颜色
			InternalString                 // FString 可直接传
			);
			break;
		case ELxLogeLevelType::Warn:
			GEngine->AddOnScreenDebugMessage(
			-1,								// Key，-1 表示每次新建一条
			5.0f,							// 显示时间（秒）
			FColor::Yellow,					// 颜色
			InternalString                 // FString 可直接传
			);
			break;
		case ELxLogeLevelType::Error:
			GEngine->AddOnScreenDebugMessage(
			-1,								// Key，-1 表示每次新建一条
			5.0f,							// 显示时间（秒）
			FColor::Red,					// 颜色
			InternalString                 // FString 可直接传
			);
			break;
	};
}


void FLxString::SendDebugMessageToChat(const UObject* WorldContextObject) const
{
	AppendDebugChatMessageToLogFile(InternalString);

	if (!GEngine || WorldContextObject == nullptr)
	{
		return;
	}

	UWorld* World = GEngine->GetWorldFromContextObject(const_cast<UObject*>(WorldContextObject), EGetWorldErrorMode::ReturnNull);
	if (!World)
	{
		return;
	}

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		ALxPlayerController* PlayerController = Cast<ALxPlayerController>(It->Get());
		if (!PlayerController || !PlayerController->IsLocalController())
		{
			continue;
		}

		if (ULxPlayerChatComponent* ChatComponent = PlayerController->GetPlayerChatComponent())
		{
			ChatComponent->AddLocalDebugMessage(ToFText());
		}
	}
}

void FLxString::LogeToChat(const UObject* WorldContextObject) const
{
	SendDebugMessageToChat(WorldContextObject);
}

void FLxString::ReplaceFlag(const FLxString& Value)
{
	// 匹配 {数字}
	FRegexPattern Pattern(TEXT("\\{(\\d+)\\}"));
	
	FRegexMatcher Matcher(Pattern, InternalString);

	if (Matcher.FindNext())
	{
		int32 Begin = Matcher.GetMatchBeginning();
		int32 End   = Matcher.GetMatchEnding();

		// 左侧
		FString OutLeft = InternalString.Left(Begin);

		// 右侧
		FString OutRight = InternalString.Mid(End);

		// 清空
		Clear();
		operator<< (OutLeft);
		operator<< (Value);
		operator<< (OutRight);
	}
	
}
