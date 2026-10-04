#include "LxMainMenuArt.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "EngineFontServices.h"
#include "EditorFramework/AssetImportData.h"
#include "HAL/FileManager.h"
#include "ImageUtils.h"
#include "Interfaces/ISlateRHIRendererModule.h"
#include "Misc/App.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "Slate/WidgetRenderer.h"
#include "TextureResource.h"
#include "TextureCompiler.h"
#include "UObject/SavePackage.h"
#include "UObject/StrongObjectPtr.h"




#include "Widgets/SCompoundWidget.h"

/** 奇幻菜单装饰框的轮廓用途。 */
enum class ELxMenuArtFrameShape : uint8
{
	/** 全高菜单面板，使用矩形双边框与四角卷叶。 */
	Panel,
	/** 主菜单按钮，使用两端收角的长形轮廓。 */
	Button,
	/** 角色切换按钮，使用紧凑的盾形轮廓。 */
	Arrow,
	/** 角色脚下铭牌，使用浅折角轮廓。 */
	Nameplate
};

/** 仅供编辑器烘焙的旧版奇幻金边容器，原始几何与颜色保持不变。 */
class SLxMenuArtFrame : public SCompoundWidget
{
public:
	/** 框型、悬停高亮、背景不透明度和内部内容的声明参数。 */
	SLATE_BEGIN_ARGS(SLxMenuArtFrame)
		: _Shape(ELxMenuArtFrameShape::Panel), _Highlight(false), _FillOpacity(0.78f)
	{}
		/** 选择面板、菜单按钮、切换箭头或角色铭牌的轮廓。 */
		SLATE_ARGUMENT(ELxMenuArtFrameShape, Shape)
		/** 绑定按钮悬停或选中状态，使金边与光晕变亮。 */
		SLATE_ATTRIBUTE(bool, Highlight)
		/** 背景填充透明度，取值限制在零至一之间。 */
		SLATE_ARGUMENT(float, FillOpacity)
		/** 内部文字、图标或布局。 */
		SLATE_DEFAULT_SLOT(FArguments, Content)
	SLATE_END_ARGS()

	/** 保存绘制参数，并为非面板内容保留装饰所需的内边距。 */
	void Construct(const FArguments& Args);

protected:
	/** 在子控件后方绘制半透明轮廓、金边、高亮与角饰。 */
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
		int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	/** 保证绑定的悬停状态在缓存绘制时也能及时刷新。 */
	virtual bool ComputeVolatility() const override;

private:
	/** 当前使用的装饰轮廓。 */
	ELxMenuArtFrameShape Shape = ELxMenuArtFrameShape::Panel;
	/** 外部提供的高亮状态。 */
	TAttribute<bool> Highlight;
	/** 深蓝灰背景的不透明度。 */
	float FillOpacity = 0.78f;
};



#include "Framework/Application/SlateApplication.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderer.h"
#include "Styling/CoreStyle.h"

/** 奇幻装饰绘制的局部辅助函数。 */
namespace LxMenuArtFramePrivate
{
	/** 生成顺时针轮廓；所有形状均可从中心向边缘作扇形三角剖分。 */
	static TArray<FVector2f> MakeOutline(const FVector2f& Size, float Inset, ELxMenuArtFrameShape Shape)
	{
		const float Left = Inset;
		const float Right = Size.X - Inset;
		const float Top = Inset;
		const float Bottom = Size.Y - Inset;
		if (Shape == ELxMenuArtFrameShape::Panel)
		{
			return { {Left, Top}, {Right, Top}, {Right, Bottom}, {Left, Bottom} };
		}
		const float Height = Bottom - Top;
		const float Width = Right - Left;
		const float Cut = FMath::Min(Width * 0.22f, Height * (Shape == ELxMenuArtFrameShape::Arrow ? 0.23f : 0.18f));
		const float Shoulder = Height * 0.15f;
		const float Middle = (Top + Bottom) * 0.5f;
		return {
			{Left + Cut, Top}, {Right - Cut, Top},
			{Right - Cut * 0.70f, Top + Shoulder}, {Right, Middle},
			{Right - Cut * 0.70f, Bottom - Shoulder}, {Right - Cut, Bottom},
			{Left + Cut, Bottom}, {Left + Cut * 0.70f, Bottom - Shoulder},
			{Left, Middle}, {Left + Cut * 0.70f, Top + Shoulder}
		};
	}

