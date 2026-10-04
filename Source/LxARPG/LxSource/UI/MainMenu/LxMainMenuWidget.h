#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LxARPG/LxSource/Systems/SaveSystem/LxSaveProfiles.h"
#include "LxARPG/LxSource/Systems/DatabaseSystem/LxCharacterRaceConfig.h"
#include "LxMainMenuWidget.generated.h"

class ULxMainMenuSubsystem;
class ULxSettingsWidget;

/** 主菜单当前功能页；蓝图可自由决定各页的容器与显示方式。 */
UENUM(BlueprintType, DisplayName="主菜单面板")
enum class ELxMainMenuPanel : uint8
{
	MainMenu UMETA(DisplayName="主界面"),
	Worlds UMETA(DisplayName="地图存档"),
	Settings UMETA(DisplayName="设置"),
	CreateCharacter UMETA(DisplayName="新建角色")
};

/** 主菜单逻辑基类：处理操作与状态通知，全部控件和布局由蓝图子类提供。 */
UCLASS(Blueprintable, BlueprintType, DisplayName="主菜单界面")
class LXARPG_API ULxMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	/** 打开地图存档页，存档数据通过刷新事件提供给蓝图。 */
	UFUNCTION(BlueprintCallable, Category="主菜单|面板", DisplayName="打开地图存档面板")
	void OpenWorldPanel();
	/** 打开设置页，同时为已注册的设置控件开始一次新的编辑。 */
	UFUNCTION(BlueprintCallable, Category="主菜单|面板", DisplayName="打开设置面板")
	void OpenSettingsPanel();
	/** 打开新建角色页，可选种族始终读取项目配置。 */
	UFUNCTION(BlueprintCallable, Category="主菜单|面板", DisplayName="打开新建角色面板")
	void OpenCharacterPanel();
	/** 关闭功能页；离开设置时丢弃尚未应用的编辑。 */
	UFUNCTION(BlueprintCallable, Category="主菜单|面板", DisplayName="返回主界面")
	void ClosePanel();
	/** 注册蓝图布局中的设置控件，自动衔接打开、应用与取消流程，不限制控件名称。 */
	UFUNCTION(BlueprintCallable, Category="主菜单|设置", DisplayName="注册设置界面")
	void RegisterSettingsWidget(ULxSettingsWidget* InSettingsWidget);
	/** 主动补发当前数据通知，供蓝图重建显示或动态创建子控件后使用。 */
	UFUNCTION(BlueprintCallable, Category="主菜单|数据", DisplayName="刷新主菜单显示")
	void RefreshMenu();
	/** 在主界面切换角色，加载或功能页打开期间拒绝切换。 */
	UFUNCTION(BlueprintCallable, Category="主菜单|角色", DisplayName="切换角色")
	void SwitchCharacter(int32 Direction);
	/** 按稳定档案身份选择地图。 */
	UFUNCTION(BlueprintCallable, Category="主菜单|存档", DisplayName="选择地图存档")
	void SelectWorld(const FGuid& WorldID);
	/** 根据当前地图列表的索引选择档案，供下拉框使用。 */
	UFUNCTION(BlueprintCallable, Category="主菜单|存档", DisplayName="按索引选择地图存档")
	void SelectWorldByIndex(int32 Index);
	/** 创建独立地图档；错误与最新目录通过菜单刷新事件通知。 */
	UFUNCTION(BlueprintCallable, Category="主菜单|存档", DisplayName="新建地图存档")
	void CreateWorld(const FString& Name);
	/** 创建角色，成功后回到主界面；失败时保留当前页供用户修正输入。 */
	UFUNCTION(BlueprintCallable, Category="主菜单|角色", DisplayName="新建角色")
	void CreateCharacter(const FString& Name, ELxCharacterRaceType Race);
	/** 使用当前种族列表的索引创建角色，供下拉框使用。 */
	UFUNCTION(BlueprintCallable, Category="主菜单|角色", DisplayName="按种族索引新建角色")
	void CreateCharacterByRaceIndex(const FString& Name, int32 RaceIndex);
	/** 验证当前选择并进入正式游戏。 */
	UFUNCTION(BlueprintCallable, Category="主菜单|流程", DisplayName="进入选中存档")
	void EnterGame();
	/** 执行退出流程，加载中不会重复提交。 */
	UFUNCTION(BlueprintCallable, Category="主菜单|流程", DisplayName="退出游戏")
	void QuitGame();
	/** 获取当前功能页，供蓝图自由设置可见性。 */
	UFUNCTION(BlueprintPure, Category="主菜单|面板", DisplayName="获取当前面板")
	ELxMainMenuPanel GetActivePanel() const { return ActivePanel; }
	/** 获取从零开始的功能页索引，便于连接控件切换器。 */
	UFUNCTION(BlueprintPure, Category="主菜单|面板", DisplayName="获取当前面板索引")
	int32 GetActivePanelIndex() const { return static_cast<int32>(ActivePanel); }
	/** 获取独立角色档案摘要，不加载其完整数据。 */
	UFUNCTION(BlueprintPure, Category="主菜单|角色", DisplayName="获取角色存档列表")
	TArray<FLxSaveProfile> GetCharacters() const;
	/** 获取地图档案摘要，蓝图可自行生成列表条目。 */
	UFUNCTION(BlueprintPure, Category="主菜单|存档", DisplayName="获取地图存档列表")
	TArray<FLxSaveProfile> GetWorlds() const;
	/** 读取当前角色摘要；不存在时清空输出并返回假。 */
	UFUNCTION(BlueprintPure, Category="主菜单|角色", DisplayName="获取选中角色")
	bool GetSelectedCharacter(FLxSaveProfile& Profile) const;
	/** 读取当前地图档案身份。 */
	UFUNCTION(BlueprintPure, Category="主菜单|存档", DisplayName="获取选中地图ID")
	FGuid GetSelectedWorldID() const;
	/** 获取可用于新建角色的完整种族配置。 */
	UFUNCTION(BlueprintPure, Category="主菜单|角色", DisplayName="获取可选角色种族")
	TArray<FLxCharacterRaceConfig> GetAvailableCharacterRaces() const;
	/** 获取地图下拉选项；同名且同时间的存档追加序号，选择仍通过档案身份处理。 */
	UFUNCTION(BlueprintPure, Category="主菜单|存档", DisplayName="获取地图选项")
	TArray<FString> GetWorldOptions() const;
	/** 获取当前地图在目录中的索引，无选择时返回负一。 */
	UFUNCTION(BlueprintPure, Category="主菜单|存档", DisplayName="获取选中地图索引")
	int32 GetSelectedWorldIndex() const;
	/** 获取按配置排序的种族名称，索引与完整种族列表一致。 */
	UFUNCTION(BlueprintPure, Category="主菜单|角色", DisplayName="获取种族选项")
	TArray<FString> GetRaceOptions() const;
	/** 获取所选角色昵称，不附加布局使用的字段标题。 */
	UFUNCTION(BlueprintPure, Category="主菜单|角色", DisplayName="获取角色昵称")
	FText GetCharacterNickname() const;
	/** 获取所选角色的种族显示名称。 */
	UFUNCTION(BlueprintPure, Category="主菜单|角色", DisplayName="获取角色种族文本")
	FText GetCharacterRaceText() const;
	/** 获取当前加载进度或错误文本。 */
	UFUNCTION(BlueprintPure, Category="主菜单|状态", DisplayName="获取菜单提示")
	FText GetStatusText() const;
	/** 是否正在加载；蓝图据此控制遮罩和动画。 */
	UFUNCTION(BlueprintPure, Category="主菜单|状态", DisplayName="菜单是否忙碌")
	bool IsMenuBusy() const;
	/** 是否允许提交菜单操作，未初始化与正式游戏期间均不可操作。 */
	UFUNCTION(BlueprintPure, Category="主菜单|状态", DisplayName="菜单是否可操作")
	bool CanInteract() const;
	/** 是否允许切换角色，包括当前页与角色数量限制。 */
	UFUNCTION(BlueprintPure, Category="主菜单|状态", DisplayName="是否可切换角色")
	bool CanSwitchCharacter() const;
	/** 当前角色与地图组合是否可进入正式游戏。 */
	UFUNCTION(BlueprintPure, Category="主菜单|状态", DisplayName="是否可进入游戏")
	bool CanEnterGame() const;
	/** 数据、选择、加载状态或错误变化时通知蓝图，蓝图负责刷新自己的控件。 */
	UFUNCTION(BlueprintImplementableEvent, Category="主菜单|通知", DisplayName="主菜单数据更新")
	void ReceiveMenuChanged();
	/** 功能页变化时通知蓝图，不对任何布局控件作假设。 */
	UFUNCTION(BlueprintImplementableEvent, Category="主菜单|通知", DisplayName="主菜单面板改变")
	void ReceivePanelChanged(ELxMainMenuPanel Panel);

