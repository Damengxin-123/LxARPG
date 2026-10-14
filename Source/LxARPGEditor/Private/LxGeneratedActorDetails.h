#pragma once
#include "CoreMinimal.h"
class FPropertyEditorModule;

/** 自动化场景类型的精简详情面板。 */
namespace LxGeneratedActorDetails
{
 /** 注册功能参数过滤和内部组件详情过滤，保留原有引擎定制。 */
 void Register(FPropertyEditorModule& Module);
 /** 恢复注册前的详情布局。 */
 void Unregister(FPropertyEditorModule& Module);
 /** 获取使用精简详情的自动化类型名称。 */
 const TArray<FName>& Classes();
}
