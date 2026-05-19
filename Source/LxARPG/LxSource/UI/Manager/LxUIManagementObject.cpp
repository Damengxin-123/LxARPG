#include "LxUIManagementObject.h"

#include "LxARPG/LxSource/Core/Database/LxUIBaseObject.h"
#include "LxARPG/LxSource/UI/Manager/LxUIManager.h"

void ULxUIManagementObject::InitializeUIManagement(ULxUIManager* InOwningUIManager)
{
	OwningUIManager = InOwningUIManager;
}

void ULxUIManagementObject::SetPlayerController(ALxPlayerController* InPlayerController)
{
	PlayerController = InPlayerController;
}

void ULxUIManagementObject::SetCharacterDataTransferComponent(ULxCharacterDataTransferComponent* InCharacterDataTransferComponent)
{
	if (CharacterDataTransferComponent == InCharacterDataTransferComponent)
	{
		return;
	}

	CharacterDataTransferComponent = InCharacterDataTransferComponent;
	RefreshManagedUI();
}

void ULxUIManagementObject::RefreshManagedUI()
{
}

bool ULxUIManagementObject::ContainsWidget(const ULxUIBaseObject* InWidget) const
{
	return false;
}

UWorld* ULxUIManagementObject::GetWorld() const
{
	if (OwningUIManager)
	{
		return OwningUIManager->GetWorld();
	}

	if (const UObject* OuterObject = GetOuter())
	{
		return OuterObject->GetWorld();
	}

	return nullptr;
}

void ULxUIManagementObject::RefreshWidgetData(ULxUIBaseObject* InWidget) const
{
	if (InWidget)
	{
		InWidget->UpdateUIComponents(CharacterDataTransferComponent);
	}
}

bool ULxUIManagementObject::IsWidgetVisible(const ULxUIBaseObject* InWidget) const
{
	return InWidget && InWidget->GetVisibility() != ESlateVisibility::Collapsed
		&& InWidget->GetVisibility() != ESlateVisibility::Hidden;
}

void ULxUIManagementObject::SetWidgetVisibility(ULxUIBaseObject* InWidget, bool bInVisible) const
{
	if (InWidget)
	{
		InWidget->SetVisibility(bInVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}
