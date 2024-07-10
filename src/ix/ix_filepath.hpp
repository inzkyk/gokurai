#pragma once

#include "ix.hpp"

bool ix_is_path_separator(char c);
char *ix_next_path_separator(const char *p);
size_t ix_dirname_length(const char *filename);
size_t ix_dirname_length(const char *filename, size_t filename_length);
size_t ix_basename_length(const char *filename);
size_t ix_basename_length(const char *filename, size_t filename_length);
