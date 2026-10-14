"""通过 UE 5.8 官方 MCP 创建与验收自动魔力灯资产。"""
import json
import time
import traceback
from pathlib import Path
import math
import unreal

# 中文内容目录与独立验收目录。
BASE = "/Game/项目内容/美术资产/海港城市/魔灯"
OUT = Path(unreal.Paths.project_dir()).resolve() / "Saved/MagicLamp"
OUT.mkdir(parents=True, exist_ok=True)
# 分帧离屏截图状态。
STATE = {}
# 名称、类型、纹饰、色温和尺寸的四种可复用预设。
PRESETS = (("方晶壁灯", False, False, 3800, 30), ("方晶顶灯", False, True, 4600, 38),
           ("星环球灯", True, False, 5200, 32), ("花冠球灯", True, True, 3400, 32))


def expression(mat, cls, **props):
    """创建具有中文描述的材质节点。"""
    node = unreal.MaterialEditingLibrary.create_material_expression(mat, cls)
    for key, value in props.items():
        node.set_editor_property(key, value)
    return node


def connect(mat, node, prop):
    """连接并检查材质输出。"""
    if not unreal.MaterialEditingLibrary.connect_material_property(node, "", prop):
        raise RuntimeError("材质连接失败：" + str(prop))


def material(name, tint, metallic, roughness, crystal=False, emission_scale=1.0):
    """创建哑光瓷石、浅金镶边或具有柔和魔纹的乳白晶核。"""
    path = BASE + "/材质/" + name
    mat = unreal.load_asset(path)
    if not mat:
        mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, BASE + "/材质", unreal.Material, unreal.MaterialFactoryNew())
    unreal.MaterialEditingLibrary.delete_all_material_expressions(mat)
    color = expression(mat, unreal.MaterialExpressionVectorParameter, parameter_name="底色", default_value=unreal.LinearColor(*tint, 1), desc="清爽明亮的灯具底色")
    connect(mat, color, unreal.MaterialProperty.MP_BASE_COLOR)
    for value, prop in ((metallic, unreal.MaterialProperty.MP_METALLIC), (roughness, unreal.MaterialProperty.MP_ROUGHNESS)):
        connect(mat, expression(mat, unreal.MaterialExpressionConstant, r=value, desc="适度概括的材质反光"), prop)
    if crystal:
        color = expression(mat, unreal.MaterialExpressionVectorParameter, parameter_name="魔力颜色", default_value=unreal.LinearColor(1, .85, .65, 1), desc="随开发色温同步的晶核颜色")
        intensity = expression(mat, unreal.MaterialExpressionScalarParameter, parameter_name="发光强度", default_value=1.5, desc="随光源亮度同步，零亮度熄灭")
        uv = expression(mat, unreal.MaterialExpressionTextureCoordinate, coordinate_index=0, desc="晶体内部魔纹坐标")
        custom = expression(mat, unreal.MaterialExpressionCustom, output_type=unreal.CustomMaterialOutputType.CMOT_FLOAT3,
                            description="细菱形魔纹与柔和晶体层次，不使用闪烁或霓虹光效",
                            code="float2 p=UV.xy; float d=abs(frac((p.x+p.y)*8)-.5); float e=abs(frac((p.x-p.y)*8)-.5); float rune=1-smoothstep(.015,.045,min(d,e)); float cloud=.88+.08*sin(p.x*15+sin(p.y*11)); return Tint.rgb*Power*(cloud+rune*.18);".replace("Power*", f"Power*{float(emission_scale):.3f}*"))
        inputs = []
        for key in ("UV", "Tint", "Power"):
            item = unreal.CustomInput()
            item.set_editor_property("input_name", key)
            inputs.append(item)
        custom.set_editor_property("inputs", inputs)
        for key, node in (("UV", uv), ("Tint", color), ("Power", intensity)):
            if not unreal.MaterialEditingLibrary.connect_material_expressions(node, "", custom, key):
                raise RuntimeError("晶核材质节点连接失败")
        # 灯芯按屏幕曝光补偿，保持日间可见；零发光输入仍为零，继续服从昼夜渐变。
        compensated = expression(mat, unreal.MaterialExpressionEyeAdaptationInverse, desc="灯芯曝光补偿")
        light_pin = unreal.MaterialEditingLibrary.get_material_expression_input_names(compensated)[0]
        if not unreal.MaterialEditingLibrary.connect_material_expressions(custom, "", compensated, light_pin):
            raise RuntimeError("灯芯曝光补偿连接失败")
        connect(mat, compensated, unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    unreal.MaterialEditingLibrary.layout_material_expressions(mat)
    unreal.MaterialEditingLibrary.recompile_material(mat)
    unreal.EditorAssetLibrary.save_loaded_asset(mat, only_if_is_dirty=False)
    return mat


def build_assets():
    """保存四个中文蓝图预设，全部生成与调光逻辑来自 C++。"""
    for folder in ("预设", "材质", "演示"):
        unreal.EditorAssetLibrary.make_directory(BASE + "/" + folder)
    housing = material("白瓷石", (.68, .71, .73), .06, .64)
    gold = material("浅金纹饰", (.55, .39, .19), .35, .55)
    core = material("魔纹晶核", (.42, .53, .57), .08, .32, True)
    result = []
    for name, sphere, petal, kelvin, size in PRESETS:
        bp = unreal.load_asset(BASE + "/预设/" + name)
        if not bp:
            factory = unreal.BlueprintFactory()
            factory.set_editor_property("parent_class", unreal.LxSphereMagicLampActor if sphere else unreal.LxBoxMagicLampActor)
            bp = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, BASE + "/预设", unreal.Blueprint, factory)
        default = unreal.get_default_object(bp.generated_class())
        props = {"housing_material": housing, "ornament_material": gold, "crystal_material": core,
                 "temperature": float(kelvin), "brightness_ev": math.log2(650. / (4 * math.pi)), "light_enabled": True, "light_range": 500.,
                 "ornament": unreal.LxLampOrnament.PETAL if petal else unreal.LxLampOrnament.STAR}
        props.update({"radius": float(size)} if sphere else {"size_handle": unreal.Vector(size, 0, size if petal else 45), "thickness": 10. if petal else 12.})
        for key, value in props.items():
            default.set_editor_property(key, value)
        unreal.BlueprintEditorLibrary.compile_blueprint(bp)
        unreal.EditorAssetLibrary.save_loaded_asset(bp, only_if_is_dirty=False)
        result.append(bp.get_path_name())
    unreal.AutomationLibrary.finish_loading_before_screenshot()
    return result


