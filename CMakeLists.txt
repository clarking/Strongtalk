
####
####  (C) 1994 - 2021, The Strongtalk authors and contributors
####  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
####

cmake_minimum_required(VERSION 3.0)
project(Strongtalk)

set(CMAKE_CXX_EXTENSIONS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_BUILD_TYPE Debug)
set(USE_CCACHE ON)

find_library(LIB_RLINE readline usr/include/readline )

if(${LIB_RLINE} MATCHES "NOTFOUND")
    message(FATAL_ERROR "libreadline required")
endif()

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    set(build-type debug)
else ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    set(build-type release)
endif ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")

set(CMAKE_BINARY_DIR 'build')
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -m32)

if ("${USE_CCACHE}" STREQUAL "ON")
    set(CMAKE_C_COMPILER_LAUNCHER CCACHE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/.ccache /usr/bin/ccache)
    set(CMAKE_CXX_COMPILER_LAUNCHER CCACHE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/.ccache /usr/bin/ccache)
endif ()

set(VM_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/vm)
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(EXT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ext)


add_subdirectory(${EXT_DIR}/spdlog)

# disabled as they are not required by the vm

#add_subdirectory(${EXT_DIR}/absl)
#add_subdirectory(${EXT_DIR}/fmt)
#add_subdirectory(${EXT_DIR}/googletest)
#add_subdirectory(${EXT_DIR}/nasm)

execute_process(COMMAND git branch --show-current WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}" OUTPUT_VARIABLE GIT_CURRENT_BRANCH ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND git rev-parse ${GIT_CURRENT_BRANCH} WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}" OUTPUT_VARIABLE GIT_CURRENT_REVISION ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)

add_compile_options(-fpermissive)
add_compile_definitions(GIT_CURRENT_BRANCH=${GIT_CURRENT_BRANCH})
add_compile_definitions(GIT_CURRENT_REVISION=${GIT_CURRENT_REVISION})

