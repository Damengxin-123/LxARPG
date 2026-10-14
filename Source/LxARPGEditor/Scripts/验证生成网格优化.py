"""通过官方 MCP 在专用编辑器会话验证生成网格；不保存用户关卡。"""
import json
from pathlib import Path
import unreal

# 只保存专用验证关卡与报告，原有关卡只读打开。
OUT = Path(unreal.Paths.project_saved_dir()) / 'GeneratedMeshOptimization'
OUT.mkdir(parents=True, exist_ok=True)
TEST_MAP = '/Game/开发测试/生成网格验证'


def summarize():
    """记录当前关卡生成器的渲染缓存、碰撞与实际重建次数。"""
    rows = []
    subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    for actor in subsystem.get_all_level_actors():
        if not isinstance(actor, (unreal.LxBrickWallActor, unreal.LxStoneFloorActor, unreal.LxStonePillarActor, unreal.LxTimberPanelActor, unreal.LxTileRoofActor, unreal.LxCargoPileActor)) and not actor.get_components_by_class(unreal.LxGeneratedMeshRenderComponent):
            continue
        meshes = actor.get_components_by_class(unreal.StaticMeshComponent)
        for component in meshes:
            mesh = component.get_editor_property('static_mesh')
            if not mesh:
                continue
            row = dict(actor=actor.get_actor_label(), cls=actor.get_class().get_name(), mesh=mesh.get_path_name(),
                       nanite=mesh.get_editor_property('nanite_settings').get_editor_property('enabled'),
                       visible=component.is_visible())
            try:
                row['builds'] = actor.get_editor_property('geometry_build_count')
            except Exception:
                pass
            rows.append(row)
    return rows


@unreal.uclass()
class GeneratedMeshVerificationTools(unreal.ToolsetDefinition):
    """生成网格优化的隔离验证入口。"""
    @unreal.ufunction(static=True, params=[str], ret=str,
                     meta={'AICallable':'', 'DisplayName':'验证生成网格优化', 'Category':'生成优化|验证'})
    def verify_generated_mesh(stage):
        """串行执行回归测试、保存重载和只读场景检查。"""
        world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
        if stage == 'tests':
            unreal.SystemLibrary.execute_console_command(world, 'Automation RunTests LxARPG.World.GeneratedMesh')
            return '已启动生成网格回归测试'
        if stage == 'regression':
            unreal.SystemLibrary.execute_console_command(world, 'Automation RunTests LxARPG.World.GeneratedMesh+LxARPG.World.BrickWall+LxARPG.World.StoneFloor+LxARPG.World.StonePillar+LxARPG.World.TimberPanel+LxARPG.World.TileRoof+LxARPG.World.CargoPile+LxARPG.World.HarborRoad+LxARPG.World.RoundBuilding')
            return '已启动场景生成器回归测试'
        if stage == 'incremental_load':
            filename = str(Path(unreal.Paths.project_saved_dir()) / 'Automation' / '增量网格验证.umap')
            world = unreal.EditorLoadingAndSavingUtils.load_map(filename)
            if not world:
                raise RuntimeError('无法重载增量测试关卡，请先运行 tests')
            rows = []
            for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
                for component in actor.get_components_by_class(unreal.LxGeneratedMeshRenderComponent):
                    component.finalize_mesh()
                    row = dict(actor=actor.get_actor_label(),
                               updates=component.get_editor_property('source_update_count'),
                               conversions=component.get_editor_property('geometry_conversion_count'))
                    rows.append(row)
                    if row['updates'] or row['conversions']:
                        raise RuntimeError('重载后重复生成或转换：' + json.dumps(row, ensure_ascii=False))
            if len(rows) != 7:
                raise RuntimeError('重载测试应包含七种程序化生成器')
            (OUT/'增量重载.json').write_text(json.dumps(rows, ensure_ascii=False, indent=2), encoding='utf-8')
            # 复用网格后仍须保留可编辑源数据，不能只保存最终显示结果。
            for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
                if isinstance(actor, unreal.LxBlockChunkActor):
                    if len(actor.get_editor_property('blocks')) != 4096:
                        raise RuntimeError('重载后方块源数据丢失')
                if isinstance(actor, unreal.LxSplineSurfaceActor):
                    spline = actor.get_editor_property('surface_spline')
                    count = spline.get_number_of_spline_points()
                    if actor.get_editor_property('last_applied_control_point_count') != count:
                        raise RuntimeError('重载后丢失上次应用的控制点数量')
                    spline.add_spline_point(unreal.Vector(-300, 0, 0), unreal.SplineCoordinateSpace.LOCAL, True)
                    actor.rebuild_surface()
                    if spline.get_number_of_spline_points() != count + 1:
                        raise RuntimeError('重载后手动新增的样条点被错误重采样')
            return json.dumps(rows, ensure_ascii=False)
        if stage == 'save':
            world = unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
            subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
            classes = [unreal.LxBrickWallActor, unreal.LxStoneFloorActor, unreal.LxStonePillarActor,
                       unreal.LxTimberPanelActor, unreal.LxTileRoofActor, unreal.LxCargoPileActor,
                       unreal.LxSplineRoadActor, unreal.LxSplineSurfaceActor, unreal.LxRingSurfaceActor,
                       unreal.LxRingCylinderActor, unreal.LxStraightStairActor, unreal.LxBlockChunkActor]
            for index, cls in enumerate(classes):
                subsystem.spawn_actor_from_class(cls, unreal.Vector(index*3000, 0, 0))
            if not unreal.EditorLoadingAndSavingUtils.save_map(world, TEST_MAP):
                raise RuntimeError('验证关卡保存失败')
            rows = summarize()
            (OUT/'保存前.json').write_text(json.dumps(rows,ensure_ascii=False,indent=2),encoding='utf-8')
            return json.dumps(rows,ensure_ascii=False)
        if stage == 'load':
            unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
            unreal.EditorLoadingAndSavingUtils.load_map(TEST_MAP)
            rows = summarize()
            (OUT/'重载后.json').write_text(json.dumps(rows,ensure_ascii=False,indent=2),encoding='utf-8')
            return json.dumps(rows,ensure_ascii=False)
        if stage == 'audit':
            unreal.EditorLoadingAndSavingUtils.load_map('/Game/项目内容/关卡/唯一关卡')
            rows = summarize()
            (OUT/'唯一关卡检查.json').write_text(json.dumps(rows,ensure_ascii=False,indent=2),encoding='utf-8')
            return json.dumps(rows,ensure_ascii=False)
        if stage == 'play':
            unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play()
            return '已请求在编辑器运行当前关卡'
        if stage == 'stop':
            unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_end_play()
            return '已请求停止运行'
        if stage == 'quit':
            unreal.SystemLibrary.execute_console_command(world, 'QUIT_EDITOR')
            return '已结束验证会话'
        raise ValueError(stage)


unreal.ToolsetRegistry.register_toolset_class(GeneratedMeshVerificationTools)
