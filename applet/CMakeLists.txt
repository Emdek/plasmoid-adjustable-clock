project(adjustableclock)

find_package(KDE4 REQUIRED)
include(KDE4Defaults)

add_definitions(${QT_DEFINITIONS} ${KDE4_DEFINITIONS})

include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR} ${KDE4_INCLUDES})

set(adjustableclock_SRCS Applet.cpp DataSource.cpp Clock.cpp Configuration.cpp ThemeWidget.cpp EditorWidget.cpp ComponentWidget.cpp OptionWidget.cpp ThemeDelegate.cpp ExpressionDelegate.cpp ExpressionLineEdit.cpp)

file(GLOB _themes themes/*)

foreach(_current_theme ${_themes})
	get_filename_component(_theme_name ${_current_theme} NAME)

	install(DIRECTORY ${_current_theme} DESTINATION ${DATA_INSTALL_DIR}/plasma/adjustableclock)
endforeach(_current_theme)

qt4_add_resources(adjustableclock_RESOURCES_RCC data/resources.qrc)
kde4_add_ui_files(adjustableclock_SRCS ui/appearance.ui ui/clipboard.ui ui/weather.ui ui/editor.ui ui/component.ui)
kde4_add_plugin(plasma_applet_adjustableclock ${adjustableclock_SRCS} ${adjustableclock_RESOURCES_RCC})

target_link_libraries(plasma_applet_adjustableclock
	plasmaclock
	${QT_QTDECLARATIVE_LIBRARY}
	${QT_QTSCRIPT_LIBRARY}
	${QT_QTWEBKIT_LIBRARY}
	${KDE4_PLASMA_LIBS}
	${KDE4_KTEXTEDITOR_LIBS}
	${KDE4_KDEUI_LIBS}
	)

install(TARGETS plasma_applet_adjustableclock DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES plasma-applet-adjustableclock.desktop DESTINATION ${SERVICES_INSTALL_DIR})
