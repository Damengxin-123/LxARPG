"""通过虚幻5.8官方MCP检查、制作和验收闭合样条地面。"""
import json
import time
import traceback
import math
from pathlib import Path
import unreal

# 专用地面资产目录与隔离验收输出。
BASE = "/Game/项目内容/美术资产/海港城市/样条地面"
PRESET = "/Game/项目内容/美术资产/自动化类型/地面/自动样条地面"
OUT = Path(unreal.Paths.project_saved_dir()) / "Diagnostics/样条地面"
OUT.mkdir(parents=True, exist_ok=True)
STATE = {}


def inspect_ground():
    """检查原生生成能力和项目内已有地面蓝图，不修改原关卡。"""
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    actor = actors.spawn_actor_from_class(unreal.LxSplineSurfaceActor, unreal.Vector(0, 0, -10000))
    try:
        actor.rebuild_surface()
        result = {"class": actor.get_class().get_path_name(),
                  "sections": actor.surface_mesh.get_num_sections(),
                  "source_component": actor.surface_mesh.get_class().get_name(),
                  "collision": str(actor.surface_mesh.get_collision_enabled()),
                  "material": actor.surface_material.get_path_name() if actor.surface_material else None,
                  "presets": []}
        for asset_path in unreal.EditorAssetLibrary.list_assets("/Game/项目内容/美术资产", recursive=True):
            if any(word in asset_path for word in ("地面", "平面", "地板/自动石地板")):
                asset = unreal.load_asset(asset_path)
                if isinstance(asset, unreal.Blueprint):
                    defaults = unreal.get_default_object(asset.generated_class())
                    result["presets"].append({"path": asset_path, "class": asset.generated_class().get_path_name(),
                                              "spline_ground": isinstance(defaults, unreal.LxSplineSurfaceActor)})
        (OUT / "原始检查.json").write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding="utf-8")
        return result
    finally:
        actors.destroy_actor(actor)


def build_ground():
    """复用海港建筑石板母材质，制作可调色实例和中文自动地面预设。"""
    parent = unreal.load_asset("/Game/项目内容/美术资产/海港城市/圆形建筑/材质/大块浅灰石板地面")
    if not parent:
        raise RuntimeError("现有海港石板母材质缺失")
    folder = BASE + "/材质"
    unreal.EditorAssetLibrary.make_directory(folder)
    path = folder + "/浅灰石板地面"
    material = unreal.load_asset(path)
    if not material:
        material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            "浅灰石板地面", folder, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    unreal.MaterialEditingLibrary.set_material_instance_parent(material, parent)
    unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(
        material, "石板颜色", unreal.LinearColor(.50, .515, .50, 1))
    unreal.MaterialEditingLibrary.update_material_instance(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=False)
    folder, name = PRESET.rsplit("/", 1)
    unreal.EditorAssetLibrary.make_directory(folder)
    blueprint = unreal.load_asset(PRESET)
    if not blueprint:
        factory = unreal.BlueprintFactory()
        factory.set_editor_property("parent_class", unreal.LxSplineSurfaceActor)
        blueprint = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, folder, unreal.Blueprint, factory)
    defaults = unreal.get_default_object(blueprint.generated_class())
    defaults.set_editor_property("surface_material", material)
    defaults.set_editor_property("material_density", .5)
    defaults.set_editor_property("edge_sample_spacing_meters", 1.0)
    defaults.set_editor_property("preview_sample_spacing_meters", 4.0)
    defaults.set_editor_property("collision_thickness_meters", .10)
    blueprint.set_editor_property("run_construction_script_on_drag", False)
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False)
    result = {"preset": PRESET, "material": path, "parent_material": parent.get_path_name(),
              "style": "海港建筑同款浅灰石板、细灰缝、浅倒角、柔和色差及哑光高光"}
    (OUT / "资产制作.json").write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding="utf-8")
    return result


