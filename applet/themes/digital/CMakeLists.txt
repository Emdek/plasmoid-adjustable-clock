configure_file(${CMAKE_CURRENT_SOURCE_DIR}/metadata.desktop ${CMAKE_CURRENT_BINARY_DIR}/plasma-adjustable-clock-addon-digital.desktop COPYONLY)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/plasma-adjustable-clock-addon-digital.desktop
	DESTINATION ${SERVICES_INSTALL_DIR})

install(FILES metadata.desktop
	DESTINATION ${DATA_INSTALL_DIR}/plasma/adjustableclock/digital)

install(FILES contents/ui/main.html
	DESTINATION ${DATA_INSTALL_DIR}/plasma/adjustableclock/digital/contents/ui/)
