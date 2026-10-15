"""通过 UE 5.8 官方 MCP 备份、修复和验证场景自发光曝光。"""
import json
import shutil
import traceback
from pathlib import Path
import unreal

# 本工具的报告与原始资产备份目录。
OUT = (Path(unreal.Paths.project_saved_dir()) / "黑色显示修复").resolve()
OUT.mkdir(parents=True, exist_ok=True)
# UI 项目母材质及实例集中保存到中文目录。
UI_ROOT = "/Game/项目内容/美术资产/材质/场景界面/"
# 只迁移现有技能美术目录，避免触碰其他受光材质。
SKILL_ROOT = "/Game/项目内容/美术资产/特效/技能表现/材质"
# 这两个共享模板及实际子类承载 NPC、AI 和木桩的世界 UI。
BLUEPRINT_ROOTS = ["/Game/项目内容/基础资产/基础类型", "/Game/项目内容/功能资产/角色资产"]
# 跨帧离屏验证状态由后续验证脚本使用。
STATE = {}


def properties(obj, names):
    """读取可用的反射属性，保留对象的真实路径。"""
    result = {}
    for name in names:
        try:
            value = obj.get_editor_property(name)
            result[name] = value.get_path_name() if isinstance(value, unreal.Object) else str(value)
        except Exception:
            pass
    return result


def backup(path):
    """首次修改前复制磁盘原包，后续调用绝不覆盖原始备份。"""
    package = path.split(".")[0]
    if not package.startswith("/Game/"):
        return None
    relative = package.removeprefix("/Game/") + ".uasset"
    source = Path(unreal.Paths.project_content_dir()) / relative
    target = OUT / "备份" / relative
    if source.exists() and not target.exists():
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, target)
    return str(target)


