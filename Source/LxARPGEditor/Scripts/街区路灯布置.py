"""通过官方 MCP 给已经完成的山侧街区补齐双侧悬灯。"""

# 本次灯具独立标记，避免重复执行时与原有路灯混淆。
LAMP_TAG = "海港街区双侧悬灯_20261013"
LAMP_PRESET = AUTO + "灯具/双侧悬灯"
LAMP_BASELINE = OUT / "加路灯前对象清单.json"


def lamp_actors():
    """仅取得本次补充的灯具，保护已有场景对象。"""
    return [a for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
            if LAMP_TAG in [str(t) for t in a.tags]]


def lamp_actor_stamp(actor):
    """记录对象名称、变换和归属，供保存前后核对原有街区。"""
    rotation = actor.get_actor_rotation()
    scale = actor.get_actor_scale3d()
    return {"name": actor.get_name(), "label": actor.get_actor_label(),
            "position_m": vec_m(actor.get_actor_location()),
            "rotation": [round(v, 2) for v in (rotation.pitch, rotation.yaw, rotation.roll)],
            "scale": [round(v, 4) for v in (scale.x, scale.y, scale.z)],
            "folder": str(actor.get_folder_path()),
            "parent": actor.get_attach_parent_actor().get_name() if actor.get_attach_parent_actor() else None}


def inspect_lamps():
    """确认双侧预设及现有灯具，同时建立不可覆盖的本次施工基线。"""
    world()
    all_actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
    if not LAMP_BASELINE.exists():
        before = [lamp_actor_stamp(a) for a in all_actors if a not in lamp_actors()]
        LAMP_BASELINE.write_text(json.dumps(before, ensure_ascii=False, indent=2), encoding="utf-8")
    default = unreal.get_default_object(asset(LAMP_PRESET).generated_class())
    properties = {key: str(default.get_editor_property(key)) for key in (
        "layout", "pole_length", "road_arm_length", "sidewalk_arm_length", "road_light_enabled",
        "sidewalk_light_enabled", "road_light_range", "sidewalk_light_range", "road_brightness_ev",
        "sidewalk_brightness_ev", "auto_enable", "auto_on_hour", "auto_off_hour")}
    existing = [dict(lamp_actor_stamp(a), class_path=a.get_class().get_path_name()) for a in all_actors
                if isinstance(a, unreal.LxMagicStreetLampActor)]
    return {"level": world().get_path_name(), "actor_count": len(all_actors),
            "preset": LAMP_PRESET, "properties": properties, "existing_lamps": existing}


def lamp_plan():
    """按房前铺装边缘及直线街道规划灯位，坐标和距离均采用米。"""
    world()
    rows = []
    for building in layout():
        group = building["group"]
        # 灯座外缘仍在独立铺装内，距店门和门前花槽保留通行空间。
        local_y = -4.00 if group == "内街" else -4.05
        position = offset(building["origin_m"], building["yaw"], 1.20, local_y)
        rows.append({"name": building["name"] + "路灯", "group": group, "position_m": position,
                     "yaw": round(building["yaw"] - 90.00, 2), "ground": building["name"] + "地面",
                     "road": "山侧内街" if group == "内街" else ("主干道1" if group == "港街" else "主干道3")})
    # 在铺装之间及内街端部补灯，避让主街两处丁字路口。
    origin = [448.30, 12.89, -33.32]
    supplements = [("织衣街口路灯", "主街", 80.50, 6.95, "主干道3")]
    supplements += [(name, "内街", station, 51.00, "山侧内街") for name, station in (
        ("内街南口路灯", 22.00), ("花窗巷北路灯", 80.00), ("内街北段路灯", 180.00), ("内街北口路灯", 206.00))]
    for name, group, station, lateral, road in supplements:
        rows.append({"name": name, "group": group, "position_m": offset(origin, AVENUE_YAW, station, lateral),
                     "yaw": round(AVENUE_YAW - 90.00, 2), "ground": None, "road": road})
    # 支巷两侧交错布置，长灯臂统一指向各自的巷道中心线。
    for name, station in (("花窗巷", 88.00), ("石榴巷", 190.00)):
        for suffix, side, lateral in (("南灯", -1.00, 18.00), ("北灯", 1.00, 33.00)):
            rows.append({"name": name + suffix, "group": name,
                         "position_m": offset(origin, AVENUE_YAW, station + side * 3.15, lateral),
                         "yaw": round(AVENUE_YAW + (180.00 if side > 0.00 else 0.00), 2),
                         "ground": None, "road": name})
    all_actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
    existing = [a for a in all_actors if isinstance(a, unreal.LxMagicStreetLampActor) and a not in lamp_actors()]
    ground_types = (unreal.LandscapeProxy, unreal.LxStoneTerraceActor, unreal.LxSplineRoadActor)
    ignore = [a for a in all_actors if not isinstance(a, ground_types)]
    for row in rows:
        samples = []
        for x, y in ((0.00, 0.00), (-0.29, -0.29), (-0.29, 0.29), (0.29, -0.29), (0.29, 0.29)):
            point = offset(row["position_m"], row["yaw"], x, y)
            hit = unreal.SystemLibrary.line_trace_single(world(), vector([point[0], point[1], -30.00]),
                  vector([point[0], point[1], -36.00]), unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,
                  False, ignore, unreal.DrawDebugTrace.NONE, True)
            if not hit:
                raise RuntimeError("灯位缺少支撑地面：" + row["name"])
            samples.append(vec_m(hit.to_tuple()[4])[2])
        if max(samples) - min(samples) > 0.08:
            raise RuntimeError("灯座跨越地面高差：" + row["name"] + str(samples))
        row["position_m"][2] = min(samples)
        row["support_heights_m"] = samples
        row["closest_existing_m"] = round(min(math.dist(row["position_m"][:2], vec_m(a.get_actor_location())[:2])
                                                   for a in existing), 2) if existing else None
        if row["closest_existing_m"] is not None and row["closest_existing_m"] < 5.00:
            raise RuntimeError("新增灯具过近于已有路灯：" + row["name"])
    (OUT / "路灯布局.json").write_text(json.dumps(rows, ensure_ascii=False, indent=2), encoding="utf-8")
    return {"planned": len(rows), "by_road": {name: sum(r["road"] == name for r in rows)
            for name in sorted(set(r["road"] for r in rows))}, "positions": rows}


