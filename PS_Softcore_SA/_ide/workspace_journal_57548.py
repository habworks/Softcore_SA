# 2026-03-10T05:32:44.035329600
import vitis

client = vitis.create_client()
client.set_workspace(path="PS_Softcore_SA")

platform = client.get_component(name="MB_SSA_Platform")
status = platform.build()

comp = client.get_component(name="MB_SSA_App")
comp.build()

vitis.dispose()

