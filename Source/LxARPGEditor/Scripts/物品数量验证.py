"""通过 UE 5.8 官方 MCP 检查物品数量与装备强化显示，不修改玩家存档。"""
import json
from pathlib import Path
import unreal


@unreal.uclass()
class ItemCountTools(unreal.ToolsetDefinition):
    """物品数量和装备词条显示链路的官方 MCP 验证入口。"""

    @unreal.ufunction(static=True, params=[str], ret=str,
                     meta={"AICallable": "", "DisplayName": "验证物品数量", "Category": "物品|数量验证"})
    def verify_counts(stage):
        """读取实际物品表与金币堆叠配置、执行独立测试或关闭本次验证会话。"""
        if stage == "quit":
            unreal.SystemLibrary.execute_console_command(None, "QUIT_EDITOR")
            return "已关闭物品数量专用验证会话"
        if stage == "tests":
            unreal.SystemLibrary.execute_console_command(None,
                "Automation RunTests LxARPG.Items.CountWidth+LxARPG.Items.SafeOperations"
                "+LxARPG.Save.InteractionContainers+LxARPG.EquipmentCrafting.Rules"
                "+LxARPG.EquipmentCrafting.Transactions")
            return "已启动大堆叠数量、交易扣款、存档及物品事务回归"
        if stage == "equipment_tests":
            unreal.SystemLibrary.execute_console_command(None,
                "Automation RunTests LxARPG.UI.ItemTooltip.EnhancementText+LxARPG.EquipmentCrafting"
                "+LxARPG.Skill.EquipmentGrant+LxARPG.EntryData.DisplayText+LxARPG.EntryData.BuffLifecycle"
                "+LxARPG.Items.SafeOperations+LxARPG.Items.CountWidth+LxARPG.Save.InteractionContainers")
            return "已启动装备强化显示、原始词条缓存、效果包、装备存档与物品操作回归"
        if stage == "inspect":
            table = unreal.load_asset("/Game/项目内容/功能资产/数据表格/物品表格/材料/材料数据表")
            rows = json.loads(unreal.DataTableFunctionLibrary.export_data_table_to_json_string(table))
            gold = next(row for row in rows if row["ItemIDTag"]["TagName"] == "物品.材料.货币.金币")
            result = {"金币行名": gold["Name"], "最大堆叠数量": gold["ItemCountMax"],
                      "配置物品数量": gold["ItemCount"], "数量字段类型": "int32（32位有符号整数）",
                      "运行时数量类型": "FLxItemCount = int32", "单堆允许范围": "0～9999"}
            checked_tables = []
            invalid_rows = []
            row_count = 0
            for asset_path in unreal.EditorAssetLibrary.list_assets("/Game/项目内容/功能资产/数据表格/物品表格", recursive=True):
                asset = unreal.load_asset(asset_path)
                if not isinstance(asset, unreal.DataTable):
                    continue
                item_rows = json.loads(unreal.DataTableFunctionLibrary.export_data_table_to_json_string(asset))
                if not item_rows or "ItemCountMax" not in item_rows[0]:
                    continue
                checked_tables.append(asset_path)
                row_count += len(item_rows)
                for row in item_rows:
                    if not (0 <= row["ItemCountMax"] <= 9999 and 0 <= row["ItemCount"] <= row["ItemCountMax"]):
                        invalid_rows.append({"数据表": asset_path, "行名": row["Name"],
                                             "最大堆叠数量": row["ItemCountMax"], "物品数量": row["ItemCount"]})
            result.update({"检查数据表": checked_tables, "检查物品数": row_count, "越界物品": invalid_rows})
            output = Path(unreal.Paths.project_dir()).resolve() / "Saved/物品数量验证"
            output.mkdir(parents=True, exist_ok=True)
            (output / "金币配置检查.json").write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding="utf-8")
            return json.dumps(result, ensure_ascii=False)
        raise ValueError(stage)


unreal.EditorPythonScripting.set_keep_python_script_alive(True)
unreal.ToolsetRegistry.register_toolset_class(ItemCountTools)
unreal.SystemLibrary.execute_console_command(None, "ModelContextProtocol.StartServer 8052")
unreal.log("物品数量官方 MCP 验证已就绪，端口8052")