	/** 通过白色画刷和顶点颜色填充真正的多边形，避免折角外出现矩形底色。 */
	static void PaintFill(FSlateWindowElementList& Elements, int32 Layer, const FGeometry& Geometry,
		const TArray<FVector2f>& Outline, const FLinearColor& Color, ESlateDrawEffect Effects)
	{
		TArray<FSlateVertex> Vertices;
		TArray<SlateIndex> Indices;
		Vertices.Reserve(Outline.Num() + 1);
		Indices.Reserve(Outline.Num() * 3);
		const FSlateRenderTransform& Transform = Geometry.GetAccumulatedRenderTransform();
		const FColor VertexColor = Color.ToFColor(true);
		Vertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(Transform,
			FVector2f(Geometry.GetLocalSize()) * 0.5f, FVector2f(0.5f, 0.5f), VertexColor));
		for (const FVector2f& Point : Outline)
		{
			Vertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(Transform,
				Point, FVector2f(0.5f, 0.5f), VertexColor));
		}
		for (int32 Index = 0; Index < Outline.Num(); ++Index)
		{
			Indices.Add(0);
			Indices.Add(static_cast<SlateIndex>(Index + 1));
			Indices.Add(static_cast<SlateIndex>((Index + 1) % Outline.Num() + 1));
		}
		const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush("WhiteBrush");
		const FSlateResourceHandle Resource = FSlateApplication::Get().GetRenderer()->GetResourceHandle(*WhiteBrush);
		FSlateDrawElement::MakeCustomVerts(Elements, Layer, Resource, Vertices, Indices, nullptr, 0, 0, Effects);
	}

	/** 绘制首尾连接的抗锯齿轮廓。 */
	static void PaintOutline(FSlateWindowElementList& Elements, int32 Layer, const FGeometry& Geometry,
		TArray<FVector2f> Points, const FLinearColor& Color, float Thickness, ESlateDrawEffect Effects)
	{
		if (Points.Num() < 2) return;
		const FVector2f FirstPoint = Points[0];
		Points.Add(FirstPoint);
		FSlateDrawElement::MakeLines(Elements, Layer, Geometry.ToPaintGeometry(), MoveTemp(Points), Effects, Color, true, Thickness);
	}

	/** 根据镜像方向把角饰局部坐标映射到面板四角。 */
	static FVector2f CornerPoint(const FVector2f& Origin, const FVector2f& Direction, const FVector2f& Point, float Scale)
	{
		return Origin + FVector2f(Direction.X * Point.X, Direction.Y * Point.Y) * Scale;
	}

	/** 绘制由两条二次曲线围成的卷叶，无需额外贴图或字体符号。 */
	static void PaintLeaf(FSlateWindowElementList& Elements, int32 Layer, const FGeometry& Geometry,
		const FVector2f& Origin, const FVector2f& Direction, float Scale,
		const FVector2f& Start, const FVector2f& Tip, const FVector2f& FirstControl,
		const FVector2f& SecondControl, const FLinearColor& Color, ESlateDrawEffect Effects)
	{
		TArray<FVector2f> Points;
		Points.Reserve(19);
		for (int32 Side = 0; Side < 2; ++Side)
		{
			const FVector2f A = Side == 0 ? Start : Tip;
			const FVector2f B = Side == 0 ? FirstControl : SecondControl;
			const FVector2f C = Side == 0 ? Tip : Start;
			for (int32 Step = 0; Step <= 8; ++Step)
			{
				const float T = Step / 8.0f;
				const FVector2f Point = A * FMath::Square(1.0f - T) + B * (2.0f * T * (1.0f - T)) + C * FMath::Square(T);
				Points.Add(CornerPoint(Origin, Direction, Point, Scale));
			}
		}
		PaintOutline(Elements, Layer, Geometry, MoveTemp(Points), Color, 1.0f, Effects);
	}

	/** 四角卷叶围绕小菱形展开，形成简洁的中世纪花饰。 */
	static void PaintCorner(FSlateWindowElementList& Elements, int32 Layer, const FGeometry& Geometry,
		const FVector2f& Origin, const FVector2f& Direction, float Scale,
		const FLinearColor& Color, ESlateDrawEffect Effects)
	{
		PaintLeaf(Elements, Layer, Geometry, Origin, Direction, Scale,
			{8, 9}, {34, 3}, {18, -5}, {28, 15}, Color, Effects);
		PaintLeaf(Elements, Layer, Geometry, Origin, Direction, Scale,
			{9, 8}, {3, 34}, {-5, 18}, {15, 28}, Color, Effects);
		TArray<FVector2f> Diamond;
		for (const FVector2f& Point : TArray<FVector2f>{ {3, 2}, {11, 5}, {14, 14}, {5, 11} })
		{
			Diamond.Add(CornerPoint(Origin, Direction, Point, Scale));
		}
		PaintOutline(Elements, Layer, Geometry, MoveTemp(Diamond), Color, 1.0f, Effects);
	}
}

