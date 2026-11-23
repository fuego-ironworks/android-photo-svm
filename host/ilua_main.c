#include "photo_math.h"
#include "host_context.h"
#include "photo_regions.h"
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#if LUA_VERSION_NUM != 505
#error This consumer requires the pinned modified Lua 5.5 runtime
#endif
#define COORDINATES_TYPE "photo.named_coordinates.f32.v1"
#define REGION_FRAME_TYPE "photo.oriented_rgba_region_frame.v1"

static void read_name(lua_State *state, const char *field, char target[64])
{
    lua_getfield(state, 1, field);
    luaL_checktype(state, -1, LUA_TSTRING);
    size_t length ← 0;
    const char *text ← luaL_checklstring(state, -1, &length);
    if (!length || length >= 64 || memchr(text, 0, length)) luaL_error(state, "invalid %s identity", field);
    memcpy(target, text, length); target[length] ← 0;
    lua_pop(state, 1);
}

static int bind_coordinates(lua_State *state)
{
    luaL_checktype(state, 1, LUA_TTABLE); luaL_checktype(state, 2, LUA_TTABLE);
    size_t dimension ← lua_rawlen(state, 2);
    if (!dimension || dimension > 32) return luaL_error(state, "coordinate count must be 1..32");
    photo_coordinates *coordinates ← lua_newuserdatauv(state, sizeof(*coordinates), 0);
    memset(coordinates, 0, sizeof(*coordinates)); luaL_setmetatable(state, COORDINATES_TYPE);
    read_name(state, "representation", coordinates->representation);
    read_name(state, "space", coordinates->space);
    read_name(state, "basis", coordinates->basis);
    read_name(state, "metric", coordinates->metric);
    lua_getfield(state, 1, "scalar"); luaL_checktype(state, -1, LUA_TSTRING);
    size_t scalar_length ← 0;
    const char *scalar ← luaL_checklstring(state, -1, &scalar_length);
    if (scalar_length != 3 || memcmp(scalar, "f32", 3)) return luaL_error(state, "only the declared f32 carrier is available");
    lua_pop(state, 1);
    lua_getfield(state, 1, "variance"); luaL_checktype(state, -1, LUA_TSTRING);
    size_t variance_length ← 0;
    const char *variance ← luaL_checklstring(state, -1, &variance_length);
    if (variance_length == 6 && !memcmp(variance, "vector", 6)) coordinates->variance ← PHOTO_VECTOR;
    else if (variance_length == 8 && !memcmp(variance, "covector", 8)) coordinates->variance ← PHOTO_COVECTOR;
    else return luaL_error(state, "variance must be vector or covector");
    lua_pop(state, 1);
    coordinates->dimension ← (uint32_t)dimension;
    for (size_t index ← 0; index < dimension; ++index) {
        lua_rawgeti(state, 2, (lua_Integer)index + 1);
        luaL_checktype(state, -1, LUA_TNUMBER);
        /* Runtime transport only; no numerical kernel in the Lua bridge. */
        lua_Number value ← luaL_checknumber(state, -1);
        if (!isfinite(value) || value > FLT_MAX || value < -FLT_MAX) return luaL_error(state, "coordinate is not representable as finite f32");
        coordinates->values[index] ← (float)value;
        lua_pop(state, 1);
    }
    return 1;
}

static photo_math *bound_engine(lua_State *state)
{
    return lua_touserdata(state, lua_upvalueindex(1));
}

static int bind_projection(lua_State *state)
{
    const photo_coordinates *vector ← luaL_checkudata(state, 1, COORDINATES_TYPE);
    const photo_coordinates *covector ← luaL_checkudata(state, 2, COORDINATES_TYPE);
    float score;
    photo_status status ← photo_project32(bound_engine(state), vector, covector, &score);
    if (status != PHOTO_OK) return luaL_error(state, "%s", photo_status_text(status));
    lua_pushnumber(state, score); return 1;
}

static int bind_alignment_report(lua_State *state)
{
    const photo_coordinates *left ← luaL_checkudata(state, 1, COORDINATES_TYPE);
    const photo_coordinates *right ← luaL_checkudata(state, 2, COORDINATES_TYPE);
    photo_alignment_report report;
    photo_status status ← photo_inspect_alignment8(bound_engine(state), left, right, &report);
    if (status != PHOTO_OK) return luaL_error(state, "%s", photo_status_text(status));
    lua_createtable(state, 0, 2);
    lua_pushnumber(state, report.axis_ratio); lua_setfield(state, -2, "axis_ratio");
    lua_pushnumber(state, report.residual); lua_setfield(state, -2, "residual"); return 1;
}

