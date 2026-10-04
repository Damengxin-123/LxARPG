#include "LxHarborLandscapeLibrary.h"

#include "Editor.h"
#include "Engine/World.h"
#include "Landscape.h"
#include "LandscapeComponent.h"
#include "LandscapeEditLayer.h"
#include "LandscapeInfo.h"
#include "Materials/MaterialInterface.h"
#include "Misc/FileHelper.h"

AActor* ULxHarborLandscapeLibrary::CreateHarborLandscape(const FString& HeightmapFilename, UMaterialInterface* TerrainMaterial)
{
	// 固定目标世界与高度图规格，避免把半岛数据导入其他正在编辑的地图。
	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!World || World->GetOutermost()->GetName() != TEXT("/Game/项目内容/关卡/海港城/白石海港城") || !TerrainMaterial)
	{
		UE_LOG(LogTemp, Error, TEXT("海港地形：请先打开白石海港城并提供地形材质。"));
		return nullptr;
	}
	constexpr int32 Resolution = 1009;
	TArray<uint8> RawData;
	if (!FFileHelper::LoadFileToArray(RawData, *HeightmapFilename) || RawData.Num() != Resolution * Resolution * 2)
	{
		UE_LOG(LogTemp, Error, TEXT("海港地形：高度图必须是 1009×1009 的小端 R16 文件。"));
		return nullptr;
	}
	TArray<uint16> Heights;
	Heights.SetNumUninitialized(Resolution * Resolution);
	for (int32 Index = 0; Index < Heights.Num(); ++Index)
	{
		Heights[Index] = static_cast<uint16>(RawData[Index * 2]) | (static_cast<uint16>(RawData[Index * 2 + 1]) << 8);
	}

	// 每个顶点间距 1.5 米，64 个组件各有 2×2 个 63 四边形分区。
	ALandscape* Landscape = World->SpawnActor<ALandscape>(FVector(-75600.0, -60600.0, 0.0), FRotator::ZeroRotator);
	if (!Landscape)
	{
		return nullptr;
	}
	Landscape->SetActorLabel(TEXT("半岛地形"));
	Landscape->SetFolderPath(FName(TEXT("自然地形")));
	Landscape->Tags.Add(FName(TEXT("白石海港城_原生地形")));
	// UE 5.8 地形始终使用编辑层，无需旧版启用标记。
	Landscape->LandscapeMaterial = TerrainMaterial;
	Landscape->SetActorScale3D(FVector(150.0, 150.0, 100.0));
	Landscape->StaticLightingLOD = 0;
	TMap<FGuid, TArray<uint16>> HeightLayers;
	HeightLayers.Add(FGuid(), MoveTemp(Heights));
	TMap<FGuid, TArray<FLandscapeImportLayerInfo>> MaterialLayers;
	MaterialLayers.Add(FGuid(), TArray<FLandscapeImportLayerInfo>());
	Landscape->Import(FGuid::NewGuid(), 0, 0, Resolution - 1, Resolution - 1, 2, 63,
		HeightLayers, *HeightmapFilename, MaterialLayers, ELandscapeImportAlphamapType::Additive,
		TArrayView<const FLandscapeLayer>());
	if (Landscape->LandscapeComponents.Num() != 64 || !Landscape->GetLandscapeInfo())
	{
		World->DestroyActor(Landscape);
		UE_LOG(LogTemp, Error, TEXT("海港地形：地形组件创建不完整，已撤销新地形。"));
		return nullptr;
	}
	// 获取导入后创建的基础编辑层，保留原有中文层名称。
	if (ULandscapeEditLayerBase* BaseEditLayer = Landscape->GetEditLayer(0))
	{
		BaseEditLayer->SetName(FName(TEXT("半岛基础")), true);
	}
	Landscape->GetLandscapeInfo()->UpdateLayerInfoMap(Landscape);
	Landscape->UpdateAllComponentMaterialInstances();
	Landscape->MarkPackageDirty();
	UE_LOG(LogTemp, Display, TEXT("海港地形：已生成 64 个原生地形组件。"));
	return Landscape;
}
