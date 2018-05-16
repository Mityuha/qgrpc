###############################################################################
#
# Включаемый файл QMake для использования GRPC
#
###############################################################################

!defined(GRPC_VERSION, var){
    GRPC_VERSION=1.9
}

GRPC_SUPPORT_VERSIONS = "1.0" "1.9"

for(v, GRPC_SUPPORT_VERSIONS){
    GRPC_VERSION_OK=
    isEqual(GRPC_VERSION, $${v}){
        GRPC_VERSION_OK=1
        break()
    }
}

isEmpty(GRPC_VERSION_OK) {
    error("$$TARGET: Unknown GRPC version. Possible versions: $$GRPC_SUPPORT_VERSIONS")
}


PROTOC3_NAME=protoc3
GRPC_PREFIX_PATH=
WIN32_SOME_SDK="C:\\some_sdk"

unix {
    GRPC_CANDIDATES = "/opt/fort" "/usr/local"
    GRPC_PREFIX_PATH=
    for(c, GRPC_CANDIDATES){
		exists($${c}/bin/$$PROTOC3_NAME){
			GRPC_PREFIX_PATH=$${c}
			break()
		}
	}

	 isEmpty( GRPC_PREFIX_PATH ) {
	    error("$$TARGET: Cannot find GRPC path")
	}
}

!build_pass:message("$$TARGET: GRPC prefix path = $$GRPC_PREFIX_PATH")

INCLUDEPATH = "$$GRPC_PREFIX_PATH/include" $$INCLUDEPATH
win32: LIBS += -L"$$WIN32_SOME_SDK\win32-msvc2015\grpc" #path to libs
###error("$$INCLUDEPATH")

### Описываем "компилятор" .proto --> C++ Source, .cc файл:
# Наш вход - это файлы, перечисленные в PROTOC.


protoc3.input = GRPC
protoc3.output  = ${QMAKE_FILE_BASE}.pb.cc
protoc3.CONFIG += no_link target_predeps
protoc3.variable_out = SOURCES
unix:  protoc3.commands = $$GRPC_PREFIX_PATH/bin/$$PROTOC3_NAME --proto_path=${QMAKE_FILE_PATH} --cpp_out="$$_PRO_FILE_PWD_"  ${QMAKE_FILE_IN}
win32: protoc3.commands = $$PROTOC3_NAME --proto_path=${QMAKE_FILE_PATH} --cpp_out="$$_PRO_FILE_PWD_"  ${QMAKE_FILE_IN}
protoc3.dependency_type = TYPE_C                                         

## Описываем "компилятор" .proto --> C++ Source, .h файл:
protoc3_h.input = GRPC
protoc3_h.output  =${QMAKE_FILE_BASE}.pb.h
protoc3_h.CONFIG += no_link target_predeps
protoc3_h.variable_out = HEADERS
protoc3_h.commands = $${protoc3.commands}
protoc3_h.dependency_type = TYPE_C

### Подключаем "компилятор" PROTOC --> C к системе сборки QMake:
QMAKE_EXTRA_COMPILERS += protoc3 protoc3_h

### Описываем "компилятор" .proto --> C++ Source, .cc файл:
# Наш вход - это файлы, перечисленные в PROTOC.
grpc.input = GRPC
grpc.output = ${QMAKE_FILE_BASE}.grpc.pb.cc
grpc.CONFIG += no_link target_predeps
grpc.variable_out = SOURCES
win32: grpc_cpp_plugin = \"$$GRPC_PREFIX_PATH/bin/grpc_cpp_plugin.exe\"
unix: grpc_cpp_plugin =  $$GRPC_PREFIX_PATH/bin/grpc_cpp_plugin
unix:  grpc.commands = $$GRPC_PREFIX_PATH/bin/$$PROTOC3_NAME --proto_path=${QMAKE_FILE_PATH} --grpc_out="$$_PRO_FILE_PWD_" --plugin=protoc-gen-grpc=$${grpc_cpp_plugin} ${QMAKE_FILE_IN}
win32: grpc.commands = $$PROTOC3_NAME --proto_path=${QMAKE_FILE_PATH} --grpc_out="$$_PRO_FILE_PWD_" --plugin=protoc-gen-grpc=$${grpc_cpp_plugin} ${QMAKE_FILE_IN}
grpc.dependency_type = TYPE_C

## Описываем "компилятор" .proto --> C++ Source, .h файл:
grpc_h.input = GRPC
grpc_h.output  =${QMAKE_FILE_BASE}.grpc.pb.h
grpc_h.CONFIG += no_link target_predeps
grpc_h.variable_out = HEADERS
grpc_h.commands = $${grpc.commands}
grpc_h.dependency_type = TYPE_C

### Подключаем "компилятор" PROTOC --> C к системе сборки QMake:
QMAKE_EXTRA_COMPILERS += grpc grpc_h

