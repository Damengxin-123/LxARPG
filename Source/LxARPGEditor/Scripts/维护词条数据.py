"""读取与维护项目词条数据，所有资产均通过虚幻编辑器 API 处理。"""
import json
import copy
import shutil
from pathlib import Path
import unreal


# 所有维护输出仅写入本项目，路径转换为绝对路径便于追踪与复核。
ROOT = Path(unreal.Paths.project_dir()).resolve()
OUT = ROOT / "Saved/词条数据维护"
BASE = "/Game/项目内容/功能资产/数据表格/"
TABLES = {
    "属性词条": BASE + "词条表格/属性数值修改词条",
    "属性影响": BASE + "词条表格/属性影响词条表",
    "属性回复": BASE + "词条表格/属性回复词条表",
    "状态词条": BASE + "词条表格/状态改变词条表",
    "Buff词条": BASE + "词条表格/创建buff词条表",
    "显示词条": BASE + "词条表格/显示文本词条表",
    "技能词条": BASE + "词条表格/授予技能词条",
    "伤害词条": BASE + "词条表格/技能伤害词条",
    "职业词条": BASE + "词条表格/赋予职业词条表",
    "Buff": BASE + "物品表格/buff/buff数据表",
    "角色属性": BASE + "角色表格/角色基础属性表",
    "样式映射": BASE + "可视化表格/文本样式/文本样式表",
    "文本样式": BASE + "可视化表格/文本样式/文本样式集表",
    "明亮样式": "/Game/项目内容/美术资产/界面主题/明亮文本样式集表",
    "富文本标签": BASE + "可视化表格/文本样式/富文本标签",
    "明亮富文本标签": "/Game/项目内容/美术资产/界面主题/明亮富文本标签",
}

# 缺少直接属性修改词条的属性候选，以及使用三种新词条的对应增益。
BUFF_SPECS = [
    ("Stamina", "强健耐力", 20.00, "生命回复"),
    ("CriticalChance", "精准祝福", 0.05, "精准瞄准"),
    ("CriticalDamage", "致命战意", 0.20, "嗜血"),
    ("BlockDamageReduction", "盾卫誓约", 0.05, "钢铁护体"),
    ("AttackPower", "剑锋祝福", 10.00, "力量增加"),
    ("AttackSpeed", "迅捷攻势", 0.10, "嗜血"),
    ("MovementSpeedBonus", "疾风祝福", 0.05, "疾风步"),
    ("CarryWeight", "轻装行军", 20.00, "力量增加"),
    ("BaseMovementSpeed", "轻盈步伐", 0.50, "疾风步"),
    ("Luck", "幸运祈愿", 5.00, "精准瞄准"),
    ("WaterAffinity", "清泉共鸣", 5.00, "冰霜抗性"),
    ("ElectricAffinity", "雷霆共鸣", 5.00, "精准瞄准"),
    ("LightGodFaith", "晨光祝福", 5.00, "魔力护盾"),
    ("NatureFaith", "森林祝福", 5.00, "生命回复"),
]
# 三种作用方式的数值沿用项目百分数约定：提高与总增的1表示1%。
MODES = [("增加", "BasicValue", "普通"),
         ("提高", "BasicImprove", "高级"),
         ("总增", "AdditionalImprove", "稀有")]


def read_table(path):
    """导出数据表中的真实结构和文本，不直接编辑二进制资产。"""
    table = unreal.load_asset(path)
    if not isinstance(table, unreal.DataTable):
        raise RuntimeError("无法加载数据表：" + path)
    value = unreal.DataTableFunctionLibrary.export_data_table_to_json_string(table)
    return table, json.loads(value)


def tag_name(value):
    """读取导出 JSON 中的稳定 GameplayTag，不使用数据表行名作为业务主键。"""
    return value.get("TagName", "None") if isinstance(value, dict) else str(value)


def make_tag(value):
    """构建虚幻 JSON 导入格式的标签字段。"""
    return {"TagName": value}


def make_entry_text(text, description, rarity="普通"):
    """为新词条配置表中真实存在的稀有度样式与中文说明。"""
    return {"EntryDisplayName": {"TextStyleIDTag": make_tag("文本样式.稀有度样式." + rarity), "Text": text},
            "EntryDescribeText": description}


