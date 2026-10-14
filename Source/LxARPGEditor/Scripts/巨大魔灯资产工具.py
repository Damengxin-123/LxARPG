"""通过 UE 5.8 官方 MCP 创建、验收和离屏预览塔顶巨大水晶灯。"""
import json
import time
import traceback
from pathlib import Path
import unreal

# 独立资源名称与验收目录，已有小灯材质不作修改。
ROOT = Path(unreal.Paths.project_dir()).resolve()
BASE = "/Game/项目内容/美术资产/海港城市/魔灯"
COMMON = "/Game/项目内容/美术资产/自动化类型/灯具"
OUT = ROOT / "Saved/GiantMagicLamp"
OUT.mkdir(parents=True, exist_ok=True)
STATE = {}
# 复用既有材质节点工具，不执行其 MCP 注册或四种小灯的重建入口。
HELPERS = {}
SOURCE = ROOT / "Source/LxARPGEditor/Scripts/魔灯资产工具.py"
exec(compile(SOURCE.read_text(encoding="utf-8").split("@unreal.uclass()")[0], str(SOURCE), "exec"), HELPERS)


def crystal_material():
    """生成半透明冰晶、局部内辉光与细晶纹，保留切面且随灯具开关熄灭。"""
    path = BASE + "/材质/塔灯冰晶"
    mat = unreal.load_asset(path)
    if not mat:
        mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset("塔灯冰晶", BASE + "/材质", unreal.Material, unreal.MaterialFactoryNew())
    unreal.MaterialEditingLibrary.delete_all_material_expressions(mat)
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    mat.set_editor_property("translucency_lighting_mode", unreal.TranslucencyLightingMode.TLM_SURFACE_PER_PIXEL_LIGHTING)
    # 只绘制朝向视点的外壳，避免同一晶体背面叠加发光而泛白。
    mat.set_editor_property("two_sided", False)
    node = HELPERS["expression"]
    link = HELPERS["connect"]
    tint = node(mat, unreal.MaterialExpressionVectorParameter, parameter_name="底色", default_value=unreal.LinearColor(.055, .28, .42, 1), desc="降低表面漫反射，避免实心水泥质感")
    link(mat, tint, unreal.MaterialProperty.MP_BASE_COLOR)
    for value, prop in ((0., unreal.MaterialProperty.MP_METALLIC), (.13, unreal.MaterialProperty.MP_ROUGHNESS)):
        link(mat, node(mat, unreal.MaterialExpressionConstant, r=value, desc="柔和晶体反光"), prop)
    color = node(mat, unreal.MaterialExpressionVectorParameter, parameter_name="魔力颜色", default_value=unreal.LinearColor(.72, .88, 1, 1), desc="继承实例的魔力色彩")
    power = node(mat, unreal.MaterialExpressionScalarParameter, parameter_name="发光强度", default_value=6., desc="随世界时间和灯具开关渐变")
    uv = node(mat, unreal.MaterialExpressionTextureCoordinate, coordinate_index=0, desc="每个三角切面的晶纹坐标")
    normal = node(mat, unreal.MaterialExpressionPixelNormalWS, desc="保留晶面明暗层次")
    view = node(mat, unreal.MaterialExpressionCameraVectorWS, desc="根据观察角度区分通透中心和清晰棱面")
    glow = node(mat, unreal.MaterialExpressionScalarParameter, parameter_name="晶体辉光倍率", default_value=1., desc="只调节晶体发光观感，不改变照明距离")
    opacity = node(mat, unreal.MaterialExpressionScalarParameter, parameter_name="晶体不透明度", default_value=.46, desc="中心保持通透，掠射边缘自动加厚")
    custom = node(mat, unreal.MaterialExpressionCustom, output_type=unreal.CustomMaterialOutputType.CMOT_FLOAT3,
                  description="局部晶核辉光、细晶纹和保留明暗的透明切面",
                  code="""float2 p=UV.xy;
float diamond=abs(abs(p.x-.5)*1.5+abs(p.y-.43)*.65-.23);
float rune=1-smoothstep(.005,.017,diamond);
float vein=1-smoothstep(.002,.010,abs(p.x-.5-.045*sin(p.y*13)));
float shade=.42+.58*saturate(dot(normalize(Normal),normalize(float3(-.4,-.6,1))));
float edge=1-smoothstep(.005,.02,min(abs(p.x-p.y*.5),abs(1-p.x-p.y*.5)));
float center=exp(-pow((p.x-.5)*5.2,2)-pow((p.y-.22)*2.1,2));
float fresnel=pow(1-saturate(abs(dot(normalize(Normal),normalize(View)))),3);
float3 ice=Tint.rgb*float3(.30,.80,1.0);
float body=(.12+.72*center)*shade;
float detail=rune*.18+vein*.055+edge*.13+fresnel*.09;
return ice*min(Power,6.0)*clamp(Glow,0.0,2.0)*(body+detail);""")
    inputs = []
    for key in ("UV", "Tint", "Power", "Normal", "View", "Glow"):
        item = unreal.CustomInput()
        item.set_editor_property("input_name", key)
        inputs.append(item)
    custom.set_editor_property("inputs", inputs)
    for key, item in (("UV", uv), ("Tint", color), ("Power", power), ("Normal", normal), ("View", view), ("Glow", glow)):
        assert unreal.MaterialEditingLibrary.connect_material_expressions(item, "", custom, key)
    compensate = node(mat, unreal.MaterialExpressionEyeAdaptationInverse, desc="保留日间晶纹可读性")
    pin = unreal.MaterialEditingLibrary.get_material_expression_input_names(compensate)[0]
    assert unreal.MaterialEditingLibrary.connect_material_expressions(custom, "", compensate, pin)
    link(mat, compensate, unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    transmission = node(mat, unreal.MaterialExpressionCustom, output_type=unreal.CustomMaterialOutputType.CMOT_FLOAT1,
                        description="中心透景、棱面留形，避免变成隐形玻璃",
                        code="float f=pow(1-saturate(abs(dot(normalize(Normal),normalize(View)))),2); return clamp(Opacity+.30*f,.25,.88);")
    alpha_inputs = []
    for key in ("Normal", "View", "Opacity"):
        item = unreal.CustomInput()
        item.set_editor_property("input_name", key)
        alpha_inputs.append(item)
    transmission.set_editor_property("inputs", alpha_inputs)
    for key, item in (("Normal", normal), ("View", view), ("Opacity", opacity)):
        assert unreal.MaterialEditingLibrary.connect_material_expressions(item, "", transmission, key)
    link(mat, transmission, unreal.MaterialProperty.MP_OPACITY)
    unreal.MaterialEditingLibrary.layout_material_expressions(mat)
    unreal.MaterialEditingLibrary.recompile_material(mat)
    unreal.EditorAssetLibrary.save_loaded_asset(mat, only_if_is_dirty=False)
    return mat


def build_assets():
    """创建制作目录预设及常用灯具目录的子蓝图，逻辑完全由 C++ 实现。"""
    unreal.EditorAssetLibrary.make_directory(COMMON)
    housing = unreal.load_asset(BASE + "/材质/白瓷石")
    gold = unreal.load_asset(BASE + "/材质/塔灯浅金") or HELPERS["material"]("塔灯浅金", (.62, .48, .25), .25, .48)
    assert housing and gold, "缺少现有灯具白石或浅金材质"
    core = crystal_material()
    result = []
    parent = unreal.LxGiantMagicLampActor
    for folder in (BASE + "/预设", COMMON):
        path = folder + "/塔顶巨晶灯"
        bp = unreal.load_asset(path)
        if not bp:
            factory = unreal.BlueprintFactory()
            factory.set_editor_property("parent_class", parent)
            bp = unreal.AssetToolsHelpers.get_asset_tools().create_asset("塔顶巨晶灯", folder, unreal.Blueprint, factory)
        if folder != COMMON:
            default = unreal.get_default_object(bp.generated_class())
            for key, value in {"housing_material": housing, "ornament_material": gold, "crystal_material": core}.items():
                default.set_editor_property(key, value)
        unreal.BlueprintEditorLibrary.compile_blueprint(bp)
        unreal.EditorAssetLibrary.save_loaded_asset(bp, only_if_is_dirty=False)
        result.append(bp.get_path_name())
        parent = bp.generated_class()
    return {"presets": result, "material": core.get_path_name()}


def verify_assets():
    """验证两个目录中的实际蓝图、超大尺寸、光照缓存和晶核熄灭。"""
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    results = []
    for folder in (BASE + "/预设", COMMON):
        bp = unreal.load_asset(folder + "/塔顶巨晶灯")
        assert bp, "缺少巨大灯具预设"
        actor = actors.spawn_actor_from_class(bp.generated_class(), unreal.Vector(0, 0, -100000))
        try:
            mesh = actor.get_editor_property("lamp_mesh")
            light = actor.get_editor_property("lamp_light")
            assert mesh.get_num_sections() == 3
            assert all(mesh.get_material(i) for i in range(3))
            assert len(actor.get_components_by_class(unreal.PointLightComponent)) == 1
            assert light.get_editor_property("attenuation_radius") == 35000.
            assert light.get_editor_property("intensity") == 18.
            builds = actor.get_editor_property("geometry_build_count")
            actor.set_editor_property("light_range", 80000.)
            assert light.get_editor_property("attenuation_radius") == 80000.
            assert actor.get_editor_property("geometry_build_count") == builds
            actor.set_editor_property("crystal_height", 6000.)
            assert actor.get_editor_property("geometry_build_count") > builds
            core = mesh.get_material(2)
            assert core.get_scalar_parameter_value("发光强度") > 0
            actor.set_editor_property("light_enabled", False)
            assert not light.is_visible()
            assert core.get_scalar_parameter_value("发光强度") == 0
            results.append({"预设": bp.get_path_name(), "默认高度米": 24, "测试高度米": 60, "测试光照范围米": 800, "材质": True, "缓存": True, "熄灯": True})
        finally:
            actors.destroy_actor(actor)
    (OUT / "预设验收.json").write_text(json.dumps(results, ensure_ascii=False, indent=2), encoding="utf-8")
    return results


def preview(closeup=True):
    """在独立预览世界离屏拍摄晶体近景，也可选择完整塔座构图。"""
    world = unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    stone = unreal.load_asset(BASE + "/材质/白瓷石")
    gold = unreal.load_asset(BASE + "/材质/塔灯浅金")
    # 简洁白石台座显示巨型光源与塔身的尺度关系。
    for name, z, radius, height, mat in (("白石塔身", 1100, 680, 2200, stone), ("塔顶平台", 2250, 850, 100, stone), ("平台金边", 2305, 860, 20, gold), ("塔顶托座", 2420, 310, 210, stone)):
        actor = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(0, 0, z))
        actor.set_actor_label(name)
        actor.static_mesh_component.set_static_mesh(unreal.load_asset("/Engine/BasicShapes/Cylinder"))
        actor.static_mesh_component.set_material(0, mat)
        actor.set_actor_scale3d(unreal.Vector(radius / 50, radius / 50, height / 100))
    bp = unreal.load_asset(COMMON + "/塔顶巨晶灯")
    lamp = actors.spawn_actor_from_class(bp.generated_class(), unreal.Vector(0, 0, 3800))
    lamp.set_actor_label("塔顶巨晶灯")
    lamp.set_editor_property("auto_enable", False)
    floor = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(0, 0, -30))
    floor.static_mesh_component.set_static_mesh(unreal.load_asset("/Engine/BasicShapes/Cube"))
    floor.static_mesh_component.set_material(0, stone)
    floor.set_actor_scale3d(unreal.Vector(400, 400, .5))
    floor.set_actor_label("浅石广场")
    for label, rotation, intensity in (("温暖日光", unreal.Rotator(pitch=-42, yaw=125, roll=0), 35000.),):
        sun = actors.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 6000), rotation)
        sun.set_actor_label(label)
        sun.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
        sun.light_component.set_editor_property("intensity", intensity)
    sky = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector())
    sky.set_actor_label("晴蓝背景")
    sky.static_mesh_component.set_static_mesh(unreal.load_asset("/Engine/BasicShapes/Sphere"))
    sky_material = unreal.load_asset(BASE + "/材质/塔灯晴空")
    if not sky_material:
        sky_material = HELPERS["material"]("塔灯晴空", (.3, .52, .72), 0., 1.)
        sky_material.set_editor_property("two_sided", True)
        sky_color = HELPERS["expression"](sky_material, unreal.MaterialExpressionConstant3Vector, constant=unreal.LinearColor(6500, 11000, 16000, 1), desc="与日光曝光匹配的晴蓝天空")
        HELPERS["connect"](sky_material, sky_color, unreal.MaterialProperty.MP_EMISSIVE_COLOR)
        unreal.MaterialEditingLibrary.recompile_material(sky_material)
        unreal.EditorAssetLibrary.save_loaded_asset(sky_material, only_if_is_dirty=False)
    sky.static_mesh_component.set_material(0, sky_material)
    sky.static_mesh_component.set_editor_property("cast_shadow", False)
    sky.static_mesh_component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    sky.set_actor_scale3d(unreal.Vector(1600, 1600, 1600))
    ambient = actors.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 6000))
    ambient.set_actor_label("晴空环境光")
    ambient.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    ambient.light_component.set_editor_property("real_time_capture", True)
    ambient.light_component.set_editor_property("intensity", 1.)
    unreal.EditorLoadingAndSavingUtils.save_map(world, BASE + "/演示/巨晶塔灯展示")
    location = unreal.Vector(3100, -5200, 4700) if closeup else unreal.Vector(4200, -7200, 4900)
    camera = actors.spawn_actor_from_class(unreal.SceneCapture2D, location)
    camera.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(location, unreal.Vector(0, 0, 3800 if closeup else 2800)), False)
    component = camera.get_component_by_class(unreal.SceneCaptureComponent2D)
    target = unreal.RenderingLibrary.create_render_target2d(world, 1400, 1400, unreal.TextureRenderTargetFormat.RTF_RGBA8)
    for key, value in {"texture_target": target, "capture_source": unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR, "capture_every_frame": False, "always_persist_rendering_state": True, "fov_angle": 30. if closeup else 40.}.items():
        component.set_editor_property(key, value)
    settings = component.get_editor_property("post_process_settings")
    for key, value in {"override_auto_exposure_method": True, "auto_exposure_method": unreal.AutoExposureMethod.AEM_HISTOGRAM, "override_auto_exposure_min_brightness": True, "override_auto_exposure_max_brightness": True, "auto_exposure_min_brightness": 11.5, "auto_exposure_max_brightness": 11.5, "override_auto_exposure_bias": True, "auto_exposure_bias": 0., "override_bloom_intensity": True, "bloom_intensity": .08}.items():
        settings.set_editor_property(key, value)
    component.set_editor_property("post_process_settings", settings)
    unreal.AutomationLibrary.finish_loading_before_screenshot()
    filename = "水晶近景.png" if closeup else "巨晶塔灯.png"
    STATE.update(world=world, camera=camera, component=component, target=target, filename=filename, until=time.monotonic()+12)
    (OUT / "渲染设置.json").write_text(json.dumps([{"name": a.get_actor_label(), "rotation": str(a.get_actor_rotation()), "direction": str(a.get_actor_forward_vector()), "intensity": a.light_component.get_editor_property("intensity")} for a in actors.get_all_level_actors() if isinstance(a, unreal.DirectionalLight)], ensure_ascii=False, indent=2), encoding="utf-8")
    STATE["handle"] = unreal.register_slate_post_tick_callback(preview_tick)
    return {"started": True, "path": str(OUT / filename)}


