#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "LxARPG/LxSource/Systems/SaveSystem/LxSaveProfiles.h"
#include "LxMainMenuSubsystem.generated.h"

class APlayerController;
class ALxMenuPreviewActor;
class ULxMainMenuWidget;
class ULxSaveProfileStore;
class ULxGameSaveData;

/** 跨关卡保存菜单选择、只读预览状态与正式会话的角色地图组合。 */
UCLASS(DisplayName="主菜单流程")
class LXARPG_API ULxMainMenuSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	/** 游戏实例结束时解除世界引用。 */
	virtual void Deinitialize() override;
	/** 由菜单游戏模式在场景初始化后调用，创建界面并恢复当前选择。 */
	void ShowMenu(UWorld* World);
	/** 菜单游戏模式驱动区域加载与展示状态。 */
	void TickPreview(float DeltaSeconds);
	/** 切换角色档并自动显示其所在位置。 */
	UFUNCTION(BlueprintCallable, Category="主菜单|角色", DisplayName="切换角色")
	void SwitchCharacter(int32 Direction);
	/** 选择地图档，不改变角色档案内容。 */
	UFUNCTION(BlueprintCallable, Category="主菜单|存档", DisplayName="选择地图存档")
	void SelectWorld(const FGuid& WorldID);
	/** 从默认角色蓝图建立新档，首次进入时使用其初始内容。 */
	UFUNCTION(BlueprintCallable, Category="主菜单|角色", DisplayName="新建角色")
	void CreateCharacter(const FString& Name);
	/** 建立新的地图状态存档。 */
	UFUNCTION(BlueprintCallable, Category="主菜单|存档", DisplayName="新建地图存档")
	void CreateWorld(const FString& Name);
	/** 验证选中的两个档案并进入正式关卡。 */
	UFUNCTION(BlueprintCallable, Category="主菜单|流程", DisplayName="进入选中存档")
	void EnterGame();
	/** 保存正式游戏后返回菜单，失败时保留当前游戏。 */
	UFUNCTION(BlueprintCallable, Category="主菜单|流程", DisplayName="保存并返回主菜单")
	bool ReturnToMenu();
	/** 完成必要保存后退出程序。 */
	UFUNCTION(BlueprintCallable, Category="主菜单|流程", DisplayName="保存并退出游戏")
	void QuitGame();
	/** 获取目录的角色摘要列表。 */
	const TArray<FLxSaveProfile>& GetCharacters() const;
	/** 获取目录的地图摘要列表。 */
	const TArray<FLxSaveProfile>& GetWorlds() const;
	/** 当前选中角色的摘要，可能为空。 */
	const FLxSaveProfile* GetSelectedCharacter() const;
	/** 当前选中的地图档案 ID。 */
	FGuid GetSelectedWorldID() const { return SelectedWorldID; }
	/** 当前加载或错误提示。 */
	FString GetStatus() const { return Status; }
	/** 菜单是否正在执行不可重入的加载操作。 */
	bool IsBusy() const { return bBusy; }
	/** 当前是否能进入正式游戏。 */
	bool CanEnterGame() const;
	/** 当前是否存在菜单发起的正式会话。 */
	bool HasSession() const { return bEnteringGame || bPlaying; }
	/** 正式关卡是否还在等待目标区域加载。 */
	bool IsEnteringGame() const { return bEnteringGame; }
	/** 正式会话的角色生成信息，身份由档案确定。 */
	const FLxCharacterSaveRecord* GetSessionRecord() const;
	/** 正式游戏模式开始前为保存位置建立流送源和加载遮罩。 */
	void PrepareGameplayWorld(UWorld* World);
	/** 检查正式关卡目标区域已就绪，超时则返回菜单。 */
	bool IsGameplayWorldReady();
	/** 角色成功恢复并被控制后提交最近游玩记录，移除加载遮罩。 */
	void CompleteGameplayStart(APlayerController* Controller);
	/** 读档失败时回到菜单并保留错误原因。 */
	void FailGameplayStart(const FString& Reason);
	/** 给正式角色选择经过地面和碰撞检查的出生位置。 */
	FTransform GetSafeSpawnTransform(UWorld* World, UClass* PawnClass) const;
private:
	/** 首次显示菜单时加载目录并补充空目录的默认档案。 */
	bool EnsureStore();
	/** 根据当前角色加载对应背景或更新同关卡的展示对象。 */
	void RefreshPreview();
	/** 创建菜单或正式加载期间的界面。 */
	void CreateMenuWidget(UWorld* World);
	/** 清理界面和展示引用，使旧世界可正常回收。 */
	void ClearPresentation();
	/** 选中快照缺少新字段时补齐配置，保留旧进度。 */
	void ResolvePresentation(FLxCharacterSaveRecord& Record) const;
	/** 在当前世界查找默认出生点。 */
	FTransform FindFallbackTransform(UWorld* World) const;
	/** 多存档管理器的会话提交回调，使用固定的活动档案 ID。 */
	bool PersistSession(const ULxGameSaveData* Data);
	/** 执行菜单关卡跳转，清理跨世界展示引用。 */
	void TravelToMenu(const FSoftObjectPath& Level);
	/** 持有当前目录和磁盘访问接口。 */
	UPROPERTY(Transient, VisibleAnywhere, Category="主菜单|存档", DisplayName="存档仓库")
	TObjectPtr<ULxSaveProfileStore> Store;
	/** 只读加载的当前角色，确保异步场景加载期间不会被回收。 */
	UPROPERTY(Transient, VisibleAnywhere, Category="主菜单|角色", DisplayName="当前角色快照")
	TObjectPtr<ULxCharacterProfileSave> Character;
	/** 当前世界的菜单或加载界面。 */
	UPROPERTY(Transient, VisibleAnywhere, Category="主菜单|界面", DisplayName="菜单界面")
	TObjectPtr<ULxMainMenuWidget> Widget;
	/** 当前世界内仅用于视觉表现的角色。 */
	TWeakObjectPtr<ALxMenuPreviewActor> Preview;
	/** 当前世界，弱引用避免阻碍关卡卸载。 */
	TWeakObjectPtr<UWorld> MenuWorld;
	/** 菜单当前选中的角色档案。 */
	FGuid SelectedCharacterID;
	/** 菜单当前选中的地图档案。 */
	FGuid SelectedWorldID;
	/** 正式会话固定角色身份，浏览选择不会影响正在保存的档案。 */
	FGuid ActiveCharacterID;
	/** 正式会话固定地图身份。 */
	FGuid ActiveWorldID;
	/** 当前界面提示。 */
	FString Status;
	/** 正在加载或切换场景。 */
	bool bBusy = false;
	/** 正式会话正在等待场景和角色恢复。 */
	bool bEnteringGame = false;
	/** 已完成正式角色恢复。 */
	bool bPlaying = false;
	/** 场景加载开始的实际时间，用于没有游戏 Tick 的加载超时。 */
	double LoadStartedAt = 0;
};
