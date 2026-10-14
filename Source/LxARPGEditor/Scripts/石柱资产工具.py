"""通过 UE 5.8 官方 MCP 制作圆方石柱与连续灰白石纹材质。"""
import json
import time
import traceback
from pathlib import Path
import unreal

# 专用内容目录和验证输出，不改用户当前制作关卡。
BASE = "/Game/项目内容/美术资产/海港城市/石柱"
OUT = Path(unreal.Paths.project_dir()).resolve()/"Saved/StonePillar"
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


def make_material(name, tint, end):
    """连续三维浅石纹与可调装饰环带，避免方柱角部纹理断裂。"""
    path=BASE+"/材质/"+name
    mat=unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else None
    if not mat:
        mat=unreal.AssetToolsHelpers.get_asset_tools().create_asset(name,BASE+"/材质",unreal.Material,unreal.MaterialFactoryNew())
    unreal.MaterialEditingLibrary.delete_all_material_expressions(mat)
    uv=expression(mat,unreal.MaterialExpressionTextureCoordinate,coordinate_index=0,desc="石料局部平面坐标")
    depth=expression(mat,unreal.MaterialExpressionTextureCoordinate,coordinate_index=1,desc="石料高度与整石随机相位")
    ornament=expression(mat,unreal.MaterialExpressionTextureCoordinate,coordinate_index=2,desc="闭合周长与石料相对高度")
    vertex=expression(mat,unreal.MaterialExpressionVertexColor,desc="整石轻微色差")
    tint_node=expression(mat,unreal.MaterialExpressionVectorParameter,parameter_name="石材颜色",default_value=unreal.LinearColor(*tint,1),desc="明亮清爽灰白石材")
    grain=expression(mat,unreal.MaterialExpressionScalarParameter,parameter_name="石纹强度",default_value=.65,desc="柔和石英脉与细腻矿物斑纹")
    carved=expression(mat,unreal.MaterialExpressionScalarParameter,parameter_name="刻纹强度",default_value=.36 if end else .22,desc="浅灰环带与菱形雕纹")
    kind=expression(mat,unreal.MaterialExpressionConstant,r=1 if end else 0,desc="端部装饰或柱身细环带")
    code="""
float3 p=float3(UV.xy,Depth.x)*100;
float phase=Depth.y;
float n=sin(p.x*.057+p.y*.041+sin(p.z*.039+phase)*1.2)+sin(p.y*.19+p.z*.12+sin(p.x*.1))*.45;
float vein=pow(saturate(.5+.5*sin(p.x*.13+p.y*.09+p.z*.055+n*2.1+phase)),14);
float cloud=sin(p.x*.22+p.z*.14)*sin(p.y*.24-p.z*.18)*.025;
float fine=sin(p.x*2.7+p.y*.8)*sin(p.y*2.3+p.z*1.4)*.008;
float t=Ornament.y;
float bandD=Kind>.5?min(abs(t-.23),abs(t-.55)):min(abs(t-.045),abs(t-.955));
float bandWidth=Kind>.5?.009:.0016;
float band=1-smoothstep(bandWidth,bandWidth+fwidth(t)*1.5,bandD);
float u=abs(frac(Ornament.x*8+.5)-.5);
float diamondD=abs(u*1.15+abs(t-.39)*3.3-.26);
float diamond=(1-smoothstep(.018,.032+fwidth(diamondD),diamondD))*Kind;
float diamondLight=1-smoothstep(.01,.03,abs((u+.025)*1.15+abs(t-.393)*3.3-.26));
float decor=max(band,diamond);
float shade=1-Grain*(vein*.17+n*.025)+cloud+fine-Carved*decor+Carved*diamondLight*Kind*.16;
return Tint.rgb*Vertex.rgb*shade;
"""
    shade=custom(mat,code,{"UV":uv,"Depth":depth,"Ornament":ornament,"Vertex":vertex,"Tint":tint_node,
                         "Grain":grain,"Carved":carved,"Kind":kind},unreal.CustomMaterialOutputType.CMOT_FLOAT3,"连续石纹与闭合环绕浅刻纹")
    output(mat,shade,unreal.MaterialProperty.MP_BASE_COLOR)
    rough=expression(mat,unreal.MaterialExpressionScalarParameter,parameter_name="粗糙度",default_value=.79,desc="细磨天然石材的柔和反光")
    output(mat,rough,unreal.MaterialProperty.MP_ROUGHNESS)
    spec=expression(mat,unreal.MaterialExpressionConstant,r=.24,desc="克制的石材高光")
    output(mat,spec,unreal.MaterialProperty.MP_SPECULAR)
    unreal.MaterialEditingLibrary.layout_material_expressions(mat)
    unreal.MaterialEditingLibrary.recompile_material(mat)
    unreal.EditorAssetLibrary.save_loaded_asset(mat,only_if_is_dirty=False)
    return mat


# 名称、截面和两端开关定义，可复用的四个预设。
PRESETS=(("自动圆石柱",unreal.LxPillarShape.ROUND,True,True),("自动方石柱",unreal.LxPillarShape.SQUARE,True,True),
         ("裸圆石柱",unreal.LxPillarShape.ROUND,False,False),("裸方石柱",unreal.LxPillarShape.SQUARE,False,False))


def build_assets():
    """创建中文灰白石材与四个原生逻辑蓝图预设。"""
    for folder in ("材质","预设","演示","静态网格"):
        unreal.EditorAssetLibrary.make_directory(BASE+"/"+folder)
    shaft=make_material("细纹白石",(.66,.65,.61),False)
    end=make_material("刻纹灰石",(.56,.58,.575),True)
    result=[]
    for name,shape,base,capital in PRESETS:
        path=BASE+"/预设/"+name
        bp=unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else None
        if not bp:
            factory=unreal.BlueprintFactory()
            factory.set_editor_property("parent_class",unreal.LxStonePillarActor)
            bp=unreal.AssetToolsHelpers.get_asset_tools().create_asset(name,BASE+"/预设",unreal.Blueprint,factory)
        default=unreal.get_default_object(bp.generated_class())
        for prop,value in {"shaft_material":shaft,"end_material":end,"shape":shape,"base":base,"capital":capital,
                           "static_mesh_asset_path":BASE+"/静态网格/"+name}.items():
            default.set_editor_property(prop,value)
        bp.set_editor_property("run_construction_script_on_drag",False)
        unreal.BlueprintEditorLibrary.compile_blueprint(bp)
        unreal.EditorAssetLibrary.save_loaded_asset(bp,only_if_is_dirty=False)
        result.append(bp.get_path_name())
    unreal.AutomationLibrary.finish_loading_before_screenshot()
    return result


def verify_assets(save=False):
    """验证预设和保存资产的网格、石块、碰撞与实际射线阻挡。"""
    actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    meshes=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    result=[]
    for name,shape,base,capital in PRESETS:
        bp=unreal.load_asset(BASE+"/预设/"+name)
        actor=actors.spawn_actor_from_class(bp.generated_class(),unreal.Vector(0,0,-5000))
        try:
            actor.rebuild_pillar()
            comp=actor.get_editor_property("pillar_mesh")
            mesh=comp.get_editor_property("static_mesh")
            count=1+int(base)+int(capital)
            if not mesh or actor.get_editor_property("generation_diagnostic"):
                raise RuntimeError("石柱生成失败："+name)
            if len(actor.get_components_by_class(unreal.StaticMeshComponent))!=1 or (meshes.get_simple_collision_count(mesh)+meshes.get_convex_collision_count(mesh))!=count:
                raise RuntimeError("网格或碰撞形状数量错误")
            if actor.get_editor_property("generated_stone_count")!=count:
                raise RuntimeError("独立整石数量错误")
            # 实际简单碰撞射线必须命中中段；加宽端部不能把整根柱身碰撞扩大。
            hit=unreal.SystemLibrary.line_trace_single(world,unreal.Vector(-150,0,-4820),unreal.Vector(150,0,-4820),
                unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,False,[],unreal.DrawDebugTrace.NONE,True)
            miss=unreal.SystemLibrary.line_trace_single(world,unreal.Vector(-150,45,-4820),unreal.Vector(150,45,-4820),
                unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,False,[],unreal.DrawDebugTrace.NONE,True)
            if not hit or miss:
                raise RuntimeError("柱身实际碰撞未生效或范围过宽："+str((hit,miss)))
            entry={"preset":name,"stones":count,"meshes":1,"physics_components":1,"simple_shapes":count,
                   "collision_trace":True,"triangles":mesh.get_num_triangles(0)}
            if save:
                actor.save_static_mesh_asset()
                mesh=actor.get_editor_property("saved_static_mesh")
                if not mesh or (meshes.get_simple_collision_count(mesh)+meshes.get_convex_collision_count(mesh))!=count or mesh.get_num_triangles(0)!=entry["triangles"]:
                    raise RuntimeError("保存网格的几何或碰撞错误")
                slots=[meshes.get_lod_material_slot(mesh,0,i) for i in range(mesh.get_num_sections(0))]
                expected=([0] if base else [])+[1]+([2] if capital else [])
                if slots!=expected:
                    raise RuntimeError("保存网格的石块材质映射错误："+str(slots))
                entry["material_slots"]=slots
                entry["asset"]=mesh.get_path_name()
            result.append(entry)
        finally:
            actors.destroy_actor(actor)
    (OUT/("静态资产验收.json" if save else "预设验收.json")).write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding="utf-8")
    return result


def replace_preview_exports():
    """仅替换本工具验收记录中未被其他资产引用的示例导出。"""
    entries=json.loads((OUT/"静态资产验收.json").read_text(encoding="utf-8"))
    allowed={BASE+"/静态网格/"+v[0]+"."+v[0] for v in PRESETS}
    for entry in entries:
        path=entry["asset"]
        if path not in allowed or unreal.EditorAssetLibrary.find_package_referencers_for_asset(path,load_assets_to_confirm=True):
            raise RuntimeError("示例导出已被引用或不属于初始预设，保留该资产："+path)
    for entry in entries:
        if not unreal.EditorAssetLibrary.delete_asset(entry["asset"]):
            raise RuntimeError("无法替换示例导出："+entry["asset"])
    return verify_assets(True)


def reload_assets():
    """新编辑器进程验证已保存网格的几何、碰撞和石纹坐标通道。"""
    entries=json.loads((OUT/"静态资产验收.json").read_text(encoding="utf-8"))
    meshes=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    for entry in entries:
        mesh=unreal.load_asset(entry["asset"])
        if not mesh or mesh.get_num_triangles(0)!=entry["triangles"] or (meshes.get_simple_collision_count(mesh)+meshes.get_convex_collision_count(mesh))!=entry["simple_shapes"]:
            raise RuntimeError("重载资产的几何或碰撞错误")
        if mesh.get_num_tex_coords(0)<5:
            raise RuntimeError("石纹或光照纹理坐标丢失")
        slots=[meshes.get_lod_material_slot(mesh,0,i) for i in range(mesh.get_num_sections(0))]
        if slots!=entry.get("material_slots",[0,1,2] if entry["stones"]==3 else [1]):
            raise RuntimeError("重载后材质映射丢失："+str((entry["preset"],slots,[(str(v.material_slot_name),str(v.material_interface)) for v in mesh.get_editor_property("static_materials")])))
        actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
        world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
        actor=actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(0,0,-5000))
        try:
            actor.static_mesh_component.set_static_mesh(mesh)
            actor.static_mesh_component.set_collision_profile_name("BlockAll")
            hit=unreal.SystemLibrary.line_trace_single(world,unreal.Vector(-150,0,-4820),unreal.Vector(150,0,-4820),
                unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,False,[],unreal.DrawDebugTrace.NONE,True)
            if not hit:
                raise RuntimeError("重载资产的实际碰撞射线未命中")
        finally:
            actors.destroy_actor(actor)
        entry["material_slots"]=slots
        entry["reloaded"]=True
    (OUT/"重载验收.json").write_text(json.dumps(entries,ensure_ascii=False,indent=2),encoding="utf-8")
    return entries


