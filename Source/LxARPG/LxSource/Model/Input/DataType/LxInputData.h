// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LxInputEnum.h"
#include "EnhancedInput/Public/InputActionValue.h"
#include "LxARPG/LxSource/Core/Database/LxTableRowBase.h"
#include "LxInputData.generated.h"


USTRUCT(BlueprintType, DisplayName="输入行为信息")
struct FLxInputActionInfo : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="输入行为信息", DisplayName="输入行为ID")
	FName InputActionID;
	
	
	// 输入行为的可视化名称，用于在UI设置界面中显示，需要进行多语言化
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="输入行为信息|可视化名称", DisplayName="可视化名称")
	FText DisplayName;

	// 输入行为会传递的值的类型
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="输入行为信息|值的类型",DisplayName="值的类型")
	EInputActionValueType ValueType = EInputActionValueType::Boolean;

	// 输入行为输入的方式
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="输入行为信息|输入的方式",DisplayName="输入的方式")
	ELxInputInteractionType InteractionType = ELxInputInteractionType::Continuous;

	// 输入行为的默认按键 仅包含单方向按键，例如鼠标滚轮只支持上或者下滚动
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="输入行为信息|默认按键",DisplayName="默认按键")
	FKey DefaultKey;

	// 输入的值的方向，当传递的值用于向量时，需要指名此值所作用的方向，如果无需方向，则使用默认值即可
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="输入行为信息|值轴向",DisplayName="输入的值轴向")
	ELxInputValueAxial ValueDirection = ELxInputValueAxial::None;

	// 输入行为的值的缩放比例，指在输入值之后，会被放大此属性的倍数
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="输入行为信息|值的缩放比例",DisplayName="输入的值的缩放比例")
	float ValueMagnification = 0;
};

/**
 * @struct FLxInputValue
 * @brief 输入值结构体
 *
 * 该结构体用于存储和处理输入操作的值。它作为输入系统的一部分，支持不同类型的数据输入，如布尔值、一维轴、二维轴和三维轴等。
 * 通过这个结构体，可以方便地管理和传递输入数据。
 *
 * @note 可在蓝图中使用
 */
USTRUCT(BlueprintType)
struct FLxInputValue
{
	GENERATED_BODY()
	/**
	 * @var bool m_blValue
	 * @brief 存储布尔类型的输入值
	 *
	 * 该变量用于保存布尔型的输入状态，通常表示开/关或真/假的状态。
	 * 适用于需要二进制逻辑判断的输入场景。
	 */
	bool m_blValue = false;;

	/**
	 * @var float m_nVector1D
	 * @brief 存储一维向量的输入值
	 *
	 * 该变量用于保存一维方向上的数值，通常表示一个单一维度上的位置或强度。
	 * 适用于需要在一维空间内进行定位或度量的输入场景。
	 */
	float m_nVector1D = 0;

	/**
	 * @var FVector2D m_sVector2D
	 * @brief 存储二维向量的输入值
	 *
	 * 该变量用于保存二维平面上的坐标点或方向，由两个浮点数表示X和Y轴上的位置。
	 * 适用于需要在二维空间内进行定位、移动或度量的输入场景。
	 */
	FVector2D m_sVector2D = {0, 0};

	/**
	 * @var FVector m_sVector3D
	 * @brief 存储三维向量的输入值
	 *
	 * 该变量用于保存三维空间中的坐标点或方向，由三个浮点数表示X、Y和Z轴上的位置。
	 * 适用于需要在三维空间内进行定位、移动或度量的输入场景。
	 */
	FVector m_sVector3D = {0, 0, 0};
	/**
	 * @brief 默认构造函数
	 */
	FLxInputValue(){}

	/**
	 * @brief 构造函数
	 *
	 * @param bValue 布尔值
	 */
	FLxInputValue(const bool bValue)
	{
		this->m_blValue = bValue;
	}

	/**
	 * @brief 构造函数
	 *
	 * @param nVector1D 一维向量值
	 */
	FLxInputValue(const float nVector1D)
	{
		this->m_nVector1D = nVector1D;
	}

	/**
	 * @brief 构造函数
	 *
	 * @param sVector2D 二维向量值
	 */
	FLxInputValue(const FVector2D& sVector2D)
	{
		this->m_sVector2D = sVector2D;
	}

	/**
	 * @brief 构造函数
	 *
	 * @param sVector3D 三维向量值
	 */
	FLxInputValue(const FVector& sVector3D)
	{
		this->m_sVector3D = sVector3D;
	}

	/**
	 * @brief 构造函数
	 *
	 * @param bValue 布尔值
	 * @param nVector1D 一维向量值
	 * @param sVector2D 二维向量值
	 * @param sVector3D 三维向量值
	 */
	FLxInputValue(const bool bValue, const float nVector1D, const FVector2D& sVector2D, const FVector& sVector3D)
	{
		this->m_blValue = bValue;
		this->m_nVector1D = nVector1D;
		this->m_sVector2D = sVector2D;
		this->m_sVector3D = sVector3D;

	}

	
};

/**
 * @struct FLxInputKeySetting
 * @brief 玩家输入行为配置
 *
 * 在玩家设置了非默认的输入键值之后，配置会被保存到本地存档中，
 * 在初始化输入行为之前，会使用本地配置覆盖默认配置
 * @note 
 */
USTRUCT()
struct FLxInputKeySetting
{
	GENERATED_BODY()

	/**
	 * @var FName ActionID
	 * @brief 输入行为的ID
	 */
	UPROPERTY()
	FName InputActionID;

	/**
	 * @var FKey LocalSettingKey
	 * @brief 本地存档配置的键值
	 */
	UPROPERTY()
	FKey LocalSettingKey;
};