void SLxMenuArtFrame::Construct(const FArguments& Args)
{
	Shape = Args._Shape;
	Highlight = Args._Highlight;
	FillOpacity = FMath::Clamp(Args._FillOpacity, 0.0f, 1.0f);
	ChildSlot.Padding(Shape == ELxMenuArtFrameShape::Panel ? FMargin(0) : FMargin(12, 10))
	[
		Args._Content.Widget
	];
}

bool SLxMenuArtFrame::ComputeVolatility() const
{
	return SCompoundWidget::ComputeVolatility() || Highlight.IsBound();
}

int32 SLxMenuArtFrame::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
	int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	using namespace LxMenuArtFramePrivate;
	const FVector2f Size(AllottedGeometry.GetLocalSize());
	if (Size.X < 16.0f || Size.Y < 16.0f)
	{
		return SCompoundWidget::OnPaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
	}
	const bool bEnabled = ShouldBeEnabled(bParentEnabled);
	const bool bHighlight = bEnabled && Highlight.Get(false);
	const ESlateDrawEffect Effects = bEnabled ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect;
	const FLinearColor Tint = InWidgetStyle.GetColorAndOpacityTint();
	const FLinearColor Gold = (bHighlight ? FLinearColor(1.0f, 0.77f, 0.34f, 1.0f) : FLinearColor(0.64f, 0.44f, 0.19f, 0.92f)) * Tint;
	const FLinearColor InnerGold = FLinearColor(Gold.R, Gold.G, Gold.B, Gold.A * 0.6f);
	const FLinearColor Fill = FLinearColor(0.025f, 0.042f, 0.055f, FillOpacity) * Tint;
	const TArray<FVector2f> Outer = MakeOutline(Size, 2.0f, Shape);
	PaintFill(OutDrawElements, LayerId, AllottedGeometry, Outer, Fill, Effects);
	if (bHighlight)
	{
		const FLinearColor Glow(Gold.R, Gold.G, Gold.B, Gold.A * 0.12f);
		PaintOutline(OutDrawElements, LayerId + 1, AllottedGeometry, Outer, Glow, 7.0f, Effects);
		PaintOutline(OutDrawElements, LayerId + 1, AllottedGeometry, Outer, Glow, 4.0f, Effects);
	}
	PaintOutline(OutDrawElements, LayerId + 2, AllottedGeometry, Outer, Gold, bHighlight ? 1.6f : 1.2f, Effects);
	PaintOutline(OutDrawElements, LayerId + 2, AllottedGeometry, MakeOutline(Size, 5.0f, Shape), InnerGold, 0.85f, Effects);
	if (Shape == ELxMenuArtFrameShape::Panel)
	{
		const float Scale = FMath::Min(1.0f, FMath::Min(Size.X, Size.Y) / 100.0f);
		PaintCorner(OutDrawElements, LayerId + 3, AllottedGeometry, {7, 7}, {1, 1}, Scale, Gold, Effects);
		PaintCorner(OutDrawElements, LayerId + 3, AllottedGeometry, {Size.X - 7, 7}, {-1, 1}, Scale, Gold, Effects);
		PaintCorner(OutDrawElements, LayerId + 3, AllottedGeometry, {7, Size.Y - 7}, {1, -1}, Scale, Gold, Effects);
		PaintCorner(OutDrawElements, LayerId + 3, AllottedGeometry, Size - FVector2f(7, 7), {-1, -1}, Scale, Gold, Effects);
	}
	else if (Shape != ELxMenuArtFrameShape::Arrow && Size.X > 100.0f)
	{
		const float Radius = FMath::Min(5.0f, Size.Y * 0.07f);
		for (const float X : {Size.Y * 0.16f, Size.X - Size.Y * 0.16f})
		{
			const float Y = Size.Y * 0.5f;
			PaintOutline(OutDrawElements, LayerId + 3, AllottedGeometry,
				{{X, Y - Radius}, {X + Radius, Y}, {X, Y + Radius}, {X - Radius, Y}}, Gold, 1.0f, Effects);
		}
	}
	return SCompoundWidget::OnPaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId + 4, InWidgetStyle, bParentEnabled);
}