def append_unique(rows, row, id_field):
    """只添加缺失的稳定标签，拒绝行名碰撞，不覆盖已经配置的词条或 Buff。"""
    identity = tag_name(row[id_field])
    if any(tag_name(current[id_field]) == identity for current in rows):
        return False
    if any(current["Name"] == row["Name"] for current in rows):
        raise RuntimeError("数据表行名冲突：" + row["Name"])
    rows.append(row)
    return True


def build_plan():
    """先生成完整数据与标签计划并备份原资产，重启后才导入新标签的数据。"""
    before = {name: read_table(path)[1] for name, path in TABLES.items()}
    after = copy.deepcopy(before)
    attributes = {key: value for key, value in before["角色属性"][0].items()
                  if isinstance(value, dict) and "AttributeIDTag" in value}
    covered = {tag_name(row["AttributeIDTag"]) for row in before["属性词条"]}
    skipped = [tag_name(value["AttributeIDTag"]) for value in attributes.values()
               if tag_name(value["AttributeIDTag"]) in covered]
    new_entries, new_buffs, new_buff_entries, new_tags = [], [], [], {}

    # 将遗留的业务样式统一为可由当前 UI 样式表识别的标签，保留词条数值与文本本地化身份。
    for name, rows in after.items():
        for row in rows:
            styled = row.get("EntryText", {}).get("EntryDisplayName")
            if not styled:
                continue
            style = tag_name(styled["TextStyleIDTag"])
            if style.startswith("文本样式.词条样式."):
                rarity = style.rsplit(".", 1)[1].removesuffix("词条")
                styled["TextStyleIDTag"] = make_tag("文本样式.稀有度样式." + rarity)

    # 修复已有回复词条的重复百分号和负数语义，保留原有回复/扣减数值。
    for row in after["属性回复"]:
        styled = row["EntryText"]["EntryDisplayName"]
        styled["Text"] = styled["Text"].replace("{0}%", "{0}")
        if row["EntryValue"] < 0:
            styled["Text"] = styled["Text"].replace("增加 {0}点生命值", "生命值变动 {0}")
            styled["Text"] = styled["Text"].replace("增加 {0}生命值", "生命值变动 {0}")

    # 补全没有可视化文本的属性影响词条，数值与作用方式完全沿用原配置。
    for row in after["属性影响"]:
        if row["EntryText"]["EntryDisplayName"]["Text"]:
            continue
        source = tag_name(row["SourceAttributeIDTag"]).rsplit(".", 1)[1]
        target = tag_name(row["TargetAttributeIDTag"]).rsplit(".", 1)[1]
        if row["TargetEntryTarget"] == "ToValueLimit":
            target += "上限"
        action = "增加" if row["EffectiveType"] == "BasicValue" else "提高"
        suffix = "点" if action == "增加" else "%"
        row["EntryText"] = make_entry_text("每点" + source + action + "{0}" + suffix + target,
                                           "按来源属性的有效值持续影响目标属性，随来源变动重新计算。", "高级")

    # 为普通文本提供明确默认样式，并兼容两套实际在界面中使用的富文本样式表。
    for name in ("文本样式", "明亮样式", "富文本标签", "明亮富文本标签"):
        rows = after[name]
        if not any(row["Name"] == "Default" for row in rows):
            default = copy.deepcopy(next(row for row in rows if row["Name"] == "普通"))
            default["Name"] = "Default"
            if "StyleIDTag" in default:
                default["StyleIDTag"] = make_tag("None")
            rows.append(default)
    for rarity in ("普通", "高级", "稀有", "传说", "史诗", "神话"):
        mapping = {"Name": "稀有度_" + rarity, "StyleIDTag": make_tag("文本样式.稀有度样式." + rarity),
                   "TextStyleRow": {"DataTable": "/Script/Engine.DataTable'" + TABLES["文本样式"] + ".文本样式集表'",
                                    "RowName": rarity}}
        append_unique(after["样式映射"], mapping, "StyleIDTag")

    for field, buff_name, buff_add, icon_name in BUFF_SPECS:
        attribute = attributes[field]
        attribute_id = tag_name(attribute["AttributeIDTag"])
        if attribute_id in covered:
            continue
        is_resource = attribute["ValueType"] == "Resource"
        is_percentage = attribute.get("ScalarRule", {}).get("DisplayFormat") == "Percentage"
        name = attribute_id.rsplit(".", 1)[1]
        if field in ("LightGodFaith", "NatureFaith"):
            name += "信仰"
        if is_resource:
            name += "上限"
        prefix = "词条.资源修改.上限." if is_resource else "词条.属性修改.标量."
        quotes = []
        for action, mode, rarity in MODES:
            row_name = name + action + "1"
            entry_id = prefix + row_name
            value = 0.01 if action == "增加" and is_percentage else 1.00
            unit = "米/秒" if action == "增加" and field == "BaseMovementSpeed" else ""
            description = ("直接增加目标属性；比例属性的0.01表示1个百分点。" if action == "增加"
                           else "在基础值上叠加提高百分比。" if action == "提高"
                           else "在基础值与提高结果上叠加总增百分比。")
            row = {"Name": row_name, "AttributeIDTag": make_tag(attribute_id),
                   "EntryTarget": "ToValueLimit" if is_resource else "ToValue", "EffectiveType": mode,
                   "EntryValue": value, "TargetBusinessCategories": [], "EntryType": "AttributeGain",
                   "EntryID": make_tag(entry_id), "EntryText": make_entry_text(name + action + " {0}" + unit,
                                                                                 description, rarity)}
            if append_unique(after["属性词条"], row, "EntryID"):
                new_entries.append({"属性": attribute_id, "词条": entry_id, "方式": mode, "行名": row_name})
                new_tags[entry_id] = name + action + ("1个百分点" if is_percentage and action == "增加"
                                                    else "1米/秒" if field == "BaseMovementSpeed" and action == "增加"
                                                    else "1点" if action == "增加" else "1%")
            proportion = round(buff_add / value, 2) if action == "增加" else 10.00 if action == "提高" else 5.00
            quotes.append({"EntryQuote": {"EntryID": make_tag(entry_id), "EntryProportion": proportion, "EntryCD": 1.00},
                           "EntryLogicType": "Base"})
        added = (f"{buff_add * 100:.2f}个百分点" if is_percentage else
                 f"{buff_add:.2f}米/秒" if field == "BaseMovementSpeed" else f"{buff_add:.2f}")
        description = f"{name}增加{added}，提高10%，总增5%，持续30.00秒。"
        buff_id = "物品.buff.属性提升效果." + buff_name
        icon = f"/Game/项目内容/美术资产/物品图标/增益/{icon_name}.{icon_name}"
        if not unreal.EditorAssetLibrary.does_asset_exist(icon):
            raise RuntimeError("增益图标不存在：" + icon)
        buff = {"Name": buff_name, "BuffType": "AttributeGain", "ItemIDTag": make_tag(buff_id), "ItemType": "Buff",
                "ItemCountMax": 1, "ItemCount": 1, "ItemRarity": "Advanced", "ItemSellPrice": 0,
                "ItemDisplayName": buff_name, "ItemDisplayDescription": description,
                "ItemIcon": icon, "ItemEntryConfigs": quotes}
        if append_unique(after["Buff"], buff, "ItemIDTag"):
            new_buffs.append({"属性": attribute_id, "Buff": buff_id, "行名": buff_name, "说明": description})
            new_tags[buff_id] = description
        create_id = "词条.创建Buff.获得" + buff_name
        create = {"Name": "获得" + buff_name, "BuffIDTag": make_tag(buff_id), "BuffDuration": 30.00,
                  "EntryType": "CreateBuff", "EntryID": make_tag(create_id),
                  "EntryText": make_entry_text("获得[" + buff_name + "]，持续{0}秒", description, "高级")}
        if append_unique(after["Buff词条"], create, "EntryID"):
            new_buff_entries.append(create_id)
            new_tags[create_id] = "获得" + buff_name + "，持续30.00秒"

    # 修复已有生命回复 Buff 中的空引用，仍使用原来0.20秒的周期和5秒创建词条。
    for row in after["Buff"]:
        if tag_name(row["ItemIDTag"]) == "物品.buff.回复效果.生命回复":
            row["ItemCount"] = 1
            row["ItemDisplayDescription"] = "每0.20秒恢复100点生命值，持续5.00秒。"
            for config in row["ItemEntryConfigs"]:
                if tag_name(config["EntryQuote"]["EntryID"]) == "None":
                    config["EntryQuote"]["EntryID"] = make_tag("词条.资源修改.恢复.生命回复1")

    # 速度包含小数增益，避免在角色配置中取整导致这两项 Buff 的实际效果丢失。
    for row in after["角色属性"]:
        for field in ("AttackSpeed", "BaseMovementSpeed"):
            row[field]["ScalarRule"]["bRoundValue"] = False
            row[field]["ScalarRule"]["DisplayFormat"] = "Decimal"

    changed = [name for name in TABLES if before[name] != after[name]]
    for name in changed:
        table, _ = read_table(TABLES[name])
        relative = table.get_path_name().split(".", 1)[0].removeprefix("/Game/") + ".uasset"
        backup = OUT / "原始资产" / relative
        backup.parent.mkdir(parents=True, exist_ok=True)
        if not backup.exists():
            shutil.copy2(ROOT / "Content" / relative, backup)
    plan = {"维护前": before, "维护后": after, "修改表": changed, "跳过属性": skipped,
            "新增属性词条": new_entries, "新增Buff": new_buffs, "新增Buff词条": new_buff_entries,
            "新增标签": new_tags}
    OUT.mkdir(parents=True, exist_ok=True)
    (OUT / "维护计划.json").write_text(json.dumps(plan, ensure_ascii=False, indent=2), encoding="utf-8")
    ini = ROOT / "Config/Tags/新增属性词条.ini"
    if ini.exists():
        old_tags = ini.read_text(encoding="utf-8-sig")
        lines = [f'GameplayTagList=(Tag="{identity}",DevComment="{comment}")'
                 for identity, comment in new_tags.items() if f'Tag="{identity}"' not in old_tags]
        text = old_tags.rstrip("\r\n") + "\r\n" + "\r\n".join(lines) + "\r\n"
    else:
        text = "[/Script/GameplayTags.GameplayTagsList]\r\n" + "\r\n".join(
            f'GameplayTagList=(Tag="{identity}",DevComment="{comment}")' for identity, comment in new_tags.items()) + "\r\n"
    with ini.open("w", encoding="utf-8", newline="") as output:
        output.write(text)
    return {"新增属性词条": len(new_entries), "新增Buff": len(new_buffs), "新增Buff词条": len(new_buff_entries),
            "跳过属性数": len(skipped), "修改表": changed, "下一步": "重启独立编辑器以读取新增GameplayTag后执行apply"}


