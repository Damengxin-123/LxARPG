// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "LxLogeLevelEnum.h"
#include "CoreMinimal.h"
#include "LxString.generated.h"


// 便捷性宏定义
#define ERROR_TO_SCREEN(Str) FLxString(Str).LogeToScreenLog(ELxLogeLevelType::Error)


/**
 * @class FLxString
 * @brief 字符串操作与处理的基类
 *
 * 一个继承自 UObject 的类，作为在虚幻引擎环境中进行字符串操作与处理的基类。
 * 用于处理虚幻引擎中所有字符串类型及一部分基础类之间的相互转换，以及提供一些便捷的处理方法。
 *
 * @note 可在蓝图中使用
 */
USTRUCT()
struct FLxString
{
	GENERATED_BODY()
public:
/*===================== 类型转换工具函数 =====================*/
	
	// 将FName转换为FString
	static FString NameToString(const FName& InName);

	// 将C风格字符串转换为FString
	static FString CharToString(const char* InChar);

	// 将FString转换为FName
	static FName StringToName(const FString& InString);

	// 将FString转换为FText
	static FText StringToText(const FString& InString);

	// 将FText转换为FString
	static FString TextToString(const FText& InText);

	// 将FName转换为FText
	static FText NameToText(const FName& InName);

	// 将FText转换为FName
	static FName TextToName(const FText& InText);

/*===================== 类型转换工具函数 =====================*/
/*===================== 数值分割工具 统一返回本类型 =====================*/

	// 将给定的double值转换为整数字符串
	static FLxString DoubleToIntStr(double Value);

	// 将给定的double值转换为保留指定位数的小数的double字符串
	static FLxString DoubleToAccurateStr(double Value, uint8 Digit);

/*===================== 数值分割工具 统一返回本类型 =====================*/
/*===================== 构造函数 =====================*/
	
	FLxString();
	FLxString(const char* Value);
	FLxString(const wchar_t* Value);
	FLxString(const FString& Value);
	FLxString(const FLxString& Value);
	FLxString(const FName& Value);
	FLxString(const FText& Value);
	FLxString(const int32& Value);
	FLxString(const int64& Value);
	FLxString(const float& Value);
	FLxString(const double& Value);
	FLxString(const bool& Value);

/*===================== 构造函数 =====================*/
/*===================== 字符替换函数  函数替换时会查找当前通过构造函数已设置的特殊字符串段 : {0}、{1}、{2}...，并将其替换为传入的参数值。 =====================*/

	/**
	 * @brief 将下一个格式占位符替换为指定值。
	 *
	 * 这一组重载用于链式替换字符串中的 `{0}`、`{1}` 等占位符。
	 *
	 * @param Value 要写入占位符位置的值。
	 * @return 返回当前字符串对象引用，便于继续链式调用。
	 */
	FLxString& Arg(const char* Value);
	FLxString& Arg(const wchar_t* Value);
	FLxString& Arg(const FString& Value);
	FLxString& Arg(const FLxString& Value);
	FLxString& Arg(const FName& Value);
	FLxString& Arg(const FText& Value);
	FLxString& Arg(const int32& Value);
	FLxString& Arg(const int64& Value);
	FLxString& Arg(const float& Value);
	FLxString& Arg(const double& Value);
	FLxString& Arg(const bool& Value);
	
/*===================== 字符替换函数 =====================*/
/*===================== 流式追加运算符函数 =====================*/
	
	FLxString& operator<<(const FLxString& Value);
	FLxString& operator<<(const char* Value);
	FLxString& operator<<(const FString& Value);
	FLxString& operator<<(const FName& Value);
	FLxString& operator<<(const FText& Value);
	FLxString& operator<<(const wchar_t* Value);
	FLxString& operator<<(int32 Value);
	FLxString& operator<<(int64 Value);
	FLxString& operator<<(float Value);
	FLxString& operator<<(double Value);
	FLxString& operator<<(bool Value);
	
	FLxString operator+(const FString& Value);
	FLxString operator+(const FLxString& Value);
	FLxString operator+(const FName& Value);
	FLxString operator+(const FText& Value);
	FLxString operator+(const int32& Value);
	FLxString operator+(const int64& Value);
	FLxString operator+(const float& Value);
	FLxString operator+(const double& Value);
	FLxString operator+(const bool& Value);
	
/*===================== 流式追加运算符函数 =====================*/
/*===================== UE5内部字符类型输出函数 =====================*/

	/**
	 * @brief 将内部字符串转换为 FString。
	 *
	 * @return 返回当前内部字符串的 FString 副本。
	 */
	FString 		ToFString() const;
	/**
	 * @brief 将内部字符串转换为 FName。
	 *
	 * @return 返回基于当前字符串构造的 FName。
	 */
	FName   		ToFName() const;
	/**
	 * @brief 将内部字符串转换为 FText。
	 *
	 * @return 返回基于当前字符串构造的 FText。
	 */
	FText   		ToFText() const;
	/**
	 * @brief 获取内部字符串的 TCHAR 指针。
	 *
	 * @return 返回可供 UE 字符串接口使用的 TCHAR 指针。
	 */
	const TCHAR*	ToChar() const;
	/**
	 * @brief 将内部字符串转换为整数。
	 *
	 * @return 返回字符串转换后的整数值。
	 */
	int				ToInt() const;
	
/*===================== 字符串处理函数 =====================*/
	
	/**
	 * @brief 清空内部字符串内容。
	 */
	void	Clear();
	/**
	 * @brief 判断内部字符串是否为空。
	 *
	 * @return 若字符串为空则返回 true，否则返回 false。
	 */
	bool	IsEmpty() const;
	/**
	 * @brief 判断当前字符串是否包含指定子串。
	 *
	 * @param Value 要查找的子串。
	 * @return 包含返回 true，否则返回 false。
	 */
	bool	Contains(const FLxString& Value) const;

/*===================== 字符串处理函数 =====================*/
/*===================== 日志输出函数 =====================*/

	/**
	 * @brief 将内部字符串输出到控制台日志
	 *
	 * @param Level 日志级别
	 */
	void LogeToConsole(ELxLogeLevelType Level);

	/**
	 * @brief 将内部字符串输出到屏幕日志
	 *
	 * 该方法根据传入的日志级别（Level）以不同的颜色在屏幕上显示一条消息。如果GEngine未初始化，则不会执行任何操作。
	 *
	 * @param Level 日志级别，决定消息的颜色：
	 * - 0: 绿色
	 * - 1: 蓝色
	 * - 2: 黄色
	 * - 其他: 红色
	 */
	void LogeToScreenLog(ELxLogeLevelType Level);


private:

	/**
	 * @brief 替换标志位
	 *
	 * @param Value 替换值
	 */
	inline void ReplaceFlag(const FLxString& Value);

	/**
	 * @var FString InternalString
	 * @brief 内部字符串
	 *
	 * 存储实际的字符串数据。
	 */
	FString InternalString;

};
