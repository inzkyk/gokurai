cmake_minimum_required(VERSION 3.16)

if (NOT ix_DEV)
  set(CMAKE_BUILD_TYPE "Release")
endif ()

project(gokurai)

add_library(lua
  "./external/lua_5_4_4/src/luaconf.h"
  "./external/lua_5_4_4/src/lapi.h"
  "./external/lua_5_4_4/src/lapi.c"
  "./external/lua_5_4_4/src/lauxlib.h"
  "./external/lua_5_4_4/src/lauxlib.c"
  "./external/lua_5_4_4/src/lbaselib.c"
  "./external/lua_5_4_4/src/lcode.h"
  "./external/lua_5_4_4/src/lcode.c"
  "./external/lua_5_4_4/src/lcorolib.c"
  "./external/lua_5_4_4/src/lctype.h"
  "./external/lua_5_4_4/src/lctype.c"
  "./external/lua_5_4_4/src/ldblib.c"
  "./external/lua_5_4_4/src/ldebug.h"
  "./external/lua_5_4_4/src/ldebug.c"
  "./external/lua_5_4_4/src/ldo.h"
  "./external/lua_5_4_4/src/ldo.c"
  "./external/lua_5_4_4/src/ldump.c"
  "./external/lua_5_4_4/src/lfunc.h"
  "./external/lua_5_4_4/src/lfunc.c"
  "./external/lua_5_4_4/src/lgc.h"
  "./external/lua_5_4_4/src/lgc.c"
  "./external/lua_5_4_4/src/linit.c"
  "./external/lua_5_4_4/src/liolib.c"
  "./external/lua_5_4_4/src/ljumptab.h"
  "./external/lua_5_4_4/src/llex.h"
  "./external/lua_5_4_4/src/llex.c"
  "./external/lua_5_4_4/src/llimits.h"
  "./external/lua_5_4_4/src/lmathlib.c"
  "./external/lua_5_4_4/src/lmem.h"
  "./external/lua_5_4_4/src/lmem.c"
  "./external/lua_5_4_4/src/loadlib.c"
  "./external/lua_5_4_4/src/lobject.h"
  "./external/lua_5_4_4/src/lobject.c"
  "./external/lua_5_4_4/src/lopcodes.h"
  "./external/lua_5_4_4/src/lopcodes.c"
  "./external/lua_5_4_4/src/lopnames.h"
  "./external/lua_5_4_4/src/loslib.c"
  "./external/lua_5_4_4/src/lparser.h"
  "./external/lua_5_4_4/src/lparser.c"
  "./external/lua_5_4_4/src/lprefix.h"
  "./external/lua_5_4_4/src/lstate.h"
  "./external/lua_5_4_4/src/lstate.c"
  "./external/lua_5_4_4/src/lstring.h"
  "./external/lua_5_4_4/src/lstring.c"
  "./external/lua_5_4_4/src/lstrlib.c"
  "./external/lua_5_4_4/src/ltable.h"
  "./external/lua_5_4_4/src/ltable.c"
  "./external/lua_5_4_4/src/ltablib.c"
  "./external/lua_5_4_4/src/ltm.h"
  "./external/lua_5_4_4/src/ltm.c"
  "./external/lua_5_4_4/src/lua.h"
  "./external/lua_5_4_4/src/lualib.h"
  "./external/lua_5_4_4/src/lundump.h"
  "./external/lua_5_4_4/src/lundump.c"
  "./external/lua_5_4_4/src/lutf8lib.c"
  "./external/lua_5_4_4/src/lvm.h"
  "./external/lua_5_4_4/src/lvm.c"
  "./external/lua_5_4_4/src/lzio.h"
  "./external/lua_5_4_4/src/lzio.c"
)

target_include_directories(lua INTERFACE "./external/lua_5_4_4/src")

add_library(doctest
  "./external/doctest_2_4_11/doctest.hpp"
  "./external/doctest_2_4_11/doctest.cpp"
)

target_include_directories(doctest INTERFACE "./external/doctest_2_4_11/")

add_library(sokol_time
  "./external/sokol_time/sokol_time.h"
  "./external/sokol_time/sokol_time.c"
)

target_include_directories(sokol_time INTERFACE "./external/sokol_time/")

