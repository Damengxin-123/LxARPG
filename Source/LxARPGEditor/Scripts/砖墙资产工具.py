"""使用虚幻5.8官方MCP创建自动砖墙材质、中文预设和独立演示场景。"""
import json
import time
import traceback
from pathlib import Path
import unreal

# 专属资产目录与离屏预览输出，不写入现有关卡。
BASE = "/Game/项目内容/美术资产/海港城市/砖墙"
OUT = Path(unreal.Paths.project_dir()).resolve() / "Saved/BrickWall"
OUT.mkdir(parents=True, exist_ok=True)
# 分帧捕获状态，保证材质编译和渲染完成。
STATE = {}


def expression(mat, cls, **props):
    """创建带中文描述的材质节点。"""
    node = unreal.MaterialEditingLibrary.create_material_expression(mat, cls)
    for key, value in props.items():
        node.set_editor_property(key, value)
    return node


def custom(mat, code, inputs, output_type, description):
    """创建三维连续石材纹理，不按各面的UV分别平铺。"""
    node = expression(mat, unreal.MaterialExpressionCustom, code=code,
                      output_type=output_type, description=description)
    entries = []
    for name in inputs:
        entry = unreal.CustomInput()
        entry.set_editor_property("input_name", name)
        entries.append(entry)
    node.set_editor_property("inputs", entries)
    for name, source in inputs.items():
        if not unreal.MaterialEditingLibrary.connect_material_expressions(source, "", node, name):
            raise RuntimeError("材质连接失败：" + name)
    return node


def output(mat, node, prop):
    """连接材质属性并检查连接有效性。"""
    if not unreal.MaterialEditingLibrary.connect_material_property(node, "", prop):
        raise RuntimeError("材质输出失败：" + str(prop))


def make_material(name, tint):
    """生成低对比斑驳、细颗粒、适度粗糙的灰白石材。"""
    mat = unreal.load_asset(BASE + "/材质/" + name)
    if not mat:
        mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            name, BASE + "/材质", unreal.Material, unreal.MaterialFactoryNew())
    unreal.MaterialEditingLibrary.delete_all_material_expressions(mat)
    pos = expression(mat, unreal.MaterialExpressionWorldPosition, desc="连续三维位置")
    local = expression(mat, unreal.MaterialExpressionTransformPosition,
                       transform_source_type=unreal.MaterialPositionTransformSource.TRANSFORMPOSSOURCE_WORLD,
                       transform_type=unreal.MaterialPositionTransformSource.TRANSFORMPOSSOURCE_LOCAL,
                       desc="随墙移动的局部坐标")
    if not unreal.MaterialEditingLibrary.connect_material_expressions(pos, "", local, ""):
        raise RuntimeError("局部坐标连接失败")
    vertex = expression(mat, unreal.MaterialExpressionVertexColor, desc="整块石砖色差与底部积尘")
    color = expression(mat, unreal.MaterialExpressionVectorParameter, parameter_name="石材颜色",
                       default_value=unreal.LinearColor(*tint, 1), desc="石材主色")
    mottling = expression(mat, unreal.MaterialExpressionScalarParameter, parameter_name="斑驳强度",
                          default_value=0.11, desc="维护良好的轻微风化")
    code = """
float broad = sin(P.x*.081+sin(P.y*.069))*sin(P.z*.117+P.y*.037);
float grain = sin(P.x*1.7+P.z*2.3)*sin(P.y*2.1-P.x*.7);
float speck = pow(saturate(sin(P.x*4.3+P.z*3.7)*sin(P.y*3.2-P.z*2.1)),12);
return C.rgb*V.rgb*(1+Amount*broad+.014*grain-.035*speck);
"""
    shade = custom(mat, code, {"P":local,"C":color,"V":vertex,"Amount":mottling},
                   unreal.CustomMaterialOutputType.CMOT_FLOAT3, "同块砖的拐角连续斑驳")
    output(mat, shade, unreal.MaterialProperty.MP_BASE_COLOR)
    roughness = expression(mat, unreal.MaterialExpressionScalarParameter, parameter_name="粗糙度",
                           default_value=0.84, desc="石材哑光粗糙度")
    output(mat, roughness, unreal.MaterialProperty.MP_ROUGHNESS)
    specular = expression(mat, unreal.MaterialExpressionConstant, r=0.24, desc="克制的石材高光")
    output(mat, specular, unreal.MaterialProperty.MP_SPECULAR)
    unreal.MaterialEditingLibrary.layout_material_expressions(mat)
    unreal.MaterialEditingLibrary.recompile_material(mat)
    unreal.EditorAssetLibrary.save_loaded_asset(mat, only_if_is_dirty=False)
    return mat


