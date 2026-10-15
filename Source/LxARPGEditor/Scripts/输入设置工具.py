"""通过虚幻官方 MCP 检查、迁移并验证玩家输入设置，避免操控编辑器桌面。"""
import json
from pathlib import Path
import unreal


def inspect_input_tables():
    """只读取输入数据表，记录真实默认映射以供验收。"""
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    report = []
    for asset in registry.get_assets_by_class(unreal.TopLevelAssetPath("/Script/Engine", "DataTable")):
        if "输入" not in str(asset.asset_name):
            continue
        table = asset.get_asset()
        if not isinstance(table, unreal.DataTable):
            continue
        report.append({"asset": table.get_path_name(), "rows": unreal.DataTableFunctionLibrary.export_data_table_to_json_string(table)})
    result = json.dumps(report, ensure_ascii=False, indent=2)
    output = Path(unreal.Paths.project_saved_dir()) / "输入设置"
    output.mkdir(parents=True, exist_ok=True)
    (output / "默认输入表.json").write_text(result, encoding="utf-8")
    return result


@unreal.uclass()
class InputSettingsTools(unreal.ToolsetDefinition):
    """本次输入设置的受限制作和验收入口。"""

    @unreal.ufunction(static=True, params=[str], ret=str,
                     meta={"AICallable": "", "DisplayName": "处理输入设置", "Category": "输入设置|制作验证"})
    def process_input_settings(stage):
        """根据明确阶段调用原生布局迁移或隔离的自动化测试。"""
        if stage == "inspect":
            return inspect_input_tables()
        if stage == "migrate":
            if unreal.LxMainMenuLayoutCommandlet.ensure_input_settings_page():
                return "INPUT_SETTINGS_READY: 输入设置分页及按键行蓝图已编译保存"
            return "ERROR: 输入设置分页迁移或编译失败，请检查日志"
        if stage == "tests":
            unreal.SystemLibrary.execute_console_command(None, "Automation RunTests LxARPG.Input+LxARPG.Menu.UI")
            return "已启动玩家输入、独立设置存档及设置界面回归测试"
        if stage == "quit":
            unreal.SystemLibrary.execute_console_command(None, "QUIT_EDITOR")
            return "已关闭本次独立输入设置制作会话"
        return "ERROR: 未知输入设置阶段"


unreal.EditorPythonScripting.set_keep_python_script_alive(True)
unreal.ToolsetRegistry.register_toolset_class(InputSettingsTools)
unreal.SystemLibrary.execute_console_command(None, "ModelContextProtocol.StartServer 8058")
unreal.log("输入设置官方 MCP 已就绪，端口8058")
