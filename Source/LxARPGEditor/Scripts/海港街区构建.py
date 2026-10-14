"""主关卡海港街区：米制布置、现有自动化构件、独立石质地面。"""
import json
import math
import time
from pathlib import Path
import unreal

# 主关卡和输出目录；布局数据统一使用米且保留两位小数。
LEVEL = "/Game/项目内容/关卡/唯一关卡"
OUT = Path(unreal.Paths.project_dir()).resolve() / "Saved/HarborDistrict"
OUT.mkdir(parents=True, exist_ok=True)
# 离屏相机状态在官方 MCP 各次调用之间保留。
STATE = globals().get("STATE", {})
# 本次街区的对象标记、素材路径和米制基础尺寸。
TAG = "海港山侧街区_20261013"
AUTO = "/Game/项目内容/美术资产/自动化类型/"
HARBOR = "/Game/项目内容/美术资产/海港城市/"
MATERIALS = HARBOR + "山侧街区/材质/"
ASSETS = globals().get("ASSETS", {})
STORY_HEIGHT = 3.40
AVENUE_YAW = math.degrees(math.atan2(302.04-12.89, 399.76-448.30))


def vec_m(value):
    """把引擎向量换算成保留两位小数的米制数组。"""
    return [round(float(value.x) / 100.0, 2), round(float(value.y) / 100.0, 2), round(float(value.z) / 100.0, 2)]


def world():
    """仅允许处理指定主关卡。"""
    current = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    if current.get_path_name().split(".")[0] != LEVEL:
        raise RuntimeError("当前关卡不是唯一关卡，已停止：" + current.get_path_name())
    return current


def inspect():
    """导出已有道路、地面、塔楼和场景构件的坐标及材质。"""
    current = world()
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
    rows = []
    for actor in actors:
        center, extent = actor.get_actor_bounds(False)
        row = {"name": actor.get_name(), "label": actor.get_actor_label(),
               "class": actor.get_class().get_path_name(), "location_m": vec_m(actor.get_actor_location()),
               "rotation": str(actor.get_actor_rotation()), "scale": str(actor.get_actor_scale3d()),
               "bounds_m": [vec_m(center), vec_m(extent)]}
        for key in ("road_width", "terrace_height", "length_meters", "width_meters"):
            try:
                row[key] = actor.get_editor_property(key)
            except Exception:
                pass
        for key in ("road_material", "curb_material", "surface_material", "paving_material", "stone_material"):
            try:
                asset = actor.get_editor_property(key)
                row[key] = asset.get_path_name() if asset else None
            except Exception:
                pass
        splines = actor.get_components_by_class(unreal.SplineComponent)
        if splines:
            row["splines"] = [{"name": s.get_name(), "closed": s.is_closed_loop(),
                              "points_m": [vec_m(s.get_location_at_spline_point(i, unreal.SplineCoordinateSpace.WORLD))
                                           for i in range(s.get_number_of_spline_points())]}
                             for s in splines]
        rows.append(row)
    (OUT / "原场景清单.json").write_text(json.dumps(rows, ensure_ascii=False, indent=2), encoding="utf-8")
    return {"world": current.get_path_name(), "actors": len(rows), "data": str(OUT / "原场景清单.json")}


def process(stage):
    """按指定阶段检查或制作街区。"""
    if stage.startswith("lamps_"):
        lamp_script = Path(unreal.Paths.project_dir()).resolve() / "Source/LxARPGEditor/Scripts/街区路灯布置.py"
        exec(compile(lamp_script.read_text(encoding="utf-8-sig"), str(lamp_script), "exec"), globals())
        return process_lamps(stage)
    if stage == "inspect":
        return inspect()
    if stage == "survey":
        return survey()
    if stage == "assets":
        return inspect_assets()
    if stage == "plan":
        return plan()
    if stage == "grounds":
        return build_grounds()
    if stage.startswith("houses_"):
        return build_houses(int(stage.split("_")[1]))
    if stage == "roads":
        return build_roads()
    if stage == "finish_ground":
        return finish_ground()
    if stage == "diagnose_roads":
        return diagnose_roads()
    if stage == "organize":
        return organize()
    if stage == "summary":
        return completion_summary()
    if stage == "validate":
        return validate()
    if stage == "save":
        return save()
    if stage == "reload":
        return reload_check()
    if stage == "capture_overview":
        return capture("街区总览", [555.78, -53.41, 207.52], [396.42, 38.60, -33.52], 90.00)
    if stage == "capture_shops":
        return capture("高塔商业街", [490.00, -40.00, 25.00], [420.00, 15.00, -27.00], 65.00)
    if stage == "capture_houses":
        return capture("住宅与内街", [460.00, 156.00, 32.00], [393.00, 126.00, -29.00], 65.00)
    if stage == "capture_before":
        return capture("参考视角", [555.78, -53.41, 207.52], [396.42, 38.60, -33.52], 90.00)
    if stage == "quit":
        unreal.SystemLibrary.quit_editor()
        return {"quit": True}
    raise ValueError("未知制作阶段：" + stage)


