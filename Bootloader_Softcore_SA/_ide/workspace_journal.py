# 2026-03-21T07:36:09.299374500
import vitis

client = vitis.create_client()
client.set_workspace(path="Bootloader_Softcore_SA")

comp = client.get_component(name="MB_SSA_BL_App")
status = comp.clean()

client.delete_component(name="MB_SSA_BL_Platform")

platform = client.create_platform_component(name = "MB_SSA_BL_Platform",hw_design = "$COMPONENT_LOCATION/../../PL_Softcore_SA/BD_Softcore_SA_wrapper.xsa",os = "standalone",cpu = "microblaze_0",domain_name = "standalone_microblaze_0")

platform = client.get_component(name="MB_SSA_BL_Platform")
status = platform.build()

comp.build()

vitis.dispose()

