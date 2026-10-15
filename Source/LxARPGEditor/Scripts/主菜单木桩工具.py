"""通过 UE 5.8 官方 MCP 接入继续游戏与伤害测试木桩，并执行实际资产验收。"""
import json
from pathlib import Path
import unreal


def inspect_project():
    """读取当前实际配置和出生点，避免使用整理前的旧资产路径。"""
    settings = unreal.get_default_object(unreal.LxMainMenuSettings)
    report = {}
    for name in ["default_level", "menu_widget_class", "pause_menu_widget_class", "default_character"]:
        report[name] = str(settings.get_editor_property(name))
    # 软世界配置的 Python 读取会加载对象；检查阶段不切换关卡，防止持有旧世界引用。
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
    report["test_actors"] = [
        {"label": a.get_actor_label(), "type": a.get_class().get_path_name(),
         "location": str(a.get_actor_location())}
        for a in actors if isinstance(a, unreal.PlayerStart) or "木桩" in a.get_actor_label()
    ]
    output = Path(unreal.Paths.project_saved_dir()) / "主菜单木桩" / "当前配置.json"
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
    return json.dumps(report, ensure_ascii=False)


@unreal.uclass()
class MainMenuDummyTools(unreal.ToolsetDefinition):
    """本次主菜单与木桩资产创建、验证和回归测试的受限工具入口。"""

    @unreal.ufunction(static=True, params=[str], ret=str,
                     meta={"AICallable": "", "DisplayName": "处理主菜单与木桩", "Category": "主菜单木桩|制作验证"})
    def process_main_menu_dummy(stage):
        """按明确阶段调用原生制作接口，不改变无关的场景和玩法配置。"""
        if stage == "inspect":
            return inspect_project()
        if stage == "menu_inspect":
            return unreal.LxMainMenuLayoutCommandlet.inspect_main_menu_blueprint()
        if stage == "dummy_inspect":
            return unreal.LxTrainingDummyAssetLibrary.process_training_dummy("inspect")
        if stage == "menu":
            if unreal.LxMainMenuLayoutCommandlet.ensure_continue_game_button():
                return "MAIN_MENU_CONTINUE_READY: 顶部继续游戏按钮已保存并编译"
            return "ERROR: 主菜单继续游戏按钮迁移或编译失败"
        if stage == "tests":
            unreal.SystemLibrary.execute_console_command(None,
                "Automation RunTests LxARPG.TrainingDummy+LxARPG.Menu+LxARPG.Combat.Cleanup")
            return "已启动木桩、主菜单和存档目录回归测试"
        if stage == "dummy_tests":
            unreal.SystemLibrary.execute_console_command(None, "Automation RunTests LxARPG.TrainingDummy")
            return "已启动木桩回归测试"
        if stage == "menu_tests":
            unreal.SystemLibrary.execute_console_command(None, "Automation RunTests LxARPG.Menu")
            return "已启动主菜单回归测试"
        if stage == "quit":
            unreal.SystemLibrary.execute_console_command(None, "QUIT_EDITOR")
            return "已请求关闭本次独立制作会话"
        return unreal.LxTrainingDummyAssetLibrary.process_training_dummy(stage)


unreal.EditorPythonScripting.set_keep_python_script_alive(True)
unreal.ToolsetRegistry.register_toolset_class(MainMenuDummyTools)
unreal.SystemLibrary.execute_console_command(None, "ModelContextProtocol.StartServer 8054")
unreal.log("主菜单木桩官方 MCP 已就绪，端口8054")
