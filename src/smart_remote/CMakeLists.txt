#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

target_include_directories(app PRIVATE .)

# NORDIC SDK APP START
target_sources(app PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/app_esb.c
  ${CMAKE_CURRENT_SOURCE_DIR}/app_timeslot.c
  ${CMAKE_CURRENT_SOURCE_DIR}/drv_mic.c
  ${CMAKE_CURRENT_SOURCE_DIR}/dvi_adpcm.c
)