def wrap_emissive(material):
    """仅包裹 RGB 自发光输出，保持原透明度与其他材质输入。"""
    source = unreal.MaterialEditingLibrary.get_material_property_input_node(material, unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    if isinstance(source, unreal.MaterialExpressionEyeAdaptationInverse):
        return False
    assert source, "材质没有可补偿的自发光输出：" + material.get_path_name()
    output_name = unreal.MaterialEditingLibrary.get_material_property_input_node_output_name(material, unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    node = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionEyeAdaptationInverse)
    node.set_editor_property("desc", "场景曝光补偿，仅影响自发光颜色")
    inputs = unreal.MaterialEditingLibrary.get_material_expression_input_names(node)
    assert unreal.MaterialEditingLibrary.connect_material_expressions(source, output_name, node, inputs[0])
    assert unreal.MaterialEditingLibrary.connect_material_property(node, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    unreal.MaterialEditingLibrary.recompile_material(material)
    return True


def material_info(material):
    """记录材质父级、最终输出和曝光节点，供修复前后比对。"""
    result = {"path": material.get_path_name(), "class": material.get_class().get_name()}
    result.update(properties(material, ["parent", "blend_mode", "shading_model", "two_sided"]))
    if isinstance(material, unreal.Material):
        result["outputs"] = {}
        for name, prop in [("emissive", unreal.MaterialProperty.MP_EMISSIVE_COLOR), ("opacity", unreal.MaterialProperty.MP_OPACITY), ("opacity_mask", unreal.MaterialProperty.MP_OPACITY_MASK)]:
            source = unreal.MaterialEditingLibrary.get_material_property_input_node(material, prop)
            result["outputs"][name] = {"node": source.get_name(), "class": source.get_class().get_name()} if source else None
        result["nodes"] = []
        for node in unreal.ObjectIterator(unreal.MaterialExpression):
            if node.get_outer() == material:
                result["nodes"].append({"name": node.get_name(), "class": node.get_class().get_name(), **properties(node, ["desc", "parameter_name", "default_value", "code"])})
    return result


def load_widget_templates():
    """枚举现有角色蓝图及其已有继承覆盖的世界控件模板，不创建额外覆盖。"""
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    registry.search_all_assets(True)
    subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    library = unreal.SubobjectDataBlueprintFunctionLibrary
    blueprints = {}
    # 先加载完整角色蓝图范围，使已有继承组件覆盖进入对象列表。
    for root in BLUEPRINT_ROOTS:
        for data in registry.get_assets_by_path(root, recursive=True):
            if str(data.asset_class_path.asset_name) != "Blueprint":
                continue
            bp = data.get_asset()
            if isinstance(bp, unreal.Blueprint):
                blueprints[bp.get_path_name().split(".")[0]] = bp
    candidates = []
    # 保留原有父模板枚举，不调用可能创建继承覆盖的关联对象接口。
    for bp in blueprints.values():
        for handle in subsystem.k2_gather_subobject_data_for_blueprint(bp):
            obj = library.get_object(library.get_data(handle))
            if isinstance(obj, unreal.WidgetComponent):
                candidates.append((bp, obj))
    # 单独补齐各蓝图包中已存在的继承模板，避免父模板去重漏掉子类覆盖。
    for obj in unreal.ObjectIterator(unreal.WidgetComponent):
        bp = blueprints.get(obj.get_path_name().split(".")[0])
        if bp:
            candidates.append((bp, obj))
    result = []
    seen = set()
    for bp, obj in candidates:
        path = obj.get_path_name()
        if path in seen or any(marker in path for marker in ("SKEL_", "REINST_", "TRASHCLASS_", "HOTRELOADED_")):
            continue
        seen.add(path)
        if obj.get_editor_property("space") == unreal.WidgetSpace.WORLD:
            # 父节点可能由子类枚举得到，按实际所属包归属，保证备份和保存正确。
            owner = blueprints.get(path.split(".")[0], bp)
            result.append((owner, obj))
    return result

def inspect():
    """记录当前技能和 UI 模板的实际输出、空间与材质配置。"""
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    registry.search_all_assets(True)
    result = {"materials": [], "widgets": []}
    for data in registry.get_assets_by_path(SKILL_ROOT, recursive=True):
        if str(data.asset_class_path.asset_name) == "Material":
            result["materials"].append(material_info(data.get_asset()))
    for bp, obj in load_widget_templates():
        material = obj.get_material(0)
        result["widgets"].append({"blueprint": bp.get_path_name(), "component": obj.get_path_name(), "properties": properties(obj, ["space", "blend_mode", "two_sided", "widget_class", "override_materials"]), "material": material.get_path_name() if material else None})
    engine_master = unreal.load_asset("/Engine/EngineMaterials/Widget3DPassThrough")
    result["engine_widget_material"] = material_info(engine_master)
    target = OUT / ("当前检查.json" if (OUT / "修复前检查.json").exists() else "修复前检查.json")
    target.write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding="utf-8")
    return {"materials": len(result["materials"]), "widgets": result["widgets"]}


def repair():
    """迁移已审计的自发光材质及世界 UI 模板，逐包备份后保存。"""
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    registry.search_all_assets(True)
    report = {"skills": [], "ui_materials": [], "widgets": []}
    for data in registry.get_assets_by_path(SKILL_ROOT, recursive=True):
        if str(data.asset_class_path.asset_name) != "Material":
            continue
        material = data.get_asset()
        assert material.get_editor_property("shading_model") == unreal.MaterialShadingModel.MSM_UNLIT
        before = material_info(material)
        saved_backup = backup(material.get_path_name())
        changed = wrap_emissive(material)
        after = material_info(material)
        assert before["outputs"]["opacity"] == after["outputs"]["opacity"]
        assert before["outputs"]["opacity_mask"] == after["outputs"]["opacity_mask"]
        if changed:
            assert unreal.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=False)
        report["skills"].append({"material": material.get_path_name(), "changed": changed, "backup": saved_backup})
    master_path = UI_ROOT + "场景界面曝光"
    master = unreal.load_asset(master_path) if unreal.EditorAssetLibrary.does_asset_exist(master_path) else unreal.EditorAssetLibrary.duplicate_asset("/Engine/EngineMaterials/Widget3DPassThrough", master_path)
    assert master and isinstance(master, unreal.Material)
    wrap_emissive(master)
    assert unreal.EditorAssetLibrary.save_loaded_asset(master, only_if_is_dirty=False)
    mapping = {}
    for suffix, label in [("Translucent", "半透明双面"), ("Translucent_OneSided", "半透明单面"), ("Opaque", "不透明双面"), ("Opaque_OneSided", "不透明单面"), ("Masked", "遮罩双面"), ("Masked_OneSided", "遮罩单面")]:
        source = "/Engine/EngineMaterials/Widget3DPassThrough_" + suffix
        target = UI_ROOT + "场景界面" + label
        instance = unreal.load_asset(target) if unreal.EditorAssetLibrary.does_asset_exist(target) else unreal.EditorAssetLibrary.duplicate_asset(source, target)
        assert isinstance(instance, unreal.MaterialInstanceConstant), "引擎控件材质类型与预期不符：" + source
        mapping[source] = instance
        report["ui_materials"].append({"source": source, "target": target})
    # 保留引擎实例原有父链，避免单面/混合模式等继承设置发生变化。
    for source, instance in mapping.items():
        parent = unreal.load_asset(source).get_editor_property("parent")
        parent_path = parent.get_path_name().split(".")[0]
        project_parent = master if parent_path == "/Engine/EngineMaterials/Widget3DPassThrough" else mapping[parent_path]
        unreal.MaterialEditingLibrary.set_material_instance_parent(instance, project_parent)
        unreal.MaterialEditingLibrary.update_material_instance(instance)
        assert unreal.EditorAssetLibrary.save_loaded_asset(instance, only_if_is_dirty=False)
    dirty = {}
    for bp, obj in load_widget_templates():
        original = obj.get_material(0)
        if not original:
            continue
        original_path = original.get_path_name().split(".")[0]
        if original_path not in mapping:
            continue
        # 跳过原生 /Script 类模板；它们由 C++ 构造函数设置项目专用材质。
        if not obj.get_path_name().startswith("/Game/"):
            report["widgets"].append({"component": obj.get_path_name(), "native": True})
            continue
        saved_backup = backup(bp.get_path_name())
        obj.modify()
        obj.set_material(0, mapping[original_path])
        bp.modify()
        dirty[bp.get_path_name()] = bp
        report["widgets"].append({"blueprint": bp.get_path_name(), "component": obj.get_path_name(), "old": original_path, "new": mapping[original_path].get_path_name(), "backup": saved_backup})
    for bp in dirty.values():
        unreal.BlueprintEditorLibrary.compile_blueprint(bp)
        assert unreal.EditorAssetLibrary.save_loaded_asset(bp, only_if_is_dirty=False)
    (OUT / "修复结果.json").write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
    return report



