
set_directory_properties (PROPERTIES PREFIX "")

macro (add_dplugin folder)
  add_library (dplugin_${folder} SHARED ${folder}/main.cc)
  set_target_properties (dplugin_${folder} PROPERTIES PREFIX "")
  link_libraries (${PROJECT_SO_LIBRARY})
  install (
    TARGETS dplugin_${folder}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/disir/
  )
endmacro()

add_dplugin (test)
add_dplugin (test_config_toml)
add_dplugin (test_config_json)
add_dplugin (toml)
add_dplugin (json)