def verify_assets():
    """验证实际蓝图实例的光源、材质、手柄调节、零亮度和缓存。"""
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    results = []
    for name, sphere, petal, kelvin, size in PRESETS:
        bp = unreal.load_asset(BASE + "/预设/" + name)
        actor = actors.spawn_actor_from_class(bp.generated_class(), unreal.Vector(0, 0, -5000))
        try:
            # 实例构造已生成灯体；编辑器按钮不暴露为游戏蓝图函数。
            mesh = actor.get_editor_property("lamp_mesh")
            light = actor.get_editor_property("lamp_light")
            assert mesh.get_num_sections() == 3, "缺少灯体分区"
            assert len(actor.get_components_by_class(unreal.PointLightComponent)) == 1
            assert light.get_editor_property("intensity_units") == unreal.LightUnits.EV
            assert abs(light.get_editor_property("temperature") - kelvin) < .1
            assert all(mesh.get_material(i) for i in range(3)), "缺少材质"
            builds = actor.get_editor_property("geometry_build_count")
            actor.set_editor_property("brightness_ev", 8.)
            actor.set_editor_property("temperature", 6200.)
            assert abs(light.get_editor_property("intensity") - 8) < .1
            assert abs(light.get_editor_property("temperature") - 6200) < .1
            assert actor.get_editor_property("geometry_build_count") == builds, "调光意外重建网格"
            core = mesh.get_material(2)
            assert core.get_scalar_parameter_value("发光强度") > 0
            actor.set_editor_property("light_enabled", False)
            assert not light.is_visible()
            assert core.get_scalar_parameter_value("发光强度") == 0
            if sphere:
                actor.set_editor_property("radius", 60.)
                assert actor.get_editor_property("radius") == 60.
            else:
                actor.set_editor_property("size_handle", unreal.Vector(65, 17, 28))
                actor.set_editor_property("thickness", 24.)
                assert actor.get_editor_property("size_handle").y == 0
            assert actor.get_editor_property("geometry_build_count") > builds
            results.append({"预设": name, "材质分区": 3, "光源": 1, "调光与色温": True, "尺寸编辑": True, "零亮度": True})
        finally:
            actors.destroy_actor(actor)
    (OUT / "预设验收.json").write_text(json.dumps(results, ensure_ascii=False, indent=2), encoding="utf-8")
    return results