def verify_ground():
    """验证预设实例、参数修改、完整渲染与实际行走射线。"""
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    blueprint = unreal.load_asset(PRESET)
    actor = actors.spawn_actor_from_class(blueprint.generated_class(), unreal.Vector(0, 0, -10000))
    try:
        actor.set_editor_property("height_offset_meters", .25)
        actor.finalize_surface()
        mesh = actor.optimized_mesh.get_editor_property("static_mesh")
        if not mesh or actor.generation_diagnostic or actor.surface_preview:
            raise RuntimeError("预设未恢复完整地面")
        world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
        hit = unreal.SystemLibrary.line_trace_single(world, unreal.Vector(0, 0, -9900), unreal.Vector(0, 0, -10100),
            unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, False, [], unreal.DrawDebugTrace.NONE, True)
        if not hit:
            raise RuntimeError("预设地面不能阻挡角色射线")
        if mesh.get_num_triangles(0) != actor.generated_triangle_count:
            raise RuntimeError("完整渲染与源几何数量不一致")
        result = {"preset": PRESET, "vertices": actor.generated_vertex_count, "triangles": actor.generated_triangle_count,
                  "collision_trace": True, "height_m": .25, "material": actor.surface_material.get_path_name(),
                  "nanite": mesh.get_editor_property("nanite_settings").get_editor_property("enabled")}
        (OUT / "预设验收.json").write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding="utf-8")
        return result
    finally:
        actors.destroy_actor(actor)


def reload_ground():
    """在新进程中重载保存测试关卡，核对完整几何、参数和缓存复用。"""
    filename = str(OUT / "地面缓存验收.umap")
    world = unreal.EditorLoadingAndSavingUtils.load_map(filename)
    if not world:
        raise RuntimeError("独立地面验证关卡加载失败")
    result = []
    for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
        if not isinstance(actor, unreal.LxSplineSurfaceActor):
            continue
        actor.finalize_surface()
        mesh = actor.optimized_mesh.get_editor_property("static_mesh")
        row = {"actor": actor.get_actor_label(), "vertices": actor.generated_vertex_count,
               "triangles": actor.generated_triangle_count, "height_m": round(actor.height_offset_meters, 2),
               "source_builds": actor.geometry_build_count,
               "conversions": actor.optimized_mesh.get_editor_property("geometry_conversion_count"),
               "preview": actor.surface_preview, "cached_bytes": actor.surface_mesh.get_editor_property("compact_stored_bytes"),
               "control_points": actor.surface_spline.get_number_of_spline_points(),
               "material": actor.surface_material.get_path_name()}
        if not mesh or mesh.get_num_triangles(0) != row["triangles"] or row["preview"] or row["source_builds"] or row["conversions"]:
            raise RuntimeError("新进程重载后缓存未完整复用：" + str(row))
        probe = actor.get_actor_location() + (unreal.Vector(0,0,0) if row["vertices"] > 256 else unreal.Vector(100,100,0))
        hit = unreal.SystemLibrary.line_trace_single(world,probe + unreal.Vector(0,0,100),probe - unreal.Vector(0,0,100),
            unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,False,[],unreal.DrawDebugTrace.NONE,True)
        if not hit:
            raise RuntimeError("新进程重载后地面碰撞丢失")
        row["collision_trace"] = True
        # 验证恢复后仍能编辑参数并重新生成完整轮廓，修改仅发生在隔离验收关卡。
        actor.set_editor_property("height_offset_meters", .44)
        actor.finalize_surface()
        if actor.geometry_build_count < 1 or actor.surface_preview or actor.generated_vertex_count != row["vertices"] or abs(actor.height_offset_meters-.44)>.001:
            raise RuntimeError("恢复后参数编辑或完整轮廓生成失败：" + str({
                "builds":actor.geometry_build_count,"preview":actor.surface_preview,
                "vertices":actor.generated_vertex_count,"height_m":actor.height_offset_meters}))
        row["editable_after_reload"] = True
        row["edit_preview_and_full_builds"] = actor.geometry_build_count
        result.append(row)
    if len(result) != 2:
        raise RuntimeError("验证关卡应包含曲线及凹形两块地面")
    (OUT / "新进程重载.json").write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding="utf-8")
    return result


def audit_ground():
    """只读加载现有关卡，核对真实旧地面的米制迁移与轮廓恢复。"""
    world = unreal.EditorLoadingAndSavingUtils.load_map("/Game/项目内容/关卡/唯一关卡")
    if not world:
        raise RuntimeError("无法只读加载原关卡")
    rows = []
    for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
        if not isinstance(actor,unreal.LxSplineSurfaceActor):
            continue
        actor.finalize_surface()
        rows.append({"actor":actor.get_actor_label(),"control_points":actor.surface_spline.get_number_of_spline_points(),
                     "vertices":actor.generated_vertex_count,"triangles":actor.generated_triangle_count,
                     "height_m":round(actor.height_offset_meters,2),"sample_spacing_m":round(actor.edge_sample_spacing_meters,2),
                     "collision_depth_m":round(actor.collision_thickness_meters,2),"diagnostic":actor.generation_diagnostic,
                     "material":actor.surface_material.get_path_name() if actor.surface_material else None})
    (OUT / "原关卡只读验收.json").write_text(json.dumps(rows,ensure_ascii=False,indent=2),encoding="utf-8")
    return rows


