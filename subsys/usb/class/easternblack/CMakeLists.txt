# SPDX-License-Identifier: Closed

zephyr_library()

zephyr_library_include_directories(
  ${ZEPHYR_BASE}/include/usb
  ${ZEPHYR_BASE}/subsys/usb
  )

zephyr_library_sources(
  core.c
  )
