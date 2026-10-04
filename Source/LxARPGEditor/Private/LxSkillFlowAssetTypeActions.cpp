#include "LxSkillFlowAssetTypeActions.h"
#include "LxSkillFlowAssetEditor.h"
#include "LxARPG/LxSource/Model/Skill/DataType/LxSkillFlowAsset.h"

UClass* FLxSkillFlowAssetTypeActions::GetSupportedClass() const
{
	return ULxSkillFlowAsset::StaticClass();
}

void FLxSkillFlowAssetTypeActions::OpenAssetEditor(const TArray<UObject*>& Objects, TSharedPtr<IToolkitHost> Host)
{
	for (UObject* Object : Objects)
	{
		if (ULxSkillFlowAsset* Asset = Cast<ULxSkillFlowAsset>(Object))
		{
			MakeShared<FLxSkillFlowAssetEditor>()->Init(Host.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone, Host, Asset);
		}
	}
}
