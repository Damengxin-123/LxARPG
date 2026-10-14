"""通过虚幻 5.8 官方 MCP 执行缓存回归测试和独立关卡保存基准。"""
import unreal


@unreal.uclass()
class CompactMeshVerificationTools(unreal.ToolsetDefinition):
    """紧凑网格保存、恢复及性能验证入口。"""

    @unreal.ufunction(static=True, params=[str], ret=str,
                     meta={"AICallable": "", "DisplayName": "验证紧凑网格保存", "Category": "生成优化|验证"})
    def verify_compact_mesh(stage):
        """只运行隔离测试，关卡基准始终写入 Saved 内的独立副本。"""
        if stage == "quit":
            unreal.SystemLibrary.execute_console_command(None, "QUIT_EDITOR")
            return "已请求关闭网格缓存验证会话"
        filters = {
            "cache": "LxARPG.World.GeneratedMesh.Compact",
            "ocean": "LxARPG.World.Ocean",
            "benchmark": "LxARPG.Diagnostics.CompactMeshSave",
        }
        if stage not in filters:
            raise ValueError(stage)
        unreal.SystemLibrary.execute_console_command(None, "Automation RunTests " + filters[stage])
        return "已启动验证：" + filters[stage]


unreal.EditorPythonScripting.set_keep_python_script_alive(True)
unreal.ToolsetRegistry.register_toolset_class(CompactMeshVerificationTools)
unreal.SystemLibrary.execute_console_command(None, "ModelContextProtocol.StartServer 8034")
