# 2025-12-10T17:57:18.665224700
import vitis

client = vitis.create_client()
client.set_workspace(path="PS_Softcore_SA")

comp = client.get_component(name="MB_SSA_App")
status = comp.clean()

platform = client.get_component(name="MB_SSA_Platform")
status = platform.build()

status = platform.build()

comp.build()

status = comp.clean()

client.delete_component(name="MB_SSA_Platform")

platform = client.create_platform_component(name = "MB_SSA_Platform",hw_design = "$COMPONENT_LOCATION/../../PL_Softcore_SA/BD_Softcore_SA_wrapper.xsa",os = "standalone",cpu = "microblaze_0",domain_name = "standalone_microblaze_0")

status = platform.build()

comp.build()

status = comp.clean()

status = platform.build()

client.delete_component(name="MB_SSA_Platform")

platform = client.create_platform_component(name = "MB_SSA_Platform",hw_design = "$COMPONENT_LOCATION/../../PL_Softcore_SA/BD_Softcore_SA_wrapper.xsa",os = "standalone",cpu = "microblaze_0",domain_name = "standalone_microblaze_0")

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = comp.clean()

client.delete_component(name="MB_SSA_Platform")

platform = client.create_platform_component(name = "MB_SSA_Platform",hw_design = "$COMPONENT_LOCATION/../../PL_Softcore_SA/BD_Softcore_SA_wrapper.xsa",os = "standalone",cpu = "microblaze_0",domain_name = "standalone_microblaze_0")

status = platform.build()

status = platform.build()

comp.build()

status = comp.clean()

client.delete_component(name="MB_SSA_Platform")

platform = client.create_platform_component(name = "MB_SSA_Platform",hw_design = "$COMPONENT_LOCATION/../../PL_Softcore_SA/BD_Softcore_SA_wrapper.xsa",os = "standalone",cpu = "microblaze_0",domain_name = "standalone_microblaze_0")

status = platform.build()

comp.build()

vitis.dispose()