def survey():
    """取得主关卡相机、地形采样接口及实际灯光强度。"""
    current = world()
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
    land = next(a for a in actors if isinstance(a, unreal.Landscape))
    info = {"landscape_methods": [s for s in dir(land) if "height" in s or "landscape" in s],
            "camera": str(unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_level_viewport_camera_info()),
            "trace_doc": unreal.SystemLibrary.line_trace_single.__doc__,
            "hit_fields": dir(unreal.HitResult())}
    info["lights"] = [{"actor": a.get_actor_label(), "component": c.get_name(),
                       "intensity": c.get_editor_property("intensity")}
                      for a in actors for c in a.get_components_by_class(unreal.DirectionalLightComponent)]
    rows = []
    for x in range(250, 461, 10):
        for y in range(-60, 301, 10):
            hit = unreal.SystemLibrary.line_trace_single(current, unreal.Vector(x*100, y*100, 20000),
                  unreal.Vector(x*100, y*100, -10000), unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,
                  False, [], unreal.DrawDebugTrace.NONE, True)
            if hit:
                rows.append({"x": x, "y": y, "z": vec_m(hit.to_tuple()[4])[2], "details": str(hit) if not rows else ""})
    (OUT / "地形采样.json").write_text(json.dumps(rows, ensure_ascii=False), encoding="utf-8")
    info["samples"] = len(rows)
    return info


def capture(name, position, look, fov):
    """使用游戏世界离屏渲染图片，不操作屏幕或编辑器窗口。"""
    current = world()
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    pos, target_pos = unreal.Vector(*[x*100 for x in position]), unreal.Vector(*[x*100 for x in look])
    camera = actors.spawn_actor_from_class(unreal.SceneCapture2D, pos,
             unreal.MathLibrary.find_look_at_rotation(pos, target_pos))
    camera.set_actor_label("临时街区验收相机")
    component = camera.get_component_by_class(unreal.SceneCaptureComponent2D)
    component.set_editor_property("capture_every_frame", False)
    component.set_editor_property("always_persist_rendering_state", True)
    component.set_editor_property("fov_angle", fov)
    target = unreal.RenderingLibrary.create_render_target2d(current, 1920, 1200, unreal.TextureRenderTargetFormat.RTF_RGBA8)
    component.set_editor_property("texture_target", target)
    component.set_editor_property("capture_source", unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR)
    settings = component.get_editor_property("post_process_settings")
    for key, value in {"override_auto_exposure_method": True, "auto_exposure_method": unreal.AutoExposureMethod.AEM_HISTOGRAM,
                       "override_auto_exposure_bias": True, "auto_exposure_bias": 1.00,
                       "override_dynamic_global_illumination_method": True,
                       "dynamic_global_illumination_method": unreal.DynamicGlobalIlluminationMethod.LUMEN,
                       "override_reflection_method": True, "reflection_method": unreal.ReflectionMethod.LUMEN}.items():
        settings.set_editor_property(key, value)
    component.set_editor_property("post_process_settings", settings)
    STATE.update(world=current, camera=camera, component=component, target=target, name=name, next=time.monotonic()+10.00)
    STATE["handle"] = unreal.register_slate_post_tick_callback(capture_tick)
    return {"capture_started": name}


def capture_tick(delta):
    """多帧收敛后保存渲染结果，随后销毁临时摄影机。"""
    try:
        STATE["component"].capture_scene()
        if time.monotonic() < STATE["next"]:
            return
        unreal.RenderingLibrary.export_render_target(STATE["world"], STATE["target"], str(OUT), STATE["name"]+".png")
    finally:
        if time.monotonic() >= STATE["next"]:
            unreal.unregister_slate_post_tick_callback(STATE["handle"])
            unreal.get_editor_subsystem(unreal.EditorActorSubsystem).destroy_actor(STATE["camera"])


def inspect_assets():
    """读取现有窗、绿化和店铺道具的真实包围盒，避免比例错误。"""
    paths = list(unreal.EditorAssetLibrary.list_assets("/Game/项目内容/美术资产/海港城市/首批模型/模型", True, False))
    paths += list(unreal.EditorAssetLibrary.list_assets("/Game/项目内容/美术资产/海港城市/第二批模型/模型", True, False))
    rows = []
    for path in paths:
        asset = unreal.load_asset(path)
        if isinstance(asset, unreal.StaticMesh):
            bounds = asset.get_bounds()
            rows.append({"path": path, "center": vec_m(bounds.origin), "extent": vec_m(bounds.box_extent)})
    return rows


def asset(path):
    """缓存并加载现有资产，丢失素材时停止制作。"""
    if path not in ASSETS:
        ASSETS[path] = unreal.load_asset(path)
    if not ASSETS[path]:
        raise RuntimeError("资产无法加载：" + path)
    return ASSETS[path]


def vector(position):
    """米制数据仅在引擎接口边界转换成厘米。"""
    return unreal.Vector(*[round(float(v), 2)*100.0 for v in position])


def offset(origin, yaw, x, y, z=0.00):
    """从建筑或街道局部米制坐标计算世界位置。"""
    angle = math.radians(yaw)
    return [round(origin[0]+math.cos(angle)*x-math.sin(angle)*y, 2),
            round(origin[1]+math.sin(angle)*x+math.cos(angle)*y, 2), round(origin[2]+z, 2)]


