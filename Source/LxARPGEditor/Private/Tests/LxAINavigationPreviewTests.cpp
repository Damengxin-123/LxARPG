#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/App.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"
#include "Editor.h"
#include "Engine/World.h"
#include "Engine/Level.h"
#include "Components/SplineComponent.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "LevelEditorViewport.h"
#include "UnrealClient.h"
#include "ImageUtils.h"
#include "ImageCore.h"
#include "SEditorViewport.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWindow.h"
#include "LxARPG/LxSource/World/AINavigation/LxAIRouteActor.h"
#include "LxARPG/LxSource/World/AINavigation/LxAIPointActor.h"

/** 在独立离屏编辑器进程中观察未选中的路线和点位；不保存或修改用户地图。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxAINavigationEditorPreviewTest, "LxARPG.AINavigation.EditorPreview",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLxAINavigationEditorPreviewTest::RunTest(const FString& Parameters)
{
	if (!FParse::Param(FCommandLine::Get(), TEXT("SmokeAINavigationPreview")))
	{
		AddInfo(TEXT("未请求编辑态截图；仅在独立进程追加 -SmokeAINavigationPreview 时进行视觉验收。"));
		return true;
	}
	if (!FApp::IsUnattended() || !FApp::CanEverRender())
	{
		AddError(TEXT("导航视觉测试必须在启用渲染的独立 unattended 编辑器进程中运行。"));
		return false;
	}
	UWorld* World = GEditor->NewMap();
	if (!TestNotNull(TEXT("创建未保存的隔离编辑地图"), World)) return false;
	FLevelEditorViewportClient* ViewClient = nullptr;
	const FViewport* ActiveViewport = GEditor->GetActiveViewport();
	for (FLevelEditorViewportClient* Candidate : GEditor->GetLevelViewportClients())
	{
		if (!Candidate || !Candidate->Viewport || !Candidate->GetEditorViewportWidget().IsValid()) continue;
		if (!ViewClient) ViewClient = Candidate;
		if (Candidate->Viewport == ActiveViewport) { ViewClient = Candidate; break; }
	}
	if (!TestNotNull(TEXT("真实Level编辑视口可用"), ViewClient)) return false;
	const TSharedPtr<SEditorViewport> ViewWidget = ViewClient->GetEditorViewportWidget();
	if (TSharedPtr<SWindow> Window = FSlateApplication::Get().FindWidgetWindow(ViewWidget.ToSharedRef()))
		Window->Resize(FVector2D(1600, 1000));
	ALxAIRouteActor* Route = World->SpawnActor<ALxAIRouteActor>();
	ALxAIPointActor* Point = World->SpawnActor<ALxAIPointActor>();
	if (!Route || !Point)
	{
		if (Route) Route->Destroy();
		if (Point) Point->Destroy();
		AddError(TEXT("无法创建导航预览对象"));
		return false;
	}
	// 统一在世界 XY 平面布置：左侧青色折线路线，右侧橙色十五米球形范围。
	Route->SetActorLocation(FVector(-3500.0, 0.0, 0.0));
	Route->RouteSpline->ClearSplinePoints(false);
	for (const FVector& PointLocation : {FVector(0, -1200, 0), FVector(800, 800, 0), FVector(1800, -500, 0), FVector(2200, 1100, 0)})
		Route->RouteSpline->AddSplinePoint(PointLocation, ESplineCoordinateSpace::Local, false);
	Route->RouteSpline->UpdateSpline();
	Point->SetActorLocation(FVector(1800, 0, 0));
	Point->RangeRadiusMeters = 15.0f;
	Point->RangeColor = FColor(255, 160, 20);
	Point->SetActorScale3D(FVector(3.0, 0.5, 2.0));
	Point->OnConstruction(Point->GetActorTransform());
	// 保存真实地图再加载到独立包，证明预览能够从持久点位恢复，而非只在首次构造时存在。
	const FString TestDirectory = FPaths::ProjectSavedDir() / TEXT("Tests");
	IFileManager::Get().MakeDirectory(*TestDirectory, true);
	const FString MapFile = TestDirectory / TEXT("AI导航预览保存测试.umap");
	World->SetFlags(RF_Public | RF_Standalone);
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	if (TestTrue(TEXT("将导航对象保存到独立测试地图"), UPackage::SavePackage(World->GetOutermost(), World, *MapFile, SaveArgs)))
	{
		const FString LoadName = TEXT("/Temp/AI导航地图重载_") + FGuid::NewGuid().ToString(EGuidFormats::Digits);
		UPackage* LoadedPackage = LoadPackage(CreatePackage(*LoadName), *MapFile, LOAD_None);
		UWorld* LoadedWorld = LoadedPackage ? FindObject<UWorld>(LoadedPackage, *World->GetName()) : nullptr;
		ALxAIRouteActor* LoadedRoute = nullptr;
		if (LoadedWorld && LoadedWorld->PersistentLevel)
			for (AActor* Actor : LoadedWorld->PersistentLevel->Actors)
				if (ALxAIRouteActor* Candidate = Cast<ALxAIRouteActor>(Actor)) { LoadedRoute = Candidate; break; }
		if (TestNotNull(TEXT("从磁盘地图恢复路线Actor"), LoadedRoute))
		{
			TestEqual(TEXT("地图重载保留样条路线点"), LoadedRoute->RouteSpline->GetNumberOfSplinePoints(), 4);
		}
	}
	GEditor->SelectNone(true, true, false);
	ViewClient->SetViewportType(LVT_OrthoTop);
	ViewClient->SetViewLocation(FVector::ZeroVector);
	ViewClient->SetOrthoZoom(80000.0f);
	ViewClient->Invalidate();
	const TWeakObjectPtr<ALxAIRouteActor> WeakRoute = Route;
	const TWeakObjectPtr<ALxAIPointActor> WeakPoint = Point;
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ViewClient, ViewWidget, WeakRoute, WeakPoint]()
	{
		TArray<FColor> Pixels;
		FIntVector Size;
		ViewClient->Invalidate();
		// 离屏Slate视口可能直接绘制到窗口，没有供ReadPixels读取的独立纹理；捕获实际视口控件。
		if (TestTrue(TEXT("读取未选中导航对象的真实编辑视口"), FSlateApplication::Get().TakeScreenshot(ViewWidget.ToSharedRef(), Pixels, Size)))
		{
			TArray64<uint8> Png;
			const FImageView Image(Pixels.GetData(), Size.X, Size.Y, ERawImageFormat::BGRA8);
			const FString Directory = FPaths::ProjectSavedDir() / TEXT("Tests");
			IFileManager::Get().MakeDirectory(*Directory, true);
			if (TestTrue(TEXT("将导航视口截图编码为PNG"), FImageUtils::CompressImage(Png, TEXT("png"), Image)))
				TestTrue(TEXT("保存导航预览视觉证据"), FFileHelper::SaveArrayToFile(Png, *(Directory / TEXT("AI导航预览.png"))));
		}
		if (WeakRoute.IsValid()) WeakRoute->Destroy();
		if (WeakPoint.IsValid()) WeakPoint->Destroy();
	}, 1.0f));
	return true;
}

#endif
