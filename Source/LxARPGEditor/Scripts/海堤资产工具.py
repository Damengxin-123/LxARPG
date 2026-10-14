"""通过 UE 5.8 官方 MCP 制作自动海堤预设、运行验收并离屏渲染。"""
import json
import time
import traceback
from pathlib import Path
import unreal

# 只操作海堤专用资产和独立演示关卡。
BASE = "/Game/项目内容/美术资产/海港城市/海堤"
PRESET = "/Game/项目内容/美术资产/自动化类型/海堤/海港石海堤"
OUT = Path(unreal.Paths.project_dir()).resolve()/"Saved/Seawall"
OUT.mkdir(parents=True, exist_ok=True)
# 保存异步离屏渲染的跨帧对象。
STATE = {}


def node(material, cls, **properties):
    """建立带中文说明的材质节点。"""
    result = unreal.MaterialEditingLibrary.create_material_expression(material, cls)
    for key, value in properties.items():
        result.set_editor_property(key, value)
    return result


def stone_material(name, tint, brick=False):
    """按真实厘米UV生成简洁石纹；陆侧加入错缝砖和细窄砂浆。"""
    folder = BASE+"/材质"
    path = folder+"/"+name
    material = unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else None
    if not material:
        material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, folder, unreal.Material, unreal.MaterialFactoryNew())
    unreal.MaterialEditingLibrary.delete_all_material_expressions(material)
    uv = node(material, unreal.MaterialExpressionTextureCoordinate, desc="每单位代表一米石面")
    color = node(material, unreal.MaterialExpressionVectorParameter, parameter_name="石材颜色",
                 default_value=unreal.LinearColor(*tint, 1), desc="清爽灰白石材")
    code = """
float2 p=UV*100;
float2 cell=floor(p/13);
float2 f=frac(p/13); f=f*f*(3-2*f);
float a=frac(sin(dot(cell,float2(127.1,311.7)))*43758.5453);
float b=frac(sin(dot(cell+float2(1,0),float2(127.1,311.7)))*43758.5453);
float c=frac(sin(dot(cell+float2(0,1),float2(127.1,311.7)))*43758.5453);
float d=frac(sin(dot(cell+1,float2(127.1,311.7)))*43758.5453);
float grain=lerp(lerp(a,b,f.x),lerp(c,d,f.x),f.y);
float fine=frac(sin(dot(floor(p*2),float2(12.9898,78.233)))*43758.5453);
float shade=.90+grain*.15+(fine-.5)*.02*saturate(1-max(fwidth(p.x),fwidth(p.y)));
"""
    if brick:
        code += """
float row=floor(p.y/28);
float2 q=float2(p.x+fmod(row,2)*36,p.y)/float2(72,28);
float2 edge=min(frac(q),1-frac(q))*float2(72,28);
float mortar=smoothstep(.55,1.5,min(edge.x,edge.y));
float block=frac(sin(dot(floor(q),float2(39.37,71.13)))*43758.5453);
return Tint.rgb*lerp(.52,shade*(.95+.09*block),mortar);
"""
    else:
        code += "return Tint.rgb*shade;"
    custom = node(material, unreal.MaterialExpressionCustom, description="细石颗粒与错缝砌石" if brick else "粗岩颗粒与柔和色差",
                  output_type=unreal.CustomMaterialOutputType.CMOT_FLOAT3, code=code)
    inputs = []
    for key in ("UV", "Tint"):
        entry = unreal.CustomInput()
        entry.set_editor_property("input_name", key)
        inputs.append(entry)
    custom.set_editor_property("inputs", inputs)
    unreal.MaterialEditingLibrary.connect_material_expressions(uv, "", custom, "UV")
    unreal.MaterialEditingLibrary.connect_material_expressions(color, "", custom, "Tint")
    vertex = node(material, unreal.MaterialExpressionVertexColor, desc="逐块岩板的固定浅色差")
    multiply = node(material, unreal.MaterialExpressionMultiply, desc="石纹叠加岩板色差")
    unreal.MaterialEditingLibrary.connect_material_expressions(custom, "", multiply, "A")
    unreal.MaterialEditingLibrary.connect_material_expressions(vertex, "RGB", multiply, "B")
    unreal.MaterialEditingLibrary.connect_material_property(multiply, "", unreal.MaterialProperty.MP_BASE_COLOR)
    roughness = node(material, unreal.MaterialExpressionScalarParameter, parameter_name="粗糙度", default_value=.88 if not brick else .81)
    specular = node(material, unreal.MaterialExpressionConstant, r=.18)
    unreal.MaterialEditingLibrary.connect_material_property(roughness, "", unreal.MaterialProperty.MP_ROUGHNESS)
    unreal.MaterialEditingLibrary.connect_material_property(specular, "", unreal.MaterialProperty.MP_SPECULAR)
    normal_code = "float2 p=UV*100; return normalize(float3(sin(p.x*.087+cos(p.y*.061)*2)*.045,cos(p.y*.071+sin(p.x*.053)*2)*.04,1));"
    if brick:
        normal_code = "return float3(0,0,1);"
    normal = node(material, unreal.MaterialExpressionCustom, description="克制的细石面凹凸",
                  output_type=unreal.CustomMaterialOutputType.CMOT_FLOAT3,
                  code=normal_code)
    entry = unreal.CustomInput()
    entry.set_editor_property("input_name", "UV")
    normal.set_editor_property("inputs", [entry])
    unreal.MaterialEditingLibrary.connect_material_expressions(uv, "", normal, "UV")
    unreal.MaterialEditingLibrary.connect_material_property(normal, "", unreal.MaterialProperty.MP_NORMAL)
    unreal.MaterialEditingLibrary.layout_material_expressions(material)
    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=False)
    return material


def build_assets():
    """保存海侧、陆侧材质和沿用现有道路铺面的中文海堤预设。"""
    unreal.EditorAssetLibrary.make_directory(BASE+"/材质")
    unreal.EditorAssetLibrary.make_directory(PRESET.rsplit("/", 1)[0])
    rock = stone_material("海侧粗岩", (.49, .53, .54))
    brick = stone_material("陆侧砌石", (.65, .66, .63), True)
    source = unreal.load_asset("/Game/项目内容/美术资产/自动化类型/道路/道路_规整石板_A")
    if not source:
        raise RuntimeError("缺少现有规整石板道路预设")
    road = unreal.get_default_object(source.generated_class())
    bp = unreal.load_asset(PRESET) if unreal.EditorAssetLibrary.does_asset_exist(PRESET) else None
    if not bp:
        factory = unreal.BlueprintFactory()
        factory.set_editor_property("parent_class", unreal.LxSeawallActor)
        bp = unreal.AssetToolsHelpers.get_asset_tools().create_asset("海港石海堤", PRESET.rsplit("/", 1)[0], unreal.Blueprint, factory)
    default = unreal.get_default_object(bp.generated_class())
    for key, value in {"sea_stone_material": rock, "land_brick_material": brick,
                       "road_material": road.get_editor_property("road_material"),
                       "curb_material": road.get_editor_property("curb_material"),
                       "road_texture_size": road.get_editor_property("road_texture_size"),
                       "curb_texture_size": road.get_editor_property("curb_texture_size")}.items():
        default.set_editor_property(key, value)
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    unreal.EditorAssetLibrary.save_loaded_asset(bp, only_if_is_dirty=False)
    return {"preset": PRESET, "verification": verify_assets()}


def verify_assets():
    """检验实际预设、材质、路面和墙体碰撞及两套可保存的优化网格。"""
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    bp = unreal.load_asset(PRESET)
    wall = actors.spawn_actor_from_class(bp.generated_class(), unreal.Vector(0, 0, -5000))
    try:
        wall.rebuild_seawall()
        if wall.get_editor_property("seawall_diagnostic") or wall.get_editor_property("generated_slab_count") < 20:
            raise RuntimeError("海堤预设生成失败")
        for key in ("road_material", "curb_material", "sea_stone_material", "land_brick_material"):
            if not wall.get_editor_property(key):
                raise RuntimeError("材质缺失："+key)
        for a, b in (((1000,0,-4900),(1000,0,-5100)),
                     ((1000,1500,-5250),(1000,0,-5250)),
                     ((1000,-1000,-5250),(1000,0,-5250))):
            if not unreal.SystemLibrary.line_trace_single(world, unreal.Vector(*a), unreal.Vector(*b),
                    unreal.TraceTypeQuery.ECC_VISIBILITY, False, [], unreal.DrawDebugTrace.NONE, True):
                raise RuntimeError("路面或墙体碰撞缺失")
        for key in ("optimized_mesh", "wall_optimized_mesh"):
            cache = wall.get_editor_property(key)
            cache.finalize_mesh()
            if not cache.get_editor_property("static_mesh"):
                raise RuntimeError("渲染缓存缺失："+key)
        result = {"slabs": wall.get_editor_property("generated_slab_count"), "collision": "passed", "caches": "passed"}
        spline=wall.get_editor_property("road_spline")
        spline.clear_spline_points(False)
        for index,point in enumerate(((0,0,0),(6000,0,0),(6000,6000,0),(0,6000,0))):
            spline.add_spline_point(unreal.Vector(*point),unreal.SplineCoordinateSpace.LOCAL,False)
            spline.set_spline_point_type(index,unreal.SplinePointType.CURVE,False)
        spline.set_closed_loop(True,True)
        wall.rebuild_seawall()
        result["closed_diagnostic"]=wall.get_editor_property("seawall_diagnostic")
        if result["closed_diagnostic"]:
            raise RuntimeError("平滑闭环生成失败："+result["closed_diagnostic"])
        (OUT/"资产验证.json").write_text(json.dumps(result, ensure_ascii=False), encoding="utf-8")
        return result
    finally:
        actors.destroy_actor(wall)


def preview():
    """创建独立演示，展示直线、曲线、陆侧入口和两侧不同的石材。"""
    world = unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    wall = actors.spawn_actor_from_class(unreal.load_asset(PRESET).generated_class(), unreal.Vector())
    wall.set_actor_label("海港石海堤")
    spline = wall.get_editor_property("road_spline")
    spline.clear_spline_points(False)
    for index, point in enumerate(((0,0,0),(1800,0,0),(3400,600,0),(4600,1800,0))):
        spline.add_spline_point(unreal.Vector(*point), unreal.SplineCoordinateSpace.LOCAL, False)
        spline.set_spline_point_type(index, unreal.SplinePointType.CURVE, False)
    spline.set_tangent_at_spline_point(0, unreal.Vector(1800,0,0), unreal.SplineCoordinateSpace.LOCAL, False)
    spline.set_tangent_at_spline_point(1, unreal.Vector(1800,0,0), unreal.SplineCoordinateSpace.LOCAL, True)
    wall.rebuild_seawall()
    if wall.get_editor_property("seawall_diagnostic"):
        raise RuntimeError(wall.get_editor_property("seawall_diagnostic"))
    road_class = unreal.load_asset("/Game/项目内容/美术资产/自动化类型/道路/道路_规整石板_A").generated_class()
    road = actors.spawn_actor_from_class(road_class, unreal.Vector(1000,-450,0), unreal.Rotator(yaw=-90))
    road.set_actor_label("自动接入道路")
    road.set_editor_property("road_width", 360)
    road.set_editor_property("road_depth", 500)
    road.rebuild_road()
    if not wall.get_editor_property("road_connections"):
        raise RuntimeError("演示中的自动道路未接通")
    # 地面只用于烘托尺度，不遮住陆侧墙面。
    ground = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(1800,0,-525))
    ground.static_mesh_component.set_static_mesh(unreal.load_asset("/Engine/BasicShapes/Cube"))
    ground.static_mesh_component.set_material(0, unreal.load_asset("/Engine/BasicShapes/BasicShapeMaterial"))
    ground.set_actor_scale3d(unreal.Vector(180,180,.4))
    ground.set_actor_label("展示地面")
    for rotation, intensity in ((unreal.Rotator(pitch=-40,yaw=-55),7.),(unreal.Rotator(pitch=-55,yaw=100),3.)):
        light = actors.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0,0,2000), rotation)
        component = light.get_component_by_class(unreal.DirectionalLightComponent)
        component.set_mobility(unreal.ComponentMobility.MOVABLE)
        component.set_editor_property("intensity", intensity)
        component.set_editor_property("atmosphere_sun_light", intensity>5)
        if intensity<5:
            component.set_editor_property("cast_shadows", False)
    actors.spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector())
    sky = actors.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0,0,2000))
    sky.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    sky.light_component.set_editor_property("real_time_capture", True)
    unreal.EditorAssetLibrary.make_directory(BASE+"/演示")
    if not unreal.EditorLoadingAndSavingUtils.save_map(world, BASE+"/演示/海堤演示"):
        raise RuntimeError("保存演示失败")
    shots = [("海侧总览",(4900,6700,3600),(2300,600,-100)),
             ("陆侧砌石",(400,-4300,2200),(2100,500,-120)),
             ("岩板近景",(1750,2200,750),(1200,350,-180)),
             ("路口近景",(1300,-1750,1350),(1000,-180,0))]
    camera = actors.spawn_actor_from_class(unreal.SceneCapture2D, unreal.Vector(*shots[0][1]))
    camera.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(camera.get_actor_location(),unreal.Vector(*shots[0][2])),False)
    component = camera.get_component_by_class(unreal.SceneCaptureComponent2D)
    target = unreal.RenderingLibrary.create_render_target2d(world,1600,1000,unreal.TextureRenderTargetFormat.RTF_RGBA8)
    for key,value in {"texture_target":target,"capture_source":unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR,
                      "capture_every_frame":False,"always_persist_rendering_state":True,"fov_angle":50.}.items():
        component.set_editor_property(key,value)
    settings = component.get_editor_property("post_process_settings")
    for key,value in {"override_auto_exposure_min_brightness":True,"override_auto_exposure_max_brightness":True,
                      "auto_exposure_min_brightness":1.,"auto_exposure_max_brightness":1.}.items():
        settings.set_editor_property(key,value)
    component.set_editor_property("post_process_settings",settings)
    unreal.AutomationLibrary.finish_loading_before_screenshot()
    STATE.update(world=world,camera=camera,component=component,target=target,shots=shots,index=0,next=time.monotonic()+8)
    STATE["handle"]=unreal.register_slate_post_tick_callback(capture_tick)
    return {"preview_started":True,"map":BASE+"/演示/海堤演示"}


def capture_tick(delta):
    """等待渲染稳定后输出多个方向的原生引擎截图。"""
    try:
        STATE["component"].capture_scene()
        if time.monotonic()<STATE["next"]:
            return
        name=STATE["shots"][STATE["index"]][0]
        unreal.RenderingLibrary.export_render_target(STATE["world"],STATE["target"],str(OUT),name+".png")
        STATE["index"]+=1
        if STATE["index"]==len(STATE["shots"]):
            unreal.unregister_slate_post_tick_callback(STATE["handle"])
            (OUT/"预览完成.json").write_text('{"views":4}',encoding="utf-8")
            return
        name,position,look=STATE["shots"][STATE["index"]]
        STATE["camera"].set_actor_location(unreal.Vector(*position),False,False)
        STATE["camera"].set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(unreal.Vector(*position),unreal.Vector(*look)),False)
        STATE["next"]=time.monotonic()+8
    except Exception:
        unreal.unregister_slate_post_tick_callback(STATE["handle"])
        (OUT/"预览错误.txt").write_text(traceback.format_exc(),encoding="utf-8")


def reload_verify():
    """保存并重载后的两个网格缓存、简单碰撞和道路连接均需保留。"""
    unreal.EditorLoadingAndSavingUtils.load_map("/Engine/Maps/Entry")
    world=unreal.EditorLoadingAndSavingUtils.load_map(BASE+"/演示/海堤演示")
    walls=[a for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors() if isinstance(a,unreal.LxSeawallActor)]
    if len(walls)!=1:
        raise RuntimeError("重载海堤数量错误")
    wall=walls[0]
    for key in ("optimized_mesh","wall_optimized_mesh"):
        if not wall.get_editor_property(key).get_editor_property("static_mesh"):
            raise RuntimeError("重载丢失缓存："+key)
    if not wall.get_editor_property("road_connections"):
        raise RuntimeError("重载丢失道路连接")
    for a,b in (((800,0,100),(800,0,-100)),((800,1500,-250),(800,0,-250))):
        if not unreal.SystemLibrary.line_trace_single(world,unreal.Vector(*a),unreal.Vector(*b),
                unreal.TraceTypeQuery.ECC_VISIBILITY,False,[],unreal.DrawDebugTrace.NONE,True):
            raise RuntimeError("重载后丢失路面或堤身碰撞")
    result={"reload":"passed","connections":len(wall.get_editor_property("road_connections")),"collision":"passed"}
    (OUT/"重载验证.json").write_text(json.dumps(result,ensure_ascii=False),encoding="utf-8")
    return result


@unreal.uclass()
class SeawallTools(unreal.ToolsetDefinition):
    """海堤预设的官方 MCP 制作与验收接口。"""
    @unreal.ufunction(static=True,params=[str],ret=str,meta={"AICallable":"","DisplayName":"处理海堤","Category":"海堤|制作"})
    def process_seawall(stage):
        """执行制作、验收、预览、重载或自动化测试。"""
        try:
            if stage=="build":
                result=build_assets()
            elif stage=="verify":
                result=verify_assets()
            elif stage=="preview":
                result=preview()
            elif stage=="reload":
                result=reload_verify()
            elif stage=="tests":
                unreal.SystemLibrary.execute_console_command(None,"Automation RunTests LxARPG.World.Seawall+LxARPG.World.HarborRoad")
                result={"tests_started":True}
            elif stage=="refresh":
                path=Path(unreal.Paths.project_dir()).resolve()/"Source/LxARPGEditor/Scripts/海堤资产工具.py"
                exec(compile(path.read_text(encoding="utf-8").split("@unreal.uclass()")[0],str(path),"exec"),globals())
                result={"refreshed":True}
            else:
                raise ValueError(stage)
            return json.dumps(result,ensure_ascii=False)
        except Exception:
            return json.dumps({"error":traceback.format_exc()},ensure_ascii=False)


unreal.EditorPythonScripting.set_keep_python_script_alive(True)
unreal.ToolsetRegistry.register_toolset_class(SeawallTools)
unreal.SystemLibrary.execute_console_command(None,"ModelContextProtocol.StartServer 8032")
unreal.SystemLibrary.execute_console_command(None,"ModelContextProtocol.RefreshTools")