def verify():
    """核验曝光输出、透明度、控件模板及实例父链，不修改资产。"""
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    registry.search_all_assets(True)
    before = json.loads((OUT / "修复前检查.json").read_text(encoding="utf-8"))
    original = {item["path"]: item for item in before["materials"]}
    report = {"skills": [], "widgets": [], "ui_variants": []}
    for data in registry.get_assets_by_path(SKILL_ROOT, recursive=True):
        if str(data.asset_class_path.asset_name) != "Material":
            continue
        material = data.get_asset()
        info = material_info(material)
        assert info["outputs"]["emissive"]["class"] == "MaterialExpressionEyeAdaptationInverse"
        assert info["outputs"]["opacity"] == original[info["path"]]["outputs"]["opacity"]
        assert info["outputs"]["opacity_mask"] == original[info["path"]]["outputs"]["opacity_mask"]
        assert not wrap_emissive(material), "已有材质重复添加补偿"
        report["skills"].append(info["path"])
    for suffix, label in [("Translucent", "半透明双面"), ("Translucent_OneSided", "半透明单面"), ("Opaque", "不透明双面"), ("Opaque_OneSided", "不透明单面"), ("Masked", "遮罩双面"), ("Masked_OneSided", "遮罩单面")]:
        source = unreal.load_asset("/Engine/EngineMaterials/Widget3DPassThrough_" + suffix)
        target = unreal.load_asset(UI_ROOT + "场景界面" + label)
        assert source.get_editor_property("base_property_overrides").export_text() == target.get_editor_property("base_property_overrides").export_text()
        parent = target.get_editor_property("parent")
        assert parent.get_path_name().startswith(UI_ROOT)
        report["ui_variants"].append(target.get_path_name())
    for bp, component in load_widget_templates():
        if not component.get_path_name().startswith("/Game/"):
            continue
        material = component.get_material(0)
        assert material and material.get_path_name().startswith(UI_ROOT), component.get_path_name() + "没有使用项目曝光材质"
        report["widgets"].append({"blueprint": bp.get_path_name(), "component": component.get_path_name(), "material": material.get_path_name()})
    assert len(report["skills"]) == 8
    assert len(report["widgets"]) >= 20
    (OUT / "最终结构验收.json").write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
    return {"skills": len(report["skills"]), "widget_templates": len(report["widgets"]), "variants": len(report["ui_variants"]), "opacity_preserved": True, "idempotent": True}
@unreal.uclass()
class WorldEmissionTools(unreal.ToolsetDefinition):
    """仅提供本次曝光修复与隔离验证的官方 MCP 入口。"""

    @unreal.ufunction(static=True, params=[str], ret=str, meta={"AICallable": "", "DisplayName": "处理场景自发光", "Category": "场景显示|维护"})
    def process(stage):
        """检查、修复或执行固定路径中的本地后续验证脚本。"""
        try:
            if stage == "inspect":
                result = inspect()
            elif stage == "verify":
                result = verify()
            elif stage == "repair":
                result = repair()
            elif stage == "play":
                unreal.EditorLoadingAndSavingUtils.load_map("/Game/项目内容/关卡/唯一关卡")
                unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play()
                result = {"play_requested": True}
            elif stage == "capture":
                report = unreal.LxSkillArtAssetLibrary.process_skill_art("exposure_render")
                if report.startswith("ERROR:"):
                    raise RuntimeError(report)
                result = {"report": report}
            elif stage == "stop":
                unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_end_play()
                result = {"stop_requested": True}
            elif stage == "extra":
                exec((OUT / "后续验证.py").read_text(encoding="utf-8"), globals())
                result = globals().get("RESULT", "完成")
            elif stage == "quit":
                unreal.SystemLibrary.execute_console_command(None, "QUIT_EDITOR")
                result = {"quit": True}
            else:
                raise ValueError("不支持的步骤：" + stage)
            return json.dumps(result, ensure_ascii=False)
        except Exception:
            message = traceback.format_exc()
            (OUT / "操作错误.txt").write_text(message, encoding="utf-8")
            return json.dumps({"error": message}, ensure_ascii=False)


unreal.EditorPythonScripting.set_keep_python_script_alive(True)
unreal.ToolsetRegistry.register_toolset_class(WorldEmissionTools)
unreal.SystemLibrary.execute_console_command(None, "ModelContextProtocol.StartServer 8061")
unreal.log("场景自发光修复官方 MCP 已就绪，端口 8061")