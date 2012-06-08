TEMPLATE = app
CONFIG += console
CONFIG -= qt

SOURCES += \
    ../../src/testMain.cpp \
    ../../src/ObjectPoolUnitTest.cpp \
    ../../src/ObjectPool.cpp

HEADERS += \
    ../../src/ScopedAllocator.h \
    ../../src/ObjectPoolScopedAllocator.h \
    ../../src/ObjectPool.h \
    ../../src/ArrayUtils.h

