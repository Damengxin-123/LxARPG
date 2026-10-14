"""通过 UE 5.8 官方 MCP 创建木门预设、材质、合成音效并验收。"""
import json
import math
import random
import struct
import time
import traceback
import wave
from pathlib import Path
import unreal

# 中文资产目录与验收输出。
BASE = "/Game/项目内容/美术资产/海港城市/木门"
OUT = Path(unreal.Paths.project_dir()).resolve() / "Saved/WoodenDoor"
OUT.mkdir(parents=True, exist_ok=True)
# 离屏截图的跨帧状态。
STATE = {}


def material(name, tint, metal=False, flat=False, refresh=False):
    """创建清爽木纹或柔和黄铜材质，显式刷新时原位更新指定材质。"""
    path = BASE + "/材质/" + name
    exists = unreal.EditorAssetLibrary.does_asset_exist(path)
    if exists and not refresh:
        return unreal.load_asset(path)
    mat = unreal.load_asset(path) if exists else unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, BASE + "/材质", unreal.Material, unreal.MaterialFactoryNew())
    lib = unreal.MaterialEditingLibrary
    if exists:
        lib.delete_all_material_expressions(mat)
    tint_node = lib.create_material_expression(mat, unreal.MaterialExpressionVectorParameter)
    tint_node.set_editor_property("parameter_name", "基础颜色")
    tint_node.set_editor_property("default_value", unreal.LinearColor(*tint, 1))
    if metal or flat:
        color = tint_node
    else:
        uv = lib.create_material_expression(mat, unreal.MaterialExpressionTextureCoordinate)
        wood = lib.create_material_expression(mat, unreal.MaterialExpressionCustom)
        wood.set_editor_property("description", "低密度低对比的手绘感纵向木纹")
        wood.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
        wood.set_editor_property("code", "float bend=sin(UV.x*1.6)*.022+sin(UV.x*3.3)*.006; float grain=sin((UV.y+bend)*Density); float broad=sin(UV.y*8+UV.x*.15); return Tint.rgb*(1+Strength*(grain*.75+broad*.25));")
        density = lib.create_material_expression(mat, unreal.MaterialExpressionScalarParameter)
        density.set_editor_property("parameter_name", "木纹密度")
        density.set_editor_property("default_value", 30.0)
        strength = lib.create_material_expression(mat, unreal.MaterialExpressionScalarParameter)
        strength.set_editor_property("parameter_name", "木纹强度")
        strength.set_editor_property("default_value", .035)
        inputs = []
        sources = (("UV", uv), ("Tint", tint_node), ("Density", density), ("Strength", strength))
        for name_, source in sources:
            entry = unreal.CustomInput()
            entry.set_editor_property("input_name", name_)
            inputs.append(entry)
        wood.set_editor_property("inputs", inputs)
        for name_, source in sources:
            lib.connect_material_expressions(source, "", wood, name_)
        color = wood
    lib.connect_material_property(color, "", unreal.MaterialProperty.MP_BASE_COLOR)
    for prop, value in ((unreal.MaterialProperty.MP_ROUGHNESS, .48 if metal else .76),
                        (unreal.MaterialProperty.MP_METALLIC, .35 if metal else 0)):
        node = lib.create_material_expression(mat, unreal.MaterialExpressionConstant)
        node.set_editor_property("r", value)
        lib.connect_material_property(node, "", prop)
    lib.layout_material_expressions(mat)
    lib.recompile_material(mat)
    unreal.EditorAssetLibrary.save_loaded_asset(mat)
    return mat


