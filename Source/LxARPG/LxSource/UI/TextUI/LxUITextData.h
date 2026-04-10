/**
 * @file LxUITextData.h
 * @brief UI鏂囨湰鏁版嵁绫诲畾涔? *
 * 姝ゆ枃浠跺畾涔変簡UI鏂囨湰鏁版嵁绫籙LxUITextData锛岀敤浜庣鐞哢I鏂囨湰鏄剧ず鐩稿叧鐨勬暟鎹€? */

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once


#include "CoreMinimal.h"
#include "LxARPG/LxSource/Model/Attribute/DataType/LxAttributeData.h"
#include "LxARPG/LxSource/Model/Item/DataType/Entry/LxItemEntryData.h"
#include "UObject/Object.h"
#include "LxUITextData.generated.h"


/**
 * @class ULxUITextData
 * @brief UI鏂囨湰鏁版嵁绫? *
 * 鐢ㄤ簬涓轰娇鐢═ileView绫诲瀷鐨勬帶浠舵彁渚涙暟鎹€? *
 * @note 缁ф壙鑷猆Object
 */
UCLASS()
class ULxUITextData : public UObject
{
	GENERATED_BODY()

public:

	/**
	 * @var FLxAttributeData* m_pCharacterAttributeDataPtr
	 * @brief 瑙掕壊灞炴€ф寚閽?	 */
	FLxAttributeData* m_pCharacterAttributeDataPtr = nullptr;

	/**
	 * @var const FLxItemEntryData* m_pItemEntryDataPtr
	 * @brief 鐗╁搧璇嶆潯鎸囬拡
	 */
	const FLxItemEntryData* m_pItemEntryDataPtr = nullptr;

	/**
	 * @var FString m_Title
	 * @brief 鏍囬瀛楃涓?	 */
	FString m_Title;

	/**
	 * @var FString m_strChatText
	 * @brief 鑱婂ぉ淇℃伅
	 */
	FString m_strChatText;

	/**
	 * @var bool m_bIsDarkColor
	 * @brief 鏂囨湰妗嗚儗鏅樉绀洪鑹叉槸鍚︿负娣辫壊
	 *
	 * 鐢ㄤ簬鍦ㄦ湁澶氭潯鏂囨湰鏄剧ず鏃跺尯鍒嗘樉绀恒€?	 */
	bool m_bIsDarkColor = false;

};

