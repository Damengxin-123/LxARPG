#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

/** 奇幻菜单装饰框的轮廓用途。 */
enum class ELxFantasyFrameShape : uint8
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

/** 无图片依赖的半透明奇幻金边容器，可按实际布局尺寸缩放。 */
class SLxFantasyFrame : public SCompoundWidget
{
public:
	/** 框型、悬停高亮、背景不透明度和内部内容的声明参数。 */
	SLATE_BEGIN_ARGS(SLxFantasyFrame)
		: _Shape(ELxFantasyFrameShape::Panel), _Highlight(false), _FillOpacity(0.78f)
	{}
		/** 选择面板、菜单按钮、切换箭头或角色铭牌的轮廓。 */
		SLATE_ARGUMENT(ELxFantasyFrameShape, Shape)
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
	ELxFantasyFrameShape Shape = ELxFantasyFrameShape::Panel;
	/** 外部提供的高亮状态。 */
	TAttribute<bool> Highlight;
	/** 深蓝灰背景的不透明度。 */
	float FillOpacity = 0.78f;
};