def apply_plan():
    """确认原数据未变化，再导入完整计划、校验稳定标签并保存。"""
    plan = json.loads((OUT / "维护计划.json").read_text(encoding="utf-8"))
    committed_file = OUT / "已保存数据.json"
    committed = json.loads(committed_file.read_text(encoding="utf-8")) if committed_file.exists() else {}
    loaded = {}
    for name in plan["修改表"]:
        table, current = read_table(TABLES[name])
        # 完全一致的已保存数据无需重新导入，保留文本本地化键并避免反复保存资产。
        if current == committed.get(name):
            continue
        loaded[name] = table
        if current != plan["维护前"][name] and current != plan["维护后"][name]:
            raise RuntimeError("维护计划生成后数据发生变化，请重新检查：" + name)
    for name, table in loaded.items():
        payload = json.dumps(plan["维护后"][name], ensure_ascii=False)
        if not unreal.DataTableFunctionLibrary.fill_data_table_from_json_string(table, payload):
            raise RuntimeError("数据表导入失败，尚未保存：" + name)
    result = verify_data(plan)
    for name, table in loaded.items():
        if not unreal.EditorAssetLibrary.save_loaded_asset(table, only_if_is_dirty=False):
            raise RuntimeError("数据表保存失败：" + name)
    committed_file.write_text(json.dumps({name: read_table(TABLES[name])[1] for name in plan["修改表"]},
                                         ensure_ascii=False, indent=2), encoding="utf-8")
    (OUT / "维护结果.json").write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding="utf-8")
    return result