namespace LxMenuArt
{
/** 一张旧版装饰图的用途、原始尺寸与绘制参数。 */
struct FTextureSpec
{
	/** 在内容浏览器中显示的简短中文名称。 */
	const TCHAR* Name;
	/** 与旧界面对应的轮廓种类。 */
	ELxMenuArtFrameShape Shape;
	/** 以 1920×1080 设计画布计算的图像宽度。 */
	int32 Width;
	/** 以 1920×1080 设计画布计算的图像高度。 */
	int32 Height;
	/** 旧界面深蓝底色的不透明度。 */
	float FillOpacity;
	/** 是否烘焙悬停与选中时的金色光晕。 */
	bool bHighlight;
};

/** 将 Slate 预乘的颜色还原为普通 PNG 使用的透明度，防止纹理再次混合时变暗。 */
void RestoreStraightAlpha(TArray<FColor>& Pixels)
{
	for (FColor& Pixel : Pixels)
	{
		if (Pixel.A == 0)
		{
			Pixel = FColor::Transparent;
			continue;
		}
		const float InverseAlpha = 255.0f / Pixel.A;
		Pixel.R = static_cast<uint8>(FMath::Clamp(FMath::RoundToInt(Pixel.R * InverseAlpha), 0, 255));
		Pixel.G = static_cast<uint8>(FMath::Clamp(FMath::RoundToInt(Pixel.G * InverseAlpha), 0, 255));
		Pixel.B = static_cast<uint8>(FMath::Clamp(FMath::RoundToInt(Pixel.B * InverseAlpha), 0, 255));
	}
}

/** 把边缘颜色延伸至完全透明的相邻像素，避免双线性缩放出现黑色毛边。 */
void ExtendTransparentEdges(TArray<FColor>& Pixels, int32 Width, int32 Height)
{
	TArray<uint8> Known;
	Known.SetNumUninitialized(Pixels.Num());
	for (int32 Index = 0; Index < Pixels.Num(); ++Index) Known[Index] = Pixels[Index].A > 0 ? 1 : 0;
	for (int32 Pass = 0; Pass < 3; ++Pass)
	{
		const TArray<uint8> Previous = Known;
		const TArray<FColor> Source = Pixels;
		for (int32 Y = 0; Y < Height; ++Y)
		{
			for (int32 X = 0; X < Width; ++X)
			{
				const int32 Index = Y * Width + X;
				if (Previous[Index]) continue;
				for (const FIntPoint Offset : {FIntPoint(-1, 0), FIntPoint(1, 0), FIntPoint(0, -1), FIntPoint(0, 1)})
				{
					const int32 NearX = X + Offset.X;
					const int32 NearY = Y + Offset.Y;
					if (NearX < 0 || NearX >= Width || NearY < 0 || NearY >= Height) continue;
					const int32 NearIndex = NearY * Width + NearX;
					if (!Previous[NearIndex]) continue;
					Pixels[Index] = Source[NearIndex];
					Pixels[Index].A = 0;
					Known[Index] = 1;
					break;
				}
			}
		}
	}
}

/** 离屏执行原始 Slate 绘制并读取已经过显示伽马校正的像素。 */
bool RenderTexture(FWidgetRenderer& Renderer, const FTextureSpec& Spec, TArray<FColor>& Pixels)
{
	TStrongObjectPtr<UTextureRenderTarget2D> Target(NewObject<UTextureRenderTarget2D>());
	Target->ClearColor = FLinearColor::Transparent;
	Target->TargetGamma = 1.0f;
	Target->InitCustomFormat(Spec.Width, Spec.Height, PF_B8G8R8A8, true);
	Target->UpdateResourceImmediate(true);
	const TSharedRef<SWidget> Frame = SNew(SLxMenuArtFrame)
		.Shape(Spec.Shape).Highlight(Spec.bHighlight).FillOpacity(Spec.FillOpacity);
	Renderer.DrawWidget(Target.Get(), Frame, FVector2D(Spec.Width, Spec.Height), 0.0f);
	FReadSurfaceDataFlags ReadFlags(RCM_MinMax);
	// 输出已由 Slate 校正到显示颜色；读取时不能再做一次伽马转换。
	ReadFlags.SetLinearToGamma(false);
	if (!Target->GameThread_GetRenderTargetResource()->ReadPixels(Pixels, ReadFlags)) return false;
	if (Pixels.Num() != Spec.Width * Spec.Height) return false;
	RestoreStraightAlpha(Pixels);
	ExtendTransparentEdges(Pixels, Spec.Width, Spec.Height);
	return true;
}

/** 保存可重导入的 PNG 源图与无损 UI 纹理，两者使用一致的中文用途名称。 */
bool SaveTexture(const FTextureSpec& Spec, const TArray<FColor>& Pixels)
{
	const FString PackagePath = FString(TEXT("/Game/项目内容/UI界面/主菜单/图像/")) + Spec.Name;
	const FString SourceDirectory = FPaths::ProjectContentDir() / TEXT("项目内容/UI界面/主菜单/图像/源图");
	const FString SourcePath = FPaths::ConvertRelativePathToFull(SourceDirectory / (FString(Spec.Name) + TEXT(".png")));
	if (!IFileManager::Get().MakeDirectory(*SourceDirectory, true)) return false;
	TArray64<uint8> Png;
	FImageUtils::PNGCompressImageArray(Spec.Width, Spec.Height, TArrayView64<const FColor>(Pixels.GetData(), Pixels.Num()), Png);
	if (Png.IsEmpty() || !FFileHelper::SaveArrayToFile(Png, *SourcePath)) return false;

	UTexture2D* Texture = FPackageName::DoesPackageExist(PackagePath)
		? LoadObject<UTexture2D>(nullptr, *(PackagePath + TEXT(".") + Spec.Name)) : nullptr;
	const bool bNewAsset = !Texture;
	if (!Texture)
	{
		UPackage* Package = CreatePackage(*PackagePath);
		Texture = NewObject<UTexture2D>(Package, Spec.Name, RF_Public | RF_Standalone | RF_Transactional);
	}
	Texture->PreEditChange(nullptr);
	Texture->Source.Init(Spec.Width, Spec.Height, 1, 1, TSF_BGRA8, reinterpret_cast<const uint8*>(Pixels.GetData()));
	Texture->SetModernSettingsForNewOrChangedTexture();
	Texture->CompressionSettings = TC_EditorIcon;
	Texture->CompressionNoAlpha = false;
	Texture->DeferCompression = false;
	Texture->LODGroup = TEXTUREGROUP_UI;
	Texture->MipGenSettings = TMGS_NoMipmaps;
	Texture->SRGB = true;
	Texture->NeverStream = true;
	Texture->VirtualTextureStreaming = false;
	Texture->Filter = TF_Bilinear;
	Texture->AddressX = TA_Clamp;
	Texture->AddressY = TA_Clamp;
	if (!Texture->AssetImportData) Texture->AssetImportData = NewObject<UAssetImportData>(Texture);
	Texture->AssetImportData->Update(SourcePath);
	// 新建或旧版空纹理可能已有空平台缓存，普通 PostEditChange 不保证重建。
	// 同步强制构建可采样的基础级纹理，再通知编辑器更新与此纹理相关的对象。
	const UTexture::EUpdateResourceFlags BuildFlags = static_cast<UTexture::EUpdateResourceFlags>(
		static_cast<uint32>(UTexture::EUpdateResourceFlags::ForceRebuild)
		| static_cast<uint32>(UTexture::EUpdateResourceFlags::Synchronous));
	Texture->UpdateResourceWithParams(BuildFlags);
	Texture->BlockOnAnyAsyncBuild();
	Texture->PostEditChange();
	FTextureCompilingManager::Get().FinishCompilation({Texture});
	Texture->BlockOnAnyAsyncBuild();
	Texture->WaitForPendingInitOrStreaming();
	const FTexturePlatformData* Platform = Texture->GetPlatformData();
	const FTextureResource* Resource = Texture->GetResource();
	if (!Texture->Source.IsValid() || !Platform || Platform->Mips.Num() != 1
		|| Platform->SizeX != Spec.Width || Platform->SizeY != Spec.Height
		|| Platform->Mips[0].SizeX != Spec.Width || Platform->Mips[0].SizeY != Spec.Height
		|| Platform->PixelFormat != PF_B8G8R8A8 || !Resource || !Resource->TextureRHI.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("菜单图像没有生成有效的无损平台纹理：%s，源图=%d，纹理级数=%d，像素格式=%d"),
			*PackagePath, Texture->Source.IsValid(), Platform ? Platform->Mips.Num() : 0,
			Platform ? static_cast<int32>(Platform->PixelFormat) : -1);
		return false;
	}
	if (bNewAsset) FAssetRegistryModule::AssetCreated(Texture);
	Texture->MarkPackageDirty();
	const FString Filename = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	if (!UPackage::SavePackage(Texture->GetPackage(), Texture, *Filename, SaveArgs)) return false;
	UE_LOG(LogTemp, Display, TEXT("已烘焙菜单图像：%s（%d×%d）"), *PackagePath, Spec.Width, Spec.Height);
	return true;
}

