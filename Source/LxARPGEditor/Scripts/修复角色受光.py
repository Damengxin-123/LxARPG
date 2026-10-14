"""将现用角色材质迁移为正常受光，保留原始母材质和贴图。"""
import unreal
SOURCE='/Game/UnfCleric/Materials/M_Cleric_original_color_notModular'
TARGET='/Game/项目内容/美术资产/角色/材质/角色受光'
INSTANCE='/Game/UnfCleric/Materials/MI_Original_color_base'
def migrate():
    """幂等创建项目母材质，并让原实例和旧档引用一起获得修复。"""
    material=unreal.load_asset(TARGET) if unreal.EditorAssetLibrary.does_asset_exist(TARGET) else unreal.EditorAssetLibrary.duplicate_asset(SOURCE,TARGET)
    if not material:raise RuntimeError('无法创建角色受光母材质')
    expressions={e.get_name():e for e in unreal.ObjectIterator(unreal.MaterialExpression) if e.get_outer()==material}
    color=expressions['MaterialExpressionClamp_0']
    zero=expressions['MaterialExpressionConstant_0']
    if zero.get_editor_property('r')!=0:raise RuntimeError('材质零常量与预期不符')
    unreal.MaterialEditingLibrary.connect_material_property(color,'',unreal.MaterialProperty.MP_BASE_COLOR)
    unreal.MaterialEditingLibrary.connect_material_property(zero,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    unreal.MaterialEditingLibrary.recompile_material(material)
    instance=unreal.load_asset(INSTANCE)
    unreal.MaterialEditingLibrary.set_material_instance_parent(instance,material)
    unreal.MaterialEditingLibrary.update_material_instance(instance)
    assert unreal.MaterialEditingLibrary.get_material_property_input_node(material,unreal.MaterialProperty.MP_BASE_COLOR)==color
    assert unreal.MaterialEditingLibrary.get_material_property_input_node(material,unreal.MaterialProperty.MP_EMISSIVE_COLOR)==zero
    if not unreal.EditorAssetLibrary.save_loaded_asset(material) or not unreal.EditorAssetLibrary.save_loaded_asset(instance):
        raise RuntimeError('角色受光材质保存失败')
    unreal.log('角色受光修复已保存，现有网格与旧角色存档继续使用原实例路径。')
migrate()