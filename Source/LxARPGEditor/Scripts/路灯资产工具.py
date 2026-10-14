"""使用 UE 5.8 官方 MCP 制作两种石座金属悬吊路灯。"""
import json
import time
import traceback
from pathlib import Path
import math
import unreal

# 复用魔灯材质节点工具，不执行其工具注册或修改原有魔灯资产。
SOURCE = Path(unreal.Paths.project_dir()).resolve() / "Source/LxARPGEditor/Scripts/魔灯资产工具.py"
HELPERS = {"__name__": "street_material_helpers"}
exec(compile(SOURCE.read_text(encoding="utf-8").split("\n@unreal.uclass()")[0], str(SOURCE), "exec"), HELPERS)
BASE = "/Game/项目内容/美术资产/海港城市/路灯"
OUT = Path(unreal.Paths.project_dir()).resolve() / "Saved/StreetLamp"
OUT.mkdir(parents=True, exist_ok=True)
HELPERS["BASE"] = BASE
# 两种可复用的中文路灯预设。
PRESETS = (("单侧悬灯", unreal.LxStreetLampLayout.SINGLE), ("双侧悬灯", unreal.LxStreetLampLayout.DOUBLE))
# 离屏捕获的跨帧状态。
STATE = {}


def build_assets():
    """生成中文材质和两种路灯蓝图，逻辑全部继承 C++。"""
    for folder in ("材质", "预设", "演示"):
        unreal.EditorAssetLibrary.make_directory(BASE + "/" + folder)
    material = HELPERS["material"]
    stone = material("浅纹白石", (.59, .62, .63), .0, .8)
    metal = material("海蓝锻金", (.055, .105, .135), .65, .48)
    gold = material("浮雕浅金", (.58, .36, .12), .48, .48)
    crystal = material("悬灯魔晶", (.28, .34, .37), .03, .5, True, 1.0)
    result = []
    for name, layout in PRESETS:
        path = BASE + "/预设/" + name
        bp = unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else None
        if not bp:
            factory = unreal.BlueprintFactory()
            factory.set_editor_property("parent_class", unreal.LxMagicStreetLampActor)
            bp = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, BASE + "/预设", unreal.Blueprint, factory)
        default = unreal.get_default_object(bp.generated_class())
        for key, value in {"layout": layout, "stone_material": stone, "metal_material": metal,
                           "ornament_material": gold, "crystal_material": crystal,
                           "pole_length": 320., "road_arm_length": 120., "sidewalk_arm_length": 85.,
                           "suspension_length": 30., "road_brightness_ev": math.log2(2800. / (2 * math.pi * (1 - math.cos(math.radians(40))))), "road_light_enabled": True, "road_temperature": 4200.,
                           "sidewalk_brightness_ev": math.log2(1600. / (2 * math.pi * (1 - math.cos(math.radians(30))))), "sidewalk_light_enabled": True, "sidewalk_temperature": 3400.}.items():
            default.set_editor_property(key, value)
        unreal.BlueprintEditorLibrary.compile_blueprint(bp)
        unreal.EditorAssetLibrary.save_loaded_asset(bp, only_if_is_dirty=False)
        result.append(bp.get_path_name())
    unreal.AutomationLibrary.finish_loading_before_screenshot()
    return result


