# 2026-03-05T05:35:16.649430800
import vitis

client = vitis.create_client()
client.set_workspace(path="PS_Softcore_SA")

vitis.dispose()