add_executable(Strongtalk

        ${VM_DIR}/assembler/Address.cpp
        ${VM_DIR}/assembler/Assembler.cpp
        ${VM_DIR}/assembler/CodeBuffer.cpp
        ${VM_DIR}/assembler/Displacement.cpp
        ${VM_DIR}/assembler/IntegerFreeList.cpp
        ${VM_DIR}/assembler/Label.cpp
        ${VM_DIR}/assembler/Location.cpp
        ${VM_DIR}/assembler/MacroAssembler.cpp
        ${VM_DIR}/assembler/Register.cpp
        ${VM_DIR}/assembler/x86_mapping.cpp
        ${VM_DIR}/assembler/x86_registers.cpp

        ${VM_DIR}/code/BlockScopeNode.cpp
        ${VM_DIR}/code/CodeTable.cpp
        ${VM_DIR}/code/CompiledInlineCache.cpp
        ${VM_DIR}/code/InliningDatabase.cpp
        ${VM_DIR}/code/JumpTable.cpp
        ${VM_DIR}/code/Locations.cpp
        ${VM_DIR}/code/LogicalAddress.cpp
        ${VM_DIR}/code/MapConformance.cpp
        ${VM_DIR}/code/MethodScopeNode.cpp
        ${VM_DIR}/code/NameDescriptor.cpp
        ${VM_DIR}/code/NameNode.cpp
        ${VM_DIR}/code/NativeCode.cpp
        ${VM_DIR}/code/NativeInstruction.cpp
        ${VM_DIR}/code/NativeMethod.cpp
        ${VM_DIR}/code/NativeMethodScopes.cpp
        ${VM_DIR}/code/NonInlinedBlockScopeNode.cpp
        ${VM_DIR}/code/PolymorphicInlineCache.cpp
        ${VM_DIR}/code/ProgramCounterDescriptor.cpp
        ${VM_DIR}/code/ProgramCounterDescriptorInfoClass.cpp
        ${VM_DIR}/code/PseudoRegisterMapping.cpp
        ${VM_DIR}/code/RegisterMask.cpp
        ${VM_DIR}/code/RelocationInformation.cpp
        ${VM_DIR}/code/ScopeDescriptor.cpp
        ${VM_DIR}/code/ScopeDescriptorNode.cpp
        ${VM_DIR}/code/ScopeDescriptorRecorder.cpp
        ${VM_DIR}/code/StubRoutines.cpp
        ${VM_DIR}/code/Zone.cpp
        ${VM_DIR}/code/ZoneHeap.cpp
        ${VM_DIR}/compiler/BasicBlock.cpp
        ${VM_DIR}/compiler/BasicBlockIterator.cpp
        ${VM_DIR}/compiler/BitVector.cpp
        ${VM_DIR}/compiler/CodeGenerator.cpp
        ${VM_DIR}/compiler/CompiledLoop.cpp
        ${VM_DIR}/compiler/Compiler.cpp
        ${VM_DIR}/compiler/CompileTimeClosure.cpp
        ${VM_DIR}/compiler/CostModel.cpp
        ${VM_DIR}/compiler/DefinitionUsage.cpp
        ${VM_DIR}/compiler/DefinitionUsageInfo.cpp
        ${VM_DIR}/compiler/Expression.cpp
        ${VM_DIR}/compiler/Inliner.cpp
        ${VM_DIR}/compiler/InliningPolicy.cpp
        ${VM_DIR}/compiler/NodeBuilder.cpp
        ${VM_DIR}/compiler/Node.cpp
        ${VM_DIR}/compiler/NodeFactory.cpp
        ${VM_DIR}/compiler/oldCodeGenerator.cpp
        ${VM_DIR}/compiler/OpCode.cpp
        ${VM_DIR}/compiler/PerformanceDebugger.cpp
        ${VM_DIR}/compiler/PrimitiveInliner.cpp
        ${VM_DIR}/compiler/PseudoRegister.cpp
        ${VM_DIR}/compiler/RecompilationScope.cpp
        ${VM_DIR}/compiler/RegisterAllocator.cpp
        ${VM_DIR}/compiler/Scope.cpp

        ${VM_DIR}/interpreter/ByteCodes.cpp
        ${VM_DIR}/interpreter/CodeIterator.cpp
        ${VM_DIR}/interpreter/DispatchTable.cpp
        ${VM_DIR}/interpreter/Floats.cpp
        ${VM_DIR}/interpreter/HeapCodeBuffer.cpp
        ${VM_DIR}/interpreter/InlineCacheIterator.cpp
        ${VM_DIR}/interpreter/InterpretedInlineCache.cpp
        ${VM_DIR}/interpreter/Interpreter.cpp
        ${VM_DIR}/interpreter/InterpreterGenerator.cpp
        ${VM_DIR}/interpreter/InterpreterStatistics.cpp
        ${VM_DIR}/interpreter/MethodClosure.cpp
        ${VM_DIR}/interpreter/MethodInterval.cpp
        ${VM_DIR}/interpreter/MethodIntervalFactory.cpp
        ${VM_DIR}/interpreter/MethodIterator.cpp
        ${VM_DIR}/interpreter/MethodNode.cpp
        ${VM_DIR}/interpreter/MethodPrinterClosure.cpp
        ${VM_DIR}/interpreter/MissingMethodBuilder.cpp
        ${VM_DIR}/interpreter/PrettyPrinter.cpp

        ${VM_DIR}/klass/AssociationKlass.cpp
        ${VM_DIR}/klass/BlockClosureKlass.cpp
        ${VM_DIR}/klass/ByteArrayKlass.cpp
        ${VM_DIR}/klass/ContextKlass.cpp
        ${VM_DIR}/klass/DoubleByteArrayKlass.cpp
        ${VM_DIR}/klass/DoubleKlass.cpp
        ${VM_DIR}/klass/DoubleValueArrayKlass.cpp
        ${VM_DIR}/klass/Klass.cpp
        ${VM_DIR}/klass/KlassKlass.cpp
        ${VM_DIR}/klass/MemOopKlass.cpp
        ${VM_DIR}/klass/MethodKlass.cpp
        ${VM_DIR}/klass/MixinKlass.cpp
        ${VM_DIR}/klass/ObjectArrayKlass.cpp
        ${VM_DIR}/klass/ProcessKlass.cpp
        ${VM_DIR}/klass/ProxyKlass.cpp
        ${VM_DIR}/klass/SmallIntegerKlass.cpp
        ${VM_DIR}/klass/SymbolKlass.cpp
        ${VM_DIR}/klass/VirtualFrameKlass.cpp
        ${VM_DIR}/klass/WeakArrayKlass.cpp

        ${VM_DIR}/lookup/CacheElement.cpp
        ${VM_DIR}/lookup/LookupCache.cpp
        ${VM_DIR}/lookup/LookupKey.cpp
        ${VM_DIR}/lookup/LookupResult.cpp
        ${VM_DIR}/lookup/LookupType.cpp

        ${VM_DIR}/memory/AgeTable.cpp
        ${VM_DIR}/memory/allocation.cpp
        ${VM_DIR}/memory/Array.cpp
        ${VM_DIR}/memory/ClassChange.cpp
        ${VM_DIR}/memory/Closure.cpp
        ${VM_DIR}/memory/Converter.cpp
        ${VM_DIR}/memory/Generation.cpp
        ${VM_DIR}/memory/Handle.cpp
        ${VM_DIR}/memory/MarkSweep.cpp
        ${VM_DIR}/memory/NewGeneration.cpp
        ${VM_DIR}/memory/OopFactory.cpp
        ${VM_DIR}/memory/PrintObjectClosure.cpp
        ${VM_DIR}/memory/Reflection.cpp
        ${VM_DIR}/memory/RememberedSet.cpp
        ${VM_DIR}/memory/SnapshotDescriptor.cpp
        ${VM_DIR}/memory/Space.cpp
        ${VM_DIR}/memory/SpaceSizes.cpp
        ${VM_DIR}/memory/SymbolTable.cpp
        ${VM_DIR}/memory/Universe.cpp
        ${VM_DIR}/memory/util.cpp
        ${VM_DIR}/memory/WaterMark.cpp

        ${VM_DIR}/oop/AssociationOopDescriptor.cpp
        ${VM_DIR}/oop/BlockClosureOopDescriptor.cpp
        ${VM_DIR}/oop/ByteArrayOopDescriptor.cpp
        ${VM_DIR}/oop/ContextOopDescriptor.cpp
        ${VM_DIR}/oop/DoubleByteArrayOopDescriptor.cpp
        ${VM_DIR}/oop/DoubleOopDescriptor.cpp
        ${VM_DIR}/oop/DoubleValueArrayOopDescriptor.cpp
        ${VM_DIR}/oop/ExpressionStackMapper.cpp
        ${VM_DIR}/oop/KlassOopDescriptor.cpp
        ${VM_DIR}/oop/MarkOopDescriptor.cpp
        ${VM_DIR}/oop/MemOopDescriptor.cpp
        ${VM_DIR}/oop/MethodOopDescriptor.cpp
        ${VM_DIR}/oop/MixinOopDescriptor.cpp
        ${VM_DIR}/oop/ObjectArrayOopDescriptor.cpp
        ${VM_DIR}/oop/OopDescriptor.cpp
        ${VM_DIR}/oop/ProcessOopDescriptor.cpp
        ${VM_DIR}/oop/ProxyOopDescriptor.cpp
        ${VM_DIR}/oop/SmallIntegerOopDescriptor.cpp
        ${VM_DIR}/oop/SymbolOopDescriptor.cpp
        ${VM_DIR}/oop/VirtualFrameOopDescriptor.cpp

        ${VM_DIR}/platform/posix.cpp
        ${VM_DIR}/primitive/AlienByteArrayPrimitives.cpp
        ${VM_DIR}/primitive/BehaviorPrimitives.cpp
        ${VM_DIR}/primitive/block_primitives.cpp
        ${VM_DIR}/primitive/ByteArrayPrimitives.cpp
        ${VM_DIR}/primitive/CallBackPrimitives.cpp
        ${VM_DIR}/primitive/DebugPrimitives.cpp
        ${VM_DIR}/primitive/DoubleByteArray_primitives.cpp
        ${VM_DIR}/primitive/DoubleOopPrimitives.cpp
        ${VM_DIR}/primitive/DoubleValueArray_primitives.cpp
        ${VM_DIR}/primitive/GeneratedPrimitives.cpp
        ${VM_DIR}/primitive/InterpretedPrimitiveCache.cpp
        ${VM_DIR}/primitive/MethodOopPrimitives.cpp
        ${VM_DIR}/primitive/MixinOopPrimitives.cpp
        ${VM_DIR}/primitive/ObjectArrayPrimitives.cpp
        ${VM_DIR}/primitive/OopPrimitives.cpp
        ${VM_DIR}/primitive/PrimitiveDescriptor.cpp
        ${VM_DIR}/primitive/Primitives.cpp
        ${VM_DIR}/primitive/PrimitivesGenerator.cpp
        ${VM_DIR}/primitive/primitives_table.cpp
        ${VM_DIR}/primitive/ProcessOopPrimitives.cpp
        ${VM_DIR}/primitive/ProxyOopPrimitives.cpp
        ${VM_DIR}/primitive/SmallIntegerOopPrimitives.cpp
        ${VM_DIR}/primitive/SystemPrimitives.cpp
        ${VM_DIR}/primitive/VirtualFrameOopPrimitives.cpp

        ${VM_DIR}/recompiler/Recompilation.cpp
        ${VM_DIR}/recompiler/RecompilationPolicy.cpp
        ${VM_DIR}/recompiler/Recompilee.cpp
        ${VM_DIR}/recompiler/RecompilerFrame.cpp

        ${VM_DIR}/runtime/arguments.cpp
        ${VM_DIR}/runtime/Bootstrap.cpp
        ${VM_DIR}/runtime/CallBack.cpp
        ${VM_DIR}/runtime/DebugInfo.cpp
        ${VM_DIR}/runtime/DeltaCallCache.cpp
        ${VM_DIR}/runtime/Delta.cpp
        ${VM_DIR}/runtime/DeltaProcess.cpp
        ${VM_DIR}/runtime/ErrorHandler.cpp
        ${VM_DIR}/runtime/evaluator.cpp
        ${VM_DIR}/runtime/exception.cpp
        ${VM_DIR}/runtime/flags.cpp
        ${VM_DIR}/runtime/FlagSettings.cpp
        ${VM_DIR}/runtime/FlatProfiler.cpp
        ${VM_DIR}/runtime/Frame.cpp
        ${VM_DIR}/runtime/init.cpp
        ${VM_DIR}/runtime/PeriodicTask.cpp
        ${VM_DIR}/runtime/Process.cpp
        ${VM_DIR}/runtime/Processes.cpp
        ${VM_DIR}/runtime/ProcessPreemptionTask.cpp
        ${VM_DIR}/runtime/ReservedSpace.cpp
        ${VM_DIR}/runtime/ResourceArea.cpp
        ${VM_DIR}/runtime/ResourceMark.cpp
        ${VM_DIR}/runtime/ResourceObject.cpp
        ${VM_DIR}/runtime/runtime.cpp
        ${VM_DIR}/runtime/SavedRegisters.cpp
        ${VM_DIR}/runtime/SlidingSystemAverage.cpp
        ${VM_DIR}/runtime/StackChunkBuilder.cpp
        ${VM_DIR}/runtime/Sweeper.cpp
        ${VM_DIR}/runtime/TempDecoder.cpp
        ${VM_DIR}/runtime/Timer.cpp
        ${VM_DIR}/runtime/UncommonBranch.cpp
        ${VM_DIR}/runtime/UnwindInfo.cpp
        ${VM_DIR}/runtime/VirtualFrame.cpp
        ${VM_DIR}/runtime/VirtualSpace.cpp
        ${VM_DIR}/runtime/VirtualSpaces.cpp
        ${VM_DIR}/runtime/VMOperation.cpp
        ${VM_DIR}/runtime/VMProcess.cpp
        ${VM_DIR}/runtime/VMSymbol.cpp

        ${VM_DIR}/utility/Console.cpp
        ${VM_DIR}/utility/ConsoleOutputStream.cpp
        ${VM_DIR}/utility/DebugNotifier.cpp
        ${VM_DIR}/utility/disassembler.cpp
        ${VM_DIR}/utility/EventLog.cpp
        ${VM_DIR}/utility/GrowableArray.cpp
        ${VM_DIR}/utility/Integer.cpp
        ${VM_DIR}/utility/IntegerOps.cpp
        ${VM_DIR}/utility/logging.cpp
        ${VM_DIR}/utility/LongInteger64.cpp
        ${VM_DIR}/utility/lprintf.cpp
        ${VM_DIR}/utility/ObjectIDTable.cpp
        ${VM_DIR}/utility/OutputStream.cpp
        ${VM_DIR}/utility/slist.cpp
        ${VM_DIR}/utility/StringOutputStream.cpp

        ${VM_DIR}/system/debugPrintable.cpp
        ${VM_DIR}/system/dll.cpp
        ${VM_DIR}/system/error.cpp
        ${VM_DIR}/system/sizes.cpp

        ${VM_DIR}/main.cpp)

target_include_directories(Strongtalk PUBLIC
        ${SRC_DIR}
        ${SRC_DIR}/vm/system
        ${SRC_DIR}/vm
        ${SRC_DIR}/vm/assembler
        ${SRC_DIR}/vm/code
        ${SRC_DIR}/vm/compiler
        ${SRC_DIR}/vm/interpreter
        ${SRC_DIR}/vm/klass
        ${SRC_DIR}/vm/lookup
        ${SRC_DIR}/vm/memory
        ${SRC_DIR}/vm/oop
        ${SRC_DIR}/vm/platform
        ${SRC_DIR}/vm/recompiler
        ${SRC_DIR}/vm/runtime
        ${SRC_DIR}/vm/utility

#        ${EXT_DIR}/nasm
#        ${EXT_DIR}nasm/disasm
#        ${EXT_DIR}nasm/include
#        ${EXT_DIR}nasm/x86
        )

target_link_libraries(Strongtalk PUBLIC spdlog readline)



