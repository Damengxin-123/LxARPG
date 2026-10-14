"""通过 UE 官方 MCP 检查灯具材质与曝光，仅处理灯具自发光。"""
import json
import shutil
import time
import traceback
from pathlib import Path
import unreal

# 本次灯具诊断的独立输出目录。
OUT = Path(unreal.Paths.project_saved_dir()) / "LampEmission"
OUT.mkdir(parents=True, exist_ok=True)
# 仅检查项目自有的两种晶核材质。
MATERIALS = [
    "/Game/项目内容/美术资产/海港城市/路灯/材质/悬灯魔晶",
    "/Game/项目内容/美术资产/海港城市/魔灯/材质/魔纹晶核",
]
# 离屏验收的跨帧状态，所有临时镜头在完成后回收。
STATE = {}


def inspect_lamps():
    """读取当前关卡灯具、晶核参数和太阳曝光，不修改资产。"""
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
    result = {"world": unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world().get_path_name(),
              "lamps": [], "sun": [], "materials": []}
    for actor in actors:
        street = isinstance(actor, unreal.LxMagicStreetLampActor)
        magic = isinstance(actor, unreal.LxMagicLampActor)
        if street or magic:
            mesh = actor.get_editor_property("street_mesh" if street else "lamp_mesh")
            slots = [3, 4] if street else [2]
            lights = ["road_light", "sidewalk_light"] if street else ["lamp_light"]
            entry = {"name": actor.get_actor_label(), "class": actor.get_class().get_path_name(),
                     "location": str(actor.get_actor_location()), "material": str(actor.get_editor_property("crystal_material")),
                     "cores": [], "lights": []}
            for slot in slots:
                core = mesh.get_material(slot)
                entry["cores"].append({"slot": slot, "material": str(core),
                    "glow": core.get_scalar_parameter_value("发光强度") if isinstance(core, unreal.MaterialInstanceDynamic) else None})
            for name in lights:
                light = actor.get_editor_property(name)
                entry["lights"].append({"name": name, "visible": light.is_visible(), "intensity": light.get_editor_property("intensity")})
            result["lamps"].append(entry)
        if isinstance(actor, unreal.LxSunActor):
            result["sun"].append({key: str(actor.get_editor_property(key)) for key in ["exposure_ev100", "stabilize_exposure", "preview_hour"]})
    for path in MATERIALS:
        mat = unreal.load_asset(path)
        result["materials"].append({"path": path, "loaded": bool(mat),
            "emissive_node": str(unreal.MaterialEditingLibrary.get_material_property_input_node(mat, unreal.MaterialProperty.MP_EMISSIVE_COLOR)) if mat else None})
    (OUT / "诊断.json").write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding="utf-8")
    return result


def repair_materials():
    """只给现有晶核输出增加曝光补偿，备份原资产并保持材质其他输入不变。"""
    result = []
    for path in MATERIALS:
        mat = unreal.load_asset(path)
        assert mat, "缺少晶核材质：" + path
        source = unreal.MaterialEditingLibrary.get_material_property_input_node(mat, unreal.MaterialProperty.MP_EMISSIVE_COLOR)
        if isinstance(source, unreal.MaterialExpressionEyeAdaptationInverse):
            result.append({"material": path, "already_fixed": True})
            continue
        assert isinstance(source, unreal.MaterialExpressionCustom), "晶核输出结构不匹配，停止修改"
        code = source.get_editor_property("code")
        assert "Tint.rgb*Power*" in code, "晶核着色代码不匹配，停止修改"
        filename = Path(unreal.Paths.project_content_dir()) / (path.removeprefix("/Game/") + ".uasset")
        backup = OUT / "原材质备份" / filename.name
        backup.parent.mkdir(parents=True, exist_ok=True)
        if not backup.exists():
            shutil.copy2(filename, backup)
        # 旧路灯在 EV4 展示关卡额外放大八倍；曝光补偿后回到统一晶核亮度。
        source.set_editor_property("code", code.replace("Power*8.000*", "Power*1.000*"))
        inverse = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionEyeAdaptationInverse)
        inverse.set_editor_property("desc", "灯芯曝光补偿")
        pin = unreal.MaterialEditingLibrary.get_material_expression_input_names(inverse)[0]
        assert unreal.MaterialEditingLibrary.connect_material_expressions(source, "", inverse, pin)
        assert unreal.MaterialEditingLibrary.connect_material_property(inverse, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
        unreal.MaterialEditingLibrary.layout_material_expressions(mat)
        unreal.MaterialEditingLibrary.recompile_material(mat)
        assert unreal.EditorAssetLibrary.save_loaded_asset(mat, only_if_is_dirty=False)
        result.append({"material": path, "fixed": True, "backup": str(backup), "input": pin})
    unreal.AutomationLibrary.finish_loading_before_screenshot()
    (OUT / "修复结果.json").write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding="utf-8")
    return result


