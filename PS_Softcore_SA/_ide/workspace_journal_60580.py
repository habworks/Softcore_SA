# 2025-12-23T14:22:43.891673300
import vitis

client = vitis.create_client()
client.set_workspace(path="PS_Softcore_SA")

comp = client.get_component(name="MB_SSA_App")
status = comp.import_files(from_loc="$COMPONENT_LOCATION/../../../Acquire_FPGA/Vitis_Workspace/MicroBlaze_uSD/MB_uSD_App/src", files=["diskio.c", "diskio.h"], dest_dir_in_cmp = "FAT_FS")

status = comp.import_files(from_loc="C:\Users\habco\Downloads", files=["ff16.zip"], dest_dir_in_cmp = "FAT_FS")

status = comp.import_files(from_loc="C:\Users\habco\Downloads\ff16\source", files=["ff.c", "ff.h", "ffconf.h", "ffsystem.c", "ffunicode.c"], dest_dir_in_cmp = "FAT_FS")

comp = client.get_component(name="MB_SSA_App")
status = comp.clean()

client.delete_component(name="MB_SSA_Platform")

platform = client.create_platform_component(name = "MB_SSA_Platform",hw_design = "$COMPONENT_LOCATION/../../PL_Softcore_SA/BD_Softcore_SA_wrapper.xsa",os = "standalone",cpu = "microblaze_0",domain_name = "standalone_microblaze_0")

platform = client.get_component(name="MB_SSA_Platform")
status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = comp.clean()

client.delete_component(name="MB_SSA_Platform")

platform = client.create_platform_component(name = "MB_SSA_Platform",hw_design = "$COMPONENT_LOCATION/../../PL_Softcore_SA/BD_Softcore_SA_wrapper.xsa",os = "standalone",cpu = "microblaze_0",domain_name = "standalone_microblaze_0")

status = platform.build()

comp.build()

status = comp.clean()

client.delete_component(name="MB_SSA_Platform")

platform = client.create_platform_component(name = "MB_SSA_Platform",hw_design = "$COMPONENT_LOCATION/../../PL_Softcore_SA/BD_Softcore_SA_wrapper.xsa",os = "standalone",cpu = "microblaze_0",domain_name = "standalone_microblaze_0")

status = platform.build()

comp.build()

vitis.dispose()