def regression_ground():
    """启动地面、紧凑缓存及道路连接片相关的自动化回归。"""
    unreal.SystemLibrary.execute_console_command(None,
        "Automation RunTests LxARPG.World.SplineSurface+LxARPG.World.GeneratedMesh.Compact+LxARPG.World.HarborRoad")
    return {"regression_started": True}


def details_ground():
    """检查自动化类型的功能参数详情面板，旧兼容字段保持隐藏。"""
    unreal.SystemLibrary.execute_console_command(None,"Automation RunTests LxARPG.World.GeneratedMesh.SimpleDetails")
    return {"details_test_started": True}


def capture_ground():
    """创建独立日光展示场景，离屏检查曲线与凹形地面的铺装风格。"""
    if STATE.get("handle"):
        raise RuntimeError("预览正在渲染，请等待完成")
    world = unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    blueprint = unreal.load_asset(PRESET)
    for name, location, points, curved in (
        ("弧形石板地面", unreal.Vector(-350, 0, 0),
         [unreal.Vector(math.cos(i*math.tau/8)*400, math.sin(i*math.tau/8)*400, 0) for i in range(8)], True),
        ("凹形石板地面", unreal.Vector(250, -250, 0),
         [unreal.Vector(x,y,0) for x,y in ((0,0),(650,0),(650,220),(220,220),(220,600),(0,600))], False)):
        actor = actors.spawn_actor_from_class(blueprint.generated_class(), location)
        actor.set_actor_label(name)
        spline = actor.surface_spline
        spline.clear_spline_points(False)
        for point in points:
            spline.add_spline_point(point, unreal.SplineCoordinateSpace.LOCAL, False)
        for i in range(len(points)):
            spline.set_spline_point_type(i, unreal.SplinePointType.CURVE if curved else unreal.SplinePointType.LINEAR, False)
        spline.set_closed_loop(True, True)
        actor.set_editor_property("control_point_count", len(points))
        actor.set_editor_property("edge_sample_spacing_meters", .25)
        actor.finalize_surface()
    for label, rotation, intensity, shadow in (("温暖日光",(-55,-35,0),8,True),("清透补光",(-25,130,0),2,False)):
        light = actors.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0,0,1500),
            unreal.Rotator(pitch=rotation[0],yaw=rotation[1],roll=rotation[2]))
        light.set_actor_label(label)
        component = light.get_component_by_class(unreal.DirectionalLightComponent)
        component.set_mobility(unreal.ComponentMobility.MOVABLE)
        component.set_editor_property("intensity", intensity)
        component.set_editor_property("cast_shadows", shadow)
        if shadow:
            component.set_editor_property("atmosphere_sun_light", True)
    sky = actors.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0,0,1000))
    sky.get_component_by_class(unreal.SkyLightComponent).set_mobility(unreal.ComponentMobility.MOVABLE)
    sky.get_component_by_class(unreal.SkyLightComponent).set_editor_property("real_time_capture", True)
    actors.spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector())
    backdrop = unreal.load_asset("/Game/项目内容/美术资产/海港城市/石地板/演示/晴蓝背景")
    dome = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector())
    dome.set_actor_label("晴蓝展示背景")
    dome.static_mesh_component.set_static_mesh(unreal.load_asset("/Engine/BasicShapes/Sphere"))
    dome.static_mesh_component.set_material(0, backdrop)
    dome.static_mesh_component.set_editor_property("cast_shadow", False)
    dome.static_mesh_component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    dome.set_actor_scale3d(unreal.Vector(200,200,200))
    unreal.EditorAssetLibrary.make_directory(BASE + "/演示")
    if not unreal.EditorLoadingAndSavingUtils.save_map(world, BASE + "/演示/样条地面演示"):
        raise RuntimeError("展示关卡保存失败")
    camera = actors.spawn_actor_from_class(unreal.SceneCapture2D, unreal.Vector(1100,-1400,1350))
    camera.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(camera.get_actor_location(), unreal.Vector(30,0,0)), False)
    component = camera.get_component_by_class(unreal.SceneCaptureComponent2D)
    target = unreal.RenderingLibrary.create_render_target2d(world,1600,1000,unreal.TextureRenderTargetFormat.RTF_RGBA8)
    component.set_editor_property("texture_target", target)
    component.set_editor_property("capture_source", unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR)
    component.set_editor_property("capture_every_frame", False)
    component.set_editor_property("always_persist_rendering_state", True)
    component.set_editor_property("fov_angle", 55)
    settings = component.get_editor_property("post_process_settings")
    for key, value in {"override_auto_exposure_min_brightness":True,"override_auto_exposure_max_brightness":True,
                       "auto_exposure_min_brightness":1.,"auto_exposure_max_brightness":1.}.items():
        settings.set_editor_property(key, value)
    component.set_editor_property("post_process_settings", settings)
    unreal.AutomationLibrary.finish_loading_before_screenshot()
    STATE.update(world=world,camera=camera,component=component,target=target,phase=0,next=time.monotonic()+8)
    STATE["handle"] = unreal.register_slate_post_tick_callback(capture_ground_tick)
    return {"rendering": True, "demo": BASE + "/演示/样条地面演示"}


