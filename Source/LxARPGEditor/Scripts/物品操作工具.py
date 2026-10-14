"""通过虚幻 5.8 官方 MCP 构建拆分蓝图、审计绑定、离屏预览和执行安全操作回归。"""
import json
from pathlib import Path
import traceback
import unreal


# 每个专用验证会话只启动一次测试，重复 MCP 请求不会覆盖仍在运行的自动化队列。
STATE = {"tests_started": False}
OUT = Path(unreal.Paths.project_saved_dir()) / "ItemOperations"


def decode_report(value):
    """保留原生工具的 JSON 结果，不把失败信息伪装为成功。"""
    return json.loads(value)


def write_report(name, value):
    """只将验收输出写入项目 Saved，不加载或保存玩家存档。"""
    OUT.mkdir(parents=True, exist_ok=True)
    (OUT / name).write_text(json.dumps(value, ensure_ascii=False, indent=2), encoding="utf-8")
    return value


def run_tests():
    """一次提交安全事务、窗口路由和格子输入回归，重复请求只返回当前已启动状态。"""
    if STATE["tests_started"]:
        return {"tests_started": True, "already_started": True,
                "message": "本会话已提交物品操作测试，未重复启动或覆盖测试队列"}
    STATE["tests_started"] = True
    command = ("Automation RunTests LxARPG.Items.SafeOperations"
               "+LxARPG.UI.RoleWindows+LxARPG.UI.ItemOperations"
               "+LxARPG.UI.InventoryDragDrop.ListRouting+LxARPG.UI.TitleBar+LxARPG.EquipmentCrafting"
               "+LxARPG.Save.InteractionContainers")
    unreal.SystemLibrary.execute_console_command(None, command)
    return {"tests_started": True, "already_started": False,
            "report_directory": str(Path(unreal.Paths.project_saved_dir()) / "Automation" / "ItemOperations")}


@unreal.uclass()
class ItemOperationTools(unreal.ToolsetDefinition):
    """物品操作的官方 ToolsetRegistry 工具集定义。"""

    @unreal.ufunction(static=True, params=[str], ret=str,
                     meta={"AICallable": "", "DisplayName": "处理物品操作界面", "Category": "物品操作|验证"})
    def process_item_operation(stage):
        """执行构建、只读验收、离屏预览、一次性测试或关闭专用验证会话。"""
        try:
            if stage == "build":
                result = write_report("构建验收.json", decode_report(unreal.LxItemOperationAssetLibrary.build_widgets()))
            elif stage == "verify":
                result = write_report("界面验收.json", decode_report(unreal.LxItemOperationAssetLibrary.verify_widgets()))
            elif stage == "layout":
                result = write_report("角色布局验收.json", decode_report(unreal.LxItemOperationAssetLibrary.inspect_role_layout()))
            elif stage == "preview":
                result = {"success": bool(unreal.LxItemOperationAssetLibrary.preview_widgets()),
                          "design_preview": str(OUT / "设计器预览" / "物品拆分预览.png"),
                          "quantity_preview": str(OUT / "数量预览" / "物品拆分预览.png")}
                write_report("预览验收.json", result)
            elif stage == "tests":
                result = run_tests()
            elif stage == "quit":
                unreal.SystemLibrary.execute_console_command(None, "QUIT_EDITOR")
                result = {"quit_requested": True}
            else:
                raise ValueError("未知物品操作工具阶段：" + stage)
            return json.dumps(result, ensure_ascii=False)
        except Exception:
            return json.dumps({"success": False, "error": traceback.format_exc()}, ensure_ascii=False)


unreal.EditorPythonScripting.set_keep_python_script_alive(True)
unreal.ToolsetRegistry.register_toolset_class(ItemOperationTools)
unreal.SystemLibrary.execute_console_command(None, "ModelContextProtocol.StartServer 8038")
unreal.SystemLibrary.execute_console_command(None, "ModelContextProtocol.RefreshTools")
unreal.log("物品操作官方 MCP 已就绪，端口 8038；本会话不读取或保存玩家存档")
