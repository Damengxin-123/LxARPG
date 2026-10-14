"""通过 UE 官方 MCP 验证海堤编辑与保存缓存，使用独立测试世界。"""
import unreal


@unreal.uclass()
class SeawallEditingVerificationTools(unreal.ToolsetDefinition):
    """海堤编辑性能及道路兼容性回归入口。"""

    @unreal.ufunction(static=True, params=[str], ret=str,
                     meta={"AICallable": "", "DisplayName": "验证海堤编辑", "Category": "海堤|验证"})
    def verify_seawall_editing(stage):
        """在隔离会话执行回归测试或结束会话。"""
        if stage == "tests":
            unreal.SystemLibrary.execute_console_command(
                None, "Automation RunTests LxARPG.World.Seawall+LxARPG.World.HarborRoad+LxARPG.World.GeneratedMesh")
            return "已启动海堤、道路与生成缓存回归测试"
        if stage == "quit":
            unreal.SystemLibrary.execute_console_command(None, "QUIT_EDITOR")
            return "已请求关闭验证会话"
        raise ValueError(stage)


unreal.EditorPythonScripting.set_keep_python_script_alive(True)
unreal.ToolsetRegistry.register_toolset_class(SeawallEditingVerificationTools)
unreal.SystemLibrary.execute_console_command(None, "ModelContextProtocol.StartServer 8032")
