"""通过虚幻官方 MCP 创建圆形建筑预设，保留用户的现有关卡。"""
import json
import traceback
import time
from pathlib import Path
import unreal

BASE = "/Game/项目内容/美术资产/海港城市/圆形建筑"
# 离屏捕获的临时状态，不保存或修改用户关卡。
CAPTURE = {}
# 用户常用自动化目录与最初制作目录都需要检查，避免同名预设漏更新。
PRESET_PATHS = ("/Game/项目内容/美术资产/自动化类型/自动建筑/自动圆形建筑", BASE + "/自动圆形建筑")


def inspect_presets():
    """只读检查两处同名预设实际保存的材质引用。"""
    result = []
    for path in PRESET_PATHS:
        bp = unreal.load_asset(path)
        if not bp:
            continue
        defaults = unreal.get_default_object(bp.generated_class())
        entry = {"asset":path}
        for prop in ("wall_material","floor_material","ceiling_material","opening_trim_material"):
            mat = defaults.get_editor_property(prop)
            entry[prop] = mat.get_path_name() if mat else None
        result.append(entry)
    return result


def repair_presets():
    """修复同名旧预设的默认石材，并实例化验证墙面与城垛都使用砖纹。"""
    before = inspect_presets()
    wall = unreal.load_asset(BASE + "/材质/灰白错缝砖墙")
    trim = unreal.load_asset(BASE + "/材质/深灰门窗石框")
    floor = unreal.load_asset(BASE + "/材质/大块浅灰石板地面")
    ceiling = unreal.load_asset(BASE + "/材质/大块灰白石板天花")
    if not wall or not trim:
        raise RuntimeError("缺失圆形建筑专用砖纹或石框材质")
    legacy = "/Game/项目内容/美术资产/海港城市/砖墙/材质/灰白石砖.灰白石砖"
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    results = []
    for path in PRESET_PATHS:
        bp = unreal.load_asset(path)
        if not bp:
            continue
        defaults = unreal.get_default_object(bp.generated_class())
        current = defaults.get_editor_property("wall_material")
        if not current or current.get_path_name() == legacy:
            defaults.set_editor_property("wall_material",wall)
        if not defaults.get_editor_property("opening_trim_material"):
            defaults.set_editor_property("opening_trim_material",trim)
        for prop, material in (("floor_material",floor),("ceiling_material",ceiling)):
            current = defaults.get_editor_property(prop)
            if material and (not current or current.get_path_name()==legacy):
                defaults.set_editor_property(prop,material)
        unreal.BlueprintEditorLibrary.compile_blueprint(bp)
        unreal.EditorAssetLibrary.save_loaded_asset(bp,only_if_is_dirty=False)
        actor = actors.spawn_actor_from_class(bp.generated_class(),unreal.Vector(0,0,-10000))
        try:
            actor.set_editor_property("battlements",True)
            actor.rebuild_building()
            source = actor.get_editor_property("building_mesh")
            if source.get_material(0) != wall or source.get_material(3) != wall:
                raise RuntimeError("墙面或城垛仍未使用砖纹："+path)
            actor.get_editor_property("optimized_mesh").finalize_mesh()
            optimized = actor.get_editor_property("optimized_mesh")
            if optimized.get_material(0) != wall or optimized.get_material(3) != wall:
                raise RuntimeError("静态转换后墙面或城垛材质丢失："+path)
            for slot, prop in ((1,"floor_material"),(2,"ceiling_material")):
                expected = actor.get_editor_property(prop)
                if source.get_material(slot)!=expected or optimized.get_material(slot)!=expected:
                    raise RuntimeError("楼板材质在预览或静态转换时丢失："+path)
            results.append({"asset":path,"wall_and_battlements_verified":True})
        finally:
            actors.destroy_actor(actor)
    return {"before":before,"after":inspect_presets(),"verified":results}


def expression(mat, cls, **properties):
    """创建具有中文说明的材质节点。"""
    node = unreal.MaterialEditingLibrary.create_material_expression(mat, cls)
    for key, value in properties.items():
        node.set_editor_property(key, value)
    return node


def make_surface(name, tint, brick):
    """生成克制的浅灰错缝石砖或略深的门窗石框材质。"""
    path = BASE + "/材质"
    unreal.EditorAssetLibrary.make_directory(path)
    mat = unreal.load_asset(path + "/" + name)
    if not mat:
        mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, path, unreal.Material, unreal.MaterialFactoryNew())
    unreal.MaterialEditingLibrary.delete_all_material_expressions(mat)
    uv = expression(mat, unreal.MaterialExpressionTextureCoordinate, desc="连续圆周砖纹坐标")
    color = expression(mat, unreal.MaterialExpressionVectorParameter, parameter_name="石材颜色",
                       default_value=unreal.LinearColor(*tint, 1), desc="明亮干净的石灰岩色调")
    # 每个纹理周期横向四块、纵向八层，整圈整数周期让错缝砖纹闭合。
    pattern = """
float2 p = UV * float2(4,8);
p.x += fmod(floor(p.y),2)*.5;
float2 cell = floor(p), f = frac(p);
float2 d = min(f,1-f)*float2(50,25);
float edge = min(d.x,d.y);
float aa = max(fwidth(edge),.08);
float stone = smoothstep(.55-aa,.95+aa,edge);
float variation = sin(dot(cell,float2(12.9898,78.233)))*.035;
float grain = sin(UV.x*181+sin(UV.y*127)) * sin(UV.y*211)*.008;
"""
    code = pattern + "return lerp(C.rgb*.61,C.rgb*(1+variation+grain),stone);" if brick else "return C.rgb*(1+.012*sin(UV.x*73)*sin(UV.y*91));"
    shade = expression(mat, unreal.MaterialExpressionCustom, code=code,
                       output_type=unreal.CustomMaterialOutputType.CMOT_FLOAT3, description="规整错缝浅灰石砖" if brick else "略深的哑光石框")
    inputs = []
    for name in ("UV", "C"):
        entry = unreal.CustomInput()
        entry.set_editor_property("input_name", name)
        inputs.append(entry)
    shade.set_editor_property("inputs", inputs)
    unreal.MaterialEditingLibrary.connect_material_expressions(uv, "", shade, "UV")
    unreal.MaterialEditingLibrary.connect_material_expressions(color, "", shade, "C")
    unreal.MaterialEditingLibrary.connect_material_property(shade, "", unreal.MaterialProperty.MP_BASE_COLOR)
    if brick:
        normal = expression(mat, unreal.MaterialExpressionCustom,
                            code=pattern + "float2 bevel = (1-smoothstep(.8,2.0,d))*sign(f-.5)*.22; return normalize(float3(bevel,1));",
                            output_type=unreal.CustomMaterialOutputType.CMOT_FLOAT3, description="浅砖缝与柔和石砖倒角")
        normal.set_editor_property("inputs", inputs[:1])
        unreal.MaterialEditingLibrary.connect_material_expressions(uv, "", normal, "UV")
        unreal.MaterialEditingLibrary.connect_material_property(normal, "", unreal.MaterialProperty.MP_NORMAL)
    for prop, value, description in ((unreal.MaterialProperty.MP_ROUGHNESS,.84,"哑光石材"),
                                    (unreal.MaterialProperty.MP_SPECULAR,.22,"克制高光")):
        node = expression(mat, unreal.MaterialExpressionConstant, r=value, desc=description)
        unreal.MaterialEditingLibrary.connect_material_property(node, "", prop)
    unreal.MaterialEditingLibrary.layout_material_expressions(mat)
    unreal.MaterialEditingLibrary.recompile_material(mat)
    unreal.EditorAssetLibrary.save_loaded_asset(mat, only_if_is_dirty=False)
    return mat


def make_slab_surface(name, tint, roughness):
    """制作大块错缝石板，具有浅灰缝、柔和倒角和克制的逐块色差。"""
    path = BASE + "/材质"
    unreal.EditorAssetLibrary.make_directory(path)
    mat = unreal.load_asset(path + "/" + name)
    if not mat:
        mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name,path,unreal.Material,unreal.MaterialFactoryNew())
    unreal.MaterialEditingLibrary.delete_all_material_expressions(mat)
    uv = expression(mat,unreal.MaterialExpressionTextureCoordinate,desc="按实际尺寸连续铺设的大石板坐标")
    color = expression(mat,unreal.MaterialExpressionVectorParameter,parameter_name="石板颜色",
                       default_value=unreal.LinearColor(*tint,1),desc="干净的浅灰石灰岩")
    parameters = {"UV":uv,"C":color}
    for key,label,value in (("Length","石板长度",120.),("Width","石板宽度",90.),
                            ("Joint","石板缝宽",1.4),("Bevel","石板倒角宽度",1.6)):
        parameters[key] = expression(mat,unreal.MaterialExpressionScalarParameter,
                                     parameter_name=label,default_value=value,desc=label+"（厘米）")
    pattern = """
float2 size = max(float2(Length,Width),float2(10,10));
float2 p = UV*200/size;
p.x += fmod(floor(p.y),2)*.5;
float2 cell = floor(p), f = frac(p);
float2 d = min(f,1-f)*size;
float edge = min(d.x,d.y);
float halfJoint = max(.1,Joint*.5);
float aa = max(fwidth(edge),.08);
float stone = smoothstep(halfJoint-aa,halfJoint+aa,edge);
float id = frac(sin(dot(cell,float2(127.1,311.7)))*43758.5453);
float variation = (id-.5)*.12;
float mineral = sin(UV.x*17+id*8+sin(UV.y*13))*sin(UV.y*19+id*11)*.012;
float grain = sin(UV.x*371+id*14)*sin(UV.y*293)*.004;
"""
    # 同一组石板边界同时驱动颜色、倒角法线和缝内遮蔽。
    outputs = ((unreal.MaterialProperty.MP_BASE_COLOR,unreal.CustomMaterialOutputType.CMOT_FLOAT3,
                "return lerp(C.rgb*.54,C.rgb*(1+variation+mineral+grain),stone);","大石板与细灰缝"),
               (unreal.MaterialProperty.MP_NORMAL,unreal.CustomMaterialOutputType.CMOT_FLOAT3,
                "float2 slope=(1-smoothstep(halfJoint,halfJoint+max(.1,Bevel),d))*sign(f-.5)*.18; return normalize(float3(slope,1));","石板边缘的浅倒角"),
               (unreal.MaterialProperty.MP_AMBIENT_OCCLUSION,unreal.CustomMaterialOutputType.CMOT_FLOAT1,
                "return lerp(.78,1,stone);","细灰缝的轻微遮蔽"))
    for prop,output_type,code,description in outputs:
        node = expression(mat,unreal.MaterialExpressionCustom,code=pattern+code,
                          output_type=output_type,description=description)
        inputs = []
        for key in parameters:
            entry = unreal.CustomInput()
            entry.set_editor_property("input_name",key)
            inputs.append(entry)
        node.set_editor_property("inputs",inputs)
        for key,source in parameters.items():
            if not unreal.MaterialEditingLibrary.connect_material_expressions(source,"",node,key):
                raise RuntimeError("石板材质节点连接失败："+key)
        if not unreal.MaterialEditingLibrary.connect_material_property(node,"",prop):
            raise RuntimeError("石板材质输出连接失败")
    for prop,value,label in ((unreal.MaterialProperty.MP_ROUGHNESS,roughness,"石板哑光粗糙度"),
                             (unreal.MaterialProperty.MP_SPECULAR,.24,"柔和石质高光")):
        node = expression(mat,unreal.MaterialExpressionConstant,r=value,desc=label)
        unreal.MaterialEditingLibrary.connect_material_property(node,"",prop)
    unreal.MaterialEditingLibrary.layout_material_expressions(mat)
    unreal.MaterialEditingLibrary.recompile_material(mat)
    unreal.EditorAssetLibrary.save_loaded_asset(mat,only_if_is_dirty=False)
    return mat


def build_slab_materials():
    """创建地板和天花板大石板材质，并同步更新两个目录的预设。"""
    make_slab_surface("大块浅灰石板地面",(.50,.515,.50),.76)
    make_slab_surface("大块灰白石板天花",(.61,.615,.59),.83)
    return repair_presets()


def build_asset():
    """更新中文预设的砖墙及石框，验证实例化和延迟静态转换。"""
    wall = make_surface("灰白错缝砖墙", (.62,.625,.60), True)
    trim = make_surface("深灰门窗石框", (.39,.415,.42), False)
    path = BASE + "/自动圆形建筑"
    existing = unreal.EditorAssetLibrary.does_asset_exist(path)
    if existing:
        bp = unreal.load_asset(path)
    else:
        factory = unreal.BlueprintFactory()
        factory.set_editor_property("parent_class", unreal.LxRoundBuildingActor)
        bp = unreal.AssetToolsHelpers.get_asset_tools().create_asset("自动圆形建筑", BASE, unreal.Blueprint, factory)
    defaults = unreal.get_default_object(bp.generated_class())
    defaults.set_editor_property("wall_material", wall)
    defaults.set_editor_property("opening_trim_material", trim)
    if not existing:
        floor = make_slab_surface("大块浅灰石板地面",(.50,.515,.50),.76)
        ceiling = make_slab_surface("大块灰白石板天花",(.61,.615,.59),.83)
        defaults.set_editor_property("floor_material", floor)
        defaults.set_editor_property("ceiling_material", ceiling)
    bp.set_editor_property("run_construction_script_on_drag", False)
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    unreal.EditorAssetLibrary.save_loaded_asset(bp, only_if_is_dirty=False)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    actor = actors.spawn_actor_from_class(bp.generated_class(), unreal.Vector(0, 0, -10000))
    try:
        actor.add_door_opening()
        actor.add_window_opening()
        diagnostic = actor.get_editor_property("generation_diagnostic")
        if diagnostic:
            raise RuntimeError(diagnostic)
        optimized = actor.get_editor_property("optimized_mesh")
        pending = optimized.get_editor_property("pending_mesh_build")
        optimized.finalize_mesh()
        mesh = optimized.get_editor_property("static_mesh")
        if not mesh:
            raise RuntimeError("圆形建筑静态网格转换失败")
        return {"asset": bp.get_path_name(), "created": not existing, "openings": len(actor.get_editor_property("openings")),
                "deferred_before_finalize": pending, "static_mesh_ready": True,
                "material_slots": len(mesh.get_editor_property("static_materials")),
                "wall_material": wall.get_path_name(), "trim_material": trim.get_path_name(),
                "ceiling_follows_wall": actor.get_editor_property("ceiling_follows_wall")}
    finally:
        actors.destroy_actor(actor)


def capture_preview():
    """在独立空白世界中离屏验证砖墙、矩形门窗和凸起石框。"""
    world = unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    # 验证用户实际使用的自动化目录预设，避免只检查同名制作副本。
    bp = unreal.load_asset(PRESET_PATHS[0])
    actor = actors.spawn_actor_from_class(bp.generated_class(), unreal.Vector())
    actor.set_editor_property("battlements",True)
    actor.add_door_opening()
    actor.add_window_opening()
    openings = actor.get_editor_property("openings")
    window = openings[1]
    window.set_editor_property("position", unreal.Vector(350,-350,120))
    openings[1] = window
    actor.set_editor_property("openings", openings)
    actor.rebuild_building()
    if actor.get_editor_property("generation_diagnostic"):
        raise RuntimeError(actor.get_editor_property("generation_diagnostic"))
    actor.get_editor_property("optimized_mesh").finalize_mesh()
    ground = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(0,0,-10))
    ground.static_mesh_component.set_static_mesh(unreal.load_asset("/Engine/BasicShapes/Cube"))
    ground.static_mesh_component.set_material(0,unreal.load_asset("/Engine/BasicShapes/BasicShapeMaterial"))
    ground.set_actor_scale3d(unreal.Vector(60,60,.2))
    for rotation, intensity in ((unreal.Rotator(pitch=-45,yaw=145),8.0),(unreal.Rotator(pitch=-50,yaw=-45),2.5)):
        light = actors.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0,0,1000), rotation)
        component = light.get_component_by_class(unreal.DirectionalLightComponent)
        component.set_mobility(unreal.ComponentMobility.MOVABLE)
        component.set_editor_property("intensity", intensity)
        if intensity>5:
            component.set_editor_property("atmosphere_sun_light",True)
    actors.spawn_actor_from_class(unreal.SkyAtmosphere,unreal.Vector())
    sky = actors.spawn_actor_from_class(unreal.SkyLight,unreal.Vector(0,0,1000))
    sky_component = sky.get_component_by_class(unreal.SkyLightComponent)
    sky_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    sky_component.set_editor_property("real_time_capture",True)
    camera = actors.spawn_actor_from_class(unreal.SceneCapture2D, unreal.Vector(1300,-1000,850))
    camera.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(camera.get_actor_location(),unreal.Vector(0,0,200)),False)
    component = camera.get_component_by_class(unreal.SceneCaptureComponent2D)
    component.set_editor_property("fov_angle",55.0)
    target = unreal.RenderingLibrary.create_render_target2d(world,1400,1000,unreal.TextureRenderTargetFormat.RTF_RGBA8)
    component.set_editor_property("texture_target",target)
    component.set_editor_property("capture_source",unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR)
    component.set_editor_property("capture_every_frame",False)
    component.set_editor_property("always_persist_rendering_state",True)
    settings = component.get_editor_property("post_process_settings")
    for key,value in {"override_auto_exposure_min_brightness":True,"override_auto_exposure_max_brightness":True,
                      "auto_exposure_min_brightness":1.0,"auto_exposure_max_brightness":1.0}.items():
        settings.set_editor_property(key,value)
    component.set_editor_property("post_process_settings",settings)
    unreal.AutomationLibrary.finish_loading_before_screenshot()
    out = Path(unreal.Paths.project_dir()).resolve()/"Saved/RoundBuildingPreview"
    out.mkdir(parents=True,exist_ok=True)
    CAPTURE.update(world=world,actor=actor,camera=camera,component=component,target=target,out=out,phase=0,next=time.monotonic()+10)
    CAPTURE["handle"] = unreal.register_slate_post_tick_callback(capture_tick)
    return {"preview_started":True,"actor_class":actor.get_class().get_path_name(),
            "wall_material":actor.get_editor_property("wall_material").get_path_name(),
            "trim_material":actor.get_editor_property("opening_trim_material").get_path_name()}


def capture_tick(delta):
    """分别验证天花板顶面、地板及室内天花板底面的大石板纹理。"""
    try:
        CAPTURE["component"].capture_scene()
        if time.monotonic()<CAPTURE["next"]:
            return
        phase = CAPTURE["phase"]
        name = ("圆形建筑.png","大石板地板.png","大石板天花.png")[phase]
        unreal.RenderingLibrary.export_render_target(CAPTURE["world"],CAPTURE["target"],str(CAPTURE["out"]),name)
        if phase==2:
            (CAPTURE["out"]/"完成.json").write_text('{"rendered":true,"views":3}',encoding="utf-8")
            unreal.unregister_slate_post_tick_callback(CAPTURE["handle"])
            return
        actor = CAPTURE["actor"]
        actor.set_editor_property("ceiling",phase==1)
        actor.rebuild_building()
        actor.get_editor_property("optimized_mesh").finalize_mesh()
        if phase==0:
            position, look = unreal.Vector(800,-650,1500),unreal.Vector(0,0,30)
        else:
            position, look = unreal.Vector(0,-220,65),unreal.Vector(0,120,300)
            CAPTURE["component"].set_editor_property("fov_angle",95.0)
            light = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).spawn_actor_from_class(unreal.PointLight,unreal.Vector(0,0,170))
            component = light.get_component_by_class(unreal.PointLightComponent)
            component.set_mobility(unreal.ComponentMobility.MOVABLE)
            component.set_editor_property("intensity_units",unreal.LightUnits.LUMENS)
            component.set_editor_property("intensity",2500.0)
            component.set_editor_property("attenuation_radius",2000.0)
            component.set_editor_property("cast_shadows",False)
        CAPTURE["camera"].set_actor_location(position,False,False)
        CAPTURE["camera"].set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(position,look),False)
        CAPTURE["phase"] += 1
        CAPTURE["next"] = time.monotonic()+10
    except Exception:
        unreal.unregister_slate_post_tick_callback(CAPTURE["handle"])
        (CAPTURE["out"]/"错误.txt").write_text(traceback.format_exc(),encoding="utf-8")


@unreal.uclass()
class RoundBuildingTools(unreal.ToolsetDefinition):
    """圆形建筑的官方 MCP 资产入口。"""
    @unreal.ufunction(static=True, params=[], ret=str, meta={"AICallable": "", "DisplayName": "制作大石板楼板材质", "Category": "圆形建筑|制作"})
    def build_round_slabs():
        """为地板和天花板配置有清晰分块的大石板纹理。"""
        try:
            return json.dumps(build_slab_materials(),ensure_ascii=False)
        except Exception:
            return json.dumps({"error":traceback.format_exc()},ensure_ascii=False)

    @unreal.ufunction(static=True, params=[], ret=str, meta={"AICallable": "", "DisplayName": "修复圆形建筑预设材质", "Category": "圆形建筑|制作"})
    def repair_round_building():
        """更新常用目录的旧材质，保留预设尺寸及其他功能参数。"""
        try:
            return json.dumps(repair_presets(),ensure_ascii=False)
        except Exception:
            return json.dumps({"error":traceback.format_exc()},ensure_ascii=False)

    @unreal.ufunction(static=True, params=[], ret=str, meta={"AICallable": "", "DisplayName": "检查圆形建筑材质", "Category": "圆形建筑|制作"})
    def inspect_round_building():
        """读取常用预设与制作预设的持久化材质配置。"""
        try:
            return json.dumps(inspect_presets(),ensure_ascii=False)
        except Exception:
            return json.dumps({"error":traceback.format_exc()},ensure_ascii=False)

    @unreal.ufunction(static=True, params=[], ret=str, meta={"AICallable": "", "DisplayName": "预览圆形建筑", "Category": "圆形建筑|制作"})
    def preview_round_building():
        """启动独立离屏预览，不操作计算机屏幕。"""
        try:
            return json.dumps(capture_preview(),ensure_ascii=False)
        except Exception:
            return json.dumps({"error":traceback.format_exc()},ensure_ascii=False)

    @unreal.ufunction(static=True, params=[], ret=str, meta={"AICallable": "", "DisplayName": "创建圆形建筑预设", "Category": "圆形建筑|制作"})
    def build_round_building():
        """创建并验证可直接放置的圆形建筑。"""
        try:
            return json.dumps(build_asset(), ensure_ascii=False)
        except Exception:
            return json.dumps({"error": traceback.format_exc()}, ensure_ascii=False)


unreal.EditorPythonScripting.set_keep_python_script_alive(True)
unreal.ToolsetRegistry.register_toolset_class(RoundBuildingTools)
unreal.SystemLibrary.execute_console_command(None, "ModelContextProtocol.StartServer 8022")
unreal.SystemLibrary.execute_console_command(None, "ModelContextProtocol.RefreshTools")
