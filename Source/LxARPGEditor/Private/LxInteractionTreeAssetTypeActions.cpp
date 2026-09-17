#include "LxInteractionTreeAssetTypeActions.h"
#include "LxInteractionTreeAssetEditor.h"
#include "LxARPG/LxSource/Model/Interaction/DataType/LxInteractionTreeAsset.h"

UClass* FLxInteractionTreeAssetTypeActions::GetSupportedClass() const
{
	return ULxInteractionTreeAsset::StaticClass();
}

void FLxInteractionTreeAssetTypeActions::OpenAssetEditor(const TArray<UObject*>& Objects, TSharedPtr<IToolkitHost> Host)
{
	for (UObject* Object : Objects)
	{
		if (ULxInteractionTreeAsset* Asset = Cast<ULxInteractionTreeAsset>(Object))
		{
			MakeShared<FLxInteractionTreeAssetEditor>()->Init(Host.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone, Host, Asset);
		}
	}
}

