#-------------------------------------------------
#
# Project created by QtCreator 2017-07-20T10:56:27
#
#-------------------------------------------------

QT      -= core gui
#TEMPLATE = lib
# CONFIG  += staticlib
CONFIG  -= c++11

linux {

    QMAKE_CXXFLAGS += -DMACRO_PLATFORM_LINUX
    contains(QT_ARCH, i386) {
        LIBS += -L../../bin/linux/g++-x86
    } else:contains(QT_ARCH, x86_64) {
        LIBS += -L../../bin/linux/g++-x86_64 \
                -L/usr/lib/x86_64-linux-gnu
    }
}

LIBS += "-L$$PWD/../lib/cul"

debug {
    TARGET  = tmap-demo-d
    LIBS   += -lcommon
}
#release {
#    TARGET  = tmap-demo
#    LIBS   += -lcommon
#}

#unix {
    #target.path = /usr/lib
    #INSTALLS += target
#}

QMAKE_CXXFLAGS += -std=c++17
QMAKE_LFLAGS   += -std=c++17
LIBS           += -ltinyxml2 -lsfml-graphics -lsfml-window -lsfml-system -lz

SOURCES += \
    ../src/Base64.cpp        \
    ../src/ColorLayer.cpp    \
    ../src/TiledMap.cpp      \
    ../src/TiledMapImpl.cpp  \
    ../src/TileEffect.cpp    \
    ../src/TileLayer.cpp     \
    ../src/TileSet.cpp       \
    ../src/TiXmlHelpers.cpp  \
    ../src/ZLib.cpp

HEADERS += \
    ../src/ColorLayer.hpp    \
    ../src/MapLayer.hpp      \
    ../src/TiledMapImpl.hpp  \
    ../src/TileLayer.hpp     \
    ../src/TileSet.hpp       \
    ../src/TiXmlHelpers.hpp


HEADERS += \
    ../inc/tmap/Base64.hpp                  \
    ../inc/tmap/MapObject.hpp               \
    ../inc/tmap/TilePropertiesInterface.hpp \
    ../inc/tmap/TileEffect.hpp              \
    ../inc/tmap/TiledMap.hpp                \
    ../inc/tmap/ZLib.hpp

SOURCES += \
    ../demo/map-demo.cpp

INCLUDEPATH += \
    ../inc           \
    ../lib/cul/inc
