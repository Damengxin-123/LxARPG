"""通过 UE 5.8 官方 MCP 制作木地板、木天花与深色连续木纹材质。"""
import json
import time
import traceback
from pathlib import Path
import unreal

# 专用内容目录和验证输出，不改用户当前制作关卡。
BASE = "/Game/项目内容/美术资产/海港城市/木板"
OUT = Path(unreal.Paths.project_dir()).resolve()/"Saved/TimberPanel"
OUT.mkdir(parents=True, exist_ok=True)
# 分帧离屏渲染的持久状态。
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


def make_material(name, tint, grain):
    """按构件局部三维坐标生成柔和年轮和顺纹，六面与倒角连续。"""
    path = BASE+"/材质/"+name
    mat = unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else None
    if not mat:
        mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, BASE+"/材质", unreal.Material, unreal.MaterialFactoryNew())
    unreal.MaterialEditingLibrary.delete_all_material_expressions(mat)
    uv = expression(mat, unreal.MaterialExpressionTextureCoordinate, coordinate_index=0, desc="木构件顺纹与横向坐标")
    uv1 = expression(mat, unreal.MaterialExpressionTextureCoordinate, coordinate_index=1, desc="木构件厚度与固定随机相位")
    vertex = expression(mat, unreal.MaterialExpressionVertexColor, desc="每根木材的统一温和色差")
    tint_node = expression(mat, unreal.MaterialExpressionVectorParameter, parameter_name="木材颜色", default_value=unreal.LinearColor(*tint,1), desc="清爽中世纪暖木色")
    amount = expression(mat, unreal.MaterialExpressionScalarParameter, parameter_name="木纹强度", default_value=grain, desc="适度概括的低对比木纹")
    code = """
float x=UV.x*100, y=UV.y*100, z=Depth.x*100;
float bend=sin(x*.025+Depth.y)*.42+sin(x*.008)*.65;
float knot=exp(-pow(sin(x*.017)*7,2))*exp(-pow((y-3)*.16,2));
float radius=sqrt(pow(y+18+bend+knot*2,2)+pow(z+28,2)*.65);
float ring=sin(radius*.8+sin(radius*.52)*.3);
float fine=pow(saturate(.5+.5*sin(radius*2.4+x*.003)),10);
float broad=sin(radius*.39+x*.006)*.035;
return Tint.rgb*Vertex.rgb*(1+Amount*(ring*.23-fine*.24)+broad);
"""
    shade = custom(mat, code, {"UV":uv,"Depth":uv1,"Vertex":vertex,"Tint":tint_node,"Amount":amount},unreal.CustomMaterialOutputType.CMOT_FLOAT3,"跨面连续的三维木纹")
    output(mat, shade, unreal.MaterialProperty.MP_BASE_COLOR)
    rough = expression(mat, unreal.MaterialExpressionScalarParameter, parameter_name="粗糙度", default_value=.74, desc="维护良好的哑光木材")
    output(mat, rough, unreal.MaterialProperty.MP_ROUGHNESS)
    spec = expression(mat, unreal.MaterialExpressionConstant, r=.25, desc="柔和木材反光")
    output(mat, spec, unreal.MaterialProperty.MP_SPECULAR)
    unreal.MaterialEditingLibrary.layout_material_expressions(mat)
    unreal.MaterialEditingLibrary.recompile_material(mat)
    unreal.EditorAssetLibrary.save_loaded_asset(mat, only_if_is_dirty=False)
    return mat


def build_assets():
    """创建中文材质与三个可直接拖入关卡的蓝图预设。"""
    for folder in ("材质","预设","演示","静态网格"):
        unreal.EditorAssetLibrary.make_directory(BASE+"/"+folder)
    plank = make_material("浅暖木板",(.48,.295,.15),.45)
    beam = make_material("深棕木梁",(.105,.06,.034),.34)
    result = []
    for name, ceiling, border in (("自动木地板",False,True),("自动木天花",True,True),("无包边木板",False,False)):
        path = BASE+"/预设/"+name
        bp = unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else None
        if not bp:
            factory = unreal.BlueprintFactory()
            factory.set_editor_property("parent_class",unreal.LxTimberPanelActor)
            bp = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name,BASE+"/预设",unreal.Blueprint,factory)
        default = unreal.get_default_object(bp.generated_class())
        for prop, value in {"plank_material":plank,"beam_material":beam,"ceiling":ceiling,"border":border,
                            "static_mesh_asset_path":BASE+"/静态网格/"+name}.items():
            default.set_editor_property(prop,value)
        bp.set_editor_property("run_construction_script_on_drag",False)
        unreal.BlueprintEditorLibrary.compile_blueprint(bp)
        unreal.EditorAssetLibrary.save_loaded_asset(bp,only_if_is_dirty=False)
        result.append(bp.get_path_name())
    unreal.AutomationLibrary.finish_loading_before_screenshot()
    return result