def make_demo():
    """创建独立示例关卡，不覆盖项目的制作关卡。"""
    world = unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    ground = material("展示浅石", (.32, .38, .43), .0, .8)
    for index, (name, sphere, petal, kelvin, size) in enumerate(PRESETS):
        x = (index - 1.5) * 125
        cls = unreal.load_asset(BASE + "/预设/" + name).generated_class()
        lamp = actors.spawn_actor_from_class(cls, unreal.Vector(x, 0, 135))
        lamp.set_actor_label(name)
        # 方形预设保留正面视角，顶灯放置时将本地 Y 轴朝向室内即可。
        pedestal = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(x, 0, 40))
        pedestal.set_actor_label(name + "展台")
        pedestal.static_mesh_component.set_static_mesh(unreal.load_asset("/Engine/BasicShapes/Cube"))
        pedestal.static_mesh_component.set_material(0, ground)
        pedestal.set_actor_scale3d(unreal.Vector(.9, .9, .8))
    floor = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(0, 0, -5))
    floor.set_actor_label("浅石地面")
    floor.static_mesh_component.set_static_mesh(unreal.load_asset("/Engine/BasicShapes/Cube"))
    floor.static_mesh_component.set_material(0, ground)
    floor.set_actor_scale3d(unreal.Vector(30, 30, .1))
    sun = actors.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 400), unreal.Rotator(-45, -55, 0))
    sun.set_actor_label("柔和日光")
    sun.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    sun.light_component.set_editor_property("intensity", 2.5)
    fill = actors.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 300), unreal.Rotator(-20, 90, 0))
    fill.set_actor_label("环境补光")
    fill.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    fill.light_component.set_editor_property("intensity", 1.8)
    softbox = actors.spawn_actor_from_class(unreal.PointLight, unreal.Vector(0, -350, 260))
    softbox.set_actor_label("展示柔光")
    softbox.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    softbox.light_component.set_editor_property("intensity_units", unreal.LightUnits.EV)
    softbox.light_component.set_editor_property("intensity", 12000.)
    softbox.light_component.set_editor_property("attenuation_radius", 1500.)
    softbox.light_component.set_editor_property("source_radius", 100.)
    softbox.light_component.set_editor_property("cast_shadows", False)
    sky_mat = material("展示天空", (.36, .56, .75), 0., 1.)
    sky_mat.set_editor_property("two_sided", True)
    sky_color = expression(sky_mat, unreal.MaterialExpressionConstant3Vector, constant=unreal.LinearColor(.36, .56, .75, 1), desc="柔和天空背景")
    connect(sky_mat, sky_color, unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    unreal.MaterialEditingLibrary.recompile_material(sky_mat)
    unreal.EditorAssetLibrary.save_loaded_asset(sky_mat, only_if_is_dirty=False)
    dome = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(0, 0, 0))
    dome.set_actor_label("浅蓝天空")
    dome.static_mesh_component.set_static_mesh(unreal.load_asset("/Engine/BasicShapes/Sphere"))
    dome.static_mesh_component.set_material(0, sky_mat)
    dome.static_mesh_component.set_editor_property("cast_shadow", False)
    dome.static_mesh_component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    dome.set_actor_scale3d(unreal.Vector(200, 200, 200))
    ambient = actors.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 400))
    ambient.set_actor_label("天空环境光")
    ambient.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    ambient.light_component.set_editor_property("real_time_capture", True)
    ambient.light_component.set_editor_property("intensity", .8)
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    unreal.EditorLoadingAndSavingUtils.save_map(world, BASE + "/演示/魔灯展示")
    lamps = [a for a in actors.get_all_level_actors() if isinstance(a, unreal.LxMagicLampActor)]
    assert len(lamps) == 4, "展示关卡必须恰好包含四盏灯"
    return {"map": BASE + "/演示/魔灯展示", "lamps": len(lamps)}