def capture_ground_tick(delta):
    """等待材质与光照收敛后输出总览及近景，清理临时摄影机。"""
    try:
        STATE["component"].capture_scene()
        if time.monotonic() < STATE["next"]:
            return
        filename = ("样条地面总览.png", "样条地面细节.png")[STATE["phase"]]
        unreal.RenderingLibrary.export_render_target(STATE["world"],STATE["target"],str(OUT),filename)
        if STATE["phase"] == 0:
            location = unreal.Vector(-20,-500,460)
            STATE["camera"].set_actor_location(location,False,False)
            STATE["camera"].set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(location,unreal.Vector(-350,0,0)),False)
            STATE["phase"] = 1
            STATE["next"] = time.monotonic()+8
        else:
            unreal.unregister_slate_post_tick_callback(STATE.pop("handle"))
            unreal.get_editor_subsystem(unreal.EditorActorSubsystem).destroy_actor(STATE["camera"])
            (OUT / "预览完成.json").write_text('{"captures":2}',encoding="utf-8")
    except Exception:
        if STATE.get("handle"):
            unreal.unregister_slate_post_tick_callback(STATE.pop("handle"))
        (OUT / "预览错误.txt").write_text(traceback.format_exc(),encoding="utf-8")


@unreal.uclass()
class SplineGroundTools(unreal.ToolsetDefinition):
    """样条地面的官方MCP制作与验证入口。"""

    @unreal.ufunction(static=True, params=[str], ret=str,
                     meta={"AICallable": "", "DisplayName": "处理样条地面", "Category": "样条地面|制作"})
    def process_ground(stage):
        """串行执行只读检查、材质制作、功能测试与离屏预览。"""
        try:
            if stage == "inspect":
                result = inspect_ground()
            elif stage == "refresh":
                source_path = Path(unreal.Paths.project_dir()).resolve() / "Source/LxARPGEditor/Scripts/样条地面资产工具.py"
                source = source_path.read_text(encoding="utf-8")
                exec(compile(source.split("@unreal.uclass()")[0], str(source_path), "exec"), globals())
                result = {"refreshed": True}
            elif stage == "tests":
                unreal.SystemLibrary.execute_console_command(None, "Automation RunTests LxARPG.World.SplineSurface")
                result = {"tests_started": True}
            elif stage == "regression":
                unreal.SystemLibrary.execute_console_command(None,
                    "Automation RunTests LxARPG.World.SplineSurface+LxARPG.World.GeneratedMesh.Compact+LxARPG.World.HarborRoad")
                result = {"regression_started": True}
            elif stage == "quit":
                unreal.SystemLibrary.execute_console_command(None, "QUIT_EDITOR")
                result = {"quit": True}
            else:
                result = globals()[stage + "_ground"]()
            return json.dumps(result, ensure_ascii=False)
        except Exception:
            return json.dumps({"error": traceback.format_exc()}, ensure_ascii=False)


unreal.EditorPythonScripting.set_keep_python_script_alive(True)
unreal.ToolsetRegistry.register_toolset_class(SplineGroundTools)
unreal.SystemLibrary.execute_console_command(None, "ModelContextProtocol.StartServer 8035")
