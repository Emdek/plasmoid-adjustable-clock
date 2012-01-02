add_definitions (${QT_DEFINITIONS} ${KDE4_DEFINITIONS} -DQT_STRICT_ITERATORS )
add_definitions( -DQT_NO_CAST_FROM_ASCII )
add_definitions( -DQT_NO_CAST_TO_ASCII )
add_definitions( -DDISABLE_NEPOMUK_LEGACY )


cmake_minimum_required(VERSION 2.8)

add_subdirectory(applet)