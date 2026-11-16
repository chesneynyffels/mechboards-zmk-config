# SPDX-License-Identifier: MIT

if(CONFIG_NYFFELS_DISPLAY_ART)
  zephyr_library()
  zephyr_library_include_directories(${CMAKE_SOURCE_DIR}/include include)
  zephyr_library_sources(src/display_art_state.c)

  if(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
    zephyr_library_sources(src/display_art_sync.c)
  else()
    zephyr_library_sources(src/display_art_right.c src/display_art_images.c)
    # GEM calls draw_animation() once to create the peripheral decoration.
    # Wrap just that entry point; its status widgets and the entire left screen
    # continue using the original GEM implementation.
    zephyr_ld_options(-Wl,--wrap=draw_animation)
  endif()
endif()
