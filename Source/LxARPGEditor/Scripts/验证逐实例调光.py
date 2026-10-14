"""验证实际照明预设逐实例调光、复制和保存重开；仅创建隔离测试关卡。"""
import json
from pathlib import Path
import unreal

LIGHT_TEST_MAP = '/Game/开发测试/光照EV验证_20261010'
LIGHT_TEST_OUT = Path(unreal.Paths.project_saved_dir()) / 'LampEV'
LIGHT_TEST_PRESETS = [
    '/Game/项目内容/美术资产/照明/世界太阳',
    '/Game/项目内容/美术资产/海港城市/魔灯/预设/方晶壁灯',
    '/Game/项目内容/美术资产/海港城市/魔灯/预设/方晶顶灯',
    '/Game/项目内容/美术资产/海港城市/魔灯/预设/花冠球灯',
    '/Game/项目内容/美术资产/海港城市/魔灯/预设/星环球灯',
    '/Game/项目内容/美术资产/海港城市/路灯/预设/单侧悬灯',
    '/Game/项目内容/美术资产/海港城市/路灯/预设/双侧悬灯',
]

def light_test_value(value):
    """将颜色和标量转为可比较、可存档的数据。"""
    if isinstance(value, (unreal.LinearColor, unreal.Color)):
        return [round(float(value.r), 5), round(float(value.g), 5), round(float(value.b), 5)]
    return value

def light_test_snapshot(actor):
    """同时读取作者参数、实际光源和独立晶核材质。"""
    if isinstance(actor, unreal.LxSunActor):
        names = ['noon_intensity', 'light_tint', 'use_temperature', 'temperature', 'preview_hour']
        components = ['sun_light']
        mesh, slots = None, []
    elif isinstance(actor, unreal.LxMagicStreetLampActor):
        names = ['road_brightness_ev', 'road_light_color', 'road_use_temperature', 'road_temperature', 'sidewalk_brightness_ev', 'sidewalk_light_color', 'sidewalk_use_temperature', 'sidewalk_temperature']
        components = ['road_light', 'sidewalk_light']
        mesh, slots = actor.get_editor_property('street_mesh'), [3, 4]
    else:
        names = ['brightness_ev', 'light_color', 'use_temperature', 'temperature']
        components = ['lamp_light']
        mesh, slots = actor.get_editor_property('lamp_mesh'), [2]
    result = {'settings': {n: light_test_value(actor.get_editor_property(n)) for n in names}, 'lights': {}, 'crystals': []}
    for name in components:
        light = actor.get_editor_property(name)
        result['lights'][name] = {n: light_test_value(light.get_editor_property(n)) for n in ['intensity', 'light_color', 'use_temperature', 'temperature', 'visible']}
    for slot in slots:
        material = mesh.get_material(slot)
        if isinstance(material, unreal.MaterialInstanceDynamic):
            result['crystals'].append(light_test_value(material.get_vector_parameter_value('魔力颜色')))
    return result

def light_test_configure(actor, index, variant):
    """模拟详情面板编辑亮度、彩色光源及独立色温开关。"""
    color = unreal.LinearColor(1, 0, 0, 1) if variant == 0 else unreal.LinearColor(0, 0, 1, 1)
    if index == 0:
        values = {'preview_hour': 12.0, 'noon_intensity': 60000.0 + 30000.0 * variant, 'light_tint': color, 'use_temperature': bool(variant), 'temperature': 3000.0 + 4000.0 * variant}
    elif index < 5:
        values = {'brightness_ev': 4.0 + 5.0 * variant, 'light_color': color, 'use_temperature': bool(variant), 'temperature': 3000.0 + 4000.0 * variant}
    else:
        values = {'road_brightness_ev': 8.0 + 4.0 * variant, 'road_light_color': color, 'road_use_temperature': bool(variant), 'road_temperature': 3000.0 + 4000.0 * variant, 'sidewalk_brightness_ev': 6.0 + 3.0 * variant, 'sidewalk_light_color': unreal.LinearColor(0, 1, 0, 1), 'sidewalk_use_temperature': not bool(variant), 'sidewalk_temperature': 6500.0}
    for name, value in values.items():
        actor.set_editor_property(name, value)
    snapshot = light_test_snapshot(actor)
    if index == 0:
        assert snapshot['lights']['sun_light']['intensity'] == values['noon_intensity']
    elif index < 5:
        assert snapshot['lights']['lamp_light']['intensity'] == values['brightness_ev']
    else:
        assert snapshot['lights']['road_light']['intensity'] == values['road_brightness_ev']
        assert snapshot['lights']['sidewalk_light']['intensity'] == values['sidewalk_brightness_ev']
    for light in snapshot['lights'].values():
        assert sorted(light['light_color'])[:2] == [0.0, 0.0] and max(light['light_color']) > 200.0, snapshot
    return snapshot

def verify_light_instances():
    """验证七类预设共十四个独立实例及存档持久化。"""
    assert not unreal.EditorAssetLibrary.does_asset_exist(LIGHT_TEST_MAP), '测试关卡已存在，停止以避免覆盖'
    LIGHT_TEST_OUT.mkdir(parents=True, exist_ok=True)
    world = unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    expected = {}
    for index, path in enumerate(LIGHT_TEST_PRESETS):
        blueprint = unreal.load_asset(path)
        assert blueprint, path
        default = unreal.get_default_object(blueprint.generated_class())
        before_default = light_test_snapshot(default)['settings']
        first = actors.spawn_actor_from_class(blueprint.generated_class(), unreal.Vector(index * 1000, 0, 300))
        first.set_actor_label('光照验证_%d_0' % index)
        light_test_configure(first, index, 0)
        before_first = light_test_snapshot(first)
        second = actors.duplicate_actor(first, world, unreal.Vector(0, 600, 0))
        assert light_test_snapshot(second) == before_first, '复制未保留参数'
        second.set_actor_label('光照验证_%d_1' % index)
        light_test_configure(second, index, 1)
        assert light_test_snapshot(first) == before_first, '修改第二盏灯影响第一盏'
        assert light_test_snapshot(default)['settings'] == before_default, '实例编辑影响蓝图默认值'
        expected[first.get_actor_label()] = before_first
        expected[second.get_actor_label()] = light_test_snapshot(second)
    assert unreal.EditorLoadingAndSavingUtils.save_map(world, LIGHT_TEST_MAP)
    unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
    unreal.EditorLoadingAndSavingUtils.load_map(LIGHT_TEST_MAP)
    actual = {a.get_actor_label(): light_test_snapshot(a) for a in actors.get_all_level_actors() if a.get_actor_label().startswith('光照验证_')}
    assert actual == expected, '保存重开后参数或实际光源不一致'
    (LIGHT_TEST_OUT / '逐实例验证.json').write_text(json.dumps({'passed': True, 'preset_count': 7, 'instance_count': 14, 'duplicate': True, 'independent': True, 'save_reload': True, 'actors': actual}, ensure_ascii=False, indent=2), encoding='utf-8')
    unreal.log('LIGHT_INSTANCE_VERIFICATION_PASSED: 7 presets, 14 actors, duplicate, independence, save/reload')

verify_light_instances()
