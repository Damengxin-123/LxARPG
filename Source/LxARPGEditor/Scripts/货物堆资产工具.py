"""使用虚幻 5.8 官方 MCP 制作与验收海港货物堆。"""
import json
import time
import traceback
from pathlib import Path
import unreal
# 货物资产、中文自动化预设及独立验收输出。
BASE="/Game/项目内容/美术资产/海港城市/货物"
PRESET_ROOT="/Game/项目内容/美术资产/自动化类型/货物"
OUT=Path(unreal.Paths.project_dir()).resolve()/"Saved/CargoPile"
OUT.mkdir(parents=True,exist_ok=True)
# 保持跨帧离屏捕获状态。
STATE=globals().get("STATE",{})
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


def make_wood(name, tint, grain):
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


def make_surface(name,kind):
    """生成粗麻织物、捻绳或低反射铁箍，细节保持概括。"""
    path=BASE+"/材质/"+name
    mat=unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else unreal.AssetToolsHelpers.get_asset_tools().create_asset(name,BASE+"/材质",unreal.Material,unreal.MaterialFactoryNew())
    unreal.MaterialEditingLibrary.delete_all_material_expressions(mat)
    uv=expression(mat,unreal.MaterialExpressionTextureCoordinate,coordinate_index=0,desc="构件连续局部坐标")
    depth=expression(mat,unreal.MaterialExpressionTextureCoordinate,coordinate_index=1,desc="厚度和稳定纹理相位")
    vertex=expression(mat,unreal.MaterialExpressionVertexColor,desc="构件固有色差")
    colors=((.065,.075,.082),(.52,.405,.255),(.31,.205,.10),(.34,.37,.385))
    tint=expression(mat,unreal.MaterialExpressionVectorParameter,parameter_name="基础颜色",default_value=unreal.LinearColor(*colors[kind],1),desc="与清爽海港风格一致的低饱和材质")
    code="""
float3 p=float3(UV.xy,Depth.x)*100;
float weave=(sin(p.x*8)*sin(p.y*8)+sin(p.y*8)*sin(p.z*8)+sin(p.z*8)*sin(p.x*8))*.018;
float broad=sin(p.x*.14+sin(p.z*.2))*sin(p.y*.17)*.045;
return Tint.rgb*Vertex.rgb*(1+weave+broad);
"""
    node=custom(mat,code,{"UV":uv,"Depth":depth,"Vertex":vertex,"Tint":tint},unreal.CustomMaterialOutputType.CMOT_FLOAT3,"细麻布织纹及柔和色差")
    output(mat,node,unreal.MaterialProperty.MP_BASE_COLOR)
    rough=expression(mat,unreal.MaterialExpressionConstant,r=.58 if kind==0 else .88,desc="哑光表面")
    output(mat,rough,unreal.MaterialProperty.MP_ROUGHNESS)
    metallic=expression(mat,unreal.MaterialExpressionConstant,r=.7 if kind==0 else 0,desc="铁箍的克制金属感")
    output(mat,metallic,unreal.MaterialProperty.MP_METALLIC)
    unreal.MaterialEditingLibrary.recompile_material(mat)
    unreal.EditorAssetLibrary.save_loaded_asset(mat,only_if_is_dirty=False)
    return mat


# 预设名称、混乱度、密度、堆放高度。
PRESETS=(("自动货物堆",.45,.85,280),("整齐仓储货堆",0,1,340),("码头散放货物",.85,.65,150))


def build_assets():
    """创建四种统一材质和三个可拖入关卡的货物堆蓝图。"""
    for folder in ("材质","静态网格","演示"):
        unreal.EditorAssetLibrary.make_directory(BASE+"/"+folder)
    unreal.EditorAssetLibrary.make_directory(PRESET_ROOT)
    wood=make_wood("暖木货箱",(.42,.255,.125),.42)
    metal=make_surface("深灰铁箍",0)
    cloth=make_surface("粗麻包装",1)
    rope=make_surface("海运捆绳",2)
    # 主预设挑选包含全部九类货物的确定种子，打开即能查看完整货物库。
    actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    probe=actors.spawn_actor_from_class(unreal.LxCargoPileActor,unreal.Vector(0,0,-10000))
    showcase_seed=1009
    try:
        probe.set_editor_property("disorder",.45)
        probe.set_editor_property("stack_height",280)
        for seed in range(32):
            probe.set_editor_property("random_seed",seed)
            probe.rebuild_pile()
            kinds={str(p.get_editor_property("kind")) for p in probe.get_editor_property("placements")}
            if len(kinds)==9:
                showcase_seed=seed
                break
    finally:
        actors.destroy_actor(probe)
    result=[]
    for name,disorder,density,height in PRESETS:
        path=PRESET_ROOT+"/"+name
        bp=unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else None
        if not bp:
            factory=unreal.BlueprintFactory()
            factory.set_editor_property("parent_class",unreal.LxCargoPileActor)
            bp=unreal.AssetToolsHelpers.get_asset_tools().create_asset(name,PRESET_ROOT,unreal.Blueprint,factory)
        default=unreal.get_default_object(bp.generated_class())
        for prop,value in {"wood_material":wood,"metal_material":metal,"cloth_material":cloth,"rope_material":rope,
                           "random_seed":showcase_seed if name=="自动货物堆" else 1009,"disorder":disorder,"density":density,"stack_height":height,"static_mesh_asset_path":BASE+"/静态网格/"+name}.items():
            default.set_editor_property(prop,value)
        bp.set_editor_property("run_construction_script_on_drag",False)
        unreal.BlueprintEditorLibrary.compile_blueprint(bp)
        unreal.EditorAssetLibrary.save_loaded_asset(bp,only_if_is_dirty=False)
        result.append(bp.get_path_name())
    unreal.AutomationLibrary.finish_loading_before_screenshot()
    return result


def verify_assets(save=False):
    """在空白测试场景检查每件货物的碰撞、合并网格和静态导出。"""
    world=unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
    actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    meshes=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    result=[]
    for name,disorder,density,height in PRESETS:
        bp=unreal.load_asset(PRESET_ROOT+"/"+name)
        actor=actors.spawn_actor_from_class(bp.generated_class(),unreal.Vector(0,0,-5000))
        try:
            actor.rebuild_pile()
            mesh=actor.get_editor_property("pile_mesh").get_editor_property("static_mesh")
            placements=actor.get_editor_property("placements")
            if not mesh or not placements or len(actor.get_components_by_class(unreal.StaticMeshComponent))!=1:
                raise RuntimeError("货物生成失败或存在额外网格组件")
            if meshes.get_simple_collision_count(mesh)!=len(placements):
                raise RuntimeError("每件货物的简单碰撞数量不一致")
            for p in placements:
                pos=p.get_editor_property("position")
                hit=unreal.SystemLibrary.line_trace_single(world,unreal.Vector(pos.x,pos.y,-5000+height+100),unreal.Vector(pos.x,pos.y,-5001),
                    unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,False,[],unreal.DrawDebugTrace.NONE,True)
                if not hit: raise RuntimeError("货物没有真实阻挡碰撞")
            counts={}
            for p in placements:
                k=str(p.get_editor_property("kind")); counts[k]=counts.get(k,0)+1
            entry={"preset":name,"cargo":len(placements),"types":counts,"height_cm":actor.get_editor_property("generated_height"),
                   "height_limit_cm":height,"meshes":1,"sections":mesh.get_num_sections(0),"triangles":mesh.get_num_triangles(0),"collision_trace":True}
            if save:
                actor.save_static_mesh_asset()
                saved=actor.get_editor_property("saved_static_mesh")
                if not saved or saved.get_num_triangles(0)!=entry["triangles"] or saved.get_num_sections(0)!=entry["sections"]:
                    raise RuntimeError("独立静态网格导出不一致")
                entry["asset"]=saved.get_path_name()
            result.append(entry)
        finally:
            actors.destroy_actor(actor)
    (OUT/("静态资产验收.json" if save else "预设验收.json")).write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding="utf-8")
    return result


def reload_assets():
    """重新加载导出的货物堆，核对纹理展开与简单碰撞。"""
    entries=json.loads((OUT/"静态资产验收.json").read_text(encoding="utf-8"))
    meshes=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    for entry in entries:
        mesh=unreal.load_asset(entry["asset"])
        if not mesh or mesh.get_num_triangles(0)!=entry["triangles"] or mesh.get_num_tex_coords(0)<4 or meshes.get_simple_collision_count(mesh)!=entry["cargo"]:
            raise RuntimeError("重载货物堆的几何、碰撞或纹理通道错误")
        entry["reloaded"]=True
    (OUT/"重载验收.json").write_text(json.dumps(entries,ensure_ascii=False,indent=2),encoding="utf-8")
    return entries


def make_demo():
    """展示海港货箱、木桶、麻袋与木料。"""
    world=unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
    actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    bp=unreal.load_asset(PRESET_ROOT+"/自动货物堆")
    actor=actors.spawn_actor_from_class(bp.generated_class(),unreal.Vector())
    actor.set_actor_label("海港木质货物堆")
    actor.rebuild_pile()
    for name, rotation, intensity, shadow in (("温暖日光",(-50,-40,0),8,True),("清透补光",(-25,130,0),6,False)):
        light=actors.spawn_actor_from_class(unreal.DirectionalLight,unreal.Vector(0,0,1500),unreal.Rotator(pitch=rotation[0],yaw=rotation[1],roll=rotation[2]))
        light.set_actor_label(name)
        comp=light.get_component_by_class(unreal.DirectionalLightComponent)
        comp.set_mobility(unreal.ComponentMobility.MOVABLE)
        comp.set_editor_property("intensity",intensity)
        comp.set_editor_property("cast_shadows",shadow)
        if shadow:
            comp.set_editor_property("atmosphere_sun_light",True)
            comp.set_editor_property("light_source_angle",3.0)
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
    # 石质展示台提供落地阴影，仅存在于演示关卡中。
    ground=actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(500,350,-6))
    ground.set_actor_label("浅灰展示地台")
    ground.static_mesh_component.set_static_mesh(unreal.load_asset("/Engine/BasicShapes/Cube"))
    ground.static_mesh_component.set_material(0,make_surface("展示石台",3))
    ground.set_actor_scale3d(unreal.Vector(11,8, .12))
    unreal.EditorLoadingAndSavingUtils.save_map(world,BASE+"/演示/货物堆演示")
    return {"map":BASE+"/演示/货物堆演示"}


def capture_preview():
    """离屏捕获屋顶全貌及近景，检查货物造型、堆叠和暖木配色。"""
    levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    levels.load_level(BASE+"/演示/货物堆演示")
    levels.editor_set_viewport_realtime(True)
    actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    camera=actors.spawn_actor_from_class(unreal.SceneCapture2D,unreal.Vector(1400,-1200,1100))
    camera.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(camera.get_actor_location(),unreal.Vector(500,350,100)),False)
    component=camera.get_component_by_class(unreal.SceneCaptureComponent2D)
    target=unreal.RenderingLibrary.create_render_target2d(world,1600,1000,unreal.TextureRenderTargetFormat.RTF_RGBA8)
    component.set_editor_property("texture_target",target)
    component.set_editor_property("capture_source",unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR)
    component.set_editor_property("capture_every_frame",False)
    component.set_editor_property("always_persist_rendering_state",True)
    component.set_editor_property("fov_angle",44)
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
        name=("货物堆总览.png","货物堆细节.png")[STATE["phase"]]
        unreal.RenderingLibrary.export_render_target(STATE["world"],STATE["target"],str(OUT),name)
        if STATE["phase"]==0:
            location=unreal.Vector(1050,-250,550)
            look=unreal.Vector(550,330,120)
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
class CargoPileTools(unreal.ToolsetDefinition):
    """自动货物堆的官方 MCP 资产制作和验证入口。"""
    @unreal.ufunction(static=True,params=[str],ret=str,meta={"AICallable":"","DisplayName":"处理货物堆资产","Category":"货物堆|制作"})
    def process_cargo(stage):
        """按阶段创建、验证、演示、截图或运行自动化测试。"""
        try:
            if stage=="refresh":
                # 仅刷新本工具文件的函数定义，方便迭代材质与验收逻辑。
                source_path=Path(unreal.Paths.project_dir()).resolve()/"Source/LxARPGEditor/Scripts/货物堆资产工具.py"
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
                unreal.SystemLibrary.execute_console_command(None,"Automation RunTests LxARPG.World.CargoPile")
                result={"tests_started":True}
            elif stage=="quit":
                unreal.SystemLibrary.quit_editor()
                result={"quit":True}
            else: raise ValueError(stage)
            return json.dumps(result,ensure_ascii=False)
        except Exception:
            return json.dumps({"error":traceback.format_exc()},ensure_ascii=False)


unreal.EditorPythonScripting.set_keep_python_script_alive(True)
unreal.ToolsetRegistry.register_toolset_class(CargoPileTools)
unreal.SystemLibrary.execute_console_command(None,"ModelContextProtocol.StartServer 8011")
unreal.SystemLibrary.execute_console_command(None,"ModelContextProtocol.RefreshTools")
