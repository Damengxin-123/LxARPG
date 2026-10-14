"""在专用诊断会话中按固定视角比较生成物的 GPU 成本，不保存关卡。"""
import json
import time
import traceback
from pathlib import Path
import unreal

# 跨帧保存固定镜头和诊断步骤。
STATE = {}
OUT = Path(unreal.Paths.project_saved_dir()) / "GeneratedRuntime"
OUT.mkdir(parents=True, exist_ok=True)


def start():
    """创建固定尺寸离屏镜头，逐项排除海面、墙体和生成物。"""
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    editor = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
    world = editor.get_editor_world()
    location, rotation = editor.get_level_viewport_camera_info()
    camera = actors.spawn_actor_from_class(unreal.SceneCapture2D, location, rotation)
    component = camera.get_component_by_class(unreal.SceneCaptureComponent2D)
    target = unreal.RenderingLibrary.create_render_target2d(world, 1280, 720, unreal.TextureRenderTargetFormat.RTF_RGBA8)
    component.set_editor_property("texture_target", target)
    component.set_editor_property("capture_source", unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR)
    component.set_editor_property("capture_every_frame", False)
    component.set_editor_property("always_persist_rendering_state", True)
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_set_viewport_realtime(False)
    all_actors = actors.get_all_level_actors()
    oceans = [a for a in all_actors if isinstance(a, unreal.LxOceanActor)]
    for ocean in oceans:
        ocean.rebuild_ocean()
        ocean.set_editor_property("foam_observer", camera)
    walls = [a for a in all_actors if isinstance(a, unreal.LxBrickWallActor)]
    generated = [a for a in all_actors if a.get_components_by_class(unreal.ProceduralMeshComponent)]
    phases = [("完整场景", []), ("隐藏海面", oceans), ("隐藏砖墙", walls), ("隐藏所有生成物", generated)]
    STATE.update(camera=camera, component=component, target=target, world=world, phases=phases, phase=0, advance=False,
                 next=time.monotonic()+8, origins={"location":str(location), "rotation":str(rotation)})
    unreal.SystemLibrary.execute_console_command(world, "r.ProfileGPU.ShowUI 0")
    unreal.SystemLibrary.execute_console_command(world, "Automation RunTests LxARPG.Diagnostics.GeneratedWorldRuntime")
    STATE["handle"] = unreal.register_slate_post_tick_callback(tick)


def tick(delta):
    """每个阶段预热后输出截图和 GPU 分项，结束时回收临时镜头。"""
    STATE["component"].capture_scene()
    if time.monotonic() < STATE["next"]:
        return
    try:
        phase = STATE["phase"]
        name, _ = STATE["phases"][phase]
        if not STATE["advance"]:
            unreal.log("生成工具GPU阶段="+name)
            unreal.RenderingLibrary.export_render_target(STATE["world"], STATE["target"], str(OUT), name+".png")
            unreal.SystemLibrary.execute_console_command(STATE["world"], "ProfileGPU")
            STATE["advance"] = True
            STATE["next"] = time.monotonic()+1
            return
        phase += 1
        if phase < len(STATE["phases"]):
            STATE["component"].clear_hidden_components()
            for actor in STATE["phases"][phase][1]:
                STATE["component"].hide_actor_components(actor)
            STATE["phase"] = phase
            STATE["advance"] = False
            STATE["next"] = time.monotonic()+8
        else:
            unreal.unregister_slate_post_tick_callback(STATE["handle"])
            unreal.get_editor_subsystem(unreal.EditorActorSubsystem).destroy_actor(STATE["camera"])
            (OUT/"渲染诊断完成.json").write_text(json.dumps(STATE["origins"], ensure_ascii=False), encoding="utf-8")
    except Exception:
        unreal.unregister_slate_post_tick_callback(STATE["handle"])
        unreal.get_editor_subsystem(unreal.EditorActorSubsystem).destroy_actor(STATE["camera"])
        (OUT/"错误.txt").write_text(traceback.format_exc(), encoding="utf-8")


start()
