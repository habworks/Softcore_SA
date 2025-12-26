# 2025-12-25T06:12:24.287170600
import vitis

client = vitis.create_client()
client.set_workspace(path="PS_Softcore_SA")

platform = client.get_component(name="MB_SSA_Platform")
status = platform.build()

comp = client.get_component(name="MB_SSA_App")
comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

comp = client.get_component(name="MB_SSA_App")
status = comp.import_files(from_loc="$COMPONENT_LOCATION/../../../Arb_Power_Booster/Firmware/ArbPwrBooster_App/STM32CubeIDE/Application/User/App_Support/Src", files=["Terminal_Emulator_Support.c"], dest_dir_in_cmp = "src")

status = comp.import_files(from_loc="$COMPONENT_LOCATION/../../../Arb_Power_Booster/Firmware/ArbPwrBooster_App/STM32CubeIDE/Application/User/App_Support/Inc", files=["Terminal_Emulator_Support.h"], dest_dir_in_cmp = "src")

status = platform.build()

comp = client.get_component(name="MB_SSA_App")
comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

vitis.dispose()

