#include "SLxFantasyFrame.h"

#include "Framework/Application/SlateApplication.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderer.h"
#include "Styling/CoreStyle.h"

/** 奇幻装饰绘制的局部辅助函数。 */
namespace LxFantasyFramePrivate
{
	/** 生成顺时针轮廓；所有形状均可从中心向边缘作扇形三角剖分。 */
	static TArray<FVector2f> MakeOutline(const FVector2f& Size, float Inset, ELxFantasyFrameShape Shape)
	{
		const float Left = Inset;
		const float Right = Size.X - Inset;
		const float Top = Inset;
		const float Bottom = Size.Y - Inset;
		if (Shape == ELxFantasyFrameShape::Panel)
		{
			return { {Left, Top}, {Right, Top}, {Right, Bottom}, {Left, Bottom} };
		}
		const float Height = Bottom - Top;
		const float Width = Right - Left;
		const float Cut = FMath::Min(Width * 0.22f, Height * (Shape == ELxFantasyFrameShape::Arrow ? 0.23f : 0.18f));
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

void SLxFantasyFrame::Construct(const FArguments& Args)
{
	Shape = Args._Shape;
	Highlight = Args._Highlight;
	FillOpacity = FMath::Clamp(Args._FillOpacity, 0.0f, 1.0f);
	ChildSlot.Padding(Shape == ELxFantasyFrameShape::Panel ? FMargin(0) : FMargin(12, 10))
	[
		Args._Content.Widget
	];
}

bool SLxFantasyFrame::ComputeVolatility() const
{
	return SCompoundWidget::ComputeVolatility() || Highlight.IsBound();
}

int32 SLxFantasyFrame::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
	int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	using namespace LxFantasyFramePrivate;
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
	if (Shape == ELxFantasyFrameShape::Panel)
	{
		const float Scale = FMath::Min(1.0f, FMath::Min(Size.X, Size.Y) / 100.0f);
		PaintCorner(OutDrawElements, LayerId + 3, AllottedGeometry, {7, 7}, {1, 1}, Scale, Gold, Effects);
		PaintCorner(OutDrawElements, LayerId + 3, AllottedGeometry, {Size.X - 7, 7}, {-1, 1}, Scale, Gold, Effects);
		PaintCorner(OutDrawElements, LayerId + 3, AllottedGeometry, {7, Size.Y - 7}, {1, -1}, Scale, Gold, Effects);
		PaintCorner(OutDrawElements, LayerId + 3, AllottedGeometry, Size - FVector2f(7, 7), {-1, -1}, Scale, Gold, Effects);
	}
	else if (Shape != ELxFantasyFrameShape::Arrow && Size.X > 100.0f)
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
