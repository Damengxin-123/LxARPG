"""通过虚幻 5.8 官方 MCP 读取关卡重定向并运行隔离的角色存档回归。"""
import json
import unreal


@unreal.uclass()
class CharacterSaveValidationTools(unreal.ToolsetDefinition):
    """只读项目配置及执行本次独立测试的官方 MCP 工具集。"""

    @unreal.ufunction(static=True, params=[str], ret=str,
                     meta={"AICallable": "", "DisplayName": "验收角色存档", "Category": "角色存档|验证"})
    def validate_character_save(stage):
        """按阶段检查当前资产、运行存档测试或关闭本次验证会话。"""
        if stage == "inspect":
            settings = unreal.get_default_object(unreal.LxMainMenuSettings)
            return json.dumps({
                "default_level": str(settings.get_editor_property("default_level")),
                "default_character": str(settings.get_editor_property("default_character")),
                "menu_widget_class": str(settings.get_editor_property("menu_widget_class")),
            }, ensure_ascii=False)
        if stage == "tests":
            unreal.SystemLibrary.execute_console_command(None,
                "Automation RunTests LxARPG.Save.Character+LxARPG.Menu.ReadOnly+LxARPG.Menu.PersistentWorldData")
            return "已启动角色存档、快捷栏、重命名关卡预览和菜单存档隔离测试"
        if stage == "pie":
            unreal.SystemLibrary.execute_console_command(None, "Automation RunTests LxARPG.Menu.PIEFlow")
            return "已启动真实关卡主菜单与角色读档验证"
        if stage == "quit":
            unreal.SystemLibrary.execute_console_command(None, "QUIT_EDITOR")
            return "已请求关闭独立的角色存档验证会话"
        return "ERROR: 未知验收阶段"


unreal.EditorPythonScripting.set_keep_python_script_alive(True)
unreal.ToolsetRegistry.register_toolset_class(CharacterSaveValidationTools)
unreal.SystemLibrary.execute_console_command(None, "ModelContextProtocol.StartServer 8057")
unreal.log("角色存档官方 MCP 已就绪，端口8057")