def build_assets():
    """创建灰白墙面、偏冷连接石和内凹砂浆，并保存直接可用的中文蓝图。"""
    for folder in ("材质", "预设", "演示"):
        unreal.EditorAssetLibrary.make_directory(BASE + "/" + folder)
    materials = [make_material("灰白石砖", (.62,.625,.60)),
                 make_material("冷灰连接石", (.37,.405,.43)),
                 make_material("浅灰砂浆", (.30,.31,.295))]
    paths = []
    for name, mode in (("自动砖墙",unreal.LxWallConnection.STRAIGHT),
                       ("直角砖墙",unreal.LxWallConnection.RIGHT),
                       ("无柱砖墙",unreal.LxWallConnection.NONE)):
        bp = unreal.load_asset(BASE + "/预设/" + name)
        if not bp:
            factory = unreal.BlueprintFactory()
            factory.set_editor_property("parent_class", unreal.LxBrickWallActor)
            bp = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name,BASE+"/预设",unreal.Blueprint,factory)
        default = unreal.get_default_object(bp.generated_class())
        bp.set_editor_property("run_construction_script_on_drag",False)
        for prop, mat in zip(("wall_material","connection_material","mortar_material"), materials):
            default.set_editor_property(prop, mat)
        default.set_editor_property("start_connection", unreal.LxWallConnection.NONE if mode==unreal.LxWallConnection.NONE else unreal.LxWallConnection.STRAIGHT)
        default.set_editor_property("end_connection", mode)
        default.set_editor_property("cull_internal_faces",True)
        unreal.BlueprintEditorLibrary.compile_blueprint(bp)
        unreal.EditorAssetLibrary.save_loaded_asset(bp, only_if_is_dirty=False)
        paths.append(bp.get_path_name())
    unreal.AutomationLibrary.finish_loading_before_screenshot()
    return {"presets":paths, "materials":[m.get_path_name() for m in materials]}


def make_demo():
    """保存独立演示关卡，呈现直墙、双向转角与裸露砖端面。"""
    world = unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    for name, location, mode in (("直墙双柱",(0,500,0),unreal.LxWallConnection.STRAIGHT),
                                  ("朝右转角",(0,0,0),unreal.LxWallConnection.RIGHT),
                                  ("朝左转角",(0,-650,0),unreal.LxWallConnection.LEFT),
                                  ("完整砖端面",(850,250,0),unreal.LxWallConnection.NONE)):
        bp = unreal.load_asset(BASE+"/预设/自动砖墙")
        wall = actors.spawn_actor_from_class(bp.generated_class(),unreal.Vector(*location))
        wall.set_actor_label(name)
        wall.set_editor_property("end_connection",mode)
        if mode==unreal.LxWallConnection.NONE:
            wall.set_editor_property("start_connection",mode)
            wall.set_editor_property("length_handle",unreal.Vector(260,0,0))
        wall.rebuild_wall()
    floor = actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(500,0,-12))
    floor.set_actor_label("浅灰地台")
    floor.static_mesh_component.set_static_mesh(unreal.load_asset("/Engine/BasicShapes/Cube"))
    floor.static_mesh_component.set_material(0,unreal.load_asset("/Engine/BasicShapes/BasicShapeMaterial"))
    floor.set_actor_scale3d(unreal.Vector(26,24,.2))
    sun = actors.spawn_actor_from_class(unreal.DirectionalLight,unreal.Vector(0,0,2000),unreal.Rotator(pitch=-48,yaw=135,roll=0))
    sun.set_actor_label("明亮日光")
    sun.get_component_by_class(unreal.DirectionalLightComponent).set_editor_property("intensity",10.0)
    sun.get_component_by_class(unreal.DirectionalLightComponent).set_mobility(unreal.ComponentMobility.MOVABLE)
    sun.get_component_by_class(unreal.DirectionalLightComponent).set_editor_property("atmosphere_sun_light",True)
    fill = actors.spawn_actor_from_class(unreal.DirectionalLight,unreal.Vector(0,0,1800),unreal.Rotator(pitch=-35,yaw=-45,roll=0))
    fill.set_actor_label("冷色环境补光")
    fill_component = fill.get_component_by_class(unreal.DirectionalLightComponent)
    fill_component.set_editor_property("intensity",3.0)
    fill_component.set_editor_property("cast_shadows",False)
    fill_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    fill_component.set_light_color(unreal.LinearColor(.76,.86,1))
    sky = actors.spawn_actor_from_class(unreal.SkyLight,unreal.Vector(0,0,1000))
    sky.set_actor_label("清透天空光")
    sky.get_component_by_class(unreal.SkyLightComponent).set_mobility(unreal.ComponentMobility.MOVABLE)
    sky.get_component_by_class(unreal.SkyLightComponent).set_editor_property("intensity",1.1)
    sky.get_component_by_class(unreal.SkyLightComponent).set_editor_property("real_time_capture",True)
    actors.spawn_actor_from_class(unreal.SkyAtmosphere,unreal.Vector()).set_actor_label("天空")
    unreal.EditorLoadingAndSavingUtils.save_map(world,BASE+"/演示/砖墙演示")
    return {"map":BASE+"/演示/砖墙演示"}


