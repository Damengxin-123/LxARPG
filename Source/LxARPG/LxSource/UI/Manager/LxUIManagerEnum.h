#pragma once

#include "CoreMinimal.h"
#include "LxUIManagerEnum.generated.h"

UENUM(BlueprintType, DisplayName="UI功能类型")
enum class ELxUIFunctionType : uint8
{
	// 游戏的主菜单界面
	MainMenu				UMETA(DisplayName="主菜单类型"),
	// 用于角色的相关功能
	CharacterFunction		UMETA(DisplayName="角色UI功能类型"),
	// 用于角色与其他单位之间交互时的相关UI界面
	CharacterInteraction	UMETA(DisplayName="角色交互UI类型"),
	// 用于在开始游戏后持续在界面上显示的UI类型
	CharacterHUD			UMETA(DisplayName="角色HUD类型"),
	// 角色弹窗
	CHaracterPopup			UMETA(DisplayName="角色弹窗类型")
};
