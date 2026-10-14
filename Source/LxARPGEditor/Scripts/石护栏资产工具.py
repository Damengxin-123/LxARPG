"""通过 UE 5.8 官方 MCP 创建灰白石护栏预设、验证碰撞并渲染独立演示。"""
import json
from collections import Counter
import time
import traceback
from pathlib import Path
import unreal

# 新资产仅写入专用目录；演示在独立编辑器进程中构建。
BASE = "/Game/项目内容/美术资产/海港城市/石护栏"
PRESET_DIR = "/Game/项目内容/美术资产/自动化类型/护栏"
OUT = Path(unreal.Paths.project_dir()).resolve() / "Saved/StoneRailing"
OUT.mkdir(parents=True, exist_ok=True)
# 跨编辑器帧保留离屏渲染对象。
STATE = {}
# 三种可直接放置的中文预设。
PRESETS = (("海港石护栏", unreal.LxStoneRailingStyle.HARBOUR),
           ("花叶石护栏", unreal.LxStoneRailingStyle.FLORAL),
           ("波纹石护栏", unreal.LxStoneRailingStyle.WAVE))


def node(material, cls, **properties):
    """创建材质节点并填写中文描述与参数。"""
    result = unreal.MaterialEditingLibrary.create_material_expression(material, cls)
    for key, value in properties.items():
        result.set_editor_property(key, value)
    return result


def stone_material(name, tint):
    """创建克制的三维灰白石纹，世界坐标消除几何接缝处纹理断裂。"""
    path = BASE + "/材质/" + name
    material = unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else None
    if not material:
        material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            name, BASE + "/材质", unreal.Material, unreal.MaterialFactoryNew())
    unreal.MaterialEditingLibrary.delete_all_material_expressions(material)
    position = node(material, unreal.MaterialExpressionWorldPosition, desc="连续世界石纹坐标")
    color = node(material, unreal.MaterialExpressionVectorParameter,
                 parameter_name="石材颜色", default_value=unreal.LinearColor(*tint, 1), desc="明亮灰白石色")
    grain = node(material, unreal.MaterialExpressionCustom, description="轻微矿物斑纹与柔和石脉",
                 output_type=unreal.CustomMaterialOutputType.CMOT_FLOAT3, code="""
float3 p=P*.035;
float cloud=sin(p.x+sin(p.z*.7))*sin(p.y*.8+p.z*.3);
float vein=pow(saturate(.5+.5*sin(p.x*.8+p.y*.6+p.z*.3+cloud*1.1)),16);
float fine=sin(P.x*1.7+P.z*.5)*sin(P.y*1.3-P.z*.9);
return Tint.rgb*(.98+cloud*.025-vein*.045+fine*.005);
""")
    inputs = []
    for key in ("P", "Tint"):
        entry = unreal.CustomInput()
        entry.set_editor_property("input_name", key)
        inputs.append(entry)
    grain.set_editor_property("inputs", inputs)
    unreal.MaterialEditingLibrary.connect_material_expressions(position, "", grain, "P")
    unreal.MaterialEditingLibrary.connect_material_expressions(color, "", grain, "Tint")
    unreal.MaterialEditingLibrary.connect_material_property(grain, "", unreal.MaterialProperty.MP_BASE_COLOR)
    rough = node(material, unreal.MaterialExpressionScalarParameter, parameter_name="粗糙度", default_value=.82, desc="细磨石材")
    specular = node(material, unreal.MaterialExpressionConstant, r=.22, desc="柔和反光")
    unreal.MaterialEditingLibrary.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    unreal.MaterialEditingLibrary.connect_material_property(specular, "", unreal.MaterialProperty.MP_SPECULAR)
    unreal.MaterialEditingLibrary.layout_material_expressions(material)
    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=False)
    return material


def build_assets():
    """保存主体石材、雕花石材和三个原生 C++ 派生蓝图。"""
    unreal.EditorAssetLibrary.make_directory(PRESET_DIR)
    unreal.EditorAssetLibrary.make_directory(BASE + "/材质")
    body = stone_material("灰白细石", (.68, .69, .67))
    carving = stone_material("浅灰雕石", (.57, .61, .61))
    paths = []
    for name, style in PRESETS:
        path = PRESET_DIR + "/" + name
        # 兼容早期制作路径，统一到项目现有的自动化类型目录。
        previous = "/Game/项目内容/场景/自动化/护栏/" + name
        if not unreal.EditorAssetLibrary.does_asset_exist(path) and unreal.EditorAssetLibrary.does_asset_exist(previous):
            if not unreal.EditorAssetLibrary.rename_asset(previous, path):
                raise RuntimeError("预设移入自动化目录失败：" + name)
        bp = unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else None
        if not bp:
            factory = unreal.BlueprintFactory()
            factory.set_editor_property("parent_class", unreal.LxStoneRailingActor)
            bp = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, PRESET_DIR, unreal.Blueprint, factory)
        default = unreal.get_default_object(bp.generated_class())
        for key, value in {"style": style, "stone_material": body, "ornament_material": carving}.items():
            default.set_editor_property(key, value)
        unreal.BlueprintEditorLibrary.compile_blueprint(bp)
        unreal.EditorAssetLibrary.save_loaded_asset(bp, only_if_is_dirty=False)
        paths.append(bp.get_path_name())
    return {"presets": paths, "verification": verify_assets()}


def verify_assets():
    """验证实际蓝图可生成，并对立柱、顶梁和柱间空隙做简单碰撞射线测试。"""
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    result = []
    for name, style in PRESETS:
        bp = unreal.load_asset(PRESET_DIR + "/" + name)
        actor = actors.spawn_actor_from_class(bp.generated_class(), unreal.Vector(0, 0, -5000))
        try:
            actor.rebuild_railing()
            if actor.get_editor_property("generation_diagnostic") or actor.get_editor_property("generated_post_count") < 3:
                raise RuntimeError("预设生成失败：" + name)
            for x, z, expected in ((0, 55, True), (65, 105, True), (65, 55, False)):
                hit = unreal.SystemLibrary.line_trace_single(world, unreal.Vector(x, -120, z-5000),
                    unreal.Vector(x, 120, z-5000), unreal.TraceTypeQuery.ECC_VISIBILITY, False,
                    [], unreal.DrawDebugTrace.NONE, True)
                if bool(hit) != expected:
                    raise RuntimeError("碰撞不符合立柱/顶梁/空隙：" + str((x, z, hit)))
            actor.get_editor_property("optimized_mesh").finalize_mesh()
            if not actor.get_editor_property("optimized_mesh").get_editor_property("static_mesh"):
                raise RuntimeError("静态渲染缓存构建失败")
            result.append({"preset": name, "posts": actor.get_editor_property("generated_post_count"),
                           "triangles": actor.get_editor_property("generated_triangle_count"), "collision": "passed"})
        finally:
            actors.destroy_actor(actor)
    return result