def verify_assets():
    """检查已保存蓝图的实例、材质引用以及独立的连接材质。"""
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    results = []
    for name in ("自动砖墙","直角砖墙","无柱砖墙"):
        bp = unreal.load_asset(BASE+"/预设/"+name)
        if not bp: raise RuntimeError("缺失预设："+name)
        wall = actors.spawn_actor_from_class(bp.generated_class(),unreal.Vector(0,0,-5000))
        try:
            wall.rebuild_wall()
            if wall.get_editor_property("generation_diagnostic"): raise RuntimeError("生成失败："+name)
            if wall.get_editor_property("generated_brick_count")<=0: raise RuntimeError("无石砖："+name)
            for prop in ("wall_material","connection_material","mortar_material"):
                if not wall.get_editor_property(prop): raise RuntimeError("材质缺失："+prop)
            if wall.get_editor_property("wall_material")==wall.get_editor_property("connection_material"):
                raise RuntimeError("连接处材质未独立")
            results.append({"preset":name,"bricks":wall.get_editor_property("generated_brick_count")})
        finally:
            actors.destroy_actor(wall)
    (OUT/"资产验证.json").write_text(json.dumps(results,ensure_ascii=False,indent=2),encoding="utf-8")
    return results


def capture_preview():
    """离屏渲染总体和砖角细节，避免计算机屏幕控制。"""
    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    levels.load_level(BASE+"/演示/砖墙演示")
    levels.editor_set_viewport_realtime(True)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    camera = actors.spawn_actor_from_class(unreal.SceneCapture2D,unreal.Vector(1500,-1800,1050))
    camera.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(camera.get_actor_location(),unreal.Vector(400,-50,100)),False)
    component = camera.get_component_by_class(unreal.SceneCaptureComponent2D)
    target = unreal.RenderingLibrary.create_render_target2d(world,1600,1000,unreal.TextureRenderTargetFormat.RTF_RGBA8)
    component.set_editor_property("texture_target",target)
    component.set_editor_property("capture_source",unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR)
    component.set_editor_property("capture_every_frame",False)
    component.set_editor_property("always_persist_rendering_state",True)
    pp = component.get_editor_property("post_process_settings")
    for prop, value in {"override_auto_exposure_min_brightness":True,"override_auto_exposure_max_brightness":True,
                        "auto_exposure_min_brightness":1.0,"auto_exposure_max_brightness":1.0}.items():
        pp.set_editor_property(prop,value)
    component.set_editor_property("post_process_settings",pp)
    unreal.AutomationLibrary.finish_loading_before_screenshot()
    STATE.update(world=world,camera=camera,component=component,target=target,phase=0,next=time.monotonic()+8)
    STATE["handle"] = unreal.register_slate_post_tick_callback(capture_tick)
    return {"capture_started":True}


