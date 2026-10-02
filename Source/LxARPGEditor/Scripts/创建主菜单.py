"""创建主菜单入口关卡和中文界面蓝图，并接入项目启动地图；可重复执行。"""
import unreal
from pathlib import Path


def replace_ini_value(path, section, key, value):
    """只更新指定配置项，保留原文件编码和换行形式。"""
    raw = path.read_bytes() if path.exists() else b''
    bom = raw.startswith(b'\xef\xbb\xbf')
    text = raw.decode('utf-8-sig')
    newline = '\r\n' if '\r\n' in text else '\n'
    lines = text.splitlines()
    header = '[' + section + ']'
    if header not in lines:
        lines.extend(['', header, key + '=' + value])
    else:
        start = lines.index(header) + 1
        end = next((i for i in range(start, len(lines)) if lines[i].startswith('[')), len(lines))
        matches = [i for i in range(start, end) if lines[i].startswith(key + '=')]
        if matches:
            lines[matches[0]] = key + '=' + value
        else:
            lines.insert(end, key + '=' + value)
    path.write_bytes((b'\xef\xbb\xbf' if bom else b'') + (newline.join(lines) + newline).encode('utf-8'))


def create_menu_assets():
    """创建入口关卡和可复用界面父类资产，已有资产保持不变。"""
    destination = '/Game/项目内容/关卡/主菜单'
    if not unreal.EditorAssetLibrary.does_asset_exist(destination):
        world = unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
        world.get_world_settings().set_editor_property('default_game_mode', unreal.LxMainMenuGameMode)
        unreal.EditorLoadingAndSavingUtils.save_map(world, destination)
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    ui_path = '/Game/项目内容/UI界面/主菜单'
    if not unreal.EditorAssetLibrary.does_asset_exist(ui_path + '/主菜单'):
        factory = unreal.WidgetBlueprintFactory()
        factory.set_editor_property('parent_class', unreal.LxMainMenuWidget)
        asset = asset_tools.create_asset('主菜单', ui_path, unreal.WidgetBlueprint, factory)
        unreal.EditorAssetLibrary.save_loaded_asset(asset)
    config = Path(unreal.Paths.project_config_dir())
    replace_ini_value(config / 'DefaultEngine.ini', '/Script/EngineSettings.GameMapsSettings',
                      'GameDefaultMap', destination + '.主菜单')
    replace_ini_value(config / 'DefaultGame.ini', '/Script/LxARPG.LxMainMenuSettings',
                      'MenuLevel', destination + '.主菜单')
    # 追加打包目录而不覆盖项目已有的其它目录条目。
    game_path = config / 'DefaultGame.ini'
    raw = game_path.read_bytes()
    cook_entry = '+DirectoriesToAlwaysCook=(Path="/Game/项目内容")'
    if cook_entry not in raw.decode('utf-8-sig'):
        newline = '\r\n' if b'\r\n' in raw else '\n'
        with game_path.open('ab') as output:
            output.write((newline + '[/Script/UnrealEd.ProjectPackagingSettings]' + newline + cook_entry + newline).encode('utf-8'))
    unreal.log('MAIN_MENU_SETUP_OK: ' + destination)


create_menu_assets()
