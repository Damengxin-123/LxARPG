#pragma once

/** 仅在编辑器中把旧版菜单装饰烘焙成蓝图可直接引用的纹理。 */
namespace LxMenuArt
{
/** 重建菜单图像与源图；运行时只使用保存的纹理，不再执行几何绘制。 */
bool BakeTextures();
}
