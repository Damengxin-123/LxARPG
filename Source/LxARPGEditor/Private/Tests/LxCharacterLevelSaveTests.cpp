#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "LxARPG/LxSource/Systems/MainMenu/LxMainMenuSettings.h"
#include "LxARPG/LxSource/Systems/MainMenu/LxMenuPreviewActor.h"
#include "LxARPG/LxSource/Systems/SaveSystem/LxCharacterPresentation.h"
#include "LxARPG/LxSource/Systems/SaveSystem/LxCharacterSaveData.h"

/** 用实际重命名关卡和角色资产验证旧路径仍保留位置并显示角色。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxCharacterLevelSaveTest,
	"LxARPG.Save.CharacterRenamedLevelPreview", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/** 不修改资产和磁盘存档，只在独立世界检查关卡规范路径与展示组件。 */
bool FLxCharacterLevelSaveTest::RunTest(const FString& Parameters)
{
	const ULxMainMenuSettings* Settings = GetDefault<ULxMainMenuSettings>();
	UWorld* ConfiguredWorld = Settings->DefaultLevel.LoadSynchronous();
	if (!TestNotNull(TEXT("配置关卡及重定向可加载"), ConfiguredWorld)) return false;
	const FSoftObjectPath Canonical = LxCharacterPresentation::GetLevelPath(ConfiguredWorld);
	TestTrue(TEXT("旧配置软路径对应实际规范关卡"), LxCharacterPresentation::IsInLevel(Settings->DefaultLevel.ToSoftObjectPath(), ConfiguredWorld));
	TestTrue(TEXT("规范关卡路径直接匹配"), LxCharacterPresentation::IsInLevel(Canonical, ConfiguredWorld));
	TestFalse(TEXT("空路径不代表已有位置"), LxCharacterPresentation::IsInLevel(FSoftObjectPath(), ConfiguredWorld));
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	ON_SCOPE_EXIT { World->DestroyWorld(false); };
	ALxMenuPreviewActor* Preview = World->SpawnActor<ALxMenuPreviewActor>();
	FLxCharacterSaveRecord Record;
	Record.CharacterClass = Settings->DefaultCharacter.ToSoftObjectPath();
	Record.bHasSavedTransform = true;
	// 对外用米描述，传给引擎世界变换时转换为厘米。
	const FVector SavedPositionMeters(12.50f, -8.75f, 2.00f);
	Record.SavedTransform = FTransform(FRotator(0, 35, 0), SavedPositionMeters * 100.0f);
	if (!TestTrue(TEXT("新建角色可以从蓝图默认网格配置预览"), Preview->Configure(Record, FTransform::Identity))) return false;
	Preview->Reveal();
	USkeletalMeshComponent* Mesh = Preview->FindComponentByClass<USkeletalMeshComponent>();
	if (!TestNotNull(TEXT("展示对象有网格组件"), Mesh)) return false;
	TestNotNull(TEXT("整理后的玩家网格可加载"), Mesh->GetSkeletalMeshAsset());
	TestTrue(TEXT("角色网格已显示"), Mesh->IsVisible());
	TestTrue(TEXT("预览保留实际存档位置与朝向"), Preview->GetActorTransform().Equals(Record.SavedTransform));
	Record.bHasSavedTransform = false;
	const FTransform Fallback(FVector(3.00f, 5.00f, 2.00f) * 100.0f);
	TestTrue(TEXT("无位置角色可以使用出生点预览"), Preview->Configure(Record, Fallback));
	TestTrue(TEXT("无位置角色使用指定出生点"), Preview->GetActorTransform().Equals(Fallback));
	return true;
}

#endif