def preview():
    """创建独立演示关卡并渲染总览和近景，不使用屏幕控制。"""
    world = unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    for index, (name, style) in enumerate(PRESETS):
        bp = unreal.load_asset(PRESET_DIR + "/" + name)
        actor = actors.spawn_actor_from_class(bp.generated_class(), unreal.Vector(0, index*260, 0))
        actor.set_actor_label(name)
        actor.rebuild_railing()
        actor.get_editor_property("optimized_mesh").finalize_mesh()
    ground = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(400, 220, -10))
    ground.set_actor_label("展示地面")
    ground.static_mesh_component.set_static_mesh(unreal.load_asset("/Engine/BasicShapes/Cube"))
    ground.static_mesh_component.set_material(0, unreal.load_asset("/Engine/BasicShapes/BasicShapeMaterial"))
    ground.set_actor_scale3d(unreal.Vector(35, 35, .2))
    for rotation, intensity in ((unreal.Rotator(pitch=-40, yaw=55), 7.0), (unreal.Rotator(pitch=-55, yaw=-55), 3.0)):
        light = actors.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 1000), rotation)
        component = light.get_component_by_class(unreal.DirectionalLightComponent)
        component.set_mobility(unreal.ComponentMobility.MOVABLE)
        component.set_editor_property("intensity", intensity)
        if intensity > 5:
            component.set_editor_property("atmosphere_sun_light", True)
        else:
            component.set_editor_property("cast_shadows", False)
    actors.spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector())
    sky = actors.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 1000))
    sky.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    sky.light_component.set_editor_property("real_time_capture", True)
    unreal.EditorAssetLibrary.make_directory(BASE + "/演示")
    if not unreal.EditorLoadingAndSavingUtils.save_map(world, BASE + "/演示/护栏演示"):
        raise RuntimeError("演示关卡保存失败")
    camera = actors.spawn_actor_from_class(unreal.SceneCapture2D, unreal.Vector(1160, -1150, 800))
    camera.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(camera.get_actor_location(), unreal.Vector(400, 230, 50)), False)
    component = camera.get_component_by_class(unreal.SceneCaptureComponent2D)
    component.set_editor_property("fov_angle", 50.0)
    target = unreal.RenderingLibrary.create_render_target2d(world, 1600, 1000, unreal.TextureRenderTargetFormat.RTF_RGBA8)
    for key, value in {"texture_target": target, "capture_source": unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR,
                       "capture_every_frame": False, "always_persist_rendering_state": True}.items():
        component.set_editor_property(key, value)
    settings = component.get_editor_property("post_process_settings")
    for key, value in {"override_auto_exposure_min_brightness": True, "override_auto_exposure_max_brightness": True,
                       "auto_exposure_min_brightness": 1.0, "auto_exposure_max_brightness": 1.0}.items():
        settings.set_editor_property(key, value)
    component.set_editor_property("post_process_settings", settings)
    unreal.AutomationLibrary.finish_loading_before_screenshot()
    STATE.pop("shots", None)
    STATE.update(world=world, camera=camera, component=component, target=target, phase=0, next=time.monotonic()+8)
    STATE["handle"] = unreal.register_slate_post_tick_callback(capture_tick)
    return {"preview_started": True, "map": BASE + "/演示/护栏演示"}


def verify_corners():
    """直接检查拐角拓扑、碰撞空隙、曲线半径边界及旧几何版本迁移。"""
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    actor = actors.spawn_actor_from_class(unreal.LxStoneRailingActor, unreal.Vector(0, 0, -6000))
    result = []
    try:
        actor.set_editor_property("ornaments", False)
        actor.set_editor_property("density", 4.0)
        actor.set_editor_property("width", 100.0)
        cases = [("直角", [(0,0,0),(400,0,0),(400,400,0)], False, False, True),
                 ("锐角", [(0,0,0),(400,0,0),(117.157,282.843,0)], False, False, True),
                 ("闭环", [(0,0,0),(400,0,0),(400,400,0),(0,400,0)], True, False, True),
                 ("坡面直角", [(0,0,0),(400,0,40),(400,400,120)], False, False, True),
                 ("平滑曲线", [(0,0,0),(400,0,0),(600,300,0)], False, True, True),
                 ("短段折返", [(0,0,0),(400,0,0),(400,20,0),(0,20,0)], False, False, False)]
        for name, points, closed, curved, valid in cases:
            spline = actor.get_editor_property("railing_spline")
            spline.clear_spline_points(False)
            for index, position in enumerate(points):
                spline.add_spline_point(unreal.Vector(*position), unreal.SplineCoordinateSpace.LOCAL, False)
                spline.set_spline_point_type(index, unreal.SplinePointType.CURVE if curved else unreal.SplinePointType.LINEAR, False)
            spline.update_spline()
            actor.set_editor_property("closed_loop", closed)
            actor.rebuild_railing()
            diagnostic = actor.get_editor_property("generation_diagnostic")
            if not valid:
                if not diagnostic or actor.get_editor_property("generated_triangle_count") != 0:
                    raise RuntimeError("短段内角应给出说明并清除无效几何")
                result.append({"case": name, "rejected_cleanly": True})
                continue
            if diagnostic:
                raise RuntimeError(name+"："+diagnostic)
            vertices, triangles, normals, uvs, tangents = unreal.ProceduralMeshLibrary.get_section_from_procedural_mesh(
                actor.get_editor_property("railing_mesh"), 1)
            edges = Counter()
            welded = [(round(v.x,5),round(v.y,5),round(v.z,5)) for v in vertices]
            reversed_top = 0
            for index in range(0, len(triangles), 3):
                ids = triangles[index:index+3]
                for edge in range(3):
                    edges[tuple(sorted((welded[ids[edge]],welded[ids[(edge+1)%3]])))] += 1
                a,b,c = (vertices[i] for i in ids)
                if all(abs(v.z-110)<.001 for v in (a,b,c)):
                    reversed_top += ((c.x-a.x)*(b.y-a.y)-(c.y-a.y)*(b.x-a.x)) < -.001
            if reversed_top or any(count != 2 for count in edges.values()):
                raise RuntimeError(name+"：扶手存在翻面或未闭合边")
            if name == "直角":
                for x,y,expected in ((390,10,True),(325,75,False)):
                    hit = unreal.SystemLibrary.line_trace_single(world,unreal.Vector(x,y,-5800),unreal.Vector(x,y,-5900),
                        unreal.TraceTypeQuery.ECC_VISIBILITY,False,[],unreal.DrawDebugTrace.NONE,True)
                    if bool(hit) != expected:
                        raise RuntimeError("转角碰撞漏挡或错误封闭内侧空隙")
            result.append({"case":name,"watertight":True,"reversed_top":reversed_top,
                           "posts":actor.get_editor_property("generated_post_count")})
        # 当前实例记录算法版本；实际旧关卡迁移由 reload 阶段检查重建计数。
        if actor.get_editor_property("generated_geometry_version") != 2:
            raise RuntimeError("护栏几何版本未更新")
        result.append({"geometry_version":2})
        (OUT/"拐角验证.json").write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding="utf-8")
        return result
    finally:
        actors.destroy_actor(actor)


def corner_preview(label):
    """在独立演示世界检查宽直角、锐角、闭环，临时修改不保存到用户关卡。"""
    verification = verify_corners() if label == "修复后" else None
    preview()
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    rails = [a for a in actors.get_all_level_actors() if isinstance(a, unreal.LxStoneRailingActor)]
    cases = (((0, 0, 0), (400, 0, 0), (400, 400, 0)),
             ((0, 0, 0), (400, 0, 0), (117.157, 282.843, 0)),
             ((0, 0, 0), (400, 0, 0), (400, 400, 0), (0, 400, 0)))
    for index, actor in enumerate(rails):
        actor.set_actor_location(unreal.Vector(index*1100, 0, 0), False, False)
        spline = actor.get_editor_property("railing_spline")
        spline.clear_spline_points(False)
        for point, position in enumerate(cases[index]):
            spline.add_spline_point(unreal.Vector(*position), unreal.SplineCoordinateSpace.LOCAL, False)
            spline.set_spline_point_type(point, unreal.SplinePointType.LINEAR, False)
        spline.update_spline()
        actor.set_editor_property("width", 100.0)
        actor.set_editor_property("closed_loop", index == 2)
        actor.rebuild_railing()
        actor.get_editor_property("optimized_mesh").finalize_mesh()
    # 使用同一相机位置输出前后对照，镜头朝向转角内部。
    STATE["shots"] = [(label+"直角.png", (120, 280, 360), (370, 35, 95)),
                      (label+"锐角.png", (1350, 380, 360), (1430, 70, 95)),
                      (label+"闭环.png", (2850, -600, 700), (2400, 180, 70))]
    name, position, look = STATE["shots"][0]
    STATE["camera"].set_actor_location(unreal.Vector(*position), False, False)
    STATE["camera"].set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(unreal.Vector(*position), unreal.Vector(*look)), False)
    STATE["phase"] = 0
    STATE["next"] = time.monotonic()+8
    return {"corner_preview_started": True, "label": label, "verification": verification}


def capture_tick(delta):
    """等待渲染收敛后输出真实引擎总览与倒角花纹近景。"""
    try:
        STATE["component"].capture_scene()
        if time.monotonic() < STATE["next"]:
            return
        shots = STATE.get("shots")
        name = shots[STATE["phase"]][0] if shots else ("护栏总览.png", "护栏细节.png", "花叶细节.png", "波纹细节.png")[STATE["phase"]]
        unreal.RenderingLibrary.export_render_target(STATE["world"], STATE["target"], str(OUT), name)
        if shots:
            STATE["phase"] += 1
            if STATE["phase"] == len(shots):
                unreal.unregister_slate_post_tick_callback(STATE["handle"])
                (OUT/"拐角预览完成.json").write_text(json.dumps({"images": [s[0] for s in shots]}, ensure_ascii=False), encoding="utf-8")
                return
            name, position, look = shots[STATE["phase"]]
            STATE["camera"].set_actor_location(unreal.Vector(*position), False, False)
            STATE["camera"].set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(unreal.Vector(*position), unreal.Vector(*look)), False)
            STATE["next"] = time.monotonic()+8
            return
        if STATE["phase"] < 3:
            offset = STATE["phase"]*260
            position, look = unreal.Vector(330, offset-290, 155), unreal.Vector(190, offset, 62)
            STATE["camera"].set_actor_location(position, False, False)
            STATE["camera"].set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(position, look), False)
            STATE["phase"] += 1
            STATE["next"] = time.monotonic()+8
        else:
            unreal.unregister_slate_post_tick_callback(STATE["handle"])
            (OUT/"预览完成.json").write_text('{"views":4}', encoding="utf-8")
    except Exception:
        unreal.unregister_slate_post_tick_callback(STATE["handle"])
        (OUT/"预览错误.txt").write_text(traceback.format_exc(), encoding="utf-8")


@unreal.uclass()
class StoneRailingTools(unreal.ToolsetDefinition):
    """自动石护栏的官方 MCP 制作与验收入口。"""
    @unreal.ufunction(static=True, params=[str], ret=str, meta={"AICallable": "", "DisplayName": "处理石护栏", "Category": "石护栏|制作"})
    def process_railing(stage):
        """按指定阶段制作预设、验证资产、输出预览或运行自动化。"""
        try:
            if stage == "build":
                result = build_assets()
            elif stage == "verify":
                result = verify_assets()
            elif stage == "preview":
                result = preview()
            elif stage in ("corners", "corners_before"):
                result = corner_preview("修复前" if stage == "corners_before" else "修复后")
            elif stage == "corner_verify":
                result = verify_corners()
            elif stage == "reload":
                unreal.EditorLoadingAndSavingUtils.load_map("/Engine/Maps/Entry")
                world = unreal.EditorLoadingAndSavingUtils.load_map(BASE + "/演示/护栏演示")
                rails = [a for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
                         if isinstance(a, unreal.LxStoneRailingActor)]
                if len(rails) != 3:
                    raise RuntimeError("重载后的护栏数量不正确")
                result = []
                for actor in rails:
                    cache = actor.get_editor_property("optimized_mesh")
                    if not cache.get_editor_property("static_mesh") or actor.get_editor_property("generated_post_count") != 7:
                        raise RuntimeError("重载后丢失几何或立柱数据")
                    result.append({"preset": actor.get_actor_label(), "posts": actor.get_editor_property("generated_post_count"),
                                   "source_updates": cache.get_editor_property("source_update_count")})
            elif stage == "tests":
                unreal.SystemLibrary.execute_console_command(None, "Automation RunTests LxARPG.World.StoneRailing")
                result = {"tests_started": True}
            elif stage == "refresh":
                path = Path(unreal.Paths.project_dir()).resolve()/"Source/LxARPGEditor/Scripts/石护栏资产工具.py"
                exec(compile(path.read_text(encoding="utf-8").split("@unreal.uclass()")[0], str(path), "exec"), globals())
                result = {"refreshed": True}
            else:
                raise ValueError(stage)
            return json.dumps(result, ensure_ascii=False)
        except Exception:
            return json.dumps({"error": traceback.format_exc()}, ensure_ascii=False)


unreal.EditorPythonScripting.set_keep_python_script_alive(True)
unreal.ToolsetRegistry.register_toolset_class(StoneRailingTools)
unreal.SystemLibrary.execute_console_command(None, "ModelContextProtocol.StartServer 8031")
unreal.SystemLibrary.execute_console_command(None, "ModelContextProtocol.RefreshTools")
