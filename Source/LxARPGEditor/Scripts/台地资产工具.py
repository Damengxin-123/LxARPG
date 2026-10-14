"""通过 UE 5.8 官方 MCP 制作、验收和预览石质台地。"""
import json
import time
import traceback
from pathlib import Path
import unreal

# 独立资产和结果目录。
BASE = "/Game/项目内容/美术资产/海港城市/石质台地"
OUT = Path(unreal.Paths.project_dir()).resolve()/"Saved/StoneTerrace"
OUT.mkdir(parents=True, exist_ok=True)
# 离屏截图的跨帧状态。
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
        source, channel = source if isinstance(source,tuple) else (source, "")
        if not unreal.MaterialEditingLibrary.connect_material_expressions(source, channel, node, name):
            raise RuntimeError("材质连接失败：" + name)
    return node


def output(mat, node, prop):
    """连接材质属性并检查连接有效性。"""
    if not unreal.MaterialEditingLibrary.connect_material_property(node, "", prop):
        raise RuntimeError("材质输出失败：" + str(prop))



def material(name, tint):
    """生成低对比矿物石纹、细凿凹凸、哑光和侧面错缝砌石。"""
    path = BASE+"/材质/"+name
    mat = unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else None
    if not mat:
        mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, BASE+"/材质", unreal.Material, unreal.MaterialFactoryNew())
    unreal.MaterialEditingLibrary.delete_all_material_expressions(mat)
    mat.set_editor_property("tangent_space_normal",False)
    uv=expression(mat,unreal.MaterialExpressionTextureCoordinate,coordinate_index=0,desc="以米平铺的局部石纹，移动台地时不会滑动")
    vertex=expression(mat,unreal.MaterialExpressionVertexColor,desc="每块石砖的浅冷色差")
    brick_uv=expression(mat,unreal.MaterialExpressionTextureCoordinate,coordinate_index=1,desc="与实际尺寸一致的错缝铺砖坐标")
    joints=expression(mat,unreal.MaterialExpressionTextureCoordinate,coordinate_index=2,desc="真实砖缝比例，轻量预览与完整几何保持一致")
    normal=expression(mat,unreal.MaterialExpressionVertexNormalWS,desc="区分顶面和台身砌石侧面")
    tint_node=expression(mat,unreal.MaterialExpressionVectorParameter,parameter_name="石材底色",default_value=unreal.LinearColor(*tint,1),desc="清爽灰白石材预设")
    amount=expression(mat,unreal.MaterialExpressionScalarParameter,parameter_name="石纹强度",default_value=.85,desc="概括石纹的深浅")
    masonry=expression(mat,unreal.MaterialExpressionScalarParameter,parameter_name="侧面砌石",default_value=1.0 if name=="灰白边石" else 0.0,desc="边石和台身侧面的规整错缝")
    code=r"""
// 平滑矿物颗粒，不使用规则正弦波伪装石纹。
struct StonePattern {
 // 稳定的二维格点色差。
 float hash(float2 p) { return frac(sin(dot(p,float2(127.1,311.7)))*43758.5453); }
 // 双线性平滑噪声，三种尺度分别用于石色、矿脉和细颗粒。
 float noise(float2 p) {
  float2 i=floor(p),f=frac(p);f=f*f*(3-2*f);
  return lerp(lerp(hash(i),hash(i+float2(1,0)),f.x),lerp(hash(i+float2(0,1)),hash(i+1),f.x),f.y);
 }
};
StonePattern r;
float2 p=UV.xy;
float cloud=r.noise(p*2.3),chip=r.noise(p*17.0+cloud*1.8),fine=r.noise(p*145.0);
float warped=r.noise(p*float2(5.0,10.0)+float2(chip,cloud)*1.3);
float vein=1-smoothstep(.014,.045,abs(warped-.48));
float fleck=smoothstep(.71,.91,fine);
float shade=1+Amount*((cloud-.5)*.24+(chip-.5)*.15+(fine-.5)*.06-vein*.065-fleck*.06);
float3 hue=lerp(float3(.94,.98,1.02),float3(1.035,1.015,.975),cloud);
// 仅立面出现大块错缝，台面保留生成器实际砖块的尺寸和接缝。
float row=floor(p.y/.5);
float2 cell=float2(p.x/1.5+fmod(abs(row),2)*.5,p.y/.5);
float2 edge=min(frac(cell),1-frac(cell))*float2(1.5,.5);
float seam=1-smoothstep(.005,.013,min(edge.x,edge.y));
seam*=Masonry*(1-smoothstep(.35,.75,abs(Normal.z)));
shade*=lerp(1,.75,seam);
float height=(cloud*.09+chip*.055+fine*.012-vein*.012)*Amount-seam*.10;
float3 color=Tint.rgb*Vertex.rgb*hue*shade;
if(Joints.x>0.0000001) {
 // 单片预览也显示逐砖颜色，完整版使用相同规则生成的顶点色。
 float brickRow=floor(Brick.y);
 float2 cell=float2(Brick.x+fmod(brickRow,2)*.5,Brick.y);
 float tone=fmod(brickRow*17+floor(cell.x)*13,11);
 float3 brickHue=lerp(float3(.96,.98,1.02),float3(1.04,1.02,.98),frac(tone*.381966));
 float3 brickColor=(.80+tone*.024)*brickHue;
 color=Tint.rgb*hue*shade*(BrickMask<.5?brickColor:Vertex.rgb);
 float2 distance=min(frac(cell),1-frac(cell));
 float2 aa=max(fwidth(cell)*.6,float2(.00005,.00005));
 float2 groutLine=1-smoothstep(Joints.xy-aa,Joints.xy+aa,distance);
 float grout=max(groutLine.x,groutLine.y);
 color=lerp(color,Tint.rgb*float3(.48,.53,.58),grout);
 height-=grout*.12;
}
return float4(color,height);
"""
    surface=custom(mat,code,{"UV":uv,"Vertex":vertex,"Normal":normal,"Tint":tint_node,"Amount":amount,"Masonry":masonry,"Brick":brick_uv,"Joints":joints,"BrickMask":(vertex,"A")},unreal.CustomMaterialOutputType.CMOT_FLOAT4,"柔和矿脉、细凿颗粒与侧面石缝")
    rgb=expression(mat,unreal.MaterialExpressionComponentMask,r=True,g=True,b=True,a=False,desc="矿物色差和手绘概括块面")
    unreal.MaterialEditingLibrary.connect_material_expressions(surface,"",rgb,"")
    output(mat,rgb,unreal.MaterialProperty.MP_BASE_COLOR)
    position=expression(mat,unreal.MaterialExpressionWorldPosition,desc="仅用于凹凸求导，不参与石纹平铺")
    bump=expression(mat,unreal.MaterialExpressionScalarParameter,parameter_name="浅凿凹凸",default_value=1.3 if name=="灰白边石" else .75,desc="干净石面的小幅起伏")
    bump_code=r"""
float3 n=normalize(Normal),dx=ddx(Position),dy=ddy(Position);
float3 rx=cross(dy,n),ry=cross(n,dx);
float det=dot(dx,rx);
float3 grad=sign(det)*(ddx(Surface.a)*rx+ddy(Surface.a)*ry);
return normalize(max(abs(det),.000001)*n-Strength*grad);
"""
    perturbed=custom(mat,bump_code,{"Surface":surface,"Position":position,"Normal":normal,"Strength":bump},unreal.CustomMaterialOutputType.CMOT_FLOAT3,"石纹和砖缝的克制凹凸")
    output(mat,perturbed,unreal.MaterialProperty.MP_NORMAL)
    rough=custom(mat,"return clamp(.84+(Surface.a-.08)*.35,.72,.94);",{"Surface":surface},unreal.CustomMaterialOutputType.CMOT_FLOAT1,"矿物颗粒的哑光变化")
    output(mat,rough,unreal.MaterialProperty.MP_ROUGHNESS)
    spec=expression(mat,unreal.MaterialExpressionConstant,r=.18,desc="低强度石材反光")
    output(mat,spec,unreal.MaterialProperty.MP_SPECULAR)
    unreal.MaterialEditingLibrary.layout_material_expressions(mat)
    unreal.MaterialEditingLibrary.recompile_material(mat)
    unreal.EditorAssetLibrary.save_loaded_asset(mat,only_if_is_dirty=False)
    return mat


def build_assets():
    """创建矩形、圆角和高台三种简短中文预设。"""
    unreal.EditorAssetLibrary.make_directory(BASE+"/预设")
    unreal.EditorAssetLibrary.make_directory(BASE+"/材质")
    paving = material("浅灰石砖",(.59,.62,.64))
    stone = material("灰白边石",(.68,.70,.71))
    result = []
    for name, height, radius in (("自动台地",100,0),("圆角台地",100,180),("石质高台",300,80)):
        path = BASE+"/预设/"+name
        bp = unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else None
        if not bp:
            factory = unreal.BlueprintFactory()
            factory.set_editor_property("parent_class",unreal.LxStoneTerraceActor)
            bp = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name,BASE+"/预设",unreal.Blueprint,factory)
        default = unreal.get_default_object(bp.generated_class())
        for key,value in {"terrace_height":height,"corner_radius":radius,"paving_material":paving,"stone_material":stone}.items():
            default.set_editor_property(key,value)
        bp.set_editor_property("run_construction_script_on_drag",False)
        unreal.BlueprintEditorLibrary.compile_blueprint(bp)
        unreal.EditorAssetLibrary.save_loaded_asset(bp,only_if_is_dirty=False)
        result.append(bp.get_path_name())
    (OUT/"资产结果.json").write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding="utf-8")
    return result


def demo():
    """仅在独立演示关卡摆放台地、拼接台地和道路，不加载用户制作关卡。"""
    # 跟随用户整理目录后留下的重定向器，不重建或覆盖已移动的预设。
    round_preset=unreal.load_asset(BASE+"/预设/圆角台地")
    rectangle_preset=unreal.load_asset(BASE+"/预设/自动台地")
    cls=round_preset.generated_class() if isinstance(round_preset,unreal.Blueprint) else None
    rectangle=rectangle_preset.generated_class() if isinstance(rectangle_preset,unreal.Blueprint) else None
    if not cls or not rectangle:
        raise RuntimeError("演示预设无法加载，未修改关卡")
    subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    destination=BASE+"/演示/台地演示"
    if unreal.EditorAssetLibrary.does_asset_exist(destination):
        if not subsystem.load_level(destination):
            raise RuntimeError("无法加载独立演示关卡")
        cleanup=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
        for actor in cleanup.get_all_level_actors():
            if not isinstance(actor,unreal.WorldSettings):
                cleanup.destroy_actor(actor)
    elif not subsystem.new_level(destination):
        raise RuntimeError("无法创建独立演示关卡")
    world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    if world.get_path_name().split(".")[0]!=destination:
        raise RuntimeError("目标不是台地演示关卡，已停止生成")
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    a = actors.spawn_actor_from_class(cls,unreal.Vector(0,0,0))
    a.set_actor_label("圆角主台地")
    b = actors.spawn_actor_from_class(rectangle,unreal.Vector(1650,100,0))
    b.set_actor_label("拼接台地")
    b.set_footprint_meters(12.00,8.00)
    b.rebuild_terrace()
    r = actors.spawn_actor_from_class(unreal.LxSplineRoadActor,unreal.Vector(0,-700,100),unreal.Rotator(pitch=0,yaw=-90,roll=0))
    r.set_actor_label("接入道路")
    r.set_editor_property("road_width",400)
    r.set_editor_property("road_material",unreal.load_asset(BASE+"/材质/浅灰石砖"))
    r.set_editor_property("curb_material",unreal.load_asset(BASE+"/材质/灰白边石"))
    r.rebuild_road()
    light = actors.spawn_actor_from_class(unreal.DirectionalLight,unreal.Vector(0,0,1500),unreal.Rotator(pitch=-48,yaw=-35,roll=0))
    light.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    light.light_component.set_editor_property("intensity",8.0)
    light.light_component.set_editor_property("atmosphere_sun_light",True)
    fill = actors.spawn_actor_from_class(unreal.DirectionalLight,unreal.Vector(0,0,1000),unreal.Rotator(pitch=-25,yaw=130,roll=0))
    fill.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    fill.light_component.set_editor_property("intensity",3.0)
    fill.light_component.set_editor_property("cast_shadows",False)
    sky = actors.spawn_actor_from_class(unreal.SkyLight,unreal.Vector(0,0,1000))
    sky.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    sky.light_component.set_editor_property("real_time_capture",True)
    sky.light_component.set_editor_property("intensity",.7)
    actors.spawn_actor_from_class(unreal.SkyAtmosphere,unreal.Vector(0,0,0))
    floor = actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(400,-500,-35))
    floor.static_mesh_component.set_static_mesh(unreal.load_asset("/Engine/BasicShapes/Cube"))
    floor.set_actor_scale3d(unreal.Vector(250,250,.5))
    floor.static_mesh_component.set_material(0,unreal.load_asset(BASE+"/材质/浅灰石砖"))
    for actor in (a,b):
        actor.rebuild_terrace()
        actor.get_editor_property("optimized_mesh").finalize_mesh()
    r.get_editor_property("optimized_mesh").finalize_mesh()
    for name in ("浅灰石砖","灰白边石"):
        unreal.EditorAssetLibrary.save_loaded_asset(unreal.load_asset(BASE+"/材质/"+name),only_if_is_dirty=False)
    if not subsystem.save_current_level():
        raise RuntimeError("演示关卡保存失败")
    return {"level":BASE+"/演示/台地演示","main_connections":len(a.get_editor_property("connections")),"second_connections":len(b.get_editor_property("connections"))}


def capture():
    """使用真实引擎离屏渲染输出总览和边石细节。"""
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    position,look = unreal.Vector(3100,-3700,3300),unreal.Vector(500,-250,70)
    camera = actors.spawn_actor_from_class(unreal.SceneCapture2D,position,unreal.MathLibrary.find_look_at_rotation(position,look))
    component = camera.get_component_by_class(unreal.SceneCaptureComponent2D)
    component.set_mobility(unreal.ComponentMobility.MOVABLE)
    component.set_editor_property("capture_every_frame",False)
    component.set_editor_property("always_persist_rendering_state",True)
    target = unreal.RenderingLibrary.create_render_target2d(world,1600,1000,unreal.TextureRenderTargetFormat.RTF_RGBA8)
    component.set_editor_property("fov_angle",55.0)
    component.set_editor_property("texture_target",target)
    component.set_editor_property("capture_source",unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR)
    settings = component.get_editor_property("post_process_settings")
    for key,value in {"override_auto_exposure_method":True,"auto_exposure_method":unreal.AutoExposureMethod.AEM_MANUAL,
                      "override_auto_exposure_bias":True,"auto_exposure_bias":0.0,
                      "override_auto_exposure_apply_physical_camera_exposure":True,"auto_exposure_apply_physical_camera_exposure":False}.items():
        settings.set_editor_property(key,value)
    component.set_editor_property("post_process_settings",settings)
    STATE.update(world=world,camera=camera,component=component,target=target,phase=0,next=time.monotonic()+8)
    STATE["handle"] = unreal.register_slate_post_tick_callback(capture_tick)
    return {"capture_started":True}


def capture_tick(delta):
    """等待渲染收敛后截图，并清理临时摄影机。"""
    try:
        STATE["component"].capture_scene()
        if time.monotonic()<STATE["next"]:
            return
        filename=("台地总览.png","石砖与包边.png")[STATE["phase"]]
        unreal.RenderingLibrary.export_render_target(STATE["world"],STATE["target"],str(OUT),filename)
        if STATE["phase"]==0:
            position,look=unreal.Vector(2100,-950,1100),unreal.Vector(1000,0,100)
            STATE["camera"].set_actor_location(position,False,False)
            STATE["camera"].set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(position,look),False)
            STATE["phase"]=1
            STATE["next"]=time.monotonic()+6
        else:
            unreal.unregister_slate_post_tick_callback(STATE["handle"])
            unreal.get_editor_subsystem(unreal.EditorActorSubsystem).destroy_actor(STATE["camera"])
            (OUT/"预览完成.json").write_text('{"captures":2}',encoding="utf-8")
    except Exception:
        unreal.unregister_slate_post_tick_callback(STATE["handle"])
        (OUT/"预览错误.txt").write_text(traceback.format_exc(),encoding="utf-8")


def verify_saved():
    """重载已保存的演示关卡，验证预设材质、连接记录和渲染缓存。"""
    levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    levels.load_level("/Engine/Maps/Entry")
    levels.load_level(BASE+"/演示/台地演示")
    actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    terraces=[a for a in actors.get_all_level_actors() if isinstance(a,unreal.LxStoneTerraceActor)]
    results=[]
    for actor in terraces:
        mesh=actor.get_editor_property("terrace_mesh")
        optimized=actor.get_editor_property("optimized_mesh")
        if not actor.get_editor_property("paving_material") or not actor.get_editor_property("stone_material"):
            raise RuntimeError("台地预设缺少石材")
        if not optimized.get_editor_property("static_mesh"):
            raise RuntimeError("保存后缺少静态渲染缓存")
        connections=actor.get_editor_property("connections")
        expected=2 if actor.get_actor_label()=="圆角主台地" else 1
        if len(connections)!=expected:
            raise RuntimeError(actor.get_actor_label()+"连接数不正确："+str(len(connections))+"；所有台地尺寸："+str([(t.get_actor_label(),str(t.get_editor_property("size")),t.get_editor_property("length_meters"),t.get_editor_property("width_meters"),str(t.get_editor_property("synchronized_size")),len(t.get_editor_property("connections"))) for t in terraces]))
        results.append({"name":actor.get_actor_label(),"connections":len(connections),"render_cache":True,"materials":True})
    if len(terraces)!=2:
        raise RuntimeError("演示关卡台地数量不正确")
    (OUT/"重载验证.json").write_text(json.dumps(results,ensure_ascii=False,indent=2),encoding="utf-8")
    return results

