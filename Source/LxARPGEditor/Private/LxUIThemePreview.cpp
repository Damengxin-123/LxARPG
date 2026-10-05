#include "LxUIThemePreview.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/SizeBox.h"
#include "Engine/TextureRenderTarget2D.h"
#include "EngineFontServices.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/FileManager.h"
#include "ImageUtils.h"
#include "Interfaces/ISlateRHIRendererModule.h"
#include "LxARPG/LxSource/UI/ItemGrid/LxItemGridWidget.h"
#include "Misc/App.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "Modules/ModuleManager.h"
#include "Slate/WidgetRenderer.h"
#include "Styling/CoreStyle.h"
#include "TextureCompiler.h"
#include "TextureResource.h"
#include "UObject/StrongObjectPtr.h"
#include "WidgetBlueprint.h"
#include "WidgetBlueprintEditorUtils.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SDPIScaler.h"

/** 独立收纳设计器预览助手，避免与其他编辑器工具发生合并编译名称冲突。 */
namespace LxUIThemePreviewPrivate
{
	/** 复用或初始化离屏 Slate 与字体服务，整个过程不创建桌面窗口。 */
	bool EnsurePreviewRendering()
	{
		if (!FApp::CanEverRender())
		{
			UE_LOG(LogTemp, Error, TEXT("界面预览需要 -AllowCommandletRendering，且不能启用 -NullRHI。"));
			return false;
		}
		if (!FSlateApplication::IsInitialized()) FSlateApplication::Create();
		if (!FSlateApplication::Get().GetRenderer())
		{
			ISlateRHIRendererModule* Module = FModuleManager::LoadModulePtr<ISlateRHIRendererModule>(TEXT("SlateRHIRenderer"));
			if (!Module || !FSlateApplication::Get().InitializeRenderer(Module->CreateSlateRHIRenderer(), true))
			{
				UE_LOG(LogTemp, Error, TEXT("无法初始化界面预览的离屏 Slate 渲染器。"));
				return false;
			}
		}
		if (!FEngineFontServices::IsInitialized()) FEngineFontServices::Create();
		return true;
	}

	/** 启用设计时预构造以应用子控件尺寸与标题参数，同时禁用游戏绘制和逐帧事件。 */
	void PrepareDesignWidget(UUserWidget* Widget)
	{
		Widget->SetDesignerFlags(EWidgetDesignFlags::Designing | EWidgetDesignFlags::Previewing | EWidgetDesignFlags::ExecutePreConstruct);
		Widget->bHasScriptImplementedTick = false;
		Widget->bHasScriptImplementedPaint = false;
	}

	/** 通过真实数据与选中回调准备临时物品格子，仅改变预览实例的尺寸，不写入蓝图或存档。 */
	bool PrepareSelectedItemGrid(UUserWidget* Widget)
	{
		ULxItemGridWidget* ItemGrid = Cast<ULxItemGridWidget>(Widget);
		UBorder* Selection = Widget && Widget->WidgetTree
			? Cast<UBorder>(Widget->WidgetTree->FindWidget(TEXT("选中效果"))) : nullptr;
		USizeBox* RootSize = Widget && Widget->WidgetTree
			? Cast<USizeBox>(Widget->WidgetTree->RootWidget) : nullptr;
		if (!ItemGrid || !Selection || !RootSize)
		{
			UE_LOG(LogTemp, Error, TEXT("选中预览需要包含选中效果边框和尺寸框根节点的物品格子。"));
			return false;
		}

		// 公开的列表数据入口会调用 InitItemData(nullptr)，以空槽位刷新图标、数量和稀有度背景。
		// 不构造实际物品或槽位，也不创建 World、玩家对象与存档管理器。
		ItemGrid->NativeOnListItemObjectSet(nullptr);
		ItemGrid->SetShortcutSelected(true);
		// 不直接设置可见性；由蓝图的选中状态更新事件显示边框，确保图片来自真实状态回调。
		if (!ItemGrid->IsShortcutSelected() || !Selection->IsVisible())
		{
			UE_LOG(LogTemp, Error, TEXT("物品格子选中回调未显示选中效果：%s"), *Widget->GetClass()->GetPathName());
			return false;
		}
		RootSize->SetWidthOverride(96.0f);
		RootSize->SetHeightOverride(96.0f);
		RootSize->SetMinDesiredWidth(96.0f);
		RootSize->SetMaxDesiredWidth(96.0f);
		RootSize->SetMinDesiredHeight(96.0f);
		RootSize->SetMaxDesiredHeight(96.0f);
		return true;
	}

