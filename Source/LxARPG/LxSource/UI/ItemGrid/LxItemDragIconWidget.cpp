#include "LxItemDragIconWidget.h"

#include "Components/Image.h"

void ULxItemDragIconWidget::SetIcon(UTexture2D* IconTexture)
{
	if (IconImage && IconTexture)
	{
		IconImage->SetBrushFromTexture(IconTexture);
	}
}

void ULxItemDragIconWidget::SetIcon(FString IconPath)
{
	UTexture2D* IconTexture = LoadObject<UTexture2D>(nullptr, *IconPath);
	SetIcon(IconTexture);
}