def update_paving_style():
    """仅更新已有预设的砖缝和倒角，不改变用户整理的目录或尺寸。"""
    result=[]
    for name in ("自动台地","圆角台地"):
        bp=unreal.load_asset(BASE+"/预设/"+name)
        if not isinstance(bp,unreal.Blueprint):
            raise RuntimeError("找不到已有台地预设："+name)
        default=unreal.get_default_object(bp.generated_class())
        default.set_editor_property("joint_width",3.0)
        default.set_editor_property("border_bevel_meters",.02)
        unreal.BlueprintEditorLibrary.compile_blueprint(bp)
        unreal.EditorAssetLibrary.save_loaded_asset(bp,only_if_is_dirty=False)
        result.append(bp.get_path_name())
    material("浅灰石砖",(.59,.62,.64))
    material("灰白边石",(.68,.70,.71))
    return result


def inspect_assets():
    """读取预设对象类型及生成类，定位资产加载问题而不改写关卡。"""
    result=[]
    for name in ("自动台地","圆角台地","石质高台"):
        asset=unreal.load_asset(BASE+"/预设/"+name)
        result.append({"name":name,"type":asset.get_class().get_name() if asset else None,
                       "path":asset.get_path_name() if asset else None,
                       "class":asset.generated_class().get_path_name() if isinstance(asset,unreal.Blueprint) else None,
                       "paving":str(unreal.get_default_object(asset.generated_class()).get_editor_property("paving_material")) if isinstance(asset,unreal.Blueprint) else None,
                       "stone":str(unreal.get_default_object(asset.generated_class()).get_editor_property("stone_material")) if isinstance(asset,unreal.Blueprint) else None})
    return result


@unreal.uclass()
class StoneTerraceTools(unreal.ToolsetDefinition):
    """石质台地的官方 MCP 制作入口。"""
    @unreal.ufunction(static=True,params=[str],ret=str,meta={"AICallable":"","DisplayName":"处理石质台地","Category":"石质台地|制作"})
    def process_terrace(stage):
        """按阶段创建资产、演示、离屏渲染或执行测试。"""
        try:
            if stage=="build": result=build_assets()
            elif stage=="style": result=update_paving_style()
            elif stage=="inspect": result=inspect_assets()
            elif stage=="materials": result=[material("浅灰石砖",(.59,.62,.64)).get_path_name(),material("灰白边石",(.68,.70,.71)).get_path_name()]
            elif stage=="demo": result=demo()
            elif stage=="capture": result=capture()
            elif stage=="verify": result=verify_saved()
            elif stage=="tests":
                unreal.SystemLibrary.execute_console_command(None,"Automation RunTests LxARPG.World.StoneTerrace+LxARPG.World.HarborRoad+LxARPG.World.Seawall+LxARPG.World.GeneratedMesh")
                result={"tests_started":True}
            elif stage=="refresh":
                path=Path(unreal.Paths.project_dir()).resolve()/"Source/LxARPGEditor/Scripts/台地资产工具.py"
                exec(compile(path.read_text(encoding="utf-8-sig").split("@unreal.uclass()")[0],str(path),"exec"),globals())
                result={"refreshed":True}
            elif stage=="quit":
                unreal.SystemLibrary.quit_editor()
                result={"quit":True}
            else: raise ValueError(stage)
            return json.dumps(result,ensure_ascii=False)
        except Exception:
            return json.dumps({"error":traceback.format_exc()},ensure_ascii=False)


unreal.EditorPythonScripting.set_keep_python_script_alive(True)
unreal.ToolsetRegistry.register_toolset_class(StoneTerraceTools)
unreal.SystemLibrary.execute_console_command(None,"ModelContextProtocol.StartServer 8011")
unreal.SystemLibrary.execute_console_command(None,"ModelContextProtocol.RefreshTools")