def verify_assets(save=False):
    """验证实际蓝图只生成一个网格和一个碰撞组件，可选保存独立网格资产。"""
    actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    meshes=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    result=[]
    for name in ("自动木地板","自动木天花","无包边木板"):
        bp=unreal.load_asset(BASE+"/预设/"+name)
        actor=actors.spawn_actor_from_class(bp.generated_class(),unreal.Vector(0,0,-5000))
        try:
            actor.rebuild_panel()
            mesh=actor.get_editor_property("panel_mesh").get_editor_property("static_mesh")
            if not mesh or actor.get_editor_property("generation_diagnostic"):
                raise RuntimeError("木板生成失败："+name)
            if len(actor.get_components_by_class(unreal.StaticMeshComponent))!=1 or len(actor.get_components_by_class(unreal.BoxComponent))!=1:
                raise RuntimeError("未满足单网格单碰撞要求")
            entry={"preset":name,"planks":actor.get_editor_property("generated_plank_count"),
                   "beams":actor.get_editor_property("generated_beam_count"),"meshes":1,"collision_boxes":1,
                   "triangles":mesh.get_num_triangles(0)}
            if save:
                actor.save_static_mesh_asset()
                mesh=actor.get_editor_property("saved_static_mesh")
                if not mesh or meshes.get_simple_collision_count(mesh)!=1:
                    raise RuntimeError("保存资产缺少唯一碰撞盒")
                if mesh.get_num_triangles(0)!=entry["triangles"]:
                    raise RuntimeError("保存改变了几何")
                entry["asset"]=mesh.get_path_name()
            result.append(entry)
        finally:
            actors.destroy_actor(actor)
    (OUT/("静态资产验收.json" if save else "预设验收.json")).write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding="utf-8")
    return result


def reload_assets():
    """新编辑器进程验证已保存网格的几何、碰撞和木纹坐标通道。"""
    entries=json.loads((OUT/"静态资产验收.json").read_text(encoding="utf-8"))
    meshes=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    for entry in entries:
        mesh=unreal.load_asset(entry["asset"])
        if not mesh or mesh.get_num_triangles(0)!=entry["triangles"] or meshes.get_simple_collision_count(mesh)!=1:
            raise RuntimeError("重载资产的几何或碰撞错误")
        if mesh.get_num_tex_coords(0)<4:
            raise RuntimeError("木纹或光照纹理坐标丢失")
        entry["reloaded"]=True
    (OUT/"重载验收.json").write_text(json.dumps(entries,ensure_ascii=False,indent=2),encoding="utf-8")
    return entries


def make_demo():
    """创建独立的地板、无包边拼板与悬空天花板演示关卡。"""
    world=unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
    actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    for name, preset, location, size in (("木地板","自动木地板",(0,0,0),(600,400,0)),
        ("换向无边拼板","无包边木板",(0,510,0),(520,320,0)),
        ("木天花","自动木天花",(820,0,320),(600,400,0))):
        bp=unreal.load_asset(BASE+"/预设/"+preset)
        actor=actors.spawn_actor_from_class(bp.generated_class(),unreal.Vector(*location))
        actor.set_actor_label(name)
        actor.set_editor_property("size_handle",unreal.Vector(*size))
        if preset=="无包边木板": actor.set_editor_property("along_y",True)
        actor.rebuild_panel()
    floor=actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(300,350,-38))
    floor.set_actor_label("浅灰展示地台")
    floor.static_mesh_component.set_static_mesh(unreal.load_asset("/Engine/BasicShapes/Cube"))
    floor.static_mesh_component.set_material(0,unreal.load_asset("/Engine/BasicShapes/BasicShapeMaterial"))
    floor.set_actor_scale3d(unreal.Vector(13,15,.3))
    for name, rotation, intensity, shadow in (("温暖日光",(-50,-40,0),8,True),("清透补光",(50,130,0),6,False)):
        light=actors.spawn_actor_from_class(unreal.DirectionalLight,unreal.Vector(0,0,1500),unreal.Rotator(pitch=rotation[0],yaw=rotation[1],roll=rotation[2]))
        light.set_actor_label(name)
        comp=light.get_component_by_class(unreal.DirectionalLightComponent)
        comp.set_mobility(unreal.ComponentMobility.MOVABLE)
        comp.set_editor_property("intensity",intensity)
        comp.set_editor_property("cast_shadows",shadow)
        if shadow: comp.set_editor_property("atmosphere_sun_light",True)
    sky=actors.spawn_actor_from_class(unreal.SkyLight,unreal.Vector(0,0,1000))
    sky.set_actor_label("天空环境光")
    sky.get_component_by_class(unreal.SkyLightComponent).set_mobility(unreal.ComponentMobility.MOVABLE)
    sky.get_component_by_class(unreal.SkyLightComponent).set_editor_property("real_time_capture",True)
    actors.spawn_actor_from_class(unreal.SkyAtmosphere,unreal.Vector()).set_actor_label("晴朗天空")
    unreal.EditorLoadingAndSavingUtils.save_map(world,BASE+"/演示/木板演示")
    return {"map":BASE+"/演示/木板演示"}


def capture_preview():
    """离屏捕获地板正面与天花板背面，检查收边、木纹和凹凸。"""
    levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    levels.load_level(BASE+"/演示/木板演示")
    levels.editor_set_viewport_realtime(True)
    actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    camera=actors.spawn_actor_from_class(unreal.SceneCapture2D,unreal.Vector(820,-600,750))
    camera.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(camera.get_actor_location(),unreal.Vector(300,210,-5)),False)
    component=camera.get_component_by_class(unreal.SceneCaptureComponent2D)
    target=unreal.RenderingLibrary.create_render_target2d(world,1600,1000,unreal.TextureRenderTargetFormat.RTF_RGBA8)
    component.set_editor_property("texture_target",target)
    component.set_editor_property("capture_source",unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR)
    component.set_editor_property("capture_every_frame",False)
    component.set_editor_property("always_persist_rendering_state",True)
    component.set_editor_property("fov_angle",52)
    pp=component.get_editor_property("post_process_settings")
    for key,value in {"override_auto_exposure_min_brightness":True,"override_auto_exposure_max_brightness":True,
                      "auto_exposure_min_brightness":1.0,"auto_exposure_max_brightness":1.0}.items():
        pp.set_editor_property(key,value)
    component.set_editor_property("post_process_settings",pp)
    unreal.AutomationLibrary.finish_loading_before_screenshot()
    STATE.update(world=world,camera=camera,component=component,target=target,phase=0,next=time.monotonic()+8)
    STATE["handle"]=unreal.register_slate_post_tick_callback(capture_tick)
    return {"capture_started":True}


def capture_tick(delta):
    """逐帧等待渲染收敛，保存两种视角后清理临时摄影机。"""
    try:
        STATE["component"].capture_scene()
        if time.monotonic()<STATE["next"]: return
        name=("木地板.png","木天花.png")[STATE["phase"]]
        unreal.RenderingLibrary.export_render_target(STATE["world"],STATE["target"],str(OUT),name)
        if STATE["phase"]==0:
            location=unreal.Vector(1840,-450,-260)
            look=unreal.Vector(1120,200,300)
            STATE["camera"].set_actor_location(location,False,False)
            STATE["camera"].set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(location,look),False)
            STATE["phase"]=1
            STATE["next"]=time.monotonic()+8
        else:
            unreal.unregister_slate_post_tick_callback(STATE["handle"])
            unreal.get_editor_subsystem(unreal.EditorActorSubsystem).destroy_actor(STATE["camera"])
            (OUT/"预览完成.json").write_text('{"captures":2}',encoding="utf-8")
    except Exception:
        unreal.unregister_slate_post_tick_callback(STATE["handle"])
        (OUT/"预览错误.txt").write_text(traceback.format_exc(),encoding="utf-8")


@unreal.uclass()
class TimberPanelTools(unreal.ToolsetDefinition):
    """木质拼板的官方 MCP 资产制作和验证入口。"""
    @unreal.ufunction(static=True,params=[str],ret=str,meta={"AICallable":"","DisplayName":"处理木板资产","Category":"木板|制作"})
    def process_panel(stage):
        """按阶段创建、验证、演示、截图或运行自动化测试。"""
        try:
            if stage=="build": result=build_assets()
            elif stage=="verify": result=verify_assets()
            elif stage=="static": result=verify_assets(True)
            elif stage=="reload": result=reload_assets()
            elif stage=="demo": result=make_demo()
            elif stage=="capture": result=capture_preview()
            elif stage=="tests":
                unreal.SystemLibrary.execute_console_command(None,"Automation RunTests LxARPG.World.TimberPanel")
                result={"tests_started":True}
            elif stage=="quit":
                unreal.SystemLibrary.quit_editor()
                result={"quit":True}
            else: raise ValueError(stage)
            return json.dumps(result,ensure_ascii=False)
        except Exception:
            return json.dumps({"error":traceback.format_exc()},ensure_ascii=False)


unreal.EditorPythonScripting.set_keep_python_script_alive(True)
unreal.ToolsetRegistry.register_toolset_class(TimberPanelTools)
unreal.SystemLibrary.execute_console_command(None,"ModelContextProtocol.StartServer 8007")
unreal.SystemLibrary.execute_console_command(None,"ModelContextProtocol.RefreshTools")
