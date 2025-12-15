# 2025-12-10T17:24:03.353735
import vitis

client = vitis.create_client()
client.set_workspace(path="PS_Softcore_SA")

vitis.dispose()