def preview_tick(delta):
    """等待着色器和时序渲染收敛后保存图像并销毁临时相机。"""
    try:
        STATE["component"].capture_scene()
        if time.monotonic() < STATE["until"]:
            return
        unreal.RenderingLibrary.export_render_target(STATE["world"], STATE["target"], str(OUT), STATE["filename"])
        unreal.unregister_slate_post_tick_callback(STATE["handle"])
        unreal.get_editor_subsystem(unreal.EditorActorSubsystem).destroy_actor(STATE["camera"])
        (OUT / "预览完成.json").write_text('{"complete":true}', encoding="utf-8")
    except Exception:
        unreal.unregister_slate_post_tick_callback(STATE["handle"])
        (OUT / "预览错误.txt").write_text(traceback.format_exc(), encoding="utf-8")


@unreal.uclass()
class GiantMagicLampTools(unreal.ToolsetDefinition):
    """巨大魔灯的官方 MCP 制作入口。"""
    @unreal.ufunction(static=True, params=[str], ret=str, meta={"AICallable": "", "DisplayName": "处理巨大魔灯", "Category": "魔灯|制作"})
    def process_giant_lamp(stage):
        """按阶段创建、验证、预览或运行灯具回归测试。"""
        try:
            if stage == "build": result = build_assets()
            elif stage == "verify": result = verify_assets()
            elif stage == "preview": result = preview()
            elif stage == "tests":
                unreal.SystemLibrary.execute_console_command(None, "Automation RunTests LxARPG.World.MagicLamp")
                result = {"tests_started": True}
            elif stage == "refresh":
                path = ROOT / "Source/LxARPGEditor/Scripts/巨大魔灯资产工具.py"
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
unreal.ToolsetRegistry.register_toolset_class(GiantMagicLampTools)
unreal.SystemLibrary.execute_console_command(None, "ModelContextProtocol.StartServer 8027")
unreal.SystemLibrary.execute_console_command(None, "ModelContextProtocol.RefreshTools")
