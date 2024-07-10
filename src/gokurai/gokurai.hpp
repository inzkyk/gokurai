#pragma once

#include <ix.hpp>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#else
#define EMSCRIPTEN_KEEPALIVE
#endif

class ix_FileHandle;
using GokuraiContext = void *;
using GokuraiResult = void *;

extern "C"
{
EMSCRIPTEN_KEEPALIVE GokuraiContext gokurai_context_create(const ix_FileHandle *out_handle,
                                                           const ix_FileHandle *err_handle);
EMSCRIPTEN_KEEPALIVE void gokurai_context_clear(GokuraiContext ctx);
EMSCRIPTEN_KEEPALIVE void gokurai_context_destroy(GokuraiContext ctx);
EMSCRIPTEN_KEEPALIVE void gokurai_context_feed_input(GokuraiContext ctx, const char *input, size_t input_length);
EMSCRIPTEN_KEEPALIVE void gokurai_context_feed_str(GokuraiContext ctx, const char *str);
EMSCRIPTEN_KEEPALIVE void gokurai_context_end_input(GokuraiContext ctx, GokuraiResult result);

EMSCRIPTEN_KEEPALIVE GokuraiResult gokurai_result_create();
EMSCRIPTEN_KEEPALIVE void gokurai_result_destroy(GokuraiResult result);
EMSCRIPTEN_KEEPALIVE const char *gokurai_result_get_output(GokuraiResult result);
EMSCRIPTEN_KEEPALIVE size_t gokurai_result_get_output_length(GokuraiResult result);
}