static int bind_rotation(lua_State *state)
{
    const photo_coordinates *first ← luaL_checkudata(state, 1, COORDINATES_TYPE);
    const photo_coordinates *second ← luaL_checkudata(state, 2, COORDINATES_TYPE);
    const photo_coordinates *argument ← luaL_checkudata(state, 3, COORDINATES_TYPE);
    luaL_checktype(state, 4, LUA_TBOOLEAN);
    int inverse ← lua_toboolean(state, 4);
    photo_coordinates *output ← lua_newuserdatauv(state, sizeof(*output), 0);
    luaL_setmetatable(state, COORDINATES_TYPE);
    photo_status status ← photo_rotate8(bound_engine(state), first, second, argument, inverse, output);
    if (status != PHOTO_OK) return luaL_error(state, "%s", photo_status_text(status));
    return 1;
}

static int bind_components(lua_State *state)
{
    const photo_coordinates *coordinates ← luaL_checkudata(state, 1, COORDINATES_TYPE);
    lua_createtable(state, (int)coordinates->dimension, 0);
    for (uint32_t index ← 0; index < coordinates->dimension; ++index) {
        lua_pushnumber(state, coordinates->values[index]);
        lua_rawseti(state, -2, (lua_Integer)index + 1);
    }
    return 1; /* A copy: editing the table cannot mutate the coordinate handle. */
}

static int close_region_frame(lua_State *state)
{
    photo_region_frame **frame ← luaL_checkudata(state, 1, REGION_FRAME_TYPE);
    photo_region_close(frame);
    return 0;
}

static int bind_regions(lua_State *state)
{
    lua_Integer width ← luaL_checkinteger(state, 1);
    lua_Integer height ← luaL_checkinteger(state, 2);
    lua_Integer orientation ← luaL_checkinteger(state, 3);
    if (width < 1 || height < 1 || width > PHOTO_SIDE_LIMIT || height > PHOTO_SIDE_LIMIT ||
        width * height > PHOTO_PIXEL_LIMIT || orientation < 1 || orientation > 8)
        return luaL_error(state, "invalid RGBA image dimensions or orientation");
    size_t bytes ← 0;
    luaL_checktype(state, 4, LUA_TSTRING);
    const unsigned char *rgba ← (const unsigned char *)luaL_checklstring(state, 4, &bytes);
    size_t count ← (size_t)(width * height);
    if (bytes != 4u * count) return luaL_error(state, "invalid RGBA byte count");
    photo_region_frame **frame ← lua_newuserdatauv(state, sizeof(*frame), 0);
    *frame ← NULL;
    luaL_setmetatable(state, REGION_FRAME_TYPE);
    uint32_t *argb ← lua_newuserdatauv(state, count * sizeof(*argb), 0);
    for (size_t index ← 0; index < count; ++index) {
        const unsigned char *p ← rgba + 4u * index;
        argb[index] ← (uint32_t)p[3] << 24 | (uint32_t)p[0] << 16 |
                      (uint32_t)p[1] << 8 | p[2];
    }
    int status ← photo_region_open(argb, count, (uint32_t)width, (uint32_t)height,
                                 (size_t)width, (unsigned)orientation, frame);
    if (status != PHOTO_REGION_OK) return luaL_error(state, "region construction failed (%d)", status);
    lua_pop(state, 1);
    return 1;
}

static int bind_region_at(lua_State *state)
{
    photo_region_frame **frame ← luaL_checkudata(state, 1, REGION_FRAME_TYPE);
    lua_Integer x ← luaL_checkinteger(state, 2);
    lua_Integer y ← luaL_checkinteger(state, 3);
    if (x < 0 || y < 0 || x >= photo_region_width(*frame) || y >= photo_region_height(*frame))
        return luaL_error(state, "position is outside the oriented frame");
    lua_pushinteger(state, photo_region_at(*frame, (uint32_t)x, (uint32_t)y));
    return 1;
}

