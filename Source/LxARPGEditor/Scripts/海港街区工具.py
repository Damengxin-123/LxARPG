"""通过 UE 5.8 官方 MCP 分阶段搭建海港主街山侧街区。"""
import json
import traceback
from pathlib import Path
import unreal

# 制作脚本固定在工程内，便于保存结果和重复检查。
ROOT = Path(unreal.Paths.project_dir()).resolve()
WORK = ROOT / "Saved/HarborDistrict"
WORK.mkdir(parents=True, exist_ok=True)
# 跨阶段保留离屏捕获状态与本次创建的对象。
CONTEXT = {"__name__": "harbor_district_worker"}


@unreal.uclass()
class HarborDistrictTools(unreal.ToolsetDefinition):
    """海港街区的官方 MCP 操作入口。"""

    @unreal.ufunction(static=True, params=[str], ret=str, meta={
        "AICallable": "", "DisplayName": "处理海港街区", "Category": "海港街区|制作"})
    def process_district(stage):
        """重载固定的街区脚本，执行指定制作或验证阶段。"""
        try:
            path = ROOT / "Source/LxARPGEditor/Scripts/海港街区构建.py"
            exec(compile(path.read_text(encoding="utf-8-sig"), str(path), "exec"), CONTEXT)
            result = CONTEXT["process"](stage)
            (WORK / (stage + "_result.json")).write_text(
                json.dumps(result, ensure_ascii=False, indent=2), encoding="utf-8")
            return json.dumps(result, ensure_ascii=False)
        except Exception:
            error = {"error": traceback.format_exc()}
            (WORK / "最近错误.json").write_text(json.dumps(error, ensure_ascii=False), encoding="utf-8")
            return json.dumps(error, ensure_ascii=False)


unreal.EditorPythonScripting.set_keep_python_script_alive(True)
unreal.ToolsetRegistry.register_toolset_class(HarborDistrictTools)
unreal.SystemLibrary.execute_console_command(None, "ModelContextProtocol.StartServer 8045")
unreal.SystemLibrary.execute_console_command(None, "ModelContextProtocol.RefreshTools")