def verify_data(plan=None):
    """核对新增属性的三种方式、Buff引用闭环、真实样式行和原属性词条未被重复新增。"""
    plan = plan or json.loads((OUT / "维护计划.json").read_text(encoding="utf-8"))
    data = {name: read_table(path)[1] for name, path in TABLES.items()}
    entries = {tag_name(row["EntryID"]): row for rows in data.values() for row in rows if "EntryID" in row}
    buffs = {tag_name(row["ItemIDTag"]): row for row in data["Buff"]}
    mappings = {tag_name(row["StyleIDTag"]): row for row in data["样式映射"]}
    for spec in plan["新增属性词条"]:
        row = entries.get(spec["词条"])
        if not row or tag_name(row["AttributeIDTag"]) != spec["属性"] or row["EffectiveType"] != spec["方式"]:
            raise RuntimeError("新增属性词条标签或作用方式无效：" + spec["词条"])
    for attribute in plan["跳过属性"]:
        old_ids = {tag_name(row["EntryID"]) for row in plan["维护前"]["属性词条"]
                   if tag_name(row["AttributeIDTag"]) == attribute}
        new_ids = {tag_name(row["EntryID"]) for row in data["属性词条"]
                   if tag_name(row["AttributeIDTag"]) == attribute}
        if old_ids != new_ids:
            raise RuntimeError("已有词条的属性被新增或删除：" + attribute)
    for spec in plan["新增Buff"]:
        buff = buffs.get(spec["Buff"])
        if not buff or len(buff["ItemEntryConfigs"]) != 3:
            raise RuntimeError("新增Buff没有引用三个属性词条：" + spec["Buff"])
        modes = set()
        for config in buff["ItemEntryConfigs"]:
            entry = entries.get(tag_name(config["EntryQuote"]["EntryID"]))
            if not entry or tag_name(entry["AttributeIDTag"]) != spec["属性"]:
                raise RuntimeError("Buff的属性词条引用无效：" + spec["Buff"])
            modes.add(entry["EffectiveType"])
        if modes != {mode for _, mode, _ in MODES}:
            raise RuntimeError("Buff的三种作用方式不完整：" + spec["Buff"])
    for identity in plan["新增Buff词条"]:
        entry = entries.get(identity)
        if not entry or tag_name(entry["BuffIDTag"]) not in buffs or entry["BuffDuration"] != 30.00:
            raise RuntimeError("创建Buff词条引用或时间无效：" + identity)
    for identity, row in entries.items():
        style = tag_name(row["EntryText"]["EntryDisplayName"]["TextStyleIDTag"])
        if style == "None":
            continue
        mapping = mappings.get(style)
        if not mapping:
            raise RuntimeError("词条的样式标签没有映射：" + identity)
        handle = mapping["TextStyleRow"]
        _, styles = read_table(handle["DataTable"].split("'")[1])
        if handle["RowName"] not in {item["Name"] for item in styles}:
            raise RuntimeError("词条映射到不存在的样式行：" + identity)
    result = {"验证": "通过", "新增属性词条": len(plan["新增属性词条"]),
              "新增Buff": len(plan["新增Buff"]), "新增Buff词条": len(plan["新增Buff词条"]),
              "已有词条的属性数": len(plan["跳过属性"]), "跳过属性": plan["跳过属性"],
              "修改表": plan["修改表"], "表行数": {name: len(rows) for name, rows in data.items()},
              "Buff清单": plan["新增Buff"]}
    (OUT / "维护后.json").write_text(json.dumps(data, ensure_ascii=False, indent=2), encoding="utf-8")
    (OUT / "数据验证.json").write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding="utf-8")
    return result


