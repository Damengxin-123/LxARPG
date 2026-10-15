#pragma once

#include "CoreMinimal.h"
#include "LxInputData.h"
#include "LxInputReceiveInterface.h"

class UInputAction;
class APlayerController;

namespace LxInputActionConfig
{
	/** 判断配置表中的操作是否可由玩家改键，排除视角、连续轴和显式锁定的操作。 */
	LXARPG_API bool IsUserRemappableAction(ELxInputActionID InputActionID);
	/** 仅接受键盘和鼠标的离散按键；Esc 保留给菜单和取消捕获使用。 */
	LXARPG_API bool IsSupportedUserKey(const FKey& Key);
	/** 在数据表未配置冲刺时补充默认Shift按住输入，保留用户改键。 */
	LXARPG_API void EnsureDefaultSprintInputActionInfo();
	LXARPG_API void ClearInputActionConfig();
	LXARPG_API void SetInputActionInfo(const FLxInputActionInfo& InInputActionInfo);

	LXARPG_API const FLxInputActionInfo* GetInputActionInfo(ELxInputActionID InInputActionID);
	LXARPG_API const TMap<ELxInputActionID, FLxInputActionInfo>& GetInputActionInfoMap();

	LXARPG_API FName GetInputActionName(ELxInputActionID InInputActionID);

	LXARPG_API void SetInputActionObject(ELxInputActionID InInputActionID, UInputAction* InInputAction);
	LXARPG_API ELxInputActionID GetInputActionIDByAction(const UInputAction* InInputAction);

	LXARPG_API void RegisterInputReceive(ELxInputActionID InInputActionID, TScriptInterface<ILxInputReceiveInterface> InRegisterObj);
	LXARPG_API void UnregisterInputReceive(ELxInputActionID InInputActionID, const UObject* InRegisterObj);

	LXARPG_API void SendInputEvent(ELxInputActionID InInputActionID, FLxInputValue& InInputValue, const APlayerController* SourcePlayerController);
}
