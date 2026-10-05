#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineBaseTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "LxARPG/LxSource/Systems/SaveSystem/LxSaveProfiles.h"
#include "LxARPG/LxSource/Systems/DatabaseSystem/LxCharacterRaceConfig.h"
#include "LxMainMenuSubsystem.generated.h"

class APlayerController;
class ALxMenuPreviewActor;
class ULxMainMenuWidget;
class ULxSaveProfileStore;
class ULxGameSaveData;

/** 菜单目录、选择、加载状态或提示变化时通知原生界面逻辑。 */
DECLARE_MULTICAST_DELEGATE(FLxMenuStateChanged);

/** 跨关卡保存菜单选择、只读预览状态与正式会话的角色地图组合。 */
UCLASS(DisplayName="主菜单流程")
class LXARPG_API ULxMainMenuSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	/** 由界面基类转发为蓝图事件，监听者无需依赖每帧轮询。 */
	FLxMenuStateChanged OnMenuStateChanged;
	/** 绑定关卡旅行失败回调，防止加载失败后界面永久锁定。 */
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
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
	/** 按种族配置建立新档，首次进入时使用对应角色蓝图的初始内容；旧调用默认选择人类。 */
	UFUNCTION(BlueprintCallable, Category="主菜单|角色", DisplayName="新建角色")
	void CreateCharacter(const FString& Name, ELxCharacterRaceType Race = ELxCharacterRaceType::Human);
	/** 获取数据表配置的可玩种族，用于新建角色界面。 */
	UFUNCTION(BlueprintCallable, Category="主菜单|角色", DisplayName="获取可选角色种族")
	TArray<FLxCharacterRaceConfig> GetAvailableCharacterRaces() const;
	/** 建立新的地图状态存档。 */
	UFUNCTION(BlueprintCallable, Category="主菜单|存档", DisplayName="新建地图存档")
	void CreateWorld(const FString& Name);
	/** 验证选中的两个档案，在当前关卡中恢复单位并进入游玩状态。 */
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
	/** 当前角色职业等级与场景名称，供角色信息区域显示。 */
	FString GetCharacterDescription() const;
	/** 根据当前存档中的种族读取种族表显示名称。 */
	UFUNCTION(BlueprintPure, Category="主菜单|角色", DisplayName="获取当前角色种族名称")
	FString GetCharacterRaceName() const;
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
	/** 在状态更新完成后发布通知，包含失败分支的提示变化。 */
	void NotifyMenuStateChanged();
	/** 仅处理本游戏实例的旅行失败，并恢复可操作的菜单提示。 */
	void HandleTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& Error);
	/** 首次显示菜单时加载目录并补充空目录的默认档案。 */
	bool EnsureStore();
	/** 在总关卡中按角色存档位置更新同一个展示对象，不触发关卡旅行。 */
	void RefreshPreview();
	/** 创建菜单或正式加载期间的界面。 */
	void CreateMenuWidget(UWorld* World);
	/** 清理界面和展示引用，使旧世界可正常回收。 */
	void ClearPresentation();
	/** 从种族表解析角色类型并补齐旧档字段；失败时保留错误原因。 */
	bool ResolvePresentation(FLxCharacterSaveRecord& Record);
	/** 激活光照等仅用于展示的环境数据层。 */
	void ActivatePreviewEnvironment(UWorld* World) const;
	/** 在当前世界查找默认出生点。 */
	FTransform FindFallbackTransform(UWorld* World) const;
	/** 多存档管理器的会话提交回调，使用固定的活动档案 ID。 */
	bool PersistSession(const ULxGameSaveData* Data);
	/** 首次显示菜单时安装最后使用的地图数据，供已加载及后续流送的单位恢复。 */
	bool InitializeBackgroundSession();
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
	/** 当前世界单位实际使用的地图档，角色浏览不会改变此值。 */
	FGuid LoadedWorldID;
	/** 当前界面提示。 */
	FString Status;
	/** 正式加载失败后保留的提示，用户重新选择时清除。 */
	FString PendingError;
	/** 正在加载或切换场景。 */
	bool bBusy = false;
	/** 正式会话正在等待场景和角色恢复。 */
	bool bEnteringGame = false;
	/** 已完成正式角色恢复。 */
	bool bPlaying = false;
	/** 进入菜单前的自动镜头管理设置，正式游戏恢复时归还控制器。 */
	bool bAutomaticCameraBeforeMenu = true;
	/** 场景加载开始的实际时间，用于没有游戏 Tick 的加载超时。 */
	double LoadStartedAt = 0;
};
