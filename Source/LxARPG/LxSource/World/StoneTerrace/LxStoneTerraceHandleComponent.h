#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LxStoneTerraceHandleComponent.generated.h"

/** 编辑器专用的台地尺寸手柄载体；运行与烹饪时不需要该组件。 */
UCLASS(meta=(DisplayName="台地尺寸手柄", Category="石质台地"))
class LXARPG_API ULxStoneTerraceHandleComponent : public UActorComponent
{
 GENERATED_BODY()
public:
 /** 禁用逐帧更新并标记为编辑器专用。 */
 ULxStoneTerraceHandleComponent()
 {
  PrimaryComponentTick.bCanEverTick=false;
#if WITH_EDITORONLY_DATA
  bIsEditorOnly=true;
#endif
 }
};
