# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "")
  file(REMOVE_RECURSE
  "C:\\IMR_Projects\\IMR\\Softcore_SA\\PS_Softcore_SA\\MB_SSA_Platform\\microblaze_0\\standalone_microblaze_0\\bsp\\include\\sleep.h"
  "C:\\IMR_Projects\\IMR\\Softcore_SA\\PS_Softcore_SA\\MB_SSA_Platform\\microblaze_0\\standalone_microblaze_0\\bsp\\include\\xiltimer.h"
  "C:\\IMR_Projects\\IMR\\Softcore_SA\\PS_Softcore_SA\\MB_SSA_Platform\\microblaze_0\\standalone_microblaze_0\\bsp\\include\\xtimer_config.h"
  "C:\\IMR_Projects\\IMR\\Softcore_SA\\PS_Softcore_SA\\MB_SSA_Platform\\microblaze_0\\standalone_microblaze_0\\bsp\\lib\\libxiltimer.a"
  )
endif()
