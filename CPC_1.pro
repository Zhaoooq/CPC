QT += widgets printsupport network

CONFIG += c++11
CONFIG -= app_bundle

TEMPLATE = app
TARGET = CPC_1

INCLUDEPATH += . \
    hardware \
    control \
    algorithms \
    ui \
    state

SOURCES += \
    main.cpp \
    algorithms/OpcCounter.cpp \
    control/PressureValveController.cpp \
    hardware/Ads1115PressureSensor.cpp \
    hardware/N4IOA01Valve.cpp \
    hardware/PT100Sensor.cpp \
    hardware/PwmOutputs.cpp \
    network/RemoteDashboard.cpp \
    ui/ControlWidgets.cpp \
    ui/Formatters.cpp \
    ui/MainWindowUi.cpp \
    ui/PlotSetup.cpp \
    ui/TouchDoubleSpinBox.cpp \
    ui/WatermarkWidget.cpp \
    qcustomplot.cpp \
    LiquidControlSystem.cpp

HEADERS += \
    algorithms/OpcCounter.h \
    control/PressureValveController.h \
    control/TemperaturePid.h \
    daq_worker.h \
    hardware/Ads1115PressureSensor.h \
    hardware/N4IOA01Valve.h \
    hardware/PT100Sensor.h \
    hardware/PinMap.h \
    hardware/PwmOutputs.h \
    network/RemoteDashboard.h \
    state/AppRuntimeState.h \
    ui/ControlWidgets.h \
    ui/Formatters.h \
    ui/MainWindowUi.h \
    ui/PlotSetup.h \
    ui/TouchDoubleSpinBox.h \
    ui/WatermarkWidget.h \
    qcustomplot.h \
    LiquidControlSystem.h

RESOURCES += resources.qrc

# The vendor D2XX shared object is deployed beside the CPC executable.
LIBS += -L$$PWD -llgpio -lftd2xx
QMAKE_RPATHDIR += $$PWD