def verify_assets():
    """验收两种已保存蓝图的结构、独立调光、定向光源和灯杆联动。"""
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    result = []
    for name, layout in PRESETS:
        actor = actors.spawn_actor_from_class(unreal.load_asset(BASE + "/预设/" + name).generated_class(), unreal.Vector(0, 0, -8000))
        try:
            mesh = actor.get_editor_property("street_mesh")
            road = actor.get_editor_property("road_light")
            walk = actor.get_editor_property("sidewalk_light")
            double = layout == unreal.LxStreetLampLayout.DOUBLE
            assert mesh.get_num_sections() == 5
            assert all(mesh.get_material(i) for i in range(5))
            assert road.get_editor_property("visible")
            assert walk.get_editor_property("visible") == double
            assert walk.get_editor_property("intensity_units") == unreal.LightUnits.EV
            assert road.get_forward_vector().x > 0 and road.get_forward_vector().z < 0
            assert walk.get_forward_vector().x < 0 and walk.get_forward_vector().z < 0
            builds = actor.get_editor_property("geometry_build_count")
            actor.set_editor_property("road_brightness_ev", 12.)
            actor.set_editor_property("road_temperature", 5800.)
            actor.set_editor_property("sidewalk_brightness_ev", 9.)
            actor.set_editor_property("sidewalk_temperature", 2800.)
            assert road.get_editor_property("intensity") == 12
            assert road.get_editor_property("temperature") == 5800
            assert walk.get_editor_property("intensity") == 9
            assert walk.get_editor_property("temperature") == 2800
            assert actor.get_editor_property("geometry_build_count") == builds
            core_glow = mesh.get_material(3).get_scalar_parameter_value("发光强度")
            actor.set_editor_property("road_outer_cone_angle", 65.)
            actor.set_editor_property("sidewalk_outer_cone_angle", 15.)
            assert road.get_editor_property("outer_cone_angle") == 65.
            assert walk.get_editor_property("outer_cone_angle") == 15.
            assert road.get_editor_property("intensity") == 12.
            assert walk.is_visible() == double
            assert mesh.get_material(3).get_scalar_parameter_value("发光强度") == core_glow
            assert actor.get_editor_property("geometry_build_count") == builds
            before = road.get_editor_property("relative_location").z
            actor.set_editor_property("pole_length", 470.)
            assert abs(road.get_editor_property("relative_location").z - before - 150) < .01
            actor.set_editor_property("road_light_enabled", False)
            assert not road.is_visible()
            assert mesh.get_material(3).get_scalar_parameter_value("发光强度") == 0
            if double:
                assert mesh.get_material(4).get_scalar_parameter_value("发光强度") > 0
            result.append({"预设": name, "有效光源": 2 if double else 1, "方向": True, "独立亮度色温": True, "杆长联动": True, "零亮度": True})
        finally:
            actors.destroy_actor(actor)
    (OUT / "预设验收.json").write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding="utf-8")
    return result


def make_demo():
    """生成仅包含本次路灯的独立示例，不覆盖其他制作关卡。"""
    # 若工具用于正常制作会话，先保护尚未保存的其他游戏关卡。
    for path in [p.get_name() for p in unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages()]:
        if path.startswith("/Game/") and not path.startswith(BASE + "/演示/"):
            raise RuntimeError("请先保存当前制作关卡再生成路灯展示：" + path)
    world = unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    material = HELPERS["material"]
    ground = material("展示石板", (.24, .31, .34), 0., .85)
    for (name, layout), x in zip(PRESETS, (-220, 210)):
        lamp = actors.spawn_actor_from_class(unreal.load_asset(BASE + "/预设/" + name).generated_class(), unreal.Vector(x, 0, 0))
        lamp.set_actor_label(name)
    floor = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(0, 0, -5))
    floor.set_actor_label("浅石广场")
    floor.static_mesh_component.set_static_mesh(unreal.load_asset("/Engine/BasicShapes/Cube"))
    floor.static_mesh_component.set_material(0, ground)
    floor.set_actor_scale3d(unreal.Vector(40, 40, .1))
    # 使用可移动柔光进行离屏验收，无需烘焙，也不改灯具自己的亮度参数。
    for label, location, lumens in (("正面柔光", unreal.Vector(50, -650, 520), 28000.),
                                    ("侧面柔光", unreal.Vector(-600, 200, 600), 16000.)):
        light = actors.spawn_actor_from_class(unreal.PointLight, location)
        light.set_actor_label(label)
        comp = light.light_component
        comp.set_mobility(unreal.ComponentMobility.MOVABLE)
        for key, value in {"intensity_units": unreal.LightUnits.LUMENS, "intensity": lumens,
                           "attenuation_radius": 2200., "source_radius": 160., "cast_shadows": False}.items():
            comp.set_editor_property(key, value)
    sun = actors.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 700), unreal.Rotator(-45, -30, 0))
    sun.set_actor_label("日光")
    sun.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    sun.light_component.set_editor_property("intensity", 1.5)
    sky = material("展示蓝天", (.22, .4, .58), 0., 1.)
    sky.set_editor_property("two_sided", True)
    color = HELPERS["expression"](sky, unreal.MaterialExpressionConstant3Vector, constant=unreal.LinearColor(1.32, 2.4, 3.48, 1), desc="海滨浅蓝环境")
    HELPERS["connect"](sky, color, unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    unreal.MaterialEditingLibrary.recompile_material(sky)
    unreal.EditorAssetLibrary.save_loaded_asset(sky, only_if_is_dirty=False)
    dome = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(0, 0, 0))
    dome.set_actor_label("天空背景")
    dome.static_mesh_component.set_static_mesh(unreal.load_asset("/Engine/BasicShapes/Sphere"))
    dome.static_mesh_component.set_material(0, sky)
    dome.static_mesh_component.set_editor_property("cast_shadow", False)
    dome.static_mesh_component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    dome.set_actor_scale3d(unreal.Vector(200, 200, 200))
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    assert len([a for a in actors.get_all_level_actors() if isinstance(a, unreal.LxMagicStreetLampActor)]) == 2
    assert unreal.EditorLoadingAndSavingUtils.save_map(world, BASE + "/演示/路灯展示")
    return {"map": BASE + "/演示/路灯展示", "lamps": 2}


