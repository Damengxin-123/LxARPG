// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Model/Input/DataType/LxInputData.h"
#include "Blueprint/UserWidget.h"
#include "LxUIBaseObject.generated.h"

class ALxBaseCharacter;
class ALxPlayerController;

/**
 * @brief 所有 UI 对象的基础类
 */
UCLASS()
class LXARPG_API ULxUIBaseObject : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 初始化 UI 组件。 */
	virtual void InitializeUIComponents(){};

	/** 重置 UI 组件。 */
	virtual void ResetUIComponents(){};

	/** 根据当前玩家角色刷新 UI。 */
	virtual void UpdateUIComponents(ALxBaseCharacter* PlayerCharacter);

	/** 更新当前玩家控制器引用。 */
	virtual void UpdatePlayerController(ALxPlayerController* PlayerController);

	/** 构造时自动调用初始化函数。 */
	virtual void NativeConstruct() override;

	/** 处理输入事件，默认不拦截。 */
	virtual bool HandleInputEvent(FName InputID, const FLxInputValue& Value){ return false; };

	UPROPERTY()
	TObjectPtr<ALxBaseCharacter> m_pPlayerCharacter;

	UPROPERTY()
	TObjectPtr<ALxPlayerController> m_pPlayerController;
};
