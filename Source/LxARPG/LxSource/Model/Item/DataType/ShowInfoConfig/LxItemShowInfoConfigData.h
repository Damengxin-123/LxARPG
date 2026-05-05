// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LxItemShowInfoConfigData.generated.h"


/**
 * @brief 物品可视化信息配置数据结构
 * 该结构体继承自FTableRowBase，用于存储和管理物品在游戏中的可视化信息。它通过提供可编辑的属性来支持直接在编辑器中配置或在游戏中动态修改这些信息。
 * 此结构体旨在提高物品展示的灵活性与性能，特别适用于需要频繁更新或高度定制化显示效果的游戏场景。
 */
USTRUCT(BlueprintType, DisplayName="物品可视化信息配置类型")
struct FLxLxItemShowInfoConfigData : public FTableRowBase
{
	GENERATED_BODY()


	/**
	 * @brief 物品的可视化名称
	 * 该属性用于存储和显示物品在游戏中的名称。通过设置此属性，可以自定义物品在用户界面或其他可视化组件中展示的名字。
	 * 支持在编辑器中直接配置或在游戏中通过蓝图动态修改，以适应不同的显示需求。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="可视化名称")
	FText ItemName;

	/**
	 * @brief 物品的可视化描述
	 * 该属性用于存储和显示物品在游戏中的详细描述。通过设置此属性，可以自定义物品在用户界面或其他可视化组件中展示的详细信息。
	 * 支持在编辑器中直接配置或在游戏中通过蓝图动态修改，以适应不同的显示需求。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="可视化描述")
	FText ItemDescription;
	
	/**
	 * @brief 图像引用 * 该属性用于存储和引用UTexture2D类型的图像资源。通过使用TSoftObjectPtr，可以在不立即加载资源的情况下保持对图像的引用，
	 * 从而提高性能并允许在需要时动态加载纹理。
	 * 该属性可编辑且支持蓝图读写操作，便于在编辑器中直接配置或在游戏中动态修改图像引用。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="可视化图像引用")
	TSoftObjectPtr<UTexture2D> ImageQuote;
};