def sound(name, closing=False):
    """程序合成短木质摩擦声，非录音；保留 WAV 源文件便于替换。"""
    path = BASE + "/音效/" + name
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        return unreal.load_asset(path)
    rate, duration = 44100, 1.4
    rng, phase, samples = random.Random(417), 0.0, []
    for index in range(int(rate * duration)):
        t = index / rate
        u = t / duration
        envelope = math.sin(math.pi * u) ** 1.5
        frequency = 170 + 55 * math.sin(u * math.pi) + 8 * math.sin(t * 31)
        phase += 2 * math.pi * frequency / rate
        friction = (.065 * math.sin(phase) + .032 * math.sin(phase * 2.013)
                    + .015 * math.sin(phase * 3.01) + .014 * rng.uniform(-1, 1))
        value = envelope * friction * (.75 + .25 * math.sin(t * 47) ** 2)
        if closing and t > 1.22:
            age = t - 1.22
            value += .09 * math.exp(-age * 38) * math.sin(age * 2 * math.pi * 90)
        samples.append(struct.pack("<h", int(max(-1, min(1, value)) * 32767)))
    wav = OUT / (name + ".wav")
    with wave.open(str(wav), "wb") as writer:
        writer.setnchannels(1)
        writer.setsampwidth(2)
        writer.setframerate(rate)
        writer.writeframes(b"".join(samples))
    task = unreal.AssetImportTask()
    for prop, value in {"filename": str(wav), "destination_path": BASE + "/音效",
                        "destination_name": name, "automated": True, "save": True}.items():
        task.set_editor_property(prop, value)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    result = unreal.load_asset(path)
    if not result:
        raise RuntimeError("音效导入失败：" + name)
    return result


def build():
    """创建带实际蓝图连线、机关交互树及材质音效的两种木门。"""
    if not unreal.LxWoodenDoorAssetLibrary.create_door_presets():
        raise RuntimeError("木门蓝图或交互树创建失败")
    settings = {"wood_material": material("暖木门板", (.62, .40, .21)),
                "frame_material": material("深棕门框", (.33, .19, .09)),
                "metal_material": material("哑光黄铜", (.43, .28, .12), True),
                "open_sound": sound("木门开启"), "close_sound": sound("木门关闭", True)}
    for name in ("单扇木门", "双扇木门"):
        bp = unreal.load_asset(BASE + "/" + name)
        default = unreal.get_default_object(bp.generated_class())
        for key, value in settings.items():
            if not default.get_editor_property(key):
                default.set_editor_property(key, value)
        unreal.BlueprintEditorLibrary.compile_blueprint(bp)
        unreal.EditorAssetLibrary.save_loaded_asset(bp, only_if_is_dirty=False)
    return verify()


def refresh_style():
    """原位更新三种木门材质，关卡和已有预设继续引用相同资产。"""
    material("暖木门板", (.62, .40, .21), refresh=True)
    material("深棕门框", (.33, .19, .09), refresh=True)
    material("哑光黄铜", (.43, .28, .12), metal=True, refresh=True)
    report = verify()
    # 只更新工具自带的展示关卡，保留示例门的位置和开合姿态。
    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    levels.load_level(BASE + "/木门展示")
    for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
        if isinstance(actor, unreal.LxWoodenDoorActor):
            actor.rebuild_door()
    levels.save_current_level()
    return report


def verify():
    """实例化已保存蓝图，检查网格、音效、交互配置和开闭姿态。"""
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    report = []
    for name in ("单扇木门", "双扇木门"):
        bp = unreal.load_asset(BASE + "/" + name)
        actor = actors.spawn_actor_from_class(bp.generated_class(), unreal.Vector(0, 0, -5000))
        try:
            actor.rebuild_door()
            if actor.get_editor_property("generation_diagnostic"):
                raise RuntimeError(actor.get_editor_property("generation_diagnostic"))
            meshes = [actor.get_editor_property(key).get_editor_property("static_mesh")
                      for key in ("frame_mesh", "left_leaf", "right_leaf")]
            if not all(meshes) or not actor.get_editor_property("open_sound"):
                raise RuntimeError("木门网格或音效缺失")
            provider = actor.get_editor_property("interactable")
            if not provider.get_editor_property("interaction_tree_asset"):
                raise RuntimeError("没有连接原有机关交互树")
            actor.animate_door(True, True)
            left = actor.get_editor_property("left_leaf").get_editor_property("relative_rotation").yaw
            right = actor.get_editor_property("right_leaf").get_editor_property("relative_rotation").yaw
            if abs(left - 100) > .01 or (name == "双扇木门" and abs(right + 100) > .01):
                raise RuntimeError("门扇铰链旋转错误")
            actor.animate_door(False, True)
            report.append({"preset": name, "triangles": sum(mesh.get_num_triangles(0) for mesh in meshes),
                           "left_open": left, "right_open": right, "interaction": True, "sound": True})
        finally:
            actors.destroy_actor(actor)
    (OUT / "预设验收.json").write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
    return report


