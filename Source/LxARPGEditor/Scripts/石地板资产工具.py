"""通过 UE 5.8 官方 MCP 制作大块方形石地板与浅灰粗岩材质。"""
import json
import time
import traceback
from pathlib import Path
import unreal

# 专用内容目录和验证输出，不改用户当前制作关卡。
BASE = "/Game/项目内容/美术资产/海港城市/石地板"
OUT = Path(unreal.Paths.project_dir()).resolve()/"Saved/StoneFloor"
OUT.mkdir(parents=True, exist_ok=True)
# 分帧离屏渲染的持久状态。
STATE = globals().get("STATE", {})

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



def make_material():
    """用三维矿物颗粒、粗糙度与微表面法线表现干净的粗岩。"""
    path=BASE+"/材质/浅灰粗岩"
    mat=unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else None
    if not mat:
        mat=unreal.AssetToolsHelpers.get_asset_tools().create_asset("浅灰粗岩",BASE+"/材质",unreal.Material,unreal.MaterialFactoryNew())
    unreal.MaterialEditingLibrary.delete_all_material_expressions(mat)
    mat.set_editor_property("tangent_space_normal",False)
    uv=expression(mat,unreal.MaterialExpressionTextureCoordinate,coordinate_index=0,desc="整块石料局部平面坐标")
    depth=expression(mat,unreal.MaterialExpressionTextureCoordinate,coordinate_index=1,desc="厚度和稳定石纹相位")
    vertex=expression(mat,unreal.MaterialExpressionVertexColor,desc="整块石板色差及接缝衬底明度")
    tint=expression(mat,unreal.MaterialExpressionVectorParameter,parameter_name="岩石颜色",default_value=unreal.LinearColor(.56,.58,.56,1),desc="预设风格的清爽浅灰石材")
    grain=expression(mat,unreal.MaterialExpressionScalarParameter,parameter_name="岩石颗粒强度",default_value=.85,desc="低对比矿物颗粒和浅层断口纹理")
    code="""
// 连续三维值噪声，用于概括矿物颗粒。
struct RockNoise {
 // 为三维网格角点生成稳定随机值。
 float hash(float3 p) { p=frac(p*.1031);p+=dot(p,p.yzx+33.33);return frac((p.x+p.y)*p.z); }
 // 平滑插值相邻八个角点，避免面与倒角处接缝。
 float noise(float3 p) {
  float3 i=floor(p),f=frac(p);f=f*f*(3-2*f);
  return lerp(lerp(lerp(hash(i),hash(i+float3(1,0,0)),f.x),lerp(hash(i+float3(0,1,0)),hash(i+float3(1,1,0)),f.x),f.y),
              lerp(lerp(hash(i+float3(0,0,1)),hash(i+float3(1,0,1)),f.x),lerp(hash(i+float3(0,1,1)),hash(i+float3(1,1,1)),f.x),f.y),f.z);
 }
};
RockNoise r;
float3 p=float3(UV.xy,Depth.x)*100+float3(0,Depth.y*7,Depth.y*3);
float broad=r.noise(p*.025),chip=r.noise(p*.16),fine=r.noise(p*1.7);
float vein=abs(r.noise(p*.065+chip*.7)-.5);
float fleck=smoothstep(.63,.85,fine);
float shade=1+Amount*((broad-.5)*.19+(chip-.5)*.17+(fine-.5)*.07-fleck*.09-(1-smoothstep(.012,.036,vein))*.025);
return float4(Tint.rgb*Vertex.rgb*shade,chip*.36+fine*.09+broad*.16);
"""
    surface=custom(mat,code,{"UV":uv,"Depth":depth,"Vertex":vertex,"Tint":tint,"Amount":grain},unreal.CustomMaterialOutputType.CMOT_FLOAT4,"粗岩的概括块面与细矿物颗粒")
    rgb=expression(mat,unreal.MaterialExpressionComponentMask,r=True,g=True,b=True,a=False,desc="浅灰岩石底色")
    if not unreal.MaterialEditingLibrary.connect_material_expressions(surface,"",rgb,""):
        raise RuntimeError("粗岩底色通道连接失败")
    output(mat,rgb,unreal.MaterialProperty.MP_BASE_COLOR)
    position=expression(mat,unreal.MaterialExpressionWorldPosition,desc="用于微表面凹凸的世界位置")
    normal=expression(mat,unreal.MaterialExpressionVertexNormalWS,desc="石板和倒角的原始朝外法线")
    bump=expression(mat,unreal.MaterialExpressionScalarParameter,parameter_name="岩面凹凸强度",default_value=2.0,desc="细凿岩面的浅凹凸，避免夸张破损")
    normal_code="""
float3 n=normalize(Normal),dx=ddx(Position),dy=ddy(Position);
float3 rx=cross(dy,n),ry=cross(n,dx);
float det=dot(dx,rx);
float3 grad=sign(det)*(ddx(Surface.a)*rx+ddy(Surface.a)*ry);
return normalize(max(abs(det),.000001)*n-Strength*grad);
"""
    perturbed=custom(mat,normal_code,{"Surface":surface,"Position":position,"Normal":normal,"Strength":bump},unreal.CustomMaterialOutputType.CMOT_FLOAT3,"跨面连续的粗岩微表面凹凸")
    output(mat,perturbed,unreal.MaterialProperty.MP_NORMAL)
    rough=expression(mat,unreal.MaterialExpressionScalarParameter,parameter_name="岩石粗糙度",default_value=.92,desc="粗岩哑光表面")
    output(mat,rough,unreal.MaterialProperty.MP_ROUGHNESS)
    spec=expression(mat,unreal.MaterialExpressionConstant,r=.18,desc="克制的矿物反光")
    output(mat,spec,unreal.MaterialProperty.MP_SPECULAR)
    unreal.MaterialEditingLibrary.layout_material_expressions(mat)
    unreal.MaterialEditingLibrary.recompile_material(mat)
    unreal.EditorAssetLibrary.save_loaded_asset(mat,only_if_is_dirty=False)
    return mat


# 名称、总边长、厚度和单板目标边长，单位均为厘米。
PRESETS=(("自动石地板",800,24,200),("巨石地板",1200,36,300),("整块石板",400,40,600))


def build_assets():
    """创建粗岩石材和三种可直接拖入关卡的中文蓝图预设。"""
    for folder in ("材质","预设","演示","静态网格"):
        unreal.EditorAssetLibrary.make_directory(BASE+"/"+folder)
    stone=make_material()
    result=[]
    for name,side,thickness,tile in PRESETS:
        path=BASE+"/预设/"+name
        bp=unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else None
        if not bp:
            factory=unreal.BlueprintFactory()
            factory.set_editor_property("parent_class",unreal.LxStoneFloorActor)
            bp=unreal.AssetToolsHelpers.get_asset_tools().create_asset(name,BASE+"/预设",unreal.Blueprint,factory)
        default=unreal.get_default_object(bp.generated_class())
        for prop,value in {"stone_material":stone,"size_handle":unreal.Vector(side,0,0),"thickness":thickness,"tile_size":tile,
                           "static_mesh_asset_path":BASE+"/静态网格/"+name}.items():
            default.set_editor_property(prop,value)
        bp.set_editor_property("run_construction_script_on_drag",False)
        unreal.BlueprintEditorLibrary.compile_blueprint(bp)
        unreal.EditorAssetLibrary.save_loaded_asset(bp,only_if_is_dirty=False)
        result.append(bp.get_path_name())
    unreal.AutomationLibrary.finish_loading_before_screenshot()
    return result


def verify_assets(save=False):
    """检查单网格、单材质、单碰撞及地板和接缝上的实际射线。"""
    actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    meshes=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    result=[]
    for name,side,thickness,tile in PRESETS:
        bp=unreal.load_asset(BASE+"/预设/"+name)
        actor=actors.spawn_actor_from_class(bp.generated_class(),unreal.Vector(0,0,-5000))
        try:
            actor.rebuild_floor()
            mesh=actor.get_editor_property("panel_mesh").get_editor_property("static_mesh")
            if not mesh or actor.get_editor_property("generation_diagnostic"):
                raise RuntimeError("生成石地板失败："+name)
            if len(actor.get_components_by_class(unreal.StaticMeshComponent))!=1 or len(actor.get_components_by_class(unreal.BoxComponent))!=1:
                raise RuntimeError("未保持单网格单碰撞")
            if mesh.get_num_sections(0)!=1 or len(mesh.get_editor_property("static_materials"))!=1:
                raise RuntimeError("地板存在多余材质分区")
            for x,y in ((side/2,side/2),(side/4,side/4)):
                hit=unreal.SystemLibrary.line_trace_single(world,unreal.Vector(x,y,-4900),unreal.Vector(x,y,-5200),
                    unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,False,[],unreal.DrawDebugTrace.NONE,True)
                if not hit: raise RuntimeError("石地板或板缝上方的射线未命中")
            miss=unreal.SystemLibrary.line_trace_single(world,unreal.Vector(side+10,side/2,-4900),unreal.Vector(side+10,side/2,-5200),
                unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,False,[],unreal.DrawDebugTrace.NONE,True)
            if miss: raise RuntimeError("碰撞超出铺设范围")
            entry={"preset":name,"side_cm":side,"thickness_cm":thickness,"tiles":actor.get_editor_property("generated_tile_count"),
                   "tile_side_cm":actor.get_editor_property("generated_tile_size"),"meshes":1,"materials":1,"collision_boxes":1,
                   "collision_trace":True,"triangles":mesh.get_num_triangles(0)}
            if save:
                actor.save_static_mesh_asset()
                mesh=actor.get_editor_property("saved_static_mesh")
                if not mesh or meshes.get_simple_collision_count(mesh)!=1 or mesh.get_num_triangles(0)!=entry["triangles"] or mesh.get_num_sections(0)!=1:
                    raise RuntimeError("保存后的网格、碰撞或材质分区错误")
                entry["asset"]=mesh.get_path_name()
            result.append(entry)
        finally:
            actors.destroy_actor(actor)
    (OUT/("静态资产验收.json" if save else "预设验收.json")).write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding="utf-8")
    return result


def reload_assets():
    """在新编辑器中重新读取导出资产，检查纹理坐标、单碰撞和阻挡。"""
    entries=json.loads((OUT/"静态资产验收.json").read_text(encoding="utf-8"))
    meshes=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    for entry in entries:
        mesh=unreal.load_asset(entry["asset"])
        if not mesh or mesh.get_num_triangles(0)!=entry["triangles"] or meshes.get_simple_collision_count(mesh)!=1 or mesh.get_num_sections(0)!=1:
            raise RuntimeError("重载后的静态网格、单材质或碰撞错误")
        if mesh.get_num_tex_coords(0)<4:
            raise RuntimeError("连续岩石纹理或光照坐标丢失")
        actor=actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(0,0,-5000))
        try:
            actor.static_mesh_component.set_static_mesh(mesh)
            actor.static_mesh_component.set_collision_profile_name("BlockAll")
            side=entry["side_cm"]
            hit=unreal.SystemLibrary.line_trace_single(world,unreal.Vector(side/2,side/2,-4900),unreal.Vector(side/2,side/2,-5200),
                unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,False,[],unreal.DrawDebugTrace.NONE,True)
            if not hit: raise RuntimeError("重载资产的实际碰撞未命中")
        finally:
            actors.destroy_actor(actor)
        entry["reloaded"]=True
    (OUT/"重载验收.json").write_text(json.dumps(entries,ensure_ascii=False,indent=2),encoding="utf-8")
    return entries

def make_demo():
    """独立展示大块方形石板及其完整侧面厚度。"""
    world=unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
    actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    bp=unreal.load_asset(BASE+"/预设/自动石地板")
    actor=actors.spawn_actor_from_class(bp.generated_class(),unreal.Vector())
    actor.set_actor_label("大块正方形石地板")
    actor.rebuild_floor()
    for name, rotation, intensity, shadow in (("温暖日光",(-50,-40,0),8,True),("清透补光",(-25,130,0),3,False)):
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
    # 仅用于演示的浅蓝无光照背景，离屏捕获也保持明亮背景。
    backdrop_path=BASE+"/演示/晴蓝背景"
    backdrop=unreal.load_asset(backdrop_path) if unreal.EditorAssetLibrary.does_asset_exist(backdrop_path) else None
    if not backdrop:
        backdrop=unreal.AssetToolsHelpers.get_asset_tools().create_asset("晴蓝背景",BASE+"/演示",unreal.Material,unreal.MaterialFactoryNew())
        backdrop.set_editor_property("two_sided",True)
        backdrop.set_editor_property("shading_model",unreal.MaterialShadingModel.MSM_UNLIT)
        color=expression(backdrop,unreal.MaterialExpressionConstant3Vector,constant=unreal.LinearColor(.24,.36,.49,1),desc="清爽蓝灰演示背景")
        output(backdrop,color,unreal.MaterialProperty.MP_EMISSIVE_COLOR)
        unreal.MaterialEditingLibrary.recompile_material(backdrop)
        unreal.EditorAssetLibrary.save_loaded_asset(backdrop,only_if_is_dirty=False)
    dome=actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector())
    dome.set_actor_label("晴蓝展示背景")
    dome.static_mesh_component.set_static_mesh(unreal.load_asset("/Engine/BasicShapes/Sphere"))
    dome.static_mesh_component.set_material(0,backdrop)
    dome.static_mesh_component.set_editor_property("cast_shadow",False)
    dome.static_mesh_component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    dome.set_actor_scale3d(unreal.Vector(200,200,200))
    unreal.EditorLoadingAndSavingUtils.save_map(world,BASE+"/演示/石地板演示")
    return {"map":BASE+"/演示/石地板演示"}


def capture_preview():
    """离屏捕获铺装全貌及近景，检查粗岩凹凸、接缝和厚度。"""
    levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    levels.load_level(BASE+"/演示/石地板演示")
    levels.editor_set_viewport_realtime(True)
    actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    camera=actors.spawn_actor_from_class(unreal.SceneCapture2D,unreal.Vector(1160,-940,900))
    camera.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(camera.get_actor_location(),unreal.Vector(400,400,-8)),False)
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
        name=("石地板总览.png","石地板细节.png")[STATE["phase"]]
        unreal.RenderingLibrary.export_render_target(STATE["world"],STATE["target"],str(OUT),name)
        if STATE["phase"]==0:
            location=unreal.Vector(610,-200,310)
            look=unreal.Vector(315,180,-10)
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
class StoneFloorTools(unreal.ToolsetDefinition):
    """自动石地板的官方 MCP 资产制作和验证入口。"""
    @unreal.ufunction(static=True,params=[str],ret=str,meta={"AICallable":"","DisplayName":"处理石地板资产","Category":"石地板|制作"})
    def process_floor(stage):
        """按阶段创建、验证、演示、截图或运行自动化测试。"""
        try:
            if stage=="refresh":
                # 仅刷新本工具文件的函数定义，方便迭代材质与验收逻辑。
                source_path=Path(unreal.Paths.project_dir()).resolve()/"Source/LxARPGEditor/Scripts/石地板资产工具.py"
                source=source_path.read_text(encoding="utf-8")
                exec(compile(source.split("@unreal.uclass()")[0],str(source_path),"exec"),globals())
                result={"refreshed":True}
            elif stage=="build": result=build_assets()
            elif stage=="verify": result=verify_assets()
            elif stage=="static": result=verify_assets(True)
            elif stage=="reload": result=reload_assets()
            elif stage=="demo": result=make_demo()
            elif stage=="capture": result=capture_preview()
            elif stage=="tests":
                unreal.SystemLibrary.execute_console_command(None,"Automation RunTests LxARPG.World.StoneFloor")
                result={"tests_started":True}
            elif stage=="quit":
                unreal.SystemLibrary.quit_editor()
                result={"quit":True}
            else: raise ValueError(stage)
            return json.dumps(result,ensure_ascii=False)
        except Exception:
            return json.dumps({"error":traceback.format_exc()},ensure_ascii=False)


unreal.EditorPythonScripting.set_keep_python_script_alive(True)
unreal.ToolsetRegistry.register_toolset_class(StoneFloorTools)
unreal.SystemLibrary.execute_console_command(None,"ModelContextProtocol.StartServer 8009")
unreal.SystemLibrary.execute_console_command(None,"ModelContextProtocol.RefreshTools")
