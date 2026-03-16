# 2026-03-13T09:51:43.417234
import vitis

client = vitis.create_client()
client.set_workspace(path="Bootloader_Softcore_SA")

platform = client.create_platform_component(name = "MB_SSA_BL_Platform",hw_design = "$COMPONENT_LOCATION/../../PL_Softcore_SA/BD_Softcore_SA_wrapper.xsa",os = "standalone",cpu = "microblaze_0",domain_name = "standalone_microblaze_0")

comp = client.create_app_component(name="MB_SSA_BL_App",platform = "$COMPONENT_LOCATION/../MB_SSA_BL_Platform/export/MB_SSA_BL_Platform/MB_SSA_BL_Platform.xpfm",domain = "standalone_microblaze_0",template = "srec_spi_bootloader")

platform = client.get_component(name="MB_SSA_BL_Platform")
status = platform.build()

comp = client.get_component(name="MB_SSA_BL_App")
comp.build()

vitis.dispose()