def demo():
    """创建独立木门展示关卡，保存关闭及开启的示例。"""
    unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    for name, preset, x, opened in (("单扇木门", "单扇木门", -260, False),
                                   ("双扇木门", "双扇木门", 0, False),
                                   ("双门开启示意", "双扇木门", 320, True)):
        bp = unreal.load_asset(BASE + "/" + preset)
        actor = actors.spawn_actor_from_class(bp.generated_class(), unreal.Vector(x, 0, 0))
        actor.set_actor_label(name)
        actor.animate_door(opened, True)
    floor = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(50, 0, -12))
    floor.set_actor_label("展示地台")
    floor.static_mesh_component.set_static_mesh(unreal.load_asset("/Engine/BasicShapes/Cube"))
    floor.static_mesh_component.set_material(0, unreal.load_asset("/Engine/BasicShapes/BasicShapeMaterial"))
    floor.set_actor_scale3d(unreal.Vector(12, 7, .2))
    backdrop = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(0, 700, 650))
    backdrop.set_actor_label("浅蓝展示背景")
    backdrop.static_mesh_component.set_static_mesh(unreal.load_asset("/Engine/BasicShapes/Cube"))
    backdrop.static_mesh_component.set_material(0, material("浅蓝背景", (.32, .50, .68), flat=True))
    backdrop.static_mesh_component.set_editor_property("cast_shadow", False)
    backdrop.set_actor_scale3d(unreal.Vector(45, .2, 18))
    for label, rotation, intensity in (("温暖日光", (-40, -65, 0), 8), ("清透补光", (25, 100, 0), 6)):
        light = actors.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 1000),
                                             unreal.Rotator(pitch=rotation[0], yaw=rotation[1], roll=rotation[2]))
        light.set_actor_label(label)
        light.light_component.set_editor_property("intensity", intensity)
        light.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
        light.light_component.set_editor_property("cast_shadows", label == "温暖日光")
        light.light_component.set_editor_property("atmosphere_sun_light", label == "温暖日光")
    sky = actors.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 500))
    sky.set_actor_label("天空补光")
    sky.get_component_by_class(unreal.SkyLightComponent).set_mobility(unreal.ComponentMobility.MOVABLE)
    sky.get_component_by_class(unreal.SkyLightComponent).set_editor_property("real_time_capture", True)
    actors.spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector())
    unreal.EditorLoadingAndSavingUtils.save_map(unreal.EditorLevelLibrary.get_editor_world(), BASE + "/木门展示")
    return {"map": BASE + "/木门展示"}


def reload_level():
    """从磁盘重载展示关卡，验证保存的实例网格、材质和碰撞。"""
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level(BASE + "/木门展示")
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    doors = [actor for actor in actors.get_all_level_actors() if isinstance(actor, unreal.LxWoodenDoorActor)]
    if len(doors) != 3:
        raise RuntimeError("展示关卡木门数量错误")
    for actor in doors:
        for key in ("frame_mesh", "left_leaf", "right_leaf"):
            component = actor.get_editor_property(key)
            mesh = component.get_editor_property("static_mesh")
            if not mesh or mesh.get_num_triangles(0) <= 0:
                raise RuntimeError("重载后丢失木门网格")
        if actor.get_editor_property("left_collision").get_attach_parent() != actor.get_editor_property("left_leaf"):
            raise RuntimeError("重载后门扇碰撞连接错误")
    report = {"reloaded_doors": len(doors), "saved_geometry": True, "attached_collision": True}
    (OUT / "重载验收.json").write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
    return report


def capture(detail=False):
    """在独立展示关卡离屏渲染全景或花饰近景。"""
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level(BASE + "/木门展示")
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    location = unreal.Vector(95, -420, 195) if detail else unreal.Vector(700, -1150, 580)
    look = unreal.Vector(0, 0, 125) if detail else unreal.Vector(80, 0, 110)
    camera = actors.spawn_actor_from_class(unreal.SceneCapture2D, location)
    camera.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(camera.get_actor_location(), look), False)
    component = camera.get_component_by_class(unreal.SceneCaptureComponent2D)
    target = unreal.RenderingLibrary.create_render_target2d(camera, 1600, 900, unreal.TextureRenderTargetFormat.RTF_RGBA8)
    component.set_editor_property("texture_target", target)
    component.set_editor_property("capture_source", unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR)
    component.set_editor_property("capture_every_frame", False)
    component.set_editor_property("always_persist_rendering_state", True)
    component.set_editor_property("fov_angle", 48)
    pp = component.get_editor_property("post_process_settings")
    for key, value in {"override_auto_exposure_min_brightness": True, "override_auto_exposure_max_brightness": True,
                       "auto_exposure_min_brightness": 1.0, "auto_exposure_max_brightness": 1.0}.items():
        pp.set_editor_property(key, value)
    component.set_editor_property("post_process_settings", pp)
    unreal.AutomationLibrary.finish_loading_before_screenshot()
    STATE.update(camera=camera, component=component, target=target, ready=time.monotonic() + 8,
                 filename="木门花饰近景.png" if detail else "木门预览.png")
    STATE["handle"] = unreal.register_slate_post_tick_callback(capture_tick)
    return {"capture_started": True}


def capture_tick(delta):
    """等待光照和着色器收敛后导出预览并清理摄影机。"""
    STATE["component"].capture_scene()
    if time.monotonic() < STATE["ready"]:
        return
    unreal.unregister_slate_post_tick_callback(STATE["handle"])
    unreal.RenderingLibrary.export_render_target(STATE["camera"], STATE["target"], str(OUT), STATE["filename"])
    unreal.get_editor_subsystem(unreal.EditorActorSubsystem).destroy_actor(STATE["camera"])


@unreal.uclass()
class WoodenDoorTools(unreal.ToolsetDefinition):
    """木门自动化的官方 MCP 入口。"""
    @unreal.ufunction(static=True, params=[str], ret=str, meta={"AICallable": "", "DisplayName": "处理木门资产", "Category": "木门|制作"})
    def process_door(stage):
        """执行制作、验收、展示、截图或测试。"""
        try:
            if stage == "build": result = build()
            elif stage == "style": result = refresh_style()
            elif stage == "verify": result = verify()
            elif stage == "demo": result = demo()
            elif stage == "reload": result = reload_level()
            elif stage == "capture": result = capture()
            elif stage == "detail": result = capture(True)
            elif stage == "tests":
                unreal.SystemLibrary.execute_console_command(None, "Automation RunTests LxARPG.World.WoodenDoor")
                result = {"tests_started": True}
            elif stage == "quit":
                unreal.SystemLibrary.quit_editor()
                result = {"quit": True}
            else: raise ValueError(stage)
            return json.dumps(result, ensure_ascii=False)
        except Exception:
            return json.dumps({"error": traceback.format_exc()}, ensure_ascii=False)


unreal.EditorPythonScripting.set_keep_python_script_alive(True)
unreal.ToolsetRegistry.register_toolset_class(WoodenDoorTools)
unreal.SystemLibrary.execute_console_command(None, "ModelContextProtocol.StartServer 8007")
unreal.SystemLibrary.execute_console_command(None, "ModelContextProtocol.RefreshTools")
