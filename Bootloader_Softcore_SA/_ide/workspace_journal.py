# 2026-03-19T05:44:27.778592800
import vitis

client = vitis.create_client()
client.set_workspace(path="Bootloader_Softcore_SA")

vitis.dispose()

vitis.dispose()

comp = client.get_component(name="MB_SSA_BL_App")
status = comp.clean()

platform = client.get_component(name="MB_SSA_BL_Platform")
status = platform.build()

comp.build()

vitis.dispose()