def process(stage):
    """通过官方 MCP 分阶段完成读取、制作、验收及原生自动化验证。"""
    if stage == "plan":
        return build_plan()
    if stage == "apply":
        return apply_plan()
    if stage == "verify":
        return verify_data()
    if stage == "tests":
        unreal.SystemLibrary.execute_console_command(None, "Automation RunTests LxARPG.EntryData")
        return {"验证": "已启动词条样式、数据引用、Buff实际生效与离屏显示验证"}
    if stage != "inspect":
        raise ValueError(stage)
    OUT.mkdir(parents=True, exist_ok=True)
    data = {name: read_table(path)[1] for name, path in TABLES.items()}
    fonts = {}
    for name in ("A站酷小薇LOGO体", "思源宋体_mianfeiziti_com_Font"):
        font = unreal.load_asset("/Game/项目内容/美术资产/字体/" + name)
        fonts[name] = {"缓存类型": str(font.get_editor_property("font_cache_type")),
                       "复合字体": str(font.get_editor_property("composite_font"))}
    data["字体检查"] = fonts
    manager_class = unreal.load_class(None, BASE + "BP_数据表格配置.BP_数据表格配置_C")
    manager = unreal.get_default_object(manager_class)
    data["管理器检查"] = str(manager.get_editor_property("m_p_rich_text_style_table"))
    snapshot = OUT / "维护前.json"
    if not snapshot.exists():
        snapshot.write_text(json.dumps(data, ensure_ascii=False, indent=2), encoding="utf-8")
    (OUT / "检查.json").write_text(json.dumps(data, ensure_ascii=False, indent=2), encoding="utf-8")
    return {"输出": str(OUT / "维护前.json"), "表行数": {key: len(rows) for key, rows in data.items()}}