def capture():
    """通过离屏场景捕获检查四个预设，无需控制屏幕。"""
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level(BASE + "/演示/魔灯展示")
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_set_viewport_realtime(True)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    location = unreal.Vector(140, -740, 320)
    camera = actors.spawn_actor_from_class(unreal.SceneCapture2D, location)
    camera.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(location, unreal.Vector(0, 0, 95)), False)
    component = camera.get_component_by_class(unreal.SceneCaptureComponent2D)
    target = unreal.RenderingLibrary.create_render_target2d(world, 1600, 900, unreal.TextureRenderTargetFormat.RTF_RGBA8)
    for key, value in {"texture_target": target, "capture_source": unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR, "capture_every_frame": False, "always_persist_rendering_state": True, "fov_angle": 48.}.items():
        component.set_editor_property(key, value)
    pp = component.get_editor_property("post_process_settings")
    for key, value in {"override_auto_exposure_min_brightness": True, "override_auto_exposure_max_brightness": True, "auto_exposure_min_brightness": 1., "auto_exposure_max_brightness": 1., "override_bloom_intensity": True, "bloom_intensity": .12}.items():
        pp.set_editor_property(key, value)
    component.set_editor_property("post_process_settings", pp)
    unreal.AutomationLibrary.finish_loading_before_screenshot()
    STATE.update(world=world, camera=camera, component=component, target=target, until=time.monotonic()+8)
    STATE["handle"] = unreal.register_slate_post_tick_callback(capture_tick)
    return {"capture_started": True}


def capture_tick(delta):
    """等待渲染收敛后保存预览并清理临时摄影机。"""
    try:
        STATE["component"].capture_scene()
        if time.monotonic() < STATE["until"]:
            return
        unreal.RenderingLibrary.export_render_target(STATE["world"], STATE["target"], str(OUT), "魔灯预览.png")
        unreal.unregister_slate_post_tick_callback(STATE["handle"])
        unreal.get_editor_subsystem(unreal.EditorActorSubsystem).destroy_actor(STATE["camera"])
        (OUT / "预览完成.json").write_text('{"complete":true}', encoding="utf-8")
    except Exception:
        unreal.unregister_slate_post_tick_callback(STATE["handle"])
        (OUT / "预览错误.txt").write_text(traceback.format_exc(), encoding="utf-8")


@unreal.uclass()
class MagicLampTools(unreal.ToolsetDefinition):
    """自动魔灯官方 MCP 制作和验收入口。"""
    @unreal.ufunction(static=True, params=[str], ret=str, meta={"AICallable": "", "DisplayName": "处理魔灯资产", "Category": "魔灯|制作"})
    def process_lamp(stage):
        """创建预设、验证、生成展示或运行测试。"""
        try:
            if stage == "build": result = build_assets()
            elif stage == "verify": result = verify_assets()
            elif stage == "demo": result = make_demo()
            elif stage == "capture": result = capture()
            elif stage == "tests":
                unreal.SystemLibrary.execute_console_command(None, "Automation RunTests LxARPG.World.MagicLamp")
                result = {"tests_started": True}
            elif stage == "refresh":
                path = Path(unreal.Paths.project_dir()).resolve() / "Source/LxARPGEditor/Scripts/魔灯资产工具.py"
                exec(compile(path.read_text(encoding="utf-8").split("@unreal.uclass()")[0], str(path), "exec"), globals())
                result = {"refreshed": True}
            elif stage == "quit":
                unreal.SystemLibrary.quit_editor()
                result = {"quit": True}
            else: raise ValueError(stage)
            return json.dumps(result, ensure_ascii=False)
        except Exception:
            return json.dumps({"error": traceback.format_exc()}, ensure_ascii=False)


unreal.EditorPythonScripting.set_keep_python_script_alive(True)
unreal.ToolsetRegistry.register_toolset_class(MagicLampTools)
unreal.SystemLibrary.execute_console_command(None, "ModelContextProtocol.StartServer 8008")
unreal.SystemLibrary.execute_console_command(None, "ModelContextProtocol.RefreshTools")
