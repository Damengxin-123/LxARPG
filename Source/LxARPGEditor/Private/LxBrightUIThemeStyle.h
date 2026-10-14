#pragma once
#include "CoreMinimal.h"
class UWidgetBlueprint;
class UDataTable;
class UTexture2D;
class UEdGraphNode;
class UEdGraphPin;

/** 明亮象牙与海蓝界面的离线迁移上下文，不改变运行时控件逻辑。 */
namespace LxBrightUI
{
 /** 迁移使用的共享画刷与独立富文本表。 */
 struct FTheme
 {
  /** 象牙面板底图。 */
  UTexture2D* Panel = nullptr;
  /** 浅蓝格子底图。 */
  UTexture2D* Slot = nullptr;
  /** 象牙按钮底图。 */
  UTexture2D* Button = nullptr;
  /** 原始样式表与屏幕专用副本的映射。 */
  TMap<UDataTable*, UDataTable*> Tables;
  /** 加载已经导入的三张主题纹理。 */
  bool Initialize();
  /** 复制富文本表，避免影响场景控件和其他共享资源。 */
  UDataTable* ScreenTable(UDataTable* Source);
  /** 统一指定屏幕控件蓝图的静态外观。 */
  void Apply(UWidgetBlueprint* Blueprint);
  /** 更新主菜单运行时切换使用的默认按钮样式。 */
  void ApplyDefaults(UWidgetBlueprint* Blueprint);
 };
 /** 只放行已经核对的装饰颜色常量。 */
 bool IsStylePin(const UWidgetBlueprint* Blueprint, const UEdGraphNode* Node, const UEdGraphPin* Pin);
 /** 捕获布局、绑定和业务连线，用于迁移前后完整比对。 */
 FString CaptureStructure(UWidgetBlueprint* Blueprint);
}