def build_lamps():
    """使用既有双侧悬灯蓝图放置实例，保留预设的尺寸、材质与自动照明。"""
    if not LAMP_BASELINE.exists():
        raise RuntimeError("需要先记录加路灯前的场景清单")
    rows = json.loads((OUT / "路灯布局.json").read_text(encoding="utf-8"))
    all_actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
    grounds = {a.get_actor_label(): a for a in all_actors if isinstance(a, unreal.LxStoneTerraceActor)}
    existing = {a.get_actor_label(): a for a in lamp_actors()}
    created = []
    for row in rows:
        if row["name"] in existing:
            continue
        actor = preset("灯具/双侧悬灯", row["name"], row["position_m"], row["yaw"], "路灯/" + row["group"],
                       parent=grounds.get(row["ground"]))
        actor.set_editor_property("tags", [unreal.Name(TAG), unreal.Name(LAMP_TAG)])
        created.append(row["name"])
    return {"created": created, "new_lamp_count": len(lamp_actors())}


def validate_lamps():
    """核查底座支撑、两侧照射方向、真实光源和原有对象保持情况。"""
    world()
    all_actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
    old = json.loads(LAMP_BASELINE.read_text(encoding="utf-8"))
    by_name = {a.get_name(): a for a in all_actors}
    errors = []
    for record in old:
        if record["name"] not in by_name or lamp_actor_stamp(by_name[record["name"]]) != record:
            errors.append("原有对象被改变：" + record["label"])
    rows = json.loads((OUT / "路灯布局.json").read_text(encoding="utf-8"))
    made = {a.get_actor_label(): a for a in lamp_actors()}
    roads = {a.get_actor_label(): a for a in all_actors if isinstance(a, unreal.LxSplineRoadActor)}
    checks = []
    if len(made) != len(rows):
        errors.append("路灯数量与布置清单不一致")
    for row in rows:
        actor = made.get(row["name"])
        if not actor:
            errors.append("缺少灯具：" + row["name"])
            continue
        position = actor.get_actor_location()
        spline = roads[row["road"]].get_editor_property("road_spline")
        nearest = spline.find_location_closest_to_world_location(position, unreal.SplineCoordinateSpace.WORLD)
        toward = nearest - position
        road_light = actor.get_editor_property("road_light")
        walk_light = actor.get_editor_property("sidewalk_light")
        road_direction = road_light.get_forward_vector()
        walk_direction = walk_light.get_forward_vector()
        road_dot = road_direction.x * toward.x + road_direction.y * toward.y
        walk_dot = walk_direction.x * toward.x + walk_direction.y * toward.y
        dual = actor.get_editor_property("layout") == unreal.LxStreetLampLayout.DOUBLE
        active = road_light.is_visible() and walk_light.is_visible()
        direction_ok = road_dot > 0.00 and walk_dot < 0.00 and road_direction.z < 0.00 and walk_direction.z < 0.00
        clearance = math.hypot(toward.x, toward.y) / 100.00 - roads[row["road"]].road_width / 200.00 - 0.31
        height_ok = abs(vec_m(position)[2] - min(row["support_heights_m"])) <= 0.02
        if not (dual and active and direction_ok and height_ok and clearance > 0.20):
            errors.append("双侧照明或灯位异常：" + row["name"])
        checks.append({"name": row["name"], "position_m": vec_m(position), "road": row["road"],
                       "dual_lights": dual and active, "directions_correct": direction_ok,
                       "base_on_ground": height_ok, "carriageway_clearance_m": round(clearance, 2),
                       "road_light_range_m": round(road_light.get_editor_property("attenuation_radius") / 100.00, 2),
                       "sidewalk_light_range_m": round(walk_light.get_editor_property("attenuation_radius") / 100.00, 2),
                       "automatic": actor.get_editor_property("auto_enable")})
    for i, actor in enumerate(made.values()):
        for other in list(made.values())[i+1:]:
            if math.dist(vec_m(actor.get_actor_location())[:2], vec_m(other.get_actor_location())[:2]) < 5.00:
                errors.append("路灯过近：" + actor.get_actor_label() + "、" + other.get_actor_label())
    result = {"passed": not errors, "errors": errors, "new_lamps": len(made), "original_actors_preserved": len(old),
              "preset": LAMP_PRESET, "folder": "山侧街区/路灯", "checks": checks}
    (OUT / "路灯验收.json").write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding="utf-8")
    return {key: value for key, value in result.items() if key != "checks"}