def capture_tick(delta):
    """等待渲染收敛后输出三种视角并清理临时摄像机。"""
    try:
        STATE["component"].capture_scene()
        if time.monotonic()<STATE["next"]: return
        phase = STATE["phase"]
        name = ("砖墙总览.png","砖端细节.png","转角细节.png")[phase]
        unreal.RenderingLibrary.export_render_target(STATE["world"],STATE["target"],str(OUT),name)
        if phase<2:
            location, look = ((1350,-40,340),(1070,250,130)) if phase==0 else ((980,-310,440),(620,80,135))
            STATE["camera"].set_actor_location(unreal.Vector(*location),False,False)
            STATE["camera"].set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(unreal.Vector(*location),unreal.Vector(*look)),False)
            STATE["phase"] += 1
            STATE["next"] = time.monotonic()+8
        else:
            unreal.unregister_slate_post_tick_callback(STATE["handle"])
            unreal.get_editor_subsystem(unreal.EditorActorSubsystem).destroy_actor(STATE["camera"])
            (OUT/"预览完成.json").write_text('{"captures":3}',encoding="utf-8")
    except Exception:
        unreal.unregister_slate_post_tick_callback(STATE["handle"])
        (OUT/"预览错误.txt").write_text(traceback.format_exc(),encoding="utf-8")


def save_static_presets():
    """通过真实蓝图保存三种整墙静态网格，检查独立网格、材质和简化碰撞。"""
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    meshes = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    results = []
    for name in ("自动砖墙", "直角砖墙", "无柱砖墙"):
        preset_path = BASE+"/预设/"+name
        if not unreal.EditorAssetLibrary.does_asset_exist(preset_path):
            preset_path = BASE+"/预设/自动砖墙"
        blueprint = unreal.load_asset(preset_path)
        if not blueprint:
            raise RuntimeError("缺失基础自动砖墙预设")
        wall = actors.spawn_actor_from_class(blueprint.generated_class(), unreal.Vector(0, 0, -5000))
        try:
            wall.set_editor_property("start_connection", unreal.LxWallConnection.NONE if name == "无柱砖墙" else unreal.LxWallConnection.STRAIGHT)
            wall.set_editor_property("end_connection", unreal.LxWallConnection.NONE if name == "无柱砖墙" else unreal.LxWallConnection.RIGHT if name == "直角砖墙" else unreal.LxWallConnection.STRAIGHT)
            wall.set_editor_property("generate_static_mesh", True)
            wall.set_editor_property("static_mesh_asset_path", BASE+"/静态网格/"+name)
            wall.save_static_mesh_asset()
            mesh = wall.get_editor_property("saved_static_mesh")
            if not mesh:
                raise RuntimeError(wall.get_editor_property("generation_diagnostic"))
            if wall.get_editor_property("generated_mesh_count") != 1:
                raise RuntimeError("整墙没有合并为一个网格")
            triangles = mesh.get_num_triangles(0)
            if triangles != wall.get_editor_property("generated_triangle_count"):
                raise RuntimeError("保存后丢失三角形")
            expected = 4 if name == "直角砖墙" else 1 if name == "无柱砖墙" else 3
            collision = meshes.get_simple_collision_count(mesh)
            if collision != expected:
                raise RuntimeError("碰撞分段数量不正确："+str(collision))
            results.append({"preset": name, "asset": mesh.get_path_name(), "meshes": 1,
                            "triangles": triangles, "collision_boxes": collision,
                            "materials": [str(mesh.get_material(i).get_path_name()) for i in range(3)]})
        finally:
            actors.destroy_actor(wall)
    (OUT/"静态网格验收.json").write_text(json.dumps(results, ensure_ascii=False, indent=2), encoding="utf-8")
    return results


def verify_saved_static_meshes():
    """在新编辑器进程重新加载保存资产，验证渲染数据和独立碰撞可恢复。"""
    meshes = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    results = json.loads((OUT/"静态网格验收.json").read_text(encoding="utf-8"))
    for entry in results:
        mesh = unreal.load_asset(entry["asset"])
        if not mesh or mesh.get_num_triangles(0) != entry["triangles"]:
            raise RuntimeError("保存的静态几何无法完整加载："+entry["asset"])
        if meshes.get_simple_collision_count(mesh) != entry["collision_boxes"]:
            raise RuntimeError("重新加载后碰撞丢失")
        if [str(mesh.get_material(i).get_path_name()) for i in range(3)] != entry["materials"]:
            raise RuntimeError("重新加载后材质槽变化")
        entry["reloaded"] = True
    (OUT/"静态网格重载验收.json").write_text(json.dumps(results, ensure_ascii=False, indent=2), encoding="utf-8")
    return results


