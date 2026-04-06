#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LxItemDragIconWidget.generated.h"

class UImage;

UCLASS()
class LXARPG_API ULxItemDragIconWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	/**
	 * @brief 直接使用纹理对象设置拖拽图标。
	 *
	 * @param IconTexture 要显示的图标纹理对象。
	 */
	void SetIcon(UTexture2D* IconTexture);

	/**
	 * @brief 通过资源路径设置拖拽图标。
	 *
	 * @param IconPath 图标资源路径字符串。
	 */
	void SetIcon(FString IconPath);

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> IconImage;
};