static int bind_region_entry(lua_State *state)
{
    photo_region_frame **frame ← luaL_checkudata(state, 1, REGION_FRAME_TYPE);
    lua_Integer id ← luaL_checkinteger(state, 2);
    if (id < 0 || id >= photo_region_count(*frame)) return luaL_error(state, "invalid region ID");
    photo_coordinates feature ← {
        .representation ← PHOTO_REGION_FEATURE_REVISION,
        .space ← "photo-region-color",
        .basis ← "rgba-histogram-rgb8-intensity4-alpha4",
        .metric ← "standard-euclidean",
        .dimension ← PHOTO_REGION_FEATURE_DIMENSION,
        .variance ← PHOTO_VECTOR
    };
    int status ← photo_region_features(*frame, (uint32_t)id, feature.values);
    if (status != PHOTO_REGION_OK && status != PHOTO_REGION_NO_VISIBLE_PIXELS)
        return luaL_error(state, "region feature extraction failed (%d)", status);

    char image_key[65];
    for (unsigned word ← 0; word < 4; ++word)
        snprintf(image_key + 16u * word, 17u, "%016llx",
                 (unsigned long long)(uint64_t)photo_region_key_word(*frame, word));
    lua_createtable(state, 0, 3);
    lua_createtable(state, 0, 3);
    lua_pushstring(state, image_key); lua_setfield(state, -2, "image_sha256");
    lua_pushfstring(state, "%d", (int)id); lua_setfield(state, -2, "region_id");
    lua_pushliteral(state, PHOTO_REGION_REVISION); lua_setfield(state, -2, "region_revision");
    lua_setfield(state, -2, "target");
    if (status == PHOTO_REGION_OK) {
        photo_coordinates *coordinates ← lua_newuserdatauv(state, sizeof(*coordinates), 0);
        *coordinates ← feature;
        luaL_setmetatable(state, COORDINATES_TYPE);
        lua_setfield(state, -2, "feature");
        lua_pushliteral(state, PHOTO_REGION_FEATURE_REVISION);
        lua_setfield(state, -2, "feature_revision");
    }
    return 1;
}

static void register_native_interface(lua_State *state, photo_math *engine)
{
    luaL_newmetatable(state, COORDINATES_TYPE);
    lua_pushliteral(state, "immutable named Float32 coordinates"); lua_setfield(state, -2, "__metatable");
    lua_pop(state, 1);
    luaL_newmetatable(state, REGION_FRAME_TYPE);
    lua_pushliteral(state, "immutable oriented photo region frame");
    lua_setfield(state, -2, "__metatable");
    lua_pushcfunction(state, close_region_frame); lua_setfield(state, -2, "__gc");
    lua_pop(state, 1);
    lua_createtable(state, 0, 11);
    lua_pushcfunction(state, bind_coordinates); lua_setfield(state, -2, "coordinates");
    lua_pushcfunction(state, bind_components); lua_setfield(state, -2, "components");
    lua_pushlightuserdata(state, engine); lua_pushcclosure(state, bind_projection, 1); lua_setfield(state, -2, "project32");
    lua_pushlightuserdata(state, engine); lua_pushcclosure(state, bind_alignment_report, 1); lua_setfield(state, -2, "inspect_alignment8");
    lua_pushlightuserdata(state, engine); lua_pushcclosure(state, bind_rotation, 1); lua_setfield(state, -2, "rotate8");
    lua_pushcfunction(state, bind_regions); lua_setfield(state, -2, "regions");
    lua_pushcfunction(state, bind_region_at); lua_setfield(state, -2, "region_at");
    lua_pushcfunction(state, bind_region_entry); lua_setfield(state, -2, "region_entry");
    lua_pushliteral(state, "shared-glsles-checked-v2"); lua_setfield(state, -2, "provider");
    lua_pushliteral(state, PHOTO_MATH_REVISION); lua_setfield(state, -2, "mathematics_revision");
    lua_pushliteral(state, "87306483cec50f8c750a22dda1d0742246fad756"); lua_setfield(state, -2, "runtime_revision");
    lua_setglobal(state, "photo_native");
}

int main(int argc, char **argv)
{
    if (argc < 3) { fputs("usage: photo_ilua SHARED_BUNDLE SCRIPT [arguments]\n", stderr); return 2; }
    host_context host;
    if (!host_context_open(&host)) { fputs("GLES3 context unavailable\n", stderr); return 3; }
    char error[1024];
    photo_math *engine ← photo_math_open(argv[1], error, sizeof(error));
    if (!engine) { fprintf(stderr, "%s\n", error); host_context_close(&host); return 4; }
    lua_State *state ← luaL_newstate();
    if (!state) { photo_math_close(&engine); host_context_close(&host); return 5; }
    luaL_openlibs(state); register_native_interface(state, engine);
    lua_createtable(state, argc - 2, 0);
    for (int index ← 2; index < argc; ++index) { lua_pushstring(state, argv[index]); lua_rawseti(state, -2, index - 2); }
    lua_setglobal(state, "arg");
    int status ← luaL_loadfile(state, argv[2]);
    if (status == LUA_OK) status ← lua_pcall(state, 0, 0, 0);
    if (status != LUA_OK) {
        const char *message ← lua_tostring(state, -1);
        fprintf(stderr, "%s\n", message ? message : "iLua raised a non-text error");
    }
    lua_close(state); photo_math_close(&engine); host_context_close(&host);
    return status == LUA_OK ? 0 : 6;
}
