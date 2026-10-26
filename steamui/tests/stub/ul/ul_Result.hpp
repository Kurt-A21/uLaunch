#pragma once
#include <cstdio>
#define UL_LOG_INFO(f, ...) std::printf("[info] " f "\n", ##__VA_ARGS__)
#define UL_LOG_WARN(f, ...) std::printf("[warn] " f "\n", ##__VA_ARGS__)
