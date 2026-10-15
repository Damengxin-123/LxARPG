"""通过 UE 5.8 官方 MCP 审计、制作、接入和验收现有技能美术。"""
import json
from pathlib import Path
import unreal

# 允许本专用会话的只读资源句柄共享文件替换，避免保存原位精修资源时自占用。
unreal.SystemLibrary.execute_console_command(None, "file.allowdeleteopenfiles 1")


def inspect_skills():
    """读取实际技能流程和各蓝图组件，保存审计报告供制作工具使用。"""
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    registry.search_all_assets(True)
    assets = registry.get_assets_by_class(unreal.TopLevelAssetPath("/Script/LxARPG", "LxSkillFlowAsset"), True)
    result = []
    classes = {}
    fields = ["projectile_class", "area_class", "ray_class", "ground_bounce_class", "lob_class", "single_ray_class", "scaling_area_class", "continuous_attach_class", "periodic_attach_class", "continuous_aura_class", "periodic_aura_class", "element_abnormal_attach_class", "melee_class", "duration_area_class", "spawn_entity_class", "summon_creature_class", "marker_class", "barrier_class"]
    for data in assets:
        flow = data.get_asset()
        nodes = []
        for node in flow.get_editor_property("nodes"):
            item = {"kind": str(node.get_editor_property("kind")), "id": str(node.get_editor_property("id"))}
            for field in fields:
                try:
                    value = node.get_editor_property(field)
                    if value:
                        item[field] = value.get_path_name()
                        classes[value.get_path_name()] = value
                except Exception:
                    pass
            for field in ["aura_range", "lifetime", "entry_package_index", "next", "hit", "finished", "projectile", "area", "ray", "scaling_area", "continuous_aura", "periodic_aura", "single_ray", "lob", "ground_bounce", "periodic_attach", "element_abnormal_attach"]:
                try:
                    item[field] = str(node.get_editor_property(field))
                except Exception:
                    pass
            nodes.append(item)
        result.append({"path": flow.get_path_name(), "nodes": nodes})
    component_reports = []
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    for path, cls in classes.items():
        actor = actors.spawn_actor_from_class(cls, unreal.Vector(0, 0, 0), transient=True)
        if not actor:
            continue
        components = []
        for component in actor.get_components_by_class(unreal.SceneComponent):
            item = {"name": component.get_name(), "type": component.get_class().get_name(), "location": str(component.get_editor_property("relative_location")), "scale": str(component.get_editor_property("relative_scale3d"))}
            for field in ["static_mesh", "asset", "sphere_radius", "capsule_half_height", "capsule_radius", "hidden_in_game", "visible"]:
                try:
                    value = component.get_editor_property(field)
                    item[field] = value.get_path_name() if isinstance(value, unreal.Object) else str(value)
                except Exception:
                    pass
            components.append(item)
        component_reports.append({"class": path, "parent": cls.get_super_class().get_name() if hasattr(cls, "get_super_class") else "", "components": components})
        actors.destroy_actor(actor)
    report = {"flows": result, "classes": component_reports}
    output = Path(unreal.Paths.project_saved_dir()) / "技能美术" / "现有技能审计.json"
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
    return json.dumps({"flows": [{"path": x["path"], "nodes": [{k: v for k, v in n.items() if k == "kind" or k.endswith("_class")} for n in x["nodes"]]} for x in result], "class_count": len(component_reports)}, ensure_ascii=False)


