"""通过官方 MCP 只读检查太阳、环境光和曝光。"""
import unreal, json
from pathlib import Path
OUT=Path(unreal.Paths.project_saved_dir())/'SunLighting'
OUT.mkdir(parents=True,exist_ok=True)
def props(obj,names):
    """容忍不同组件缺失属性，记录实际可读取的数据。"""
    result={}
    for name in names:
        try: result[name]=str(obj.get_editor_property(name))
        except Exception: pass
    return result
@unreal.uclass()
class SunAuditTools(unreal.ToolsetDefinition):
    """太阳问题的隔离诊断入口。"""
    @unreal.ufunction(static=True,params=[str],ret=str,meta={'AICallable':'','DisplayName':'检查太阳','Category':'太阳|验证'})
    def audit(stage):
        """读取关卡和运行状态，不保存用户关卡。"""
        if stage=='extra':
            exec((Path(unreal.Paths.project_saved_dir())/'SunLighting/额外检查.py').read_text(encoding='utf-8'),globals())
            return 'done'
        editor=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
        if stage=='tests':
            unreal.SystemLibrary.execute_console_command(editor.get_editor_world(),'Automation RunTests LxARPG.World.Time')
            return 'tests started'
        if stage=='load':
            unreal.EditorLoadingAndSavingUtils.load_map('/Game/项目内容/关卡/唯一关卡')
        if stage=='play':
            unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play()
            return 'playing'
        if stage=='stop':
            unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_end_play()
            return 'stopping'
        if stage=='quit':
            unreal.SystemLibrary.execute_console_command(editor.get_editor_world(),'QUIT_EDITOR')
            return 'quit'
        world=editor.get_game_world() if stage=='game' else editor.get_editor_world()
        rows=[]
        for a in unreal.GameplayStatics.get_all_actors_of_class(world,unreal.Actor):
            components=a.get_components_by_class(unreal.ActorComponent)
            selected=[c for c in components if isinstance(c,(unreal.LightComponentBase,unreal.PostProcessComponent,unreal.SkyAtmosphereComponent,unreal.ExponentialHeightFogComponent,unreal.CameraComponent))]
            if not selected and not isinstance(a,unreal.PostProcessVolume):continue
            row={'actor':a.get_actor_label(),'class':a.get_class().get_name(),'location':str(a.get_actor_location()),'properties':props(a,['preview_hour','noon_intensity','is_spatially_loaded','data_layer_assets','enabled','unbound','priority','blend_weight']), 'components':[]}
            for c in selected:
                item={'name':c.get_name(),'class':c.get_class().get_name(),'values':props(c,['intensity','visible','hidden_in_game','mobility','light_color','affects_world','real_time_capture','source_type','cubemap','lower_hemisphere_is_black','lower_hemisphere_color','fog_density','fog_height_falloff','fog_inscattering_luminance','volumetric_fog','post_process_blend_weight'])}
                if isinstance(c,unreal.SceneComponent): item['rotation']=str(c.get_world_rotation())
                row['components'].append(item)
                if isinstance(c,(unreal.CameraComponent,unreal.PostProcessComponent)):
                    settings=c.get_editor_property('post_process_settings' if isinstance(c,unreal.CameraComponent) else 'settings')
                    item['exposure']=props(settings,['auto_exposure_method','auto_exposure_min_brightness','auto_exposure_max_brightness','auto_exposure_bias','auto_exposure_speed_up','auto_exposure_speed_down','local_exposure_shadow_contrast_scale','local_exposure_highlight_contrast_scale','override_auto_exposure_min_brightness','override_auto_exposure_max_brightness'])
            if isinstance(a,unreal.PostProcessVolume):row['settings']=str(a.get_editor_property('settings'))
            rows.append(row)
        (OUT/(stage+'.json')).write_text(json.dumps(rows,ensure_ascii=False,indent=2),encoding='utf-8')
        return json.dumps(rows,ensure_ascii=False)
unreal.ToolsetRegistry.register_toolset_class(SunAuditTools)