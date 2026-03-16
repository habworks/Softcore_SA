# 2026-03-13T14:24:28.544347200
import vitis

client = vitis.create_client()
client.set_workspace(path="Bootloader_Softcore_SA")

platform = client.get_component(name="MB_SSA_BL_Platform")
status = platform.build()

comp = client.get_component(name="MB_SSA_BL_App")
comp.build()

vitis.dispose()

platform = client.get_component(name="MB_SSA_BL_Platform")
status = platform.build()

comp = client.get_component(name="MB_SSA_BL_App")
comp.build()

status = platform.update_hw(hw_design = "$COMPONENT_LOCATION/../../PL_Softcore_SA/BD_Softcore_SA_wrapper.xsa")

status = platform.build()

status = platform.build()

comp.build()

vitis.dispose()