def process_lamps(stage):
    """执行本次灯具补充的检查、布置、保存和离屏预览阶段。"""
    if stage.startswith("lamps_height_"):
        return process_lamp_height(stage)
    if stage == "lamps_inspect":
        return inspect_lamps()
    if stage == "lamps_plan":
        return lamp_plan()
    if stage == "lamps_build":
        return build_lamps()
    if stage == "lamps_validate":
        return validate_lamps()
    if stage == "lamps_save":
        result = validate_lamps()
        if not result["passed"]:
            raise RuntimeError("路灯验收未通过，尚未保存：" + str(result["errors"]))
        result.update(save())
        return result
    if stage == "lamps_reload":
        result = reload_check()
        result["lamps"] = validate_lamps()
        return result
    if stage == "lamps_capture_shops":
        return capture("商业街路灯", [463.00, 0.00, -18.00], [430.00, 41.00, -31.00], 70.00)
    if stage == "lamps_capture_inner":
        return capture("内街路灯", [412.00, 63.00, -17.00], [388.00, 107.00, -31.00], 70.00)
    if stage == "lamps_capture_overview":
        return capture("路灯布置总览", [555.78, -53.41, 207.52], [396.42, 38.60, -33.52], 90.00)
    raise ValueError("未知路灯制作阶段：" + stage)


def scene_street_lamps():
    """取得当前主关卡内全部立杆路灯，包括原有单侧灯和新增双侧灯。"""
    world()
    return [actor for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
            if isinstance(actor, unreal.LxMagicStreetLampActor)]


def street_lamp_height(actor):
    """仅测量灯体网格的实际总高，排除光源照明范围造成的包围盒扩张。"""
    origin, extent, radius = unreal.SystemLibrary.get_component_bounds(actor.get_editor_property("street_mesh"))
    return extent.z * 2.00 / 100.00


def street_lamp_settings(actor):
    """记录无需改变的灯臂、外观和光照参数，供调整后的保持性核对。"""
    keys = ("layout", "road_arm_length", "sidewalk_arm_length", "suspension_length", "road_light_enabled",
            "sidewalk_light_enabled", "road_brightness_ev", "sidewalk_brightness_ev", "road_light_color",
            "sidewalk_light_color", "road_use_temperature", "sidewalk_use_temperature", "road_temperature",
            "sidewalk_temperature", "road_light_range", "sidewalk_light_range", "road_outer_cone_angle",
            "sidewalk_outer_cone_angle", "auto_enable", "auto_on_hour", "auto_off_hour", "light_fade_seconds",
            "light_shadows", "stone_material", "metal_material", "ornament_material", "crystal_material")
    settings = {}
    for key in keys:
        value = actor.get_editor_property(key)
        if isinstance(value, (bool, int, float, str)) or value is None:
            settings[key] = value
        elif isinstance(value, unreal.LinearColor):
            settings[key] = [value.r, value.g, value.b, value.a]
        elif isinstance(value, unreal.Object):
            settings[key] = value.get_path_name()
        else:
            settings[key] = str(value)
    return settings


def lamp_height_baseline():
    """建立本次调高的独立基线，重复检查不会覆盖调高前的数据。"""
    path = OUT / "路灯调高前参数.json"
    if not path.exists():
        all_actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
        records = [{"name": a.get_name(), "label": a.get_actor_label(),
                    "height_m": round(street_lamp_height(a), 2),
                    "pole_length_m": round(a.get_editor_property("pole_length") / 100.00, 2),
                    "settings": street_lamp_settings(a)} for a in scene_street_lamps()]
        path.write_text(json.dumps({"actors": [lamp_actor_stamp(a) for a in all_actors], "lamps": records},
                                   ensure_ascii=False, indent=2), encoding="utf-8")
    return json.loads(path.read_text(encoding="utf-8"))


def validate_lamp_height():
    """实测所有路灯总高，并核对灯体位置、光源联动及原有参数。"""
    baseline = lamp_height_baseline()
    all_actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
    actors = {a.get_name(): a for a in all_actors}
    old_lamps = {row["name"]: row for row in baseline["lamps"]}
    errors, rows = [], []
    if len(all_actors) != len(baseline["actors"]):
        errors.append("关卡对象数量发生变化")
    for record in baseline["actors"]:
        if record["name"] not in actors or lamp_actor_stamp(actors[record["name"]]) != record:
            errors.append("对象变换或归属发生变化：" + record["label"])
    for actor in scene_street_lamps():
        height = street_lamp_height(actor)
        pole = actor.get_editor_property("pole_length") / 100.00
        drop = actor.get_editor_property("suspension_length") / 100.00
        expected_road = pole + 0.60 - drop - 0.73
        road_z = actor.get_editor_property("road_light").get_editor_property("relative_location").z / 100.00
        walk_z = actor.get_editor_property("sidewalk_light").get_editor_property("relative_location").z / 100.00
        linked = abs(road_z - expected_road) < 0.01 and abs(walk_z - expected_road + 0.35) < 0.01
        if abs(height - 6.00) > 0.005 or not linked:
            errors.append("灯高或光源位置异常：" + actor.get_actor_label())
        old = old_lamps.get(actor.get_name())
        if not old or street_lamp_settings(actor) != old["settings"]:
            errors.append("灯具其他参数发生变化：" + actor.get_actor_label())
        rows.append({"name": actor.get_actor_label(), "layout": str(actor.get_editor_property("layout")),
                     "height_m": round(height, 2), "pole_length_m": round(pole, 2), "light_positions_linked": linked})
    if len(rows) != len(old_lamps):
        errors.append("路灯数量发生变化")
    report = {"passed": not errors, "errors": errors, "count": len(rows), "target_height_m": 6.00,
              "height_definition": "灯座底部到灯体装饰最高点", "lamps": rows}
    (OUT / "六米路灯验收.json").write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
    return {key: value for key, value in report.items() if key != "lamps"}


def process_lamp_height(stage):
    """将场景中全部立杆路灯调整至六米总高，并保存和复核。"""
    world()
    if stage == "lamps_height_inspect":
        baseline = lamp_height_baseline()
        return {"count": len(baseline["lamps"]), "heights_m": sorted(set(r["height_m"] for r in baseline["lamps"])),
                "single": sum(a.layout == unreal.LxStreetLampLayout.SINGLE for a in scene_street_lamps()),
                "double": sum(a.layout == unreal.LxStreetLampLayout.DOUBLE for a in scene_street_lamps())}
    if stage == "lamps_height_apply":
        lamp_height_baseline()
        for actor in scene_street_lamps():
            # 米制目标仅在引擎接口处换算；自动扣除石座与顶部装饰的实际高度。
            correction_m = 6.00 - street_lamp_height(actor)
            actor.set_editor_property("pole_length", actor.get_editor_property("pole_length") + correction_m * 100.00)
        return validate_lamp_height()
    if stage == "lamps_height_validate":
        return validate_lamp_height()
    if stage == "lamps_height_save":
        result = validate_lamp_height()
        if not result["passed"]:
            raise RuntimeError("路灯高度验收未通过：" + str(result["errors"]))
        if not unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level():
            raise RuntimeError("主关卡保存失败")
        result["saved"] = LEVEL
        return result
    if stage == "lamps_height_reload":
        levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
        if not levels.load_level("/Engine/Maps/Entry") or not levels.load_level(LEVEL):
            raise RuntimeError("保存后重新加载关卡失败")
        return validate_lamp_height()
    if stage == "lamps_height_capture":
        return capture("六米路灯街景", [463.00, 0.00, -18.00], [430.00, 41.00, -31.00], 70.00)
    raise ValueError("未知路灯高度阶段：" + stage)
