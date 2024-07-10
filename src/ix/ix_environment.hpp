#pragma once

#include "ix.hpp"

size_t ix_hardware_concurrency();
size_t ix_process_id();
size_t ix_thread_id();

bool ix_is_valgrind_active();
