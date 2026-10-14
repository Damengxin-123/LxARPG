"""为现有角色种族表补齐稳定标签，保留种族、显示名称与玩家角色引用。"""
import configparser
import json
import shutil
from pathlib import Path

import unreal


def configure_character_races():
    """从项目设置找到实际管理器，仅迁移已知旧行，不覆盖已有配置标签。"""
    project = Path(unreal.Paths.project_dir())
    settings = configparser.ConfigParser(interpolation=None, strict=False)
    settings.read(project / 'Config/DefaultGame.ini', encoding='utf-8-sig')
    manager_reference = settings['/Script/LxARPG.LxGameSettings']['GameDataTablesManagerClass']
    manager_path = manager_reference.split("'")[1] if "'" in manager_reference else manager_reference
    manager_class = unreal.load_class(None, manager_path)
    if not manager_class:
        raise RuntimeError('项目设置中的数据表管理器无法加载。')
    manager = unreal.get_default_object(manager_class)
    table = manager.get_editor_property('character_race_table')
    row_struct = unreal.load_object(None, '/Script/LxARPG.LxCharacterRaceConfig')
    if not table or table.get_editor_property('row_struct') != row_struct:
        raise RuntimeError('当前管理器没有绑定有效的角色种族表。')
    before = unreal.DataTableFunctionLibrary.export_data_table_to_json_string(table)
    rows = json.loads(before)
    # 只为升级前已确认的两行分配标签，不根据可重复的种族或显示名推断身份。
    legacy_ids = {'人类': '角色配置.人类女性', '人类测试': '角色配置.人类女性测试'}
    changed = False
    for row in rows:
        current = row.get('CharacterConfigID', '')
        if isinstance(current, dict):
            current = current.get('TagName', '')
        if current and current not in ('None', '(TagName="")', '(TagName="None")'):
            continue
        tag = legacy_ids.get(row['Name'])
        if not tag:
            raise RuntimeError('请先为该行配置唯一角色配置ID：' + row['Name'])
        row['CharacterConfigID'] = {'TagName': tag}
        changed = True
    output = project / 'Output/种族标签迁移_20261007'
    output.mkdir(parents=True, exist_ok=True)
    if changed:
        asset_file = project / 'Content' / (table.get_path_name().split('.')[0].removeprefix('/Game/') + '.uasset')
        backup = output / '角色种族表_迁移前.uasset'
        if not backup.exists():
            shutil.copy2(asset_file, backup)
            (output / '迁移前.json').write_text(before, encoding='utf-8')
        if not unreal.DataTableFunctionLibrary.fill_data_table_from_json_string(table, json.dumps(rows, ensure_ascii=False)):
            raise RuntimeError('角色配置标签导入失败，未保存。')
    validation = manager.get_character_race_configs()
    if validation is None:
        raise RuntimeError('角色配置校验失败，未保存：' + unreal.DataTableFunctionLibrary.export_data_table_to_json_string(table))
    if changed and not unreal.EditorAssetLibrary.save_loaded_asset(table, only_if_is_dirty=False):
        raise RuntimeError('角色配置表保存失败。')
    result = {'table': table.get_path_name(), 'changed': changed,
              'rows': json.loads(unreal.DataTableFunctionLibrary.export_data_table_to_json_string(table)),
              'validation': str(validation)}
    (output / '迁移结果.json').write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding='utf-8')
    unreal.log('CHARACTER_CONFIG_TAG_MIGRATION_OK: ' + json.dumps(result, ensure_ascii=False))
    return result


if __name__ == '__main__':
    configure_character_races()