add_library(ix
  # "./src/ix/ix.natvis"
  "./src/ix/ix.hpp"
  "./src/ix/ix_Result.hpp"
  "./src/ix/ix_test_Vector.hpp"
  "./src/ix/ix_test_Map.hpp"
  "./src/ix/ix_test_HashMap.hpp"
  # "./src/ix/ix_report_env.hpp"
  "./src/ix/ix_Windows.hpp"
  "./src/ix/ix_new.hpp"
  "./src/ix/ix_new_delete.hpp"
  "./src/ix/ix_initializer_list.hpp"
  "./src/ix/ix_limits.hpp"
  "./src/ix/ix_limits.cpp"
  # "./src/ix/ix_software_prefetch.hpp"
  "./src/ix/ix_compare.hpp"
  "./src/ix/ix_type_traits.hpp"
  "./src/ix/ix_type_traits.cpp"
  "./src/ix/ix_utility.hpp"
  "./src/ix/ix_utility.cpp"
  "./src/ix/ix_polyfill.hpp"
  "./src/ix/ix_polyfill.cpp"
  "./src/ix/ix_SystemManager.hpp"
  "./src/ix/ix_SystemManager.cpp"
  "./src/ix/ix_doctest.hpp"
  "./src/ix/ix_doctest.cpp"
  "./src/ix/ix_Logger.hpp"
  "./src/ix/ix_Logger.cpp"
  "./src/ix/ix_environment.cpp"
  "./src/ix/ix_environment.hpp"
  "./src/ix/ix_min_max.hpp"
  "./src/ix/ix_min_max.cpp"
  # "./src/ix/ix_even_odd.hpp"
  # "./src/ix/ix_even_odd.cpp"
  "./src/ix/ix_wrap.hpp"
  "./src/ix/ix_wrap.cpp"
  # "./src/ix/ix_type.cpp"
  # "./src/ix/ix_type.cpp"
  "./src/ix/ix_bulk.hpp"
  # "./src/ix/ix_slide.hpp"
  # "./src/ix/ix_slide.cpp"
  "./src/ix/ix_assert.hpp"
  "./src/ix/ix_assert.cpp"
  "./src/ix/ix_random.hpp"
  "./src/ix/ix_random.cpp"
  "./src/ix/ix_random_fill.hpp"
  "./src/ix/ix_random_fill.cpp"
  "./src/ix/ix_combinatorics.hpp"
  "./src/ix/ix_combinatorics.cpp"
  "./src/ix/ix_scanf.hpp"
  "./src/ix/ix_scanf.cpp"
  "./src/ix/ix_bit.hpp"
  "./src/ix/ix_bit.cpp"
  "./src/ix/ix_hash.hpp"
  "./src/ix/ix_hash.cpp"
  "./src/ix/ix_string.hpp"
  "./src/ix/ix_string.cpp"
  "./src/ix/ix_printf.hpp"
  "./src/ix/ix_printf.cpp"
  "./src/ix/ix_memory.hpp"
  "./src/ix/ix_memory.cpp"
  # "./src/ix/ix_color.hpp"
  # "./src/ix/ix_color.cpp"
  "./src/ix/ix_math.hpp"
  "./src/ix/ix_math.cpp"
  "./src/ix/ix_file.hpp"
  "./src/ix/ix_file.cpp"
  "./src/ix/ix_filepath.hpp"
  "./src/ix/ix_filepath.cpp"
  # "./src/ix/ix_collision.hpp"
  # "./src/ix/ix_collision.cpp"
  # "./src/ix/ix_algorithm.hpp"
  # "./src/ix/ix_algorithm.cpp"
  "./src/ix/ix_DumbInt.hpp"
  "./src/ix/ix_DumbInt.cpp"
  "./src/ix/ix_DumbString.hpp"
  "./src/ix/ix_DumbString.cpp"
  # "./src/ix/ix_Vec2.hpp"
  # "./src/ix/ix_Vec2.cpp"
  # "./src/ix/ix_Vec2i.hpp"
  # "./src/ix/ix_Vec2i.cpp"
  "./src/ix/ix_Span.hpp"
  "./src/ix/ix_Span.cpp"
  "./src/ix/ix_UniquePointer.hpp"
  "./src/ix/ix_UniquePointer.cpp"
  "./src/ix/ix_StringView.hpp"
  "./src/ix/ix_StringView.cpp"
  "./src/ix/ix_Buffer.hpp"
  "./src/ix/ix_Buffer.cpp"
  # "./src/ix/ix_ArrayBuffer.hpp"
  # "./src/ix/ix_ArrayBuffer.cpp"
  # "./src/ix/ix_StackBuffer.hpp"
  # "./src/ix/ix_StackBuffer.cpp"
  "./src/ix/ix_Vector.hpp"
  "./src/ix/ix_Vector.cpp"
  # "./src/ix/ix_StackVector.hpp"
  # "./src/ix/ix_StackVector.cpp"
  "./src/ix/ix_Array.hpp"
  "./src/ix/ix_Array.cpp"
  "./src/ix/ix_Pair.hpp"
  # "./src/ix/ix_VectorMap.hpp"
  # "./src/ix/ix_VectorMap.cpp"
  # "./src/ix/ix_StackVectorMap.hpp"
  # "./src/ix/ix_StackVectorMap.cpp"
  # "./src/ix/ix_HashMapDoubleArray.hpp"
  # "./src/ix/ix_HashMapDoubleArray.cpp"
  "./src/ix/ix_HashMapSingleArray.hpp"
  "./src/ix/ix_HashMapSingleArray.cpp"
  "./src/ix/ix_HashSet.hpp"
  "./src/ix/ix_HashSet.cpp"
  # "./src/ix/ix_BTree.hpp"
  # "./src/ix/ix_BTree.cpp"
  "./src/ix/ix_Writer.hpp"
  "./src/ix/ix_Writer.cpp"
  # "./src/ix/ix_Pool.hpp"
  # "./src/ix/ix_Pool.cpp"
  # "./src/ix/ix_FlatPool.hpp"
  # "./src/ix/ix_FlatPool.cpp"
  # "./src/ix/ix_SpanPool.hpp"
  # "./src/ix/ix_SpanPool.cpp"
  # "./src/ix/ix_Arena.hpp"
  # "./src/ix/ix_Arena.cpp"
  # "./src/ix/ix_TagArena.hpp"
  # "./src/ix/ix_TagArena.cpp"
  "./src/ix/ix_StringArena.cpp"
  "./src/ix/ix_StringArena.cpp"
  "./src/ix/ix_Clock.hpp"
  "./src/ix/ix_Clock.cpp"
  # "./src/ix/ix_Heap.hpp"
  # "./src/ix/ix_Heap.cpp"
  # "./src/ix/ix_FixedLengthBitVector.hpp"
  # "./src/ix/ix_FixedLengthBitVector.cpp"
  # "./src/ix/ix_BitVector.hpp"
  # "./src/ix/ix_BitVector.cpp"
  # "./src/ix/ix_GenerationalID.hpp"
  # "./src/ix/ix_GenerationalID.cpp"
  # "./src/ix/ix_AhoCorasick.hpp"
  # "./src/ix/ix_AhoCorasick.cpp"
  # "./src/ix/ix_DoubleArrayAhoCorasick.hpp"
  # "./src/ix/ix_DoubleArrayAhoCorasick.cpp"
  "./src/ix/ix_TempFile.hpp"
  "./src/ix/ix_TempFile.cpp"
  # "./src/ix/ix_atomic.hpp"
  # "./src/ix/ix_atomic.cpp"
  "./src/ix/ix_Thread.hpp"
  "./src/ix/ix_Thread.cpp"
  # "./src/ix/ix_ThreadPool.hpp"
  # "./src/ix/ix_ThreadPool.cpp"
  "./src/ix/ix_Mutex.hpp"
  "./src/ix/ix_Mutex.cpp"
  # "./src/ix/ix_ConditionVariable.hpp"
  # "./src/ix/ix_ConditionVariable.cpp"
  # "./src/ix/ix_RingVector.hpp"
  # "./src/ix/ix_RingVector.cpp"
  # "./src/ix/ix_RingPool.hpp"
  # "./src/ix/ix_RingPool.cpp"
  "./src/ix/ix_Function.hpp"
  "./src/ix/ix_Function.cpp"
  "./src/ix/ix_CmdArgsEater.hpp"
  "./src/ix/ix_CmdArgsEater.cpp"
  "./src/ix/ix_HollowValue.hpp"
  "./src/ix/ix_HollowValue.cpp"
  # "./src/ix/ix_PVV.hpp"
  # "./src/ix/ix_PVV.cpp"
  "./src/ix/ix_defer.hpp"
  "./src/ix/ix_defer.cpp"
)

set_property(TARGET ix PROPERTY CXX_STANDARD 17)
target_link_libraries(ix PUBLIC doctest sokol_time)
target_compile_definitions(ix PRIVATE _UNICODE UNICODE)
target_compile_definitions(ix PRIVATE ix_DO_TEST=0)
target_include_directories(ix INTERFACE "./src/ix/")

if (NOT ix_DEV)
  target_compile_definitions(ix PUBLIC ix_SHIP=1)
endif ()

add_executable(gokurai
  "./src/gokurai/gokurai.hpp"
  "./src/gokurai/gokurai.cpp"
  "./src/gokurai/gokurai_cli.cpp"
)

set_property(TARGET gokurai PROPERTY CXX_STANDARD 17)
target_link_libraries(gokurai PRIVATE lua ix)
target_include_directories(gokurai PRIVATE "./src/gokurai/")

if (NOT ix_DEV)
  target_compile_definitions(gokurai PRIVATE ix_DO_TEST=0)
endif ()
