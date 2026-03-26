/**
 * @file LxUITextData.h
 * @brief UI文本数据类定义
 *
 * 此文件定义了UI文本数据类ULxUITextData，用于管理UI文本显示相关的数据。
 */

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once


#include "CoreMinimal.h"
#include "LxARPG/LxSource/Model/Attribute/DataType/LxAttributeData.h"
#include "LxARPG/LxSource/Model/Item/DataType/Entry/LxItemEntryData.h"
#include "UObject/Object.h"
#include "LxUITextData.generated.h"


/**
 * @class ULxUITextData
 * @brief UI文本数据类
 *
 * 用于为使用TileView类型的控件提供数据。
 *
 * @note 继承自UObject
 */
UCLASS()
class ULxUITextData : public UObject
{
	GENERATED_BODY()

public:

	/**
	 * @var FLxAttributeSet* m_pCharacterAttributeDataPtr
	 * @brief 角色属性指针
	 */
	FLxAttributeSet* m_pCharacterAttributeDataPtr = nullptr;

	/**
	 * @var const FLxItemEntry* m_pItemEntryDataPtr
	 * @brief 物品词条指针
	 */
	const FLxItemEntry* m_pItemEntryDataPtr = nullptr;

	/**
	 * @var FString m_Title
	 * @brief 标题字符串
	 */
	FString m_Title;

	/**
	 * @var FString m_strChatText
	 * @brief 聊天信息
	 */
	FString m_strChatText;

	/**
	 * @var bool m_bIsDarkColor
	 * @brief 文本框背景显示颜色是否为深色
	 *
	 * 用于在有多条文本显示时区分显示。
	 */
	bool m_bIsDarkColor = false;

};
