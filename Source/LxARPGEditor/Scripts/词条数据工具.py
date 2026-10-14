"""通过 UE 5.8 官方 MCP 执行词条数据维护，使用独立编辑器会话。"""
import json
import runpy
from pathlib import Path
import unreal


@unreal.uclass()
class EntryDataTools(unreal.ToolsetDefinition):
    """词条、属性和 Buff 数据的官方 MCP 入口。"""

    @unreal.ufunction(static=True, params=[str], ret=str,
                     meta={"AICallable": "", "DisplayName": "维护词条数据", "Category": "词条|数据维护"})
    def process_entries(stage):
        """按明确的维护阶段读取、更新或校验数据。"""
        if stage == "quit":
            unreal.SystemLibrary.execute_console_command(None, "QUIT_EDITOR")
            return "已关闭词条数据专用会话"
        script = Path(unreal.Paths.project_dir()) / "Source/LxARPGEditor/Scripts/维护词条数据.py"
        result = runpy.run_path(str(script))["process"](stage)
        return json.dumps(result, ensure_ascii=False)


unreal.EditorPythonScripting.set_keep_python_script_alive(True)
unreal.ToolsetRegistry.register_toolset_class(EntryDataTools)
unreal.SystemLibrary.execute_console_command(None, "ModelContextProtocol.StartServer 8051")
unreal.log("词条数据官方 MCP 已就绪，端口8051")
