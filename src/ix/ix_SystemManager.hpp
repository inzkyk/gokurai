#pragma once

#include "ix.hpp"
#include "ix_HashMapSingleArray.hpp"
#include "ix_Result.hpp"

class ix_SystemManager
{
    ix_StringHashMapSingleArray<ix_Result (*)()> m_system_names_to_deinitializer;

  public:
    static ix_SystemManager &init();
    static ix_SystemManager &get();
    static void deinit();

    ix_SystemManager();
    ~ix_SystemManager();

    ix_SystemManager(const ix_SystemManager &) = delete;
    ix_SystemManager(ix_SystemManager &&) = default;
    ix_SystemManager &operator=(const ix_SystemManager &) = delete;
    ix_SystemManager &operator=(ix_SystemManager &&) = default;

    bool is_initialized(const char *name) const;

    // We can not create `init_system(const char *name)' because that makes it impossible to strip unused init
    // functions from the final executable.
    ix_Result init_stdio();
    ix_Result init_sokol_time();
    ix_Result init_logger();
    ix_Result init_dummy();
    ix_Result init_dummy_nullptr();
};
