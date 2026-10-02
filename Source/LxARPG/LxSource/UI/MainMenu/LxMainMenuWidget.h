#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LxMainMenuWidget.generated.h"

/** 原生主菜单界面，可作为中文命名的控件蓝图父类。 */
UCLASS(Blueprintable, DisplayName="主菜单界面")
class LXARPG_API ULxMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()
protected:
	/** 构造自适应布局的功能按钮、角色选择与地图存档面板。 */
	virtual TSharedRef<SWidget> RebuildWidget() override;
};