/** 命令工具默认跳过 Slate 渲染器初始化；仅在缺失时补齐引擎的标准初始化步骤。 */
bool EnsureSlateRendering()
{
	if (!FApp::CanEverRender())
	{
		UE_LOG(LogTemp, Error, TEXT("菜单图像烘焙需要 Slate 渲染器；命令行使用 -AllowCommandletRendering，不能使用 -NullRHI。"));
		return false;
	}
	if (!FSlateApplication::IsInitialized()) FSlateApplication::Create();
	if (!FSlateApplication::Get().GetRenderer())
	{
		ISlateRHIRendererModule* Module = FModuleManager::LoadModulePtr<ISlateRHIRendererModule>(TEXT("SlateRHIRenderer"));
		// 静默初始化不创建窗口，图形设备失败时只报告错误而不弹出系统对话框。
		if (!Module || !FSlateApplication::Get().InitializeRenderer(Module->CreateSlateRHIRenderer(), true))
		{
			UE_LOG(LogTemp, Error, TEXT("菜单图像烘焙无法初始化离屏 Slate 渲染器。"));
			return false;
		}
	}
	if (!FEngineFontServices::IsInitialized()) FEngineFontServices::Create();
	// 全局 Slate 与字体服务由引擎正常退出流程释放，后续蓝图绘制仍可复用。
	return true;
}