def definitions():
    """定义二十一栋不同体量的商铺和住宅，数据单位均为米。"""
    rows = []
    main = [(24.00,"药剂店",14.00,11.00,2,"横脊","药剂"),
            (47.00,"材料店",16.00,12.00,2,"侧翼","材料"),
            (69.00,"织衣铺",13.00,10.00,3,"退台","布匹"),
            (106.00,"海风居",13.00,12.00,2,"横脊",""),
            (131.00,"石榴居",16.00,11.00,3,"侧翼",""),
            (155.00,"白檐居",14.00,12.00,2,"低坡",""),
            (177.00,"双窗居",12.00,10.00,2,"横脊",""),
            (206.00,"长庭居",16.00,12.00,2,"侧翼",""),
            (231.00,"晴湾居",14.00,11.00,3,"退台",""),
            (253.00,"北街居",12.00,10.00,2,"低坡","")]
    quay = [(143.00,"装备店",15.00,12.00,3,"退台","装备"),
            (118.00,"杂货店",16.00,11.00,2,"侧翼","材料"),
            (91.00,"旅人客栈",18.00,14.00,3,"横脊","旅店"),
            (64.00,"面包坊",14.00,11.00,2,"低坡","面包"),
            (39.00,"行会居",16.00,12.00,3,"侧翼",""),
            (16.00,"东街居",12.00,10.00,2,"横脊","")]
    inner = [(42.00,"庭院居",12.00,10.00,2,"侧翼",""),
             (65.00,"木窗居",13.00,10.00,2,"低坡",""),
             (112.00,"书香居",14.00,11.00,3,"退台",""),
             (138.00,"花窗居",12.00,10.00,2,"横脊",""),
             (164.00,"浅石居",14.00,11.00,2,"侧翼","")]
    for group, specs, origin, yaw, front in (
            ("主街", main, [448.30,12.89,-33.32], AVENUE_YAW, 11.00),
            ("港街", quay, [316.43,-76.15,-33.32], 30.00, 11.00),
            ("内街", inner, [448.30,12.89,-33.32], AVENUE_YAW, 55.00)):
        for station, name, width, depth, floors, style, shop in specs:
            rows.append({"name":name,"group":group,"station_m":station,"width_m":width,"depth_m":depth,
                         "floors":floors,"style":style,"shop":shop,"yaw":yaw,
                         "origin_m":offset(origin,yaw,station-width/2.00,front),"top_m":-33.32})
    return rows


def terrain_height(position, ignore):
    """只对原地形取样，忽略道路与所有制作对象。"""
    hit = unreal.SystemLibrary.line_trace_single(world(), vector([position[0],position[1],150.00]),
          vector([position[0],position[1],-80.00]), unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,
          False, ignore, unreal.DrawDebugTrace.NONE, True)
    if not hit:
        raise RuntimeError("未找到地形："+str(position))
    return vec_m(hit.to_tuple()[4])[2]


def plan():
    """在施工前检查独立地面各角高度，拒绝进入山坡。"""
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
    ignore = [a for a in actors if not isinstance(a, unreal.LandscapeProxy)]
    rows = definitions()
    for b in rows:
        w,d = b["width_m"],b["depth_m"]
        points = [offset(b["origin_m"],b["yaw"],x,y) for x in (-2.00,w+2.00) for y in (-4.60,d+2.00)]
        heights = [terrain_height(p,ignore) for p in points]
        if max(heights)>-32.70 or max(heights)-min(heights)>0.70:
            raise RuntimeError("建筑地块进入山坡，需重新布置："+b["name"]+str(heights))
        top = round(max(-33.32,max(heights)+0.16),2)
        b.update(top_m=top, terrain_min_m=min(heights),terrain_max_m=max(heights),footprint_m=points)
        b["origin_m"][2] = top
    (OUT/"街区布局.json").write_text(json.dumps(rows,ensure_ascii=False,indent=2),encoding="utf-8")
    return {"buildings":len(rows),"shops":sum(bool(b["shop"]) for b in rows),
            "lots":[{"name":b["name"],"terrain":[b["terrain_min_m"],b["terrain_max_m"]],"top":b["top_m"]} for b in rows]}


def layout():
    """读取已通过地形校验的布置清单。"""
    return json.loads((OUT/"街区布局.json").read_text(encoding="utf-8"))


def actors_with_tag():
    """取得本次制作对象，不按模糊名称处理原有场景。"""
    return [a for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
            if TAG in [str(t) for t in a.tags]]


def spawn(cls, name, position, yaw, folder, props=None, parent=None, pitch=0.00, roll=0.00):
    """生成带中文名称、独立文件夹和本次标签的可编辑构件。"""
    actor = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).spawn_actor_from_class(
        cls, vector(position), unreal.Rotator(pitch=pitch,yaw=yaw,roll=roll))
    actor.set_actor_label(name)
    actor.set_folder_path("山侧街区/"+folder)
    actor.set_editor_property("tags",[unreal.Name(TAG)])
    if props:
        actor.set_editor_properties(props)
    if parent:
        # 静态装饰只能附加到静态根组件；编辑器内仍可整体移动台地和房屋。
        parent.root_component.set_mobility(unreal.ComponentMobility.STATIC)
        actor.attach_to_actor(parent,"",unreal.AttachmentRule.KEEP_WORLD,unreal.AttachmentRule.KEEP_WORLD,
                              unreal.AttachmentRule.KEEP_WORLD,False)
    return actor


def preset(path, name, position, yaw, folder, props=None, parent=None):
    """使用项目现有的自动化蓝图预设创建构件。"""
    return spawn(asset(AUTO+path).generated_class(),name,position,yaw,folder,props,parent)


def build_grounds():
    """先铺设独立石质台地，再允许摆放建筑。"""
    existing = {a.get_actor_label() for a in actors_with_tag()}
    made = []
    for b in layout():
        name=b["name"]+"地面"
        if name in existing:
            continue
        w,d=b["width_m"],b["depth_m"]
        position=offset(b["origin_m"],b["yaw"],w/2.00,(d-2.60)/2.00,-0.75)
        ground=preset("台地/自动台地",name,position,b["yaw"],b["name"],
              {"auto_connect":False,"terrace_height":75.00,"border_height":0.00,"border_width":25.00,
               "corner_radius":0.00,"brick_size":unreal.Vector2D(150.00,100.00)})
        ground.set_footprint_meters(w+4.00,d+6.60)
        ground.rebuild_terrace()
        made.append(name)
    return {"grounds":made}


def colored_material(name,color):
    """制作克制的商铺布面与金属标识颜色，并保存中文材质资产。"""
    path=MATERIALS+name
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        return asset(path)
    mat=unreal.AssetToolsHelpers.get_asset_tools().create_asset(name,MATERIALS.rstrip("/"),unreal.Material,unreal.MaterialFactoryNew())
    node=unreal.MaterialEditingLibrary.create_material_expression(mat,unreal.MaterialExpressionConstant3Vector)
    node.set_editor_property("constant",unreal.LinearColor(*color,1.00))
    node.set_editor_property("desc","清爽的街区配色")
    unreal.MaterialEditingLibrary.connect_material_property(node,"",unreal.MaterialProperty.MP_BASE_COLOR)
    rough=unreal.MaterialEditingLibrary.create_material_expression(mat,unreal.MaterialExpressionConstant)
    rough.set_editor_property("r",0.82)
    rough.set_editor_property("desc","哑光布面与招牌")
    unreal.MaterialEditingLibrary.connect_material_property(rough,"",unreal.MaterialProperty.MP_ROUGHNESS)
    unreal.MaterialEditingLibrary.recompile_material(mat)
    unreal.EditorAssetLibrary.save_loaded_asset(mat,only_if_is_dirty=False)
    ASSETS[path]=mat
    return mat


def part(b,kind,name,point,props=None,relative_yaw=0.00):
    """按建筑局部米制坐标摆放自动化构件。"""
    return preset(kind,b["name"]+name,offset(b["origin_m"],b["yaw"],*point),b["yaw"]+relative_yaw,
                  b["name"],props,b["parent"])


def wall(b,name,x,y,z,length,height,yaw=0.00,thickness=0.36):
    """用自动砖墙构建直线墙段，入口通过左右墙与过梁留出真实门洞。"""
    actor=part(b,"墙面/无柱砖墙",name,[x,y,z],
          {"length_handle":vector([length,0.00,0.00]),"wall_height":height*100.00,
           "wall_width":thickness*100.00,"brick_length":90.00,"brick_height":35.00,
           "weathering":0.02,"color_variation":0.05},yaw)
    actor.rebuild_wall()
    return actor


def mesh_part(b,path,name,point,scale=(1.00,1.00,1.00),yaw=0.00,material=None,pitch=0.00,roll=0.00):
    """为房屋添加复用的窗、花坛和小道具，保持原素材比例。"""
    actor=spawn(unreal.StaticMeshActor,b["name"]+name,offset(b["origin_m"],b["yaw"],*point),
                b["yaw"]+yaw,b["name"],parent=b["parent"],pitch=pitch,roll=roll)
    actor.static_mesh_component.set_static_mesh(asset(path))
    actor.set_actor_scale3d(unreal.Vector(*scale))
    if material:
        actor.static_mesh_component.set_material(0,material)
    return actor


def slab(b,name,x,y,z,width,depth):
    """以自动木地板形成独立楼板，维持建筑内部可站立的地面。"""
    actor=part(b,"地板/自动木地板",name,[x,y,z],
          {"size_handle":vector([width,depth,0.00]),"thickness":18.00,"plank_length":300.00,"plank_width":40.00})
    actor.rebuild_panel()
    return actor


def roof(b,name,x,y,z,width,depth,single=False):
    """以现有瓦屋顶生成低坡屋面，石质山墙向下封闭。"""
    actor=part(b,"屋顶/自动瓦屋顶" if single else "屋顶/双坡瓦屋顶",name,[x-0.28,y-0.28,z],
          {"size_handle":vector([width+0.56,depth+0.56,0.00]),"slope_angle":10.00 if single else 16.00,
           "side_wall_drop":8.00,"tile_width":65.00,"tile_length":80.00,"top_height":25.00})
    actor.rebuild_roof()
    return actor


