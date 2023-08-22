cmake_minimum_required(VERSION 3.27)

set(LLVM_INSTALL_DIR /home/owinebar/dev CACHE PATH "Install path of LLVM")
set(LLVM_ROOT "${LLVM_INSTALL_DIR}" CACHE PATH "LLVM package prefix path")
set(Clang_ROOT "${LLVM_INSTALL_DIR}" CACHE PATH "LLVM package prefix path")
set(LLVM_DIR "${LLVM_INSTALL_DIR}/lib/cmake/llvm" CACHE PATH "LLVM cmake directory")
set(CMAKE_C_COMPILER "${LLVM_INSTALL_DIR}/bin/clang" CACHE FILEPATH "C Compiler")
set(CMAKE_CXX_COMPILER "${LLVM_INSTALL_DIR}/bin/clang++" CACHE FILEPATH "C++ Compiler")
set(LLVM_USE_LINKER "${LLVM_INSTALL_DIR}/bin/lld" CACHE FILEPATH "Prefered linker")

project(cppgc-convert)


set(SOURCE_FILES ToolTemplate.cpp ResourcePath.cpp)
set(CMAKE_CXX_STANDARD 17 CACHE STRING "C++ Standard")
set(CMAKE_CXX_STANDARD_REQUIRED ON CACHE BOOL "C++ Standard is required")
#find_package(LLVM 18 REQUIRED CONFIG)
find_package(Clang 18 REQUIRED CONFIG)

list(APPEND CMAKE_MODULE_PATH ${LLVM_CMAKE_DIR} ${CLANG_CMAKE_DIR})
include(AddLLVM)
include(AddClang)
include_directories(${LLVM_INCLUDE_DIRS})
include_directories(${CLANG_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})
link_directories(${CLANG_LIBRARY_DIRS})

add_definitions(${LLVM_DEFINITIONS})
add_definitions(${CLANG_DEFINITIONS})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpie ")
if(NOT ${LLVM_ENABLE_RTTI})
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")
endif()

set(LLVM_LINK_COMPONENTS Support Analysis)
add_clang_executable(cppgc-convert ${SOURCE_FILES})
#target_link_libraries(cppgc-convert PRIVATE clangFrontendTool)
set (CLANG_LIBS
  clang
  clangTooling
  clangToolingRefactoring
  clangASTMatchers
  clangFrontend
  clangAST
  clangAnalysis
  clangBasic
  clangCodeGen
  clangDriver
  clangFrontendTool
  clangLex
  clangParse
  clangSema
  clangEdit
  clangSerialization
  clangStaticAnalyzerCheckers
  clangStaticAnalyzerCore
  clangStaticAnalyzerFrontend
)
foreach (clang_lib ${CLANG_LIBS})
  target_link_libraries( cppgc-convert PRIVATE ${clang_lib} )  
endforeach()
set (LLVM_LIBS
  LLVMFrontendOpenMP
  LLVMX86CodeGen
  LLVMX86Disassembler
  LLVMExecutionEngine
  LLVMAsmPrinter
  LLVMSelectionDAG
  LLVMX86Info
  LLVMMCParser
  LLVMCodeGen
  LLVMScalarOpts
  LLVMInstCombine
  LLVMTransformUtils
  LLVMAnalysis
  LLVMTarget
  LLVMCore
  LLVMMC
  LLVMSupport
  LLVMBitReader
  LLVMOption
)


foreach (llvm_lib ${LLVM_LIBS})
  target_link_libraries( cppgc-convert PRIVATE ${llvm_lib} )  
endforeach()

target_link_libraries( cppgc-convert PRIVATE dl )  
target_link_options( cppgc-convert PRIVATE -Wl,--warn-unresolved-symbols -Wl,-pie )  

set_target_properties(cppgc-convert PROPERTIES
  LINKER_LANGUAGE CXX
  PREFIX "")
