if(CONFIG_TOTEM_REGOUT0_3V3)
  zephyr_library()
  zephyr_library_sources(src/regout0.c)
endif()