def make_demo():
    """独立展示两种完整柱形和可自由关闭端部的整根柱身。"""
    world=unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
    actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    for i,(name,shape,base,capital) in enumerate(PRESETS):
        bp=unreal.load_asset(BASE+"/预设/"+name)
        actor=actors.spawn_actor_from_class(bp.generated_class(),unreal.Vector((i-1.5)*190,0,0))
        actor.set_actor_label(name)
        actor.rebuild_pillar()
    floor=actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(0,0,-20))
    floor.set_actor_label("浅灰展示地台")
    floor.static_mesh_component.set_static_mesh(unreal.load_asset("/Engine/BasicShapes/Cube"))
    floor.static_mesh_component.set_material(0,unreal.load_asset("/Engine/BasicShapes/BasicShapeMaterial"))
    floor.set_actor_scale3d(unreal.Vector(10,5,.4))
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
    unreal.EditorLoadingAndSavingUtils.save_map(world,BASE+"/演示/石柱演示")
    return {"map":BASE+"/演示/石柱演示"}


def capture_preview():
    """离屏捕获柱形与底墩细节，检查分块、石纹与浅刻纹。"""
    levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    levels.load_level(BASE+"/演示/石柱演示")
    levels.editor_set_viewport_realtime(True)
    actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    camera=actors.spawn_actor_from_class(unreal.SceneCapture2D,unreal.Vector(520,-1060,630))
    camera.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(camera.get_actor_location(),unreal.Vector(0,0,180)),False)
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
        name=("石柱总览.png","石柱细节.png")[STATE["phase"]]
        unreal.RenderingLibrary.export_render_target(STATE["world"],STATE["target"],str(OUT),name)
        if STATE["phase"]==0:
            location=unreal.Vector(-35,-225,150)
            look=unreal.Vector(-170,0,65)
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
class StonePillarTools(unreal.ToolsetDefinition):
    """自动石柱的官方 MCP 资产制作和验证入口。"""
    @unreal.ufunction(static=True,params=[str],ret=str,meta={"AICallable":"","DisplayName":"处理石柱资产","Category":"石柱|制作"})
    def process_pillar(stage):
        """按阶段创建、验证、演示、截图或运行自动化测试。"""
        try:
            if stage=="refresh":
                # 仅刷新本工具文件的函数定义，方便迭代材质与验收逻辑。
                source_path=Path(unreal.Paths.project_dir()).resolve()/"Source/LxARPGEditor/Scripts/石柱资产工具.py"
                source=source_path.read_text(encoding="utf-8")
                exec(compile(source.split("@unreal.uclass()")[0],str(source_path),"exec"),globals())
                result={"refreshed":True}
            elif stage=="build": result=build_assets()
            elif stage=="verify": result=verify_assets()
            elif stage=="static": result=verify_assets(True)
            elif stage=="replace": result=replace_preview_exports()
            elif stage=="reload": result=reload_assets()
            elif stage=="demo": result=make_demo()
            elif stage=="capture": result=capture_preview()
            elif stage=="tests":
                unreal.SystemLibrary.execute_console_command(None,"Automation RunTests LxARPG.World.StonePillar")
                result={"tests_started":True}
            elif stage=="quit":
                unreal.SystemLibrary.quit_editor()
                result={"quit":True}
            else: raise ValueError(stage)
            return json.dumps(result,ensure_ascii=False)
        except Exception:
            return json.dumps({"error":traceback.format_exc()},ensure_ascii=False)


unreal.EditorPythonScripting.set_keep_python_script_alive(True)
unreal.ToolsetRegistry.register_toolset_class(StonePillarTools)
unreal.SystemLibrary.execute_console_command(None,"ModelContextProtocol.StartServer 8008")
unreal.SystemLibrary.execute_console_command(None,"ModelContextProtocol.RefreshTools")