def inspect_test_profession():
    """读取实际职业体系、技能表和授予技能词条，供测试职业配置使用。"""
    manager_class = unreal.load_class(None, "/Game/项目内容/功能资产/数据表格/BP_数据表格配置.BP_数据表格配置_C")
    manager = unreal.get_default_object(manager_class)
    tree = manager.get_editor_property("ProfessionTreeAsset")
    skills = manager.get_editor_property("m_pSkillItemTable")
    grants = manager.get_editor_property("m_pGrantSkillEntryTable")
    report = {"manager": manager_class.get_path_name(), "tree": tree.get_path_name(),
              "skill_table": skills.get_path_name(), "grant_table": grants.get_path_name(),
              "professions": [{"id": str(x.get_editor_property("ProfessionID")),
                               "name": str(x.get_editor_property("DisplayName")),
                               "levels": str(x.get_editor_property("Levels"))}
                              for x in tree.get_editor_property("Professions")],
              "skills": json.loads(unreal.DataTableFunctionLibrary.export_data_table_to_json_string(skills)),
              "grants": json.loads(unreal.DataTableFunctionLibrary.export_data_table_to_json_string(grants))}
    output = Path(unreal.Paths.project_saved_dir()) / "技能美术" / "测试职业检查.json"
    output.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
    return json.dumps({"tree": report["tree"], "professions": len(report["professions"]),
                       "skills": len(report["skills"]), "grants": len(report["grants"]),
                       "report": str(output)}, ensure_ascii=False)

@unreal.uclass()
class SkillArtTools(unreal.ToolsetDefinition):
    """现有技能的美术制作、流程接入与实际渲染验收入口。"""

    @unreal.ufunction(static=True, params=[str], ret=str,
                     meta={"AICallable": "", "DisplayName": "处理技能美术", "Category": "技能美术|制作"})
    def process_skill_art(stage):
        """按阶段处理技能资产，所有外观修改保持现有玩法参数。"""
        if stage == "profession_inspect":
            return inspect_test_profession()
        if stage == "inspect":
            return inspect_skills()
        if stage == "visibility_inspect":
            # 查看真实技能蓝图的显示默认值，诊断遗留隐藏状态与主体配置。
            reports = []
            actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
            for category, name in [("光环效果", "光环测试_01"), ("投射物", "复合测试_01"), ("投射物", "火球术_01")]:
                root = "/Game/项目内容/功能资产/技能资产/技能单元实体/"
                path = root + category + "/" + name
                # 分类迁移前仍读取原蓝图；迁移后始终优先检查正式分类目录。
                if not unreal.EditorAssetLibrary.does_asset_exist(path):
                    path = root + "技能表现/" + name
                cls = unreal.load_class(None, path + "." + name + "_C")
                actor = actors.spawn_actor_from_class(cls, unreal.Vector(0, 0, 0), transient=True)
                item = {"class": cls.get_path_name(), "actor_scale": str(actor.get_actor_scale3d())}
                for field in ["hidden", "initial_life_span"]:
                    try:
                        item[field] = str(actor.get_editor_property(field))
                    except Exception:
                        pass
                for visual in actor.get_components_by_class(unreal.LxSkillVisualComponent):
                    for field in ["body_mesh", "body_material", "body_scale", "effect_scale", "body_color", "body_emissive_strength", "placement", "loop_effect"]:
                        value = visual.get_editor_property(field)
                        item[field] = value.get_path_name() if isinstance(value, unreal.Object) else str(value)
                reports.append(item)
                actors.destroy_actor(actor)
            return json.dumps(reports, ensure_ascii=False, indent=2)
        if stage == "quit":
            unreal.SystemLibrary.execute_console_command(None, "QUIT_EDITOR")
            return "已请求关闭技能美术专用会话"
        if stage == "profession_tests":
            unreal.SystemLibrary.execute_console_command(None, "Automation RunTests LxARPG.ProfessionTree.")
            return "已启动职业体系回归验证"
        if stage == "tests":
            unreal.SystemLibrary.execute_console_command(None, "Automation RunTests LxARPG.SkillFlow")
            return "已启动技能流程回归验证"
        return unreal.LxSkillArtAssetLibrary.process_skill_art(stage)


unreal.EditorPythonScripting.set_keep_python_script_alive(True)
unreal.ToolsetRegistry.register_toolset_class(SkillArtTools)
unreal.SystemLibrary.execute_console_command(None, "ModelContextProtocol.StartServer 8051")
unreal.log("技能美术官方MCP已就绪，端口8051")