protected:
	/** 在蓝图构造事件之前连接数据源，构造后补发完整初始状态。 */
	virtual void NativeConstruct() override;
	/** 移出界面时解绑通知，允许同一控件稍后重新加入。 */
	virtual void NativeDestruct() override;

private:
	/** 切换逻辑页并管理设置草稿，不创建或排列任何控件。 */
	void SetActivePanel(ELxMainMenuPanel Panel);
	/** 转发流程变化为蓝图刷新事件。 */
	void HandleMenuStateChanged();
	/** 设置应用或取消后恢复主界面。 */
	UFUNCTION(Category="主菜单|设置", DisplayName="处理设置关闭")
	void HandleSettingsClosed(bool bApplied);
	/** 当前显示的逻辑功能页。 */
	UPROPERTY(Transient, BlueprintReadOnly, Category="主菜单|面板", DisplayName="当前面板", meta=(AllowPrivateAccess="true"))
	ELxMainMenuPanel ActivePanel = ELxMainMenuPanel::MainMenu;
	/** 当前游戏实例的菜单流程，使用弱引用避免延长实例生命周期。 */
	TWeakObjectPtr<ULxMainMenuSubsystem> MenuFlow;
	/** 蓝图显式注册的设置实例，不要求固定的控件层级。 */
	UPROPERTY(Transient, VisibleInstanceOnly, Category="主菜单|设置", DisplayName="已注册设置界面")
	TObjectPtr<ULxSettingsWidget> SettingsWidget;
};
