#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxUIBaseObject.h"
#include "LxARPG/LxSource/Model/Input/DataType/LxInputEnum.h"
#include "LxShortcutBarWidget.generated.h"

class ULxItemGridWidget;

/** 快捷栏界面，负责快捷栏格子初始化和快捷键输入绑定。 */
UCLASS(Blueprintable, BlueprintType, DisplayName="快捷栏界面")
class LXARPG_API ULxShortcutBarWidget : public ULxUIBaseObject
{
	GENERATED_BODY()

public:
	/** 绑定一个快捷栏格子和输入行为，使该格子能响应对应快捷键。 */
	UFUNCTION(BlueprintCallable, Category="快捷栏", DisplayName="绑定快捷栏格子输入")
	bool BindShortcutItemGridInput(ULxItemGridWidget* InItemGridWidget, ELxInputActionID InInputActionID);

	/** 初始化快捷栏格子的槽位数据，并注册对应输入监听。 */
	static bool SetupShortcutItemGrid(ULxItemGridWidget* InItemGridWidget, ELxInputActionID InInputActionID);
};