win32 {
  DEFINES += _WIN32_WINNT=0x600 # TODO Guess correct version
  DEFINES += _SCL_SECURE_NO_WARNINGS
  DEFINES += _CRT_SECURE_NO_WARNINGS
  QMAKE_CFLAGS_WARN_ON += -wd4100
  QMAKE_CXXFLAGS_WARN_ON += -wd4100
  QMAKE_LIBDIR = "$$WIN32_SOME_SDK/lib/win32-msvc2015/grpc" $${QMAKE_LIBDIR}
  LIBS *= -lgdi32
  CONFIG(debug, debug|release) {
    LIBS *= -lgrpc++d -lgprd -lgrpcd
  } else {
    LIBS *= -lgrpc++ -lgpr -lgrpc
  }

  CONFIG(debug, debug|release) {
    !win32-g++: LIBS *= -llibprotobufd
  } else {
    !win32-g++: LIBS *= -llibprotobuf
  }

  isEqual(GRPC_VERSION, "1.0") {
      LIBS *= -llibeay32 -lssleay32 -lzlib1
  }
  isEqual(GRPC_VERSION, "1.9") {
      LIBS *= -lssl -lcrypto -lzlibstatic
  }

}
unix {
  LIBS *= -L$$GRPC_PREFIX_PATH/lib/ -lgrpc++ -lgrpc \
           -Wl,--no-as-needed -lgrpc++_reflection -Wl,--as-needed \
           -L$$GRPC_PREFIX_PATH/lib/protobuf3 -lprotobuf -lpthread -ldl
}

build_pass{
    defined(QGRPC_CONFIG, var) {
        QGRPC_PY_PATH=$$PWD/qgrpc/genQGrpc.py
        win32:QGRPC_PY_PATH=$$replace(QGRPC_PY_PATH, /, \\)
        INCLUDEPATH += $$PWD/qgrpc
        MOC_PATH=$$[QT_INSTALL_BINS]/moc
        win32:MOC_PATH=$$replace(MOC_PATH, /, \\)
        
        QGRPC_MONITOR = $$PWD/qgrpc/QGrpcMonitor.h
        qgrpc_monitor_moc.input = QGRPC_MONITOR
        qgrpc_monitor_moc.output = $${MOC_DIR}/moc_${QMAKE_FILE_BASE}.cpp
        qgrpc_monitor_moc.CONFIG = target_predeps
        qgrpc_monitor_moc.variable_out = GENERATED_SOURCES
        qgrpc_monitor_moc.commands = $${MOC_PATH} $(DEFINES) ${QMAKE_FILE_IN} -o $${MOC_DIR}/moc_${QMAKE_FILE_BASE}.cpp
        qgrpc_monitor_moc.dependency_type = TYPE_C
        QMAKE_EXTRA_COMPILERS *= qgrpc_monitor_moc
        
        CUR_CONFIG=
        contains(QGRPC_CONFIG, "client") {
            CUR_CONFIG = "client"
        }

        contains(QGRPC_CONFIG, "server") {
            CUR_CONFIG = "server"
        }
        
        isEmpty(CUR_CONFIG) {
            error("QGRPC_CONFIG variable is not (correctly) specified or is empty")
        }
        
        #YES! 'client' and 'server' are not supported together
        #To support it, create special compiler for both configurations
        #(These compilers would have no difference except '.command' section)
        
        
        OUT_H=${QMAKE_FILE_BASE}.qgrpc.$${CUR_CONFIG}.h
        qgrpc_$${CUR_CONFIG}_h.input = GRPC
        qgrpc_$${CUR_CONFIG}_h.output = $${OUT_H}
        qgrpc_$${CUR_CONFIG}_h.CONFIG += target_predeps
        qgrpc_$${CUR_CONFIG}_h.clean += $${OUT_H}
        qgrpc_$${CUR_CONFIG}_h.variable_out = HEADERS
        qgrpc_$${CUR_CONFIG}_h.commands += python $${QGRPC_PY_PATH} ${QMAKE_FILE_IN} $${CUR_CONFIG} $$_PRO_FILE_PWD_
        qgrpc_$${CUR_CONFIG}_h.dependency_type = TYPE_C
        QMAKE_EXTRA_COMPILERS *= qgrpc_$${CUR_CONFIG}_h
    

    
        for(file, GRPC) {
            QGRPC_OUT_FILENAME=$$system('python $${QGRPC_PY_PATH} $${file} $${CUR_CONFIG} --get-outfile')
            QGRPC_OUT_H=$${QGRPC_OUT_FILENAME}.h
            QGRPC_OUT_MOC_CPP=moc_$${QGRPC_OUT_FILENAME}.cpp
            MOC_OUT_FILENAME=$${MOC_DIR}/$${QGRPC_OUT_MOC_CPP}
            win32:MOC_OUT_FILENAME=$$replace(MOC_OUT_FILENAME, /, \\)
            
            #generate and compile moc file
            COMPILER_NAME=QGRPC_$${CUR_CONFIG}_MOC_GEN_$${QGRPC_OUT_FILENAME}
            $${COMPILER_NAME}.input = QGRPC_OUT_H
            $${COMPILER_NAME}.output = $${MOC_OUT_FILENAME}
            $${COMPILER_NAME}.CONFIG += target_predeps
            $${COMPILER_NAME}.variable_out = SOURCES
            $${COMPILER_NAME}.commands = $${MOC_PATH} $(DEFINES) $${QGRPC_OUT_H} -o $${MOC_OUT_FILENAME} 
            $${COMPILER_NAME}.dependency_type = TYPE_C
            QMAKE_EXTRA_COMPILERS *= $${COMPILER_NAME}
    
        }
    
    }
}
