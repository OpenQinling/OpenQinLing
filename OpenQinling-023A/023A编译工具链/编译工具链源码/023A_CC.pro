QT -= gui

CONFIG += c++11 console
CONFIG -= app_bundle

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
        OpenQinling_ASM_AnalysisBlockCode.cpp \
        OpenQinling_ASM_BinaryObjectLib.cpp \
        OpenQinling_ASM_CompilerCode.cpp \
        OpenQinling_ASM_CompilerData.cpp \
        OpenQinling_ASM_FilterMemBlock.cpp \
        OpenQinling_ASM_FormatText.cpp \
        OpenQinling_ASM_GrammarDetection.cpp \
        OpenQinling_ASM_GrammaticalAnalysis.cpp \
        OpenQinling_ASM_TextFlag.cpp \
        OpenQinling_ASM_Transplant.cpp \
        OpenQinling_ASM_Typedefs.cpp \
        OpenQinling_C_AnalysisSrc.cpp \
        OpenQinling_C_Grammarparsing.cpp \
        OpenQinling_C_Initblock.cpp \
        OpenQinling_C_JudgestrtoolFunction.cpp \
        OpenQinling_C_LexicalAnalyzer.cpp \
        OpenQinling_C_Phrase.cpp \
        OpenQinling_C_Precompile.cpp \
        OpenQinling_C_SrcObjectInfo.cpp \
        OpenQinling_CompileCmdActuator.cpp \
        OpenQinling_Compiler.cpp \
        OpenQinling_DataStruct.cpp \
        OpenQinling_DebugFunction.cpp \
        OpenQinling_Driver_Terminalparsing.cpp \
        OpenQinling_FormatText.cpp \
        OpenQinling_LINK_CompileMemBlock.cpp \
        OpenQinling_LINK_ExecutableImage.cpp \
        OpenQinling_LINK_GrammaticalAnalysis.cpp \
        OpenQinling_LINK_MemorySet.cpp \
        OpenQinling_LINK_PlaceMath.cpp \
        OpenQinling_LINK_Typedefs.cpp \
        OpenQinling_Math.cpp \
        OpenQinling_PSDL_AnalysisFunction.cpp \
        OpenQinling_PSDL_ConstValueAnalysis.cpp \
        OpenQinling_PSDL_FormatText.cpp \
        OpenQinling_PSDL_GrammarticalAnalysis.cpp \
        OpenQinling_PSDL_MiddleNode.cpp \
        main.cpp

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
HEADERS += \
    OpenQinling_ASM_AnalysisBlockCode.h \
    OpenQinling_ASM_BinaryObjectLib.h \
    OpenQinling_ASM_CompilerCode.h \
    OpenQinling_ASM_CompilerData.h \
    OpenQinling_ASM_FilterMemBlock.h \
    OpenQinling_ASM_FormatText.h \
    OpenQinling_ASM_GrammarDetection.h \
    OpenQinling_ASM_GrammaticalAnalysis.h \
    OpenQinling_ASM_TextFlag.h \
    OpenQinling_ASM_Transplant.h \
    OpenQinling_ASM_Typedefs.h \
    OpenQinling_C_AnalysisSrc.h \
    OpenQinling_C_Grammarparsing.h \
    OpenQinling_C_Initblock.h \
    OpenQinling_C_JudgestrtoolFunction.h \
    OpenQinling_C_LexicalAnalyzer.h \
    OpenQinling_C_Phrase.h \
    OpenQinling_C_Precompile.h \
    OpenQinling_C_SrcObjectInfo.h \
    OpenQinling_CompileCmdActuator.h \
    OpenQinling_Compiler.h \
    OpenQinling_DataStruct.h \
    OpenQinling_DebugFunction.h \
    OpenQinling_Driver_Terminalparsing.h \
    OpenQinling_FormatText.h \
    OpenQinling_LINK_CompileMemBlock.h \
    OpenQinling_LINK_ExecutableImage.h \
    OpenQinling_LINK_GrammaticalAnalysis.h \
    OpenQinling_LINK_MemorySet.h \
    OpenQinling_LINK_PlaceMath.h \
    OpenQinling_LINK_Typedefs.h \
    OpenQinling_Math.h \
    OpenQinling_PSDL_AnalysisFunction.h \
    OpenQinling_PSDL_ConstValueAnalysis.h \
    OpenQinling_PSDL_FormatText.h \
    OpenQinling_PSDL_GrammarticalAnalysis.h \
    OpenQinling_PSDL_MiddleNode.h \
    OpenQinling_Typedefs.h
