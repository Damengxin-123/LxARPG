"""检查总关卡交互实例的存档身份，显式应用时备份并保存对应外部对象包。"""
import json
import shutil
from pathlib import Path

import unreal


# 当前总关卡内四个已有实例的稳定对象名，不给蓝图默认对象配置共享身份。
INSTANCE_TAGS = {
    '仓库_C_UAID_ECD68AC40D35160003_1324720704': '存档.交互对象.总关卡.仓库一',
    '宝箱_C_UAID_ECD68AC40D35160003_1623075705': '存档.交互对象.总关卡.宝箱一',
    '药剂商人_C_UAID_ECD68AC40D35D1FF02_1198935559': '存档.交互对象.总关卡.药剂商人一',
    '测试单位-可交互对象_C_UAID_ECD68AC40D35150003_1725424527': '存档.交互对象.总关卡.木门一',
}


def configure_interaction_saves():
    """只修改明确匹配的场景实例；重复运行验证身份且不覆盖已有不同标签。"""
    apply = '-ApplyInteractionSaveIDs' in unreal.SystemLibrary.get_command_line()
    level_path = '/Game/项目内容/关卡/总关卡'
    level_editor = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if not level_editor.load_level(level_path):
        raise RuntimeError('无法加载总关卡。')
    # 世界分区默认不加载所有实例，按磁盘描述中已确认的对象名显式载入目标。
    descriptions = unreal.WorldPartitionBlueprintLibrary.get_actor_descs()
    target_guids = [desc.get_editor_property('guid') for desc in descriptions
                    if str(desc.get_editor_property('name')) in INSTANCE_TAGS]
    unreal.WorldPartitionBlueprintLibrary.load_actors(target_guids)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
    project_dir = Path(unreal.Paths.project_dir()).resolve()
    if apply:
        # 未注册的标签即使能暂存到结构体，下一次加载也无法作为有效存档身份。
        tag_config = (project_dir / 'Config' / 'DefaultGameplayTags.ini').read_text(encoding='utf-8-sig')
        if any('Tag="' + name + '"' not in tag_config for name in INSTANCE_TAGS.values()):
            raise RuntimeError('必须先在项目标签配置中注册全部目标存档标签。')
    output_dir = project_dir / 'Saved' / '交互存档修复'
    output_dir.mkdir(parents=True, exist_ok=True)
    records = []
    packages = []
    seen = set()
    for actor in actors:
        for component in actor.get_components_by_class(unreal.LxInteractableComponent):
            config = component.get_editor_property('feature_config')
            persistent = any(config.get_editor_property(name) for name in (
                'bEnableWarehouse', 'bEnableTreasureChest',
                'bEnableTriggerMechanism', 'bEnableTradeContainer'))
            if not persistent:
                continue
            tag = component.get_editor_property('interaction_id_tag')
            package = actor.get_package()
            record = {'actor': actor.get_name(), 'label': actor.get_actor_label(),
                      'component': component.get_path_name(), 'package': package.get_name(),
                      'current_tag': tag.export_text(), 'expected_tag': INSTANCE_TAGS.get(actor.get_name())}
            records.append(record)
            if actor.get_name() not in INSTANCE_TAGS:
                continue
            if actor.get_name() in seen:
                raise RuntimeError('同一目标实例具有多个持久交互组件，需分别指定身份。')
            seen.add(actor.get_name())
            if not apply:
                continue
            expected = unreal.GameplayTag()
            if not expected.import_text('(TagName="' + record['expected_tag'] + '")'):
                raise RuntimeError('无法构建预期存档标签。')
            if tag.export_text() == expected.export_text():
                continue
            if tag.export_text() != unreal.GameplayTag().export_text():
                raise RuntimeError('实例已经配置其他身份，禁止自动覆盖：' + actor.get_name())
            relative = package.get_name().removeprefix('/Game/')
            filename = project_dir / 'Content' / (relative + ('.umap' if package.get_name() == level_path else '.uasset'))
            if not filename.is_file():
                raise RuntimeError('未找到需备份的场景包：' + str(filename))
            backup = output_dir / '修改前' / filename.relative_to(project_dir)
            backup.parent.mkdir(parents=True, exist_ok=True)
            if not backup.exists():
                shutil.copy2(filename, backup)
            actor.modify()
            component.modify()
            component.set_editor_property('interaction_id_tag', expected)
            if component.get_editor_property('interaction_id_tag').export_text() != expected.export_text():
                raise RuntimeError('交互身份设置后校验失败。')
            record['applied_tag'] = expected.export_text()
            if package not in packages:
                packages.append(package)
    (output_dir / ('应用结果.json' if apply else '检查结果.json')).write_text(
        json.dumps(records, ensure_ascii=False, indent=2), encoding='utf-8')
    if seen != set(INSTANCE_TAGS):
        raise RuntimeError('未完整找到四个目标实例：' + ', '.join(set(INSTANCE_TAGS) - seen))
    if packages and not unreal.EditorLoadingAndSavingUtils.save_packages(packages, only_dirty=True):
        raise RuntimeError('交互实例包保存失败。')
    unreal.log('INTERACTION_SAVE_IDS_OK: ' + json.dumps({'apply': apply, 'count': len(records), 'saved_packages': len(packages)}, ensure_ascii=False))


configure_interaction_saves()
