#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <vector>
#include <memory>
#include "../state/block.h"
#include "../state/sim_allocator.h"
#include "win_types.h"
#pragma clang diagnostic ignored "-Winvalid-offsetof"
#pragma pack(push, 1)
#include "../../include/ta_types.h"

static_assert(sizeof(Bitmap_00437b50) == 25, "");
static_assert(offsetof(Bitmap_00437b50, width) == 0, "");
static_assert(offsetof(Bitmap_00437b50, height) == 2, "");
static_assert(offsetof(Bitmap_00437b50, dx) == 4, "");
static_assert(offsetof(Bitmap_00437b50, dy) == 6, "");
static_assert(offsetof(Bitmap_00437b50, colour) == 8, "");
static_assert(offsetof(Bitmap_00437b50, flag9) == 9, "");
static_assert(offsetof(Bitmap_00437b50, count) == 10, "");
static_assert(offsetof(Bitmap_00437b50, kind) == 11, "");
static_assert(offsetof(Bitmap_00437b50, unknown_c) == 12, "");
static_assert(offsetof(Bitmap_00437b50, data) == 16, "");
static_assert(offsetof(Bitmap_00437b50, depth) == 20, "");
static_assert(offsetof(Bitmap_00437b50, pixels) == 24, "");
static_assert(sizeof(Channel_004b0940) == 164, "");
static_assert(offsetof(Channel_004b0940, used) == 0, "");
static_assert(offsetof(Channel_004b0940, id) == 4, "");
static_assert(offsetof(Channel_004b0940, unknown_8) == 8, "");
static_assert(offsetof(Channel_004b0940, value) == 32, "");
static_assert(offsetof(Channel_004b0940, unknown_24) == 36, "");
static_assert(sizeof(Class_004b0a70) == 1344, "");
static_assert(offsetof(Class_004b0a70, unknown_4) == 4, "");
static_assert(offsetof(Class_004b0a70, table) == 8, "");
static_assert(offsetof(Class_004b0a70, unknown_c) == 12, "");
static_assert(offsetof(Class_004b0a70, channels) == 28, "");
static_assert(offsetof(Class_004b0a70, activeCount) == 1340, "");
static_assert(sizeof(NameTable_004b0830) == 32, "");
static_assert(offsetof(NameTable_004b0830, unknown_0) == 0, "");
static_assert(offsetof(NameTable_004b0830, count) == 4, "");
static_assert(offsetof(NameTable_004b0830, unknown_8) == 8, "");
static_assert(offsetof(NameTable_004b0830, names) == 28, "");
