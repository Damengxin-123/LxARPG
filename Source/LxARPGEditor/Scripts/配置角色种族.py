"""创建人类种族配置，并将角色种族表配置到项目的数据表管理蓝图。"""
import json
from pathlib import Path

import unreal


def configure_character_races():
    """仅补齐人类配置，保留表中后续添加的其他种族，并验证管理器查询结果。"""
    table_path = '/Game/项目内容/数据表格/角色种族表'
    manager_path = '/Game/项目内容/数据资产/类型_数据表格管理对象'
    character_path = '/Game/项目内容/实体资产/角色/测试角色-人类法师/测试角色-玩家控制角色'
    character_class = unreal.EditorAssetLibrary.load_blueprint_class(character_path)
    manager_class = unreal.EditorAssetLibrary.load_blueprint_class(manager_path)
    if not character_class or not manager_class:
        raise RuntimeError('无法加载现有玩家角色或数据表管理蓝图。')
    character_defaults = unreal.get_default_object(character_class)
    race = character_defaults.get_editor_property('character_race')
    if race not in (unreal.LxCharacterRaceType.NONE, unreal.LxCharacterRaceType.HUMAN):
        raise RuntimeError('当前玩家角色已配置为其他种族，请检查人类角色映射。')
    if race == unreal.LxCharacterRaceType.NONE:
        character_defaults.set_editor_property('character_race', unreal.LxCharacterRaceType.HUMAN)
        if not unreal.EditorAssetLibrary.save_asset(character_path, only_if_is_dirty=False):
            raise RuntimeError('人类角色种族保存失败。')

    row_struct = unreal.load_object(None, '/Script/LxARPG.LxCharacterRaceConfig')
    table = unreal.EditorAssetLibrary.load_asset(table_path) if unreal.EditorAssetLibrary.does_asset_exist(table_path) else None
    if not table:
        factory = unreal.DataTableFactory()
        factory.set_editor_property('struct', row_struct)
        table = unreal.AssetToolsHelpers.get_asset_tools().create_asset('角色种族表', '/Game/项目内容/数据表格', unreal.DataTable, factory)
    if not table or table.get_editor_property('row_struct') != row_struct:
        raise RuntimeError('角色种族表创建失败或已有资产行结构不匹配。')
    rows = json.loads(unreal.DataTableFunctionLibrary.export_data_table_to_json_string(table) or '[]')
    human_rows = [row for row in rows if row.get('Race') in ('Human', 'ELxCharacterRaceType::Human')]
    if len(human_rows) > 1:
        raise RuntimeError('表中存在重复人类配置，禁止自动覆盖。')
    row = human_rows[0] if human_rows else {'Name': '人类'}
    if not human_rows:
        if any(item.get('Name') == '人类' for item in rows):
            raise RuntimeError('人类行名已被其他种族占用。')
        rows.append(row)
    row.update(Race='Human', RaceName='人类', PlayerCharacterClass=character_class.get_path_name())
    if not unreal.DataTableFunctionLibrary.fill_data_table_from_json_string(table, json.dumps(rows, ensure_ascii=False)):
        raise RuntimeError('角色种族表导入失败。')
    if not unreal.EditorAssetLibrary.save_loaded_asset(table, only_if_is_dirty=False):
        raise RuntimeError('角色种族表保存失败。')

    manager = unreal.get_default_object(manager_class)
    manager.set_editor_property('character_race_table', table)
    manager_asset = unreal.EditorAssetLibrary.load_asset(manager_path)
    unreal.BlueprintEditorLibrary.compile_blueprint(manager_asset)
    if not unreal.EditorAssetLibrary.save_loaded_asset(manager_asset, only_if_is_dirty=False):
        raise RuntimeError('数据表管理蓝图保存失败。')
    saved_manager = unreal.get_default_object(unreal.EditorAssetLibrary.load_blueprint_class(manager_path))
    if saved_manager.get_editor_property('character_race_table') != table:
        raise RuntimeError('数据表管理器的种族表引用校验失败。')
    result = {'table': table.get_path_name(), 'manager': manager_class.get_path_name(),
              'rows': json.loads(unreal.DataTableFunctionLibrary.export_data_table_to_json_string(table))}
    output = Path(unreal.Paths.project_dir()) / 'Output/种族配置_20261003/资产配置.json'
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding='utf-8')
    unreal.log('CHARACTER_RACE_SETUP_OK: ' + json.dumps(result, ensure_ascii=False))


configure_character_races()