def verify_lamps():
    """用六种临时灯具验证材质、独立开关、调色和几何缓存，随后销毁实例。"""
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    results = []
    for name in ("单侧悬灯", "双侧悬灯", "方晶壁灯", "方晶顶灯", "星环球灯", "花冠球灯"):
        bp = unreal.load_asset("/Game/项目内容/美术资产/自动化类型/灯具/" + name)
        actor = actors.spawn_actor_from_class(bp.generated_class(), unreal.Vector(0, 0, -100000))
        try:
            actor.set_editor_property("auto_enable", False)
            street = isinstance(actor, unreal.LxMagicStreetLampActor)
            mesh = actor.get_editor_property("street_mesh" if street else "lamp_mesh")
            core = mesh.get_material(3 if street else 2)
            output = unreal.MaterialEditingLibrary.get_material_property_input_node(actor.get_editor_property("crystal_material"), unreal.MaterialProperty.MP_EMISSIVE_COLOR)
            assert isinstance(output, unreal.MaterialExpressionEyeAdaptationInverse), "未连接曝光补偿"
            enabled = "road_light_enabled" if street else "light_enabled"
            brightness = "road_brightness_ev" if street else "brightness_ev"
            color = "road_light_color" if street else "light_color"
            temperature = "road_use_temperature" if street else "use_temperature"
            light = actor.get_editor_property("road_light" if street else "lamp_light")
            builds = actor.get_editor_property("geometry_build_count")
            actor.set_editor_property(enabled, True)
            actor.set_editor_property(brightness, 8.)
            low = core.get_scalar_parameter_value("发光强度")
            actor.set_editor_property(brightness, 10.)
            assert core.get_scalar_parameter_value("发光强度") > low > 0
            actor.set_editor_property(temperature, False)
            actor.set_editor_property(color, unreal.LinearColor(.2, .5, 1., 1.))
            tint = core.get_vector_parameter_value("魔力颜色")
            assert abs(tint.r-.2) < .001 and abs(tint.b-1.) < .001
            actor.set_editor_property(enabled, False)
            assert core.get_scalar_parameter_value("发光强度") == 0 and not light.is_visible()
            if name == "双侧悬灯":
                assert mesh.get_material(4).get_scalar_parameter_value("发光强度") > 0
            actor.set_editor_property(enabled, True)
            assert core.get_scalar_parameter_value("发光强度") > 0 and light.is_visible()
            assert actor.get_editor_property("geometry_build_count") == builds
            results.append({"preset": name, "exposure_compensation": True, "switch_color_brightness": True, "no_mesh_rebuild": True})
        finally:
            actors.destroy_actor(actor)
    (OUT / "验收结果.json").write_text(json.dumps(results, ensure_ascii=False, indent=2), encoding="utf-8")
    unreal.SystemLibrary.execute_console_command(None, "Automation RunTests LxARPG.World.MagicLamp")
    return results


def start_capture(after):
    """在实际关卡固定同一镜头，分别验收 EV10 与 EV4 的灯芯外观，不保存关卡。"""
    assert not STATE, "上一组离屏验收尚未完成"
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    lamp = next(a for a in actors.get_all_level_actors() if isinstance(a, unreal.LxMagicStreetLampActor)
                and a.get_actor_label() == "双侧悬灯")
    origin = lamp.get_actor_location()
    height = 60 + lamp.get_editor_property("pole_length") - lamp.get_editor_property("suspension_length") - 30
    target_location = origin + unreal.Vector(0, 0, height)
    location = target_location + lamp.get_actor_right_vector() * -650 + lamp.get_actor_forward_vector() * 190 + unreal.Vector(0, 0, 100)
    camera = actors.spawn_actor_from_class(unreal.SceneCapture2D, location)
    camera.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(location, target_location), False)
    component = camera.get_component_by_class(unreal.SceneCaptureComponent2D)
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    target = unreal.RenderingLibrary.create_render_target2d(world, 1200, 800, unreal.TextureRenderTargetFormat.RTF_RGBA8)
    for key, value in {"texture_target": target, "capture_source": unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR,
                       "capture_every_frame": False, "always_persist_rendering_state": True, "fov_angle": 46.}.items():
        component.set_editor_property(key, value)
    prefix = "修复后" if after else "修复前"
    phases = [(prefix + "_EV10", 10.), (prefix + "_EV4", 4.)]
    STATE.update(camera=camera, component=component, target=target, world=world, phases=phases, phase=0, until=0)
    configure_capture()
    unreal.AutomationLibrary.finish_loading_before_screenshot()
    STATE["handle"] = unreal.register_slate_post_tick_callback(capture_tick)
    return {"capture_started": prefix}


def configure_capture():
    """固定曝光和辉光强度，让修复前后图像可直接比较。"""
    _, exposure = STATE["phases"][STATE["phase"]]
    pp = STATE["component"].get_editor_property("post_process_settings")
    for key, value in {"override_auto_exposure_min_brightness": True, "override_auto_exposure_max_brightness": True,
                       "auto_exposure_min_brightness": exposure, "auto_exposure_max_brightness": exposure,
                       "override_auto_exposure_bias": True, "auto_exposure_bias": 0.,
                       "override_bloom_intensity": True, "bloom_intensity": .2}.items():
        pp.set_editor_property(key, value)
    STATE["component"].set_editor_property("post_process_settings", pp)
    STATE["until"] = time.monotonic() + 5


def capture_tick(delta):
    """等待曝光稳定后导出 PNG；完成或失败均清理临时镜头。"""
    finished = False
    try:
        STATE["component"].capture_scene()
        if time.monotonic() < STATE["until"]:
            return
        name, _ = STATE["phases"][STATE["phase"]]
        unreal.RenderingLibrary.export_render_target(STATE["world"], STATE["target"], str(OUT), name + ".png")
        STATE["phase"] += 1
        finished = STATE["phase"] == len(STATE["phases"])
        if not finished:
            configure_capture()
    except Exception:
        (OUT / "截图错误.txt").write_text(traceback.format_exc(), encoding="utf-8")
        finished = True
    finally:
        if finished:
            unreal.unregister_slate_post_tick_callback(STATE["handle"])
            unreal.get_editor_subsystem(unreal.EditorActorSubsystem).destroy_actor(STATE["camera"])
            STATE.clear()


@unreal.uclass()
class LampEmissionTools(unreal.ToolsetDefinition):
    """只提供灯具自发光的检查、修复和验收入口。"""

    @unreal.ufunction(static=True, params=[str], ret=str, meta={"AICallable": "", "DisplayName": "检查灯具自发光", "Category": "灯具|维护"})
    def process_lamp_emission(stage):
        """执行固定的灯具维护步骤，或结束本工具创建的离屏会话。"""
        try:
            if stage == "inspect":
                result = inspect_lamps()
            elif stage == "repair":
                result = repair_materials()
            elif stage == "verify":
                result = verify_lamps()
            elif stage in ("capture_before", "capture_after"):
                result = start_capture(stage == "capture_after")
            elif stage == "quit":
                unreal.SystemLibrary.quit_editor()
                result = {"quit": True}
            else:
                raise ValueError("不支持的灯具维护步骤：" + stage)
            return json.dumps(result, ensure_ascii=False)
        except Exception:
            return json.dumps({"error": traceback.format_exc()}, ensure_ascii=False)


unreal.EditorPythonScripting.set_keep_python_script_alive(True)
unreal.ToolsetRegistry.register_toolset_class(LampEmissionTools)
unreal.SystemLibrary.execute_console_command(None, "ModelContextProtocol.StartServer 8008")
unreal.SystemLibrary.execute_console_command(None, "ModelContextProtocol.RefreshTools")
