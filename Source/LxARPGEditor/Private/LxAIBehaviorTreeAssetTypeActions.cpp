#include "LxAIBehaviorTreeAssetTypeActions.h"

#include "LxAIBehaviorTreeAssetEditor.h"
#include "LxARPG/LxSource/Model/AI/DataType/LxAIBehaviorTreeAsset.h"

UClass* FLxAIBehaviorTreeAssetTypeActions::GetSupportedClass() const
{
	return ULxAIBehaviorTreeAsset::StaticClass();
}

void FLxAIBehaviorTreeAssetTypeActions::OpenAssetEditor(const TArray<UObject*>& Objects, TSharedPtr<IToolkitHost> Host)
{
	for (UObject* Object : Objects)
	{
		if (ULxAIBehaviorTreeAsset* Asset = Cast<ULxAIBehaviorTreeAsset>(Object))
		{
			MakeShared<FLxAIBehaviorTreeAssetEditor>()->Init(
				Host.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone, Host, Asset);
		}
	}
}
