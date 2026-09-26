# YueManHuaDan.pro  ——  《月满华诞，码上团圆》 Qt 工程（Qt 5.15 / Qt 6 均可）

QT += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

# 想打开音效时，取消下面两行注释，并把 wav 放进 sounds/ 目录
# QT += multimedia
# DEFINES += YMH_SOUND

TEMPLATE = app
TARGET   = YueManHuaDan
CONFIG   += c++17
CONFIG   -= debug_and_release

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    nightscene.cpp \
    gamewidget.cpp \
    endingcard.cpp \
    soundfx.cpp

HEADERS += \
    theme.h \
    mainwindow.h \
    nightscene.h \
    gamewidget.h \
    endingcard.h \
    soundfx.h

RESOURCES += resources.qrc

# Windows 下的可执行图标（可选）：准备好 app.ico 后取消注释
# RC_ICONS = app.ico

# 中文源码：统一按 UTF-8 处理
*-msvc* {
    QMAKE_CXXFLAGS += /utf-8
    DEFINES += _CRT_SECURE_NO_WARNINGS
} else:*-g++ {
    QMAKE_CXXFLAGS += -finput-charset=UTF-8
    QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-parameter
}

# 默认输出、中间产物都落在构建目录里，保持源码树干净（推荐外部/shadow 构建）
DESTDIR = $$OUT_PWD
MOC_DIR = build/moc
OBJECTS_DIR = build/obj
RCC_DIR = build/rcc
UI_DIR = build/ui