def body(b,name,x,y,z,width,depth,floors,door=True):
    """构建有门洞、楼板、水平檐线和窗户的完整直角房屋体块。"""
    height=STORY_HEIGHT*floors
    door_width=1.80 if b["shop"] else 1.40
    door_height=2.65
    if door:
        half=(width-door_width)/2.00
        wall(b,name+"左前墙",x,y,z,half,height)
        wall(b,name+"右前墙",x+half+door_width,y,z,half,height)
        wall(b,name+"门上墙",x+half,y,z+door_height,door_width,height-door_height)
        entry=part(b,"门/双扇木门" if b["shop"] else "门/单扇木门",name+"木门",[x+width/2.00,y-0.02,z],
                   {"width":door_width*100.00,"height":door_height*100.00,"depth":36.00})
        entry.rebuild_door()
    else:
        wall(b,name+"前墙",x,y,z,width,height)
    wall(b,name+"后墙",x,y+depth,z,width,height)
    wall(b,name+"左墙",x,y,z,depth,height,90.00)
    wall(b,name+"右墙",x+width,y,z,depth,height,90.00)
    slab(b,name+"首层地板",x+0.20,y+0.20,z+0.02,width-0.40,depth-0.40)
    for level in range(floors):
        if level:
            slab(b,name+str(level+1)+"层楼板",x+0.18,y+0.18,z+STORY_HEIGHT*level,width-0.36,depth-0.36)
        band_z=z+STORY_HEIGHT*(level+1)-0.18
        for side,wx,wy,wl,angle in (("前",x-0.16,y-0.04,width+0.32,0.00),
                                   ("后",x-0.16,y+depth+0.04,width+0.32,0.00),
                                   ("左",x-0.04,y,depth,90.00),("右",x+width+0.04,y,depth,90.00)):
            wall(b,name+side+str(level)+"檐线",wx,wy,band_z,wl,0.20,angle,0.52)
        windows=max(2,int(width/3.20))
        for j in range(windows):
            cx=x+width*(j+0.50)/windows
            if level==0 and door and abs(cx-(x+width/2.00))<1.75:
                continue
            kind="商铺横窗" if level==0 and b["shop"] else "百叶窗"
            wz=z+STORY_HEIGHT*level+(1.12 if level==0 else 0.88)
            mesh_part(b,HARBOR+"首批模型/模型/"+kind,name+str(level)+"前窗"+str(j),[cx,y-0.23,wz],yaw=-90.00)
            if level or not b["shop"]:
                mesh_part(b,HARBOR+"首批模型/模型/百叶窗",name+str(level)+"后窗"+str(j),
                          [cx,y+depth+0.23,wz],yaw=90.00)
        for side,sx,syaw in (("左",x-0.23,180.00),("右",x+width+0.23,0.00)):
            for j in (0,1):
                mesh_part(b,HARBOR+"首批模型/模型/百叶窗",name+side+str(level)+str(j)+"侧窗",
                          [sx,y+depth*(0.28+0.44*j),z+STORY_HEIGHT*level+0.92],yaw=syaw)
    return height


def shop_details(b,width):
    """用布篷、图案招牌和少量货物表现不同商铺，入口保持清空。"""
    colors={"药剂":("药草青",(0.12,0.34,0.29)),"材料":("海蓝布",(0.15,0.29,0.42)),
            "装备":("酒红布",(0.34,0.13,0.12)),"布匹":("浅靛布",(0.28,0.25,0.42)),
            "旅店":("海蓝布",(0.15,0.29,0.42)),"面包":("麦穗布",(0.52,0.36,0.16))}
    cname,color=colors[b["shop"]]
    cloth=colored_material(cname,color)
    cream=colored_material("象牙标识",(0.80,0.73,0.51))
    wood=asset(AUTO+"地板/自动木地板")
    woodmat=unreal.get_default_object(wood.generated_class()).get_editor_property("beam_material")
    # 遮阳篷只覆盖展示窗和门前局部，不占用主干道。
    aw=round(width*0.68,2)
    mesh_part(b,"/Engine/BasicShapes/Cube","布遮阳篷",[width/2.00,-1.00,2.95],(aw,1.80,0.08),material=cloth,roll=-8.00)
    for x in (width/2.00-aw/2.00+0.10,width/2.00+aw/2.00-0.10):
        mesh_part(b,"/Engine/BasicShapes/Cube","棚柱",[x,-1.65,1.35],(0.10,0.10,2.70),material=woodmat)
    sx=width-1.20
    mesh_part(b,"/Engine/BasicShapes/Cube","招牌底板",[sx,-0.48,2.00],(1.30,0.14,1.20),material=woodmat)
    if b["shop"]=="药剂":
        mesh_part(b,"/Engine/BasicShapes/Sphere","药瓶标识",[sx,-0.59,1.88],(0.48,0.07,0.52),material=cream)
        mesh_part(b,"/Engine/BasicShapes/Cube","瓶颈标识",[sx,-0.59,2.23],(0.16,0.08,0.30),material=cream)
    elif b["shop"]=="装备":
        mesh_part(b,"/Engine/BasicShapes/Cube","剑刃标识",[sx,-0.59,2.03],(0.12,0.08,0.83),material=cream,pitch=25.00)
        mesh_part(b,"/Engine/BasicShapes/Cube","护手标识",[sx-0.12,-0.59,1.80],(0.51,0.08,0.11),material=cream,pitch=25.00)
    else:
        for j in range(3):
            mesh_part(b,"/Engine/BasicShapes/Cube","货品标识"+str(j),[sx+(j-1)*0.28,-0.59,1.96],
                      (0.20,0.08,0.52 if j==1 else 0.34),material=cream)
    for j in range(2):
        mesh_part(b,HARBOR+"第二批模型/模型/"+("木桶_储货_A" if j else "货箱_方箱_A"),
                  "门前货物"+str(j),[width-0.75+j*0.90,-2.35,0.00])


def build_houses(batch):
    """每批搭建三栋房屋，保留原有场景及已完成批次。"""
    rows=layout()[batch*3:(batch+1)*3]
    all_new=actors_with_tag()
    grounds={a.get_actor_label():a for a in all_new if isinstance(a,unreal.LxStoneTerraceActor)}
    made=[]
    for b in rows:
        # 已完整制作的建筑由清单记录，防止重复调用叠放。
        marker=OUT/(b["name"]+"完成.json")
        if marker.exists():
            continue
        b["parent"]=grounds[b["name"]+"地面"]
        width,depth=b["width_m"],b["depth_m"]
        if b["style"]=="退台":
            height=body(b,"下部",0.00,0.00,0.00,width,depth,2)
            slab(b,"退台露台",-0.05,-0.05,height,width+0.10,depth+0.10)
            top=body(b,"上部",0.90,1.40,height,width-1.80,depth-2.20,1,False)
            roof(b,"上层瓦顶",0.90,1.40,height+top,width-1.80,depth-2.20)
            wall(b,"露台前栏",0.10,-0.10,height,width-0.20,0.75,thickness=0.26)
        elif b["style"]=="侧翼":
            main=round(width*0.69,2)
            height=body(b,"主屋",0.00,0.00,0.00,main,depth,b["floors"])
            roof(b,"主屋瓦顶",0.00,0.00,height,main,depth)
            annex_depth=round(depth*0.77,2)
            body(b,"侧翼",main,-0.70,0.00,width-main,annex_depth,1,False)
            roof(b,"侧翼瓦顶",main,-0.70,STORY_HEIGHT,width-main,annex_depth,True)
        else:
            height=body(b,"主屋",0.00,0.00,0.00,width,depth,b["floors"])
            roof(b,"主屋瓦顶",0.00,0.00,height,width,depth,b["style"]=="低坡")
        display_width=round(width*0.69,2) if b["style"]=="侧翼" else width
        if b["shop"]:
            shop_details(b,display_width)
        else:
            mesh_part(b,HARBOR+"第二批模型/模型/长椅_靠背_A","门前长椅",[width-2.00,-1.60,0.00],yaw=180.00)
        mesh_part(b,HARBOR+"第二批模型/模型/花坛_长槽_A","门前花槽",[1.50,-2.50,0.00])
        mesh_part(b,HARBOR+"第二批模型/模型/花坛_方池_A","侧院花池",[width+0.90,depth-1.00,0.00])
        marker.write_text(json.dumps({"name":b["name"],"style":b["style"],"floors":b["floors"]},ensure_ascii=False),encoding="utf-8")
        made.append(b["name"])
    return {"batch":batch,"built":made,"actors":len(actors_with_tag())}


def street(name,points,width):
    """用两点线性样条创建笔直道路，路口圆角设为零。"""
    road=preset("道路/道路_规整石板_A",name,points[0],0.00,"道路",
         {"road_width":width*100.00,"road_depth":35.00,"curb_height":8.00,
          "auto_connect":True,"auto_connect_distance":125.00,"auto_connect_height":40.00,
          "start_curb":False,"end_curb":False,"sample_spacing":200.00})
    spline=road.get_editor_property("road_spline")
    spline.clear_spline_points(False)
    for p in points:
        spline.add_spline_point(vector(p),unreal.SplineCoordinateSpace.WORLD,False)
    for i in range(len(points)):
        spline.set_spline_point_type(i,unreal.SplinePointType.LINEAR,False)
    # 终点没有后续线段；补充出口方向供连接求解使用，首点仍以直线插值。
    spline.set_tangent_at_spline_point(len(points)-1,vector(points[-1])-vector(points[-2]),unreal.SplineCoordinateSpace.WORLD,False)
    spline.update_spline()
    road.rebuild_road()
    return road


def build_roads():
    """建立一条内街和两条连接主干道的直线支巷。"""
    if any(a.get_actor_label()=="山侧内街" for a in actors_with_tag()):
        return {"already_built":True}
    origin=[448.30,12.89,-33.30]
    roads=[]
    roads.append(street("山侧内街",[offset(origin,AVENUE_YAW,19.00,47.00),offset(origin,AVENUE_YAW,219.00,47.00)],5.50))
    for station,name in ((88.00,"花窗巷"),(190.00,"石榴巷")):
        points=[offset([448.30,12.89,-33.51],AVENUE_YAW,station,6.55),
                offset(origin,AVENUE_YAW,station,44.00)]
        roads.append(street(name,points,5.00))
    for road in roads:
        road.rebuild_road()
        for i in range(len(road.get_editor_property("road_connections"))):
            road.set_road_connection_parameters(i,0.00,0.00,0.00)
    return {"roads":[a.get_actor_label() for a in roads]}


def finish_ground():
    """提交地面碰撞，并使直线支巷以端点连接内街形成真实丁字路口。"""
    made=actors_with_tag()
    fixed=[]
    for a in made:
        if a.get_actor_label() in ("花窗巷","石榴巷"):
            station=88.00 if a.get_actor_label()=="花窗巷" else 190.00
            spline=a.get_editor_property("road_spline")
            spline.set_location_at_spline_point(1,vector(offset([448.30,12.89,-33.30],AVENUE_YAW,station,44.00)),
                                                unreal.SplineCoordinateSpace.WORLD,True)
            a.rebuild_road()
    for a in made:
        if isinstance(a,unreal.LxSplineRoadActor):
            spline=a.get_editor_property("road_spline")
            tangent=spline.get_location_at_spline_point(1,unreal.SplineCoordinateSpace.WORLD)-spline.get_location_at_spline_point(0,unreal.SplineCoordinateSpace.WORLD)
            spline.set_spline_point_type(0,unreal.SplinePointType.LINEAR,False)
            spline.set_tangent_at_spline_point(1,tangent,unreal.SplineCoordinateSpace.WORLD,True)
            a.clear_legacy_junctions()
            a.rebuild_road()
            for i in range(len(a.get_editor_property("road_connections"))):
                a.set_road_connection_parameters(i,0.00,0.00,0.00)
    for a in made:
        if isinstance(a,(unreal.LxStoneTerraceActor,unreal.LxSplineRoadActor)):
            a.get_editor_property("optimized_mesh").finalize_mesh()
            fixed.append(a.get_actor_label())
    return {"finalized":fixed}


def diagnose_roads():
    """导出新道路的端口方向和旧版开口状态，定位连接条件。"""
    rows=[]
    for a in actors_with_tag():
        if isinstance(a,unreal.LxSplineRoadActor):
            s=a.get_editor_property("road_spline")
            row={"name":a.get_actor_label(),"points":[vec_m(s.get_location_at_spline_point(i,unreal.SplineCoordinateSpace.WORLD)) for i in (0,1)],
                 "directions":[str(s.get_direction_at_spline_point(i,unreal.SplineCoordinateSpace.WORLD)) for i in (0,1)]}
            for key in ("auto_connect","auto_connect_distance","auto_connect_height"):
                row[key]=a.get_editor_property(key)
            rows.append(row)
    return rows


def organize():
    """将每栋房屋的全部构件附加到各自台地，便于整体编辑与移动。"""
    made=actors_with_tag()
    grounds={a.get_actor_label():a for a in made if isinstance(a,unreal.LxStoneTerraceActor)}
    attached=0
    for b in layout():
        parent=grounds[b["name"]+"地面"]
        parent.root_component.set_mobility(unreal.ComponentMobility.STATIC)
        for a in made:
            if a==parent or str(a.get_folder_path())!="山侧街区/"+b["name"]:
                continue
            if a.get_attach_parent_actor()!=parent:
                a.attach_to_actor(parent,"",unreal.AttachmentRule.KEEP_WORLD,unreal.AttachmentRule.KEEP_WORLD,
                                  unreal.AttachmentRule.KEEP_WORLD,False)
            if a.get_attach_parent_actor()!=parent:
                raise RuntimeError("房屋构件附加失败："+a.get_actor_label())
            attached+=1
    return {"attached_parts":attached,"building_roots":len(grounds)}


def polygons_overlap(a,b):
    """用分离轴检查两个凸矩形是否存在实际面积重叠。"""
    for polygon in (a,b):
        for i in range(len(polygon)):
            p,q=polygon[i],polygon[(i+1)%len(polygon)]
            axis=[q[1]-p[1],p[0]-q[0]]
            pa=[p[0]*axis[0]+p[1]*axis[1] for p in a]
            pb=[p[0]*axis[0]+p[1]*axis[1] for p in b]
            if max(pa)<=min(pb)+0.01 or max(pb)<=min(pa)+0.01:
                return False
    return True


def footprint(b):
    """按顺时针顺序取得台地四角，用于占地碰撞检查。"""
    w,d=b["width_m"],b["depth_m"]
    return [offset(b["origin_m"],b["yaw"],x,y) for x,y in
            ((-2.00,-4.60),(w+2.00,-4.60),(w+2.00,d+2.00),(-2.00,d+2.00))]


def validate():
    """检查原对象、建筑占地、地面碰撞、真实门洞和直线路口。"""
    current=world()
    all_actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
    made=actors_with_tag()
    rows=layout()
    old=json.loads((OUT/"原场景清单.json").read_text(encoding="utf-8"))
    names={a.get_name():a for a in all_actors}
    missing=[r["name"] for r in old if r["name"] not in names]
    moved=[r["label"] for r in old if r["name"] in names and
           vec_m(names[r["name"]].get_actor_location())!=r["location_m"]]
    errors=[]
    if missing or moved:
        errors.append({"missing_originals":missing,"moved_originals":moved})
    for i,b in enumerate(rows):
        for other in rows[i+1:]:
            if polygons_overlap(footprint(b),footprint(other)):
                errors.append("台地相交："+b["name"]+"与"+other["name"])
    counts={}
    for a in made:
        key=a.get_class().get_name()
        counts[key]=counts.get(key,0)+1
        for prop in ("generation_diagnostic",):
            try:
                diagnostic=a.get_editor_property(prop)
            except Exception:
                continue
            if diagnostic:
                errors.append(a.get_actor_label()+"："+str(diagnostic))
        if isinstance(a,(unreal.LxBrickWallActor,unreal.LxTileRoofActor,unreal.LxTimberPanelActor)):
            if a.get_editor_property("generated_triangle_count")<=0:
                errors.append("构件没有几何："+a.get_actor_label())
    doors=[a for a in made if isinstance(a,unreal.LxWoodenDoorActor)]
    checks=[]
    roots={a.get_actor_label():a for a in made if isinstance(a,unreal.LxStoneTerraceActor)}
    for b in rows:
        parent=roots[b["name"]+"地面"]
        detached=[a.get_actor_label() for a in made if a!=parent and
                  str(a.get_folder_path())=="山侧街区/"+b["name"] and a.get_attach_parent_actor()!=parent]
        if detached:
            errors.append({"building":b["name"],"detached_parts":detached})
        width=b["width_m"]*(0.69 if b["style"]=="侧翼" else 1.00)
        width=round(width,2)
        center=offset(b["origin_m"],b["yaw"],width/2.00,-2.80)
        ground=unreal.SystemLibrary.line_trace_single(current,vector([center[0],center[1],center[2]+1.00]),
                 vector([center[0],center[1],center[2]-0.40]),unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,
                 False,[],unreal.DrawDebugTrace.NONE,True)
        height=vec_m(ground.to_tuple()[4])[2] if ground else None
        opening=unreal.SystemLibrary.line_trace_single(current,vector(offset(b["origin_m"],b["yaw"],width/2.00,-1.00,1.30)),
                  vector(offset(b["origin_m"],b["yaw"],width/2.00,2.00,1.30)),unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,
                  False,doors,unreal.DrawDebugTrace.NONE,True)
        if height is None or abs(height-b["top_m"])>0.08:
            errors.append("独立地面高度异常："+b["name"]+str(height))
        if opening:
            errors.append("门洞被墙体封住："+b["name"])
        checks.append({"name":b["name"],"ground_m":height,"clear_doorway":not bool(opening)})
    roads=[]
    for a in made:
        if not isinstance(a,unreal.LxSplineRoadActor):
            continue
        spline=a.get_editor_property("road_spline")
        # 点的插值类型描述从该点开始的线段；最后一点仅保留有效端口切线。
        linear=all(spline.get_spline_point_type(i)==unreal.SplinePointType.LINEAR for i in range(spline.get_number_of_spline_points()-1))
        connections=a.get_editor_property("road_connections")
        record={"name":a.get_actor_label(),"linear":linear,"width_m":a.road_width/100.00,
                "connections":len(connections),"diagnostic":a.get_editor_property("connection_diagnostic")}
        if not linear or not connections:
            errors.append("道路尚未有效接入："+str(record))
        roads.append(record)
    result={"passed":not errors,"errors":errors,"buildings":len(rows),"original_actors":len(old),
            "new_actors":len(made),"classes":counts,"doors":len(doors),"ground_checks":checks,"roads":roads}
    (OUT/"街区验收.json").write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding="utf-8")
    return {"passed":not errors,"errors":errors,"new_actors":len(made),"doors":len(doors),"roads":roads}


def save():
    """仅在验收通过后保存当前主关卡，生成网格随关卡落盘。"""
    report=validate()
    if not report["passed"]:
        raise RuntimeError("街区验收未通过，尚未保存："+str(report["errors"]))
    if not unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level():
        raise RuntimeError("主关卡保存失败")
    return {"saved":LEVEL,"new_actors":len(actors_with_tag())}


def reload_check():
    """从磁盘重载主关卡，验证道路、建筑、碰撞与保存缓存。"""
    levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if not levels.load_level("/Engine/Maps/Entry"):
        raise RuntimeError("无法切换到空关卡进行重载")
    if not levels.load_level(LEVEL):
        raise RuntimeError("主关卡重载失败")
    return validate()


def completion_summary():
    """记录完成结果和预览文件，并确认场景中没有残留验收相机。"""
    current=world()
    actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
    temporary=[a.get_actor_label() for a in actors if a.get_actor_label()=="临时街区验收相机"]
    if temporary:
        raise RuntimeError("离屏渲染尚未结束")
    result={"level":current.get_path_name(),"buildings":21,"commercial_buildings":7,
            "residential_buildings":14,"independent_grounds":21,"straight_streets":3,
            "all_actors":len(actors),"added_actors":len(actors_with_tag()),
            "folder":"山侧街区","backup":str(OUT/"Backup_20261013/唯一关卡.umap"),
            "previews":[str(OUT/(name+".png")) for name in ("街区总览","高塔商业街","住宅与内街")]}
    (OUT/"制作结果.json").write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding="utf-8")
    return result
