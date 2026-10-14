#pragma once
#include "CoreMinimal.h"
#include "ComponentVisualizer.h"
#include "ScopedTransaction.h"

/** 台地正X、正Y边中点的尺寸手柄，不移动演员、不提供曲线编辑。 */
class FLxStoneTerraceVisualizer final : public FComponentVisualizer
{
public:
 /** 绘制两根尺寸引导线及可点击的手柄。 */
 virtual void DrawVisualization(const UActorComponent* Component,const FSceneView* View,FPrimitiveDrawInterface* PDI) override;
 /** 单击手柄后切换到平移编辑。 */
 virtual bool VisProxyHandleClick(FEditorViewportClient* Client,HComponentVisProxy* Proxy,const FViewportClick& Click) override;
 /** 选择长度或宽度手柄，也供自动化交互测试使用。 */
 bool SelectHandle(const UActorComponent* Component,int32 Axis);
 /** 提供当前边中点的世界位置。 */
 virtual bool GetWidgetLocation(const FEditorViewportClient* Client,FVector& Location) const override;
 /** 以台地自身朝向提供输入坐标系。 */
 virtual bool GetCustomInputCoordinateSystem(const FEditorViewportClient* Client,FMatrix& Matrix) const override;
 /** 将世界平移量换算为米制尺寸，并提交节流预览。 */
 virtual bool HandleInputDelta(FEditorViewportClient* Client,FViewport* Viewport,FVector& Translation,FRotator& Rotation,FVector& Scale) override;
 /** 一次拖动只建立一条撤销记录。 */
 virtual void TrackingStarted(FEditorViewportClient* Client) override;
 /** 松开手柄时结束事务，并提交最后一次尺寸。 */
 virtual void TrackingStopped(FEditorViewportClient* Client,bool bMoved) override;
 /** 退出编辑时清理选择与事务。 */
 virtual void EndEditing() override;
 /** 供引擎在撤销和蓝图重建后重新定位组件。 */
 virtual UActorComponent* GetEditedComponent() const override;
private:
 /** 原生组件的稳定属性路径，避免构造脚本重建后保存悬空指针。 */
 FComponentPropertyPath Edited;
 /** 当前尺寸轴，零为长度，一为宽度。 */
 int32 SelectedAxis=INDEX_NONE;
 /** 当前拖动对应的可撤销事务。 */
 TUniquePtr<FScopedTransaction> Transaction;
 /** 拖动累计长度采用引擎厘米，保留小位移后再量化米制显示值。 */
 double DragDimensionCentimeters=0;
};