	/** 处理空的期望尺寸及无效编辑器尺寸，避免创建零尺寸渲染目标。 */
	FVector2D ValidatePreviewSize(const FVector2D& Size, const FVector2D& Fallback)
	{
		return FVector2D(
			FMath::IsFinite(Size.X) && Size.X > 0.0 ? Size.X : Fallback.X,
			FMath::IsFinite(Size.Y) && Size.Y > 0.0 ? Size.Y : Fallback.Y);
	}
}

/** 按编辑器预览尺寸和设计时预构造结果绘制布局，并保存深色背景的界面核对图片。 */
bool LxUITheme::RenderPreview(UWidgetBlueprint* Blueprint, const FString& Directory, bool bSelectedItemGrid)
{
	using namespace LxUIThemePreviewPrivate;
	if (!Blueprint || !Blueprint->GeneratedClass || !Blueprint->GeneratedClass->IsChildOf(UUserWidget::StaticClass())
		|| Blueprint->GeneratedClass->HasAnyClassFlags(CLASS_Abstract))
	{
		UE_LOG(LogTemp, Error, TEXT("界面预览需要已经编译成功的具体控件蓝图。"));
		return false;
	}
	if (!EnsurePreviewRendering()) return false;
	if (!IFileManager::Get().MakeDirectory(*Directory, true))
	{
		UE_LOG(LogTemp, Error, TEXT("无法创建界面预览目录：%s"), *Directory);
		return false;
	}

	TStrongObjectPtr<UUserWidget> Preview(NewObject<UUserWidget>(GetTransientPackage(), Blueprint->GeneratedClass, NAME_None, RF_Transient));
	// 必须先设置设计器标记再初始化；嵌套控件继承标记，预构造收到 IsDesignTime=true。
	// 临时实例不创建 World、GameInstance 或存档管理器，不执行游戏初始化、Construct 或 Tick。
	PrepareDesignWidget(Preview.Get());
	Preview->Initialize();
	ON_SCOPE_EXIT { Preview->ReleaseSlateResources(true); };
	if (!Preview->WidgetTree || !Preview->WidgetTree->RootWidget)
	{
		UE_LOG(LogTemp, Error, TEXT("控件蓝图没有可预览的设计器布局：%s"), *Blueprint->GetPathName());
		return false;
	}
	Preview->WidgetTree->ForEachWidgetAndDescendants([](UWidget* Widget)
	{
		if (UUserWidget* Child = Cast<UUserWidget>(Widget)) PrepareDesignWidget(Child);
	});
	const TSharedRef<SWidget> Content = Preview->TakeWidget();
	if (bSelectedItemGrid && !PrepareSelectedItemGrid(Preview.Get())) return false;
	Content->SlatePrepass();
	const FVector2D ScreenSize(1280.0, 720.0);
	const FVector2D DesiredSize = ValidatePreviewSize(Content->GetDesiredSize(), ScreenSize);
	const TTuple<FVector2D, FVector2D> AreaAndSize = FWidgetBlueprintEditorUtils::GetWidgetPreviewAreaAndSize(
		Preview.Get(), DesiredSize, ScreenSize, Preview->DesignSizeMode, TOptional<FVector2D>());
	const FVector2D Area = bSelectedItemGrid ? FVector2D(96.0, 96.0) : ValidatePreviewSize(AreaAndSize.Get<0>(), ScreenSize);
	const FVector2D WidgetSize = bSelectedItemGrid ? Area : ValidatePreviewSize(AreaAndSize.Get<1>(), Area);
	// 选中格子以实际像素一比一输出，避免 DPI 缩放放大边框亮度或粗细。
	const float DPIScale = bSelectedItemGrid ? 1.0f
		: FMath::Max(0.01f, FWidgetBlueprintEditorUtils::GetWidgetPreviewDPIScale(Preview.Get(), ScreenSize));
	// 大型布局等比缩小到最多 4096 像素，保留完整设计器画布，避免直接截断控件。
	const double OutputScale = FMath::Min(1.0, 4096.0 / FMath::Max(Area.X, Area.Y));
	const FIntPoint ImageSize(FMath::Max(1, FMath::CeilToInt(Area.X * OutputScale)),
		FMath::Max(1, FMath::CeilToInt(Area.Y * OutputScale)));
	const TSharedRef<SWidget> Layout = SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
		.BorderBackgroundColor(FLinearColor(0.009f, 0.013f, 0.022f, 1.0f))
		.Padding(0.0f).HAlign(HAlign_Center).VAlign(VAlign_Center)
		[
			SNew(SDPIScaler).DPIScale(DPIScale * OutputScale)
			[
				SNew(SBox).WidthOverride(WidgetSize.X / DPIScale).HeightOverride(WidgetSize.Y / DPIScale)
				[
					Content
				]
			]
		];

	// 所有嵌套控件构建完毕后统一完成纹理编译，确保新载入的风格图片也进入预览。
	FTextureCompilingManager::Get().FinishAllCompilation();
	FWidgetRenderer Renderer(true, true);
	Renderer.SetApplyColorDeficiencyCorrection(false);
	// Slate 已执行颜色校正，使用线性写入的八位目标避免目标纹理再次做 sRGB 编码。
	// 与主菜单图像烘焙共用同样的颜色路径，确保深蓝面板不会在预览中变成浅灰色。
	TStrongObjectPtr<UTextureRenderTarget2D> Target(NewObject<UTextureRenderTarget2D>(GetTransientPackage(), NAME_None, RF_Transient));
	Target->ClearColor = FLinearColor::Transparent;
	Target->TargetGamma = 1.0f;
	Target->InitCustomFormat(ImageSize.X, ImageSize.Y, PF_B8G8R8A8, true);
	Target->UpdateResourceImmediate(true);
	if (!Target->GameThread_GetRenderTargetResource())
	{
		UE_LOG(LogTemp, Error, TEXT("离屏绘制界面失败：%s"), *Blueprint->GetPathName());
		return false;
	}
	Renderer.DrawWidget(Target.Get(), Layout, FVector2D(ImageSize.X, ImageSize.Y), 0.0f);
	TArray<FColor> Pixels;
	FReadSurfaceDataFlags ReadFlags(RCM_MinMax);
	// Slate 已输出显示空间颜色，读取时关闭重复伽马转换，保持深色面板的实际亮度。
	ReadFlags.SetLinearToGamma(false);
	if (!Target->GameThread_GetRenderTargetResource()->ReadPixels(Pixels, ReadFlags)
		|| Pixels.Num() != ImageSize.X * ImageSize.Y)
	{
		UE_LOG(LogTemp, Error, TEXT("无法读取界面预览像素：%s"), *Blueprint->GetPathName());
		return false;
	}
	TArray64<uint8> Png;
	FImageUtils::PNGCompressImageArray(ImageSize.X, ImageSize.Y, TArrayView64<const FColor>(Pixels.GetData(), Pixels.Num()), Png);
	const FString Filename = Directory / (FPaths::MakeValidFileName(Blueprint->GetName())
		+ (bSelectedItemGrid ? TEXT("选中预览.png") : TEXT("预览.png")));
	if (Png.IsEmpty() || !FFileHelper::SaveArrayToFile(Png, *Filename))
	{
		UE_LOG(LogTemp, Error, TEXT("无法保存界面预览图片：%s"), *Filename);
		return false;
	}
	UE_LOG(LogTemp, Display, TEXT("已输出界面设计器预览：%s（%d×%d；动态列表与运行时数据不在此次预览中）"),
		*Filename, ImageSize.X, ImageSize.Y);
	return true;
}