bool BakeTextures()
{
	if (!EnsureSlateRendering()) return false;
	const FTextureSpec Specs[] =
	{
		{TEXT("侧栏"), ELxMenuArtFrameShape::Panel, 518, 1050, 0.76f, false},
		{TEXT("按钮"), ELxMenuArtFrameShape::Button, 446, 119, 0.78f, false},
		{TEXT("按钮高亮"), ELxMenuArtFrameShape::Button, 446, 119, 0.78f, true},
		{TEXT("切换框"), ELxMenuArtFrameShape::Arrow, 86, 93, 0.78f, false},
		{TEXT("切换框高亮"), ELxMenuArtFrameShape::Arrow, 86, 93, 0.78f, true},
		{TEXT("铭牌"), ELxMenuArtFrameShape::Nameplate, 403, 91, 0.83f, false},
		{TEXT("弹窗"), ELxMenuArtFrameShape::Panel, 620, 650, 0.97f, false},
		{TEXT("小按钮"), ELxMenuArtFrameShape::Button, 256, 50, 0.78f, false},
		{TEXT("小按钮高亮"), ELxMenuArtFrameShape::Button, 256, 50, 0.78f, true}
	};
	FWidgetRenderer Renderer(true, true);
	Renderer.SetApplyColorDeficiencyCorrection(false);
	for (const FTextureSpec& Spec : Specs)
	{
		TArray<FColor> Pixels;
		if (!RenderTexture(Renderer, Spec, Pixels) || !SaveTexture(Spec, Pixels))
		{
			UE_LOG(LogTemp, Error, TEXT("烘焙菜单图像失败：%s"), Spec.Name);
			return false;
		}
	}
	return true;
}
}
