"""在旧关卡副本中验证流明兼容迁移和 EV 保存重开。"""
import json
import math
from pathlib import Path
import unreal


def verify_lamp_ev_migration():
    """保留旧实例的实际照明，验证不同锥角、关闭侧与新版本存档。"""
    saved = Path(unreal.Paths.project_saved_dir())
    out = saved / 'LampEV'
    out.mkdir(parents=True, exist_ok=True)
    old = json.loads((saved / 'LightControls/逐实例验证.json').read_text(encoding='utf-8'))['actors']
    path = '/Game/开发测试/光照参数验证_20261010'
    world = unreal.EditorLoadingAndSavingUtils.load_map(path)
    subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    result = []
    expected = {}
    for actor in subsystem.get_all_level_actors():
        label = actor.get_actor_label()
        if label not in old or isinstance(actor, unreal.LxSunActor):
            continue
        names = [('brightness', 'lamp_light', -1)] if isinstance(actor, unreal.LxMagicLampActor) else [('road_brightness', 'road_light', 40), ('sidewalk_brightness', 'sidewalk_light', 30)]
        for prop, component, angle in names:
            light = actor.get_editor_property(component)
            lumens = old[label]['settings'][prop]
            solid_angle = 4 * math.pi if angle < 0 else 2 * math.pi * (1 - math.cos(math.radians(angle)))
            ev = actor.get_editor_property(prop + '_ev')
            recovered = 2**ev * solid_angle
            assert abs(recovered - lumens) < .01, (label, prop, lumens, ev, recovered)
            assert light.get_editor_property('intensity_units') == unreal.LightUnits.EV
            assert isinstance(light, (unreal.LxLampPointLightComponent, unreal.LxLampSpotLightComponent)), light.get_class().get_name()
            result.append({'actor': label, 'property': prop, 'old_lumens': lumens, 'ev': ev, 'equivalent_lumens': recovered, 'component': light.get_class().get_name()})
            # 调整一档再保存，检测重开时是否错误地重新套用旧流明。
            actor.set_editor_property(prop + '_ev', ev + 1.0)
            expected[label + ':' + prop] = actor.get_editor_property(prop + '_ev')
    assert len(result) == 16, len(result)
    assert unreal.EditorLoadingAndSavingUtils.save_map(world, path)
    unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
    unreal.EditorLoadingAndSavingUtils.load_map(path)
    for actor in subsystem.get_all_level_actors():
        for prop in ['brightness', 'road_brightness', 'sidewalk_brightness']:
            key = actor.get_actor_label() + ':' + prop
            if key in expected:
                assert actor.get_editor_property(prop + '_ev') == expected[key], key
    (out / '旧场景EV迁移.json').write_text(json.dumps({'passed': True, 'lights': result, 'save_reload_no_double_conversion': True}, ensure_ascii=False, indent=2), encoding='utf-8')
    unreal.log('LAMP_EV_MIGRATION_PASSED')

verify_lamp_ev_migration()