def capture():
    """离屏捕获总览及灯架纹饰细节，不使用桌面屏幕控制。"""
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level(BASE + "/演示/路灯展示")
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_set_viewport_realtime(True)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    location = unreal.Vector(150, -1170, 490)
    camera = actors.spawn_actor_from_class(unreal.SceneCapture2D, location)
    camera.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(location, unreal.Vector(30, 0, 210)), False)
    component = camera.get_component_by_class(unreal.SceneCaptureComponent2D)
    target = unreal.RenderingLibrary.create_render_target2d(world, 1600, 1000, unreal.TextureRenderTargetFormat.RTF_RGBA8)
    for key, value in {"texture_target": target, "capture_source": unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR,
                       "capture_every_frame": False, "always_persist_rendering_state": True, "fov_angle": 41.}.items():
        component.set_editor_property(key, value)
    pp = component.get_editor_property("post_process_settings")
    for key, value in {"override_auto_exposure_min_brightness": True, "override_auto_exposure_max_brightness": True,
                       "auto_exposure_min_brightness": 4., "auto_exposure_max_brightness": 4.,
                       "override_bloom_intensity": True, "bloom_intensity": .1}.items():
        pp.set_editor_property(key, value)
    component.set_editor_property("post_process_settings", pp)
    unreal.AutomationLibrary.finish_loading_before_screenshot()
    STATE.update(world=world, camera=camera, component=component, target=target, phase=0, until=time.monotonic()+6)
    STATE["handle"] = unreal.register_slate_post_tick_callback(capture_tick)
    return {"capture_started": True}


def capture_tick(delta):
    """分帧完成两个视角的渲染，最后清理临时捕获组件。"""
    try:
        STATE["component"].capture_scene()
        if time.monotonic() < STATE["until"]:
            return
        name = ("路灯总览.png", "路灯花纹.png")[STATE["phase"]]
        unreal.RenderingLibrary.export_render_target(STATE["world"], STATE["target"], str(OUT), name)
        if STATE["phase"] == 0:
            location = unreal.Vector(310, -450, 450)
            STATE["camera"].set_actor_location(location, False, False)
            STATE["camera"].set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(location, unreal.Vector(225, 0, 335)), False)
            STATE["phase"] = 1
            STATE["until"] = time.monotonic()+6
        else:
            unreal.unregister_slate_post_tick_callback(STATE["handle"])
            unreal.get_editor_subsystem(unreal.EditorActorSubsystem).destroy_actor(STATE["camera"])
            STATE.clear()
            (OUT / "预览完成.json").write_text('{"captures":2}', encoding="utf-8")
    except Exception:
        unreal.unregister_slate_post_tick_callback(STATE["handle"])
        (OUT / "预览错误.txt").write_text(traceback.format_exc(), encoding="utf-8")


@unreal.uclass()
class StreetLampTools(unreal.ToolsetDefinition):
    """悬吊路灯的官方 MCP 制作入口。"""
    @unreal.ufunction(static=True, params=[str], ret=str, meta={"AICallable": "", "DisplayName": "处理路灯资产", "Category": "路灯|制作"})
    def process_street_lamp(stage):
        """创建、验证、演示或捕获路灯资产。"""
        try:
            if stage == "build": result = build_assets()
            elif stage == "verify": result = verify_assets()
            elif stage == "demo": result = make_demo()
            elif stage == "capture": result = capture()
            elif stage == "tests":
                unreal.SystemLibrary.execute_console_command(None, "Automation RunTests LxARPG.World.MagicLamp")
                result = {"tests_started": True}
            elif stage == "refresh":
                path = Path(unreal.Paths.project_dir()).resolve() / "Source/LxARPGEditor/Scripts/路灯资产工具.py"
                exec(compile(path.read_text(encoding="utf-8").split("\n@unreal.uclass()")[0], str(path), "exec"), globals())
                result = {"refreshed": True}
            elif stage == "quit":
                unreal.SystemLibrary.quit_editor()
                result = {"quit": True}
            else: raise ValueError(stage)
            return json.dumps(result, ensure_ascii=False)
        except Exception:
            return json.dumps({"error": traceback.format_exc()}, ensure_ascii=False)


unreal.EditorPythonScripting.set_keep_python_script_alive(True)
unreal.ToolsetRegistry.register_toolset_class(StreetLampTools)
unreal.SystemLibrary.execute_console_command(None, "ModelContextProtocol.StartServer 8008")
unreal.SystemLibrary.execute_console_command(None, "ModelContextProtocol.RefreshTools")
