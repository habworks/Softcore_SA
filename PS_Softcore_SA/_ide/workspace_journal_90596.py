# 2025-12-05T17:17:48.542719800
import vitis

client = vitis.create_client()
client.set_workspace(path="PS_Softcore_SA")

platform = client.create_platform_component(name = "MB_SSA_Platform",hw_design = "$COMPONENT_LOCATION/../../PL_Softcore_SA/BD_Softcore_SA_wrapper.xsa",os = "standalone",cpu = "microblaze_0",domain_name = "standalone_microblaze_0")

comp = client.create_app_component(name="MB_SSA_App",platform = "$COMPONENT_LOCATION/../MB_SSA_Platform/export/MB_SSA_Platform/MB_SSA_Platform.xpfm",domain = "standalone_microblaze_0")

comp = client.get_component(name="MB_SSA_App")
status = comp.import_files(from_loc="$COMPONENT_LOCATION/../../../Acquire_FPGA/Vitis_Workspace/MicroBlaze_VUI/MB_VUI_App/src", files=["AXI_IRQ_Controller_Support.c", "AXI_IRQ_Controller_Support.h", "AXI_SPI_Display_SSD1309.c", "AXI_SPI_Display_SSD1309.h", "AXI_Timer_PWM_Support.c", "AXI_Timer_PWM_Support.h", "AXI_UART_Lite_Support.c", "AXI_UART_Lite_Support.h", "Hab_Types.h", "main.c"], dest_dir_in_cmp = "src")

status = comp.import_files(from_loc="$COMPONENT_LOCATION/../../../Acquire_FPGA/Vitis_Workspace/MicroBlaze_VUI/MB_VUI_App/src", files=["U8G2"], dest_dir_in_cmp = "src")

platform = client.get_component(name="MB_SSA_Platform")
status = platform.build()

comp = client.get_component(name="MB_SSA_App")
comp.build()

status = comp.clean()

status = platform.build()

comp.build()

status = comp.clean()

status = platform.build()

comp.build()

vitis.dispose()

