"""通过虚幻引擎官方 MCP 在独立会话验证海洋海岸编辑与延迟生成。"""
import unreal


@unreal.uclass()
class OceanEditingVerificationTools(unreal.ToolsetDefinition):
    """海洋编辑与运行回归测试入口。"""

    @unreal.ufunction(static=True, params=[str], ret=str,
                     meta={"AICallable": "", "DisplayName": "验证海洋编辑", "Category": "海洋|验证"})
    def verify_ocean_editing(stage):
        """执行隔离回归测试或结束本次验证会话。"""
        if stage == "tests":
            unreal.SystemLibrary.execute_console_command(
                None, "Automation RunTests LxARPG.World.Ocean")
            return "已启动海洋预览、保存、海岸孔洞、海浪与交互回归测试"
        if stage == "quit":
            unreal.SystemLibrary.execute_console_command(None, "QUIT_EDITOR")
            return "已请求关闭海洋验证会话"
        raise ValueError(stage)


unreal.EditorPythonScripting.set_keep_python_script_alive(True)
unreal.ToolsetRegistry.register_toolset_class(OceanEditingVerificationTools)
unreal.SystemLibrary.execute_console_command(None, "ModelContextProtocol.StartServer 8033")
