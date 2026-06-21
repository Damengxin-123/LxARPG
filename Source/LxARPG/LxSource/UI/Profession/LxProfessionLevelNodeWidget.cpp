#include "LxProfessionLevelNodeWidget.h"

#include "LxProfessionUIData.h"

void ULxProfessionLevelNodeWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	ProfessionLevelNodeData = Cast<ULxProfessionLevelNodeUIData>(ListItemObject);
	OnProfessionLevelNodeUpdated(ProfessionLevelNodeData);
}
