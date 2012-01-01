file(GLOB _mo_files *.mo)

foreach(_current_MO_FILE ${_mo_files})
   get_filename_component(_lang ${_current_MO_FILE} NAME_WE)
   install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/${_lang}.mo DESTINATION ${LOCALE_INSTALL_DIR}/${_lang}/LC_MESSAGES/ RENAME plasma_applet_adjustableclock.mo)
endforeach(_current_MO_FILE)
