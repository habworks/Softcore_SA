# 2025-12-09T11:01:19.228204400
import vitis

client = vitis.create_client()
client.set_workspace(path="PS_Softcore_SA")

comp = client.get_component(name="MB_SSA_App")
status = comp.import_files(from_loc="$COMPONENT_LOCATION/../../../Acquire_FPGA/Vitis_Workspace/MicroBlaze_ADC/MB_ADC_App/src", files=["AXI_IMR_ADC_7476A_DUAL.c", "AXI_IMR_ADC_7476A_DUAL.h"], dest_dir_in_cmp = "src")

comp = client.get_component(name="MB_SSA_App")
status = comp.clean()

platform = client.get_component(name="MB_SSA_Platform")
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

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

vitis.dispose()

