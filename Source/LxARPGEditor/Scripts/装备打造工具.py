"""通过虚幻5.8官方MCP生成装备打造资产并执行回归验证。"""
import unreal


@unreal.uclass()
class EquipmentCraftingTools(unreal.ToolsetDefinition):
    """装备打造资产与回归测试入口。"""

    @unreal.ufunction(static=True, params=[str], ret=str,
                     meta={"AICallable": "", "DisplayName": "处理装备打造", "Category": "装备打造|验证"})
    def process_equipment(stage):
        """所有修改通过项目原生工具完成，预览不控制计算机屏幕。"""
        if stage == "inspect":
            return unreal.LxEquipmentCraftingAssetLibrary.inspect_blacksmith()
        if stage == "build":
            return unreal.LxEquipmentCraftingAssetLibrary.build_crafting_widgets()
        if stage == "npc":
            return unreal.LxEquipmentCraftingAssetLibrary.configure_blacksmith()
        if stage == "preview":
            return str(unreal.LxEquipmentCraftingAssetLibrary.preview_crafting())
        if stage == "tests":
            unreal.SystemLibrary.execute_console_command(None, "Automation RunTests LxARPG.EquipmentCrafting.Rules+LxARPG.EquipmentCrafting.SaveRoundTrip+LxARPG.EquipmentCrafting.Transactions+LxARPG.Interaction.AllFeatureAssets+LxARPG.Interaction.AutomaticRange+LxARPG.Interaction.ComponentFeatureGate+LxARPG.Interaction.DialogueOptions+LxARPG.Interaction.EntranceSelection+LxARPG.Interaction.FeatureLifecycle+LxARPG.Interaction.RequirementMigration+LxARPG.Interaction.RequirementModes+LxARPG.InteractionTree.AssetAndEditor+LxARPG.Save.SpawnPoint.AtomicCaptureAndIdentity+LxARPG.Save.SpawnPoint.DiskRoundTripAndNamespaces+LxARPG.Save.SpawnPoint.OwnerIdentityAndEmptyPopulation+LxARPG.Save.CaptureFailureProtection+LxARPG.Save.CharacterRestore+LxARPG.Save.DiskRoundTrip+LxARPG.Save.IdentityProtection+LxARPG.Save.InteractionAutomaticAttachment+LxARPG.Save.InteractionContainers+LxARPG.Save.InteractionStableIdentity+LxARPG.Save.InvalidFileProtection+LxARPG.Save.ItemInstanceRoundTrip+LxARPG.Save.RepeatedLoadPreservesCache+LxARPG.Save.WorldCleanupAndTravel+LxARPG.UI.InventoryDragDrop.ListRouting")
            return "已启动装备打造、存档、交互与物品容器回归"
        if stage == "maptests":
            unreal.SystemLibrary.execute_console_command(None, "Automation RunTests LxARPG.Save.InteractionMapPIEPersistence")
            return "已启动独立真实地图存档回归"
        if stage == "quit":
            unreal.SystemLibrary.execute_console_command(None, "QUIT_EDITOR")
            return "已请求关闭验证会话"
        raise ValueError(stage)


unreal.EditorPythonScripting.set_keep_python_script_alive(True)
unreal.ToolsetRegistry.register_toolset_class(EquipmentCraftingTools)
unreal.SystemLibrary.execute_console_command(None, "ModelContextProtocol.StartServer 8041")
unreal.log("装备打造官方MCP已就绪，端口8041")
