#include "LxProfessionTreeAssetTypeActions.h"
#include "LxProfessionTreeAssetEditor.h"

UClass* FLxProfessionTreeAssetTypeActions::GetSupportedClass() const
{
	return ULxProfessionTreeAsset::StaticClass();
}

void FLxProfessionTreeAssetTypeActions::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> Host)
{
	for (UObject* Object : InObjects)
		if (auto* Asset = Cast<ULxProfessionTreeAsset>(Object))
			MakeShared<FLxProfessionTreeAssetEditor>()->Init(Host ? EToolkitMode::WorldCentric : EToolkitMode::Standalone, Host, Asset);
}
