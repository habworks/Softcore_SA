# 2026-03-19T11:26:14.467095700
import vitis

client = vitis.create_client()
client.set_workspace(path="PS_Softcore_SA")

platform = client.get_component(name="MB_SSA_Platform")
status = platform.build()

comp = client.get_component(name="MB_SSA_App")
comp.build()

status = platform.build()

comp.build()