def inspect_scene():
    """返回离屏验证所需的日光方向、材质和几何法线样本。"""
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    result = []
    for actor in actors.get_all_level_actors():
        if isinstance(actor,unreal.DirectionalLight):
            component = actor.get_component_by_class(unreal.DirectionalLightComponent)
            result.append({"light":actor.get_actor_label(),"rotation":str(actor.get_actor_rotation()),
                           "forward":str(actor.get_actor_forward_vector()),"intensity":component.get_editor_property("intensity")})
        if isinstance(actor,unreal.LxBrickWallActor):
            mesh=actor.get_editor_property("wall_mesh")
            static_mesh = actor.get_editor_property("static_wall_mesh").get_editor_property("static_mesh")
            result.append({"wall":actor.get_actor_label(),"material":str(mesh.get_material(0)),
                           "static_mesh": static_mesh.get_path_name() if static_mesh else None,
                           "render_meshes": actor.get_editor_property("generated_mesh_count")})
    return result


@unreal.uclass()
class BrickWallTools(unreal.ToolsetDefinition):
    """自动砖墙的官方MCP工具集。"""
    @unreal.ufunction(static=True,params=[str],ret=str,meta={"AICallable":"","DisplayName":"处理砖墙资产","Category":"砖墙|制作"})
    def process_wall(stage):
        """按阶段生成资产、验证、创建演示、捕获预览或运行自动化测试。"""
        try:
            if stage=="build": result=build_assets()
            elif stage=="demo": result=make_demo()
            elif stage=="verify": result=verify_assets()
            elif stage=="capture": result=capture_preview()
            elif stage=="inspect": result=inspect_scene()
            elif stage=="static": result=save_static_presets()
            elif stage=="reloadstatic": result=verify_saved_static_meshes()
            elif stage=="benchmark": result=benchmark_wall()
            elif stage=="profile":
                script=Path(unreal.Paths.project_dir())/"Source/LxARPGEditor/Scripts/生成工具渲染诊断.py"
                exec(compile(script.read_text(encoding="utf-8"),str(script),"exec"),{})
                result={"profile_started":True}
            elif stage in ("audit", "worldtests"):
                prefix = "LxARPG.Diagnostics.GeneratedWorldRuntime" if stage=="audit" else "LxARPG.World."
                unreal.SystemLibrary.execute_console_command(None,"Automation RunTests "+prefix)
                result={"tests_started":True,"prefix":prefix}
            elif stage=="tests":
                unreal.SystemLibrary.execute_console_command(None,"Automation RunTests LxARPG.World.BrickWall")
                result={"tests_started":True}
            elif stage=="quit":
                unreal.SystemLibrary.quit_editor()
                result={"quit":True}
            else: raise ValueError(stage)
            return json.dumps(result,ensure_ascii=False)
        except Exception:
            return json.dumps({"error":traceback.format_exc()},ensure_ascii=False)


def benchmark_wall():
    """记录独立实例完整重建耗时，便于同机比较修改前后性能。"""
    actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    wall=actors.spawn_actor_from_class(unreal.LxBrickWallActor,unreal.Vector(0,0,-5000))
    results=[]
    try:
        for length,height in ((600,240),(6000,600)):
            wall.set_editor_property("length_handle",unreal.Vector(length,0,0))
            wall.set_editor_property("wall_height",height)
            samples=[]
            for _ in range(5):
                start=time.perf_counter()
                wall.rebuild_wall()
                samples.append((time.perf_counter()-start)*1000)
            result={"length":length,"height":height,"bricks":wall.get_editor_property("generated_brick_count"),
                    "rebuild_ms":samples,"median_ms":sorted(samples)[2]}
            try:
                result["triangles"]=wall.get_editor_property("generated_triangle_count")
            except Exception:
                pass
            results.append(result)
    finally:
        actors.destroy_actor(wall)
    (OUT/"性能采样.json").write_text(json.dumps(results,ensure_ascii=False,indent=2),encoding="utf-8")
    return results


unreal.EditorPythonScripting.set_keep_python_script_alive(True)
unreal.ToolsetRegistry.register_toolset_class(BrickWallTools)
unreal.SystemLibrary.execute_console_command(None,"ModelContextProtocol.StartServer 8006")
unreal.SystemLibrary.execute_console_command(None,"ModelContextProtocol.RefreshTools")
