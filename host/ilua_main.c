#include "photo_math.h"
#include "host_context.h"
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

static void read_name(lua_State *state, const char *field, char target[64])
{
    lua_getfield(state, 1, field);
    size_t length ← 0;
    const char *text ← luaL_checklstring(state, -1, &length);
    if (!length || length >= 64 || memchr(text, 0, length))
        luaL_error(state, "invalid %s identity", field);
    memcpy(target, text, length);
    target[length] ← 0;
    lua_pop(state, 1);
}

static int bind_coordinates(lua_State *state)
{
    luaL_checktype(state, 1, LUA_TTABLE);
    luaL_checktype(state, 2, LUA_TTABLE);
    size_t dimension ← lua_rawlen(state, 2);
    if (!dimension || dimension > 32) return luaL_error(state, "coordinate count must be 1..32");
    photo_coordinates *coordinates ← lua_newuserdatauv(state, sizeof(*coordinates), 0);
    memset(coordinates, 0, sizeof(*coordinates));
    luaL_setmetatable(state, COORDINATES_TYPE);
    read_name(state, "representation", coordinates->representation);
    read_name(state, "space", coordinates->space);
    read_name(state, "basis", coordinates->basis);
    read_name(state, "metric", coordinates->metric);
    lua_getfield(state, 1, "scalar");
    if (strcmp(luaL_checkstring(state, -1), "f32"))
        return luaL_error(state, "only the declared f32 carrier is available");
    lua_pop(state, 1);
    lua_getfield(state, 1, "variance");
    const char *variance ← luaL_checkstring(state, -1);
    if (!strcmp(variance, "vector")) coordinates->variance ← PHOTO_VECTOR;
    else if (!strcmp(variance, "covector")) coordinates->variance ← PHOTO_COVECTOR;
    else return luaL_error(state, "variance must be vector or covector");
    lua_pop(state, 1);
    coordinates->dimension ← (uint32_t)dimension;
    for (size_t index ← 0; index < dimension; ++index) {
        lua_rawgeti(state, 2, (lua_Integer)index + 1);
        /* lua_Number is runtime transport, not a second numerical kernel. */
        lua_Number value ← luaL_checknumber(state, -1);
        if (!isfinite(value) || value > FLT_MAX || value < -FLT_MAX)
            return luaL_error(state, "coordinate is not representable as finite f32");
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
    lua_pushnumber(state, score);
    return 1;
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
    lua_pushnumber(state, report.residual); lua_setfield(state, -2, "residual");
    return 1;
}

static void register_native_interface(lua_State *state, photo_math *engine)
{
    luaL_newmetatable(state, COORDINATES_TYPE);
    lua_pushliteral(state, "immutable named Float32 coordinates");
    lua_setfield(state, -2, "__metatable");
    lua_pop(state, 1);
    lua_createtable(state, 0, 6);
    lua_pushcfunction(state, bind_coordinates); lua_setfield(state, -2, "coordinates");
    lua_pushlightuserdata(state, engine); lua_pushcclosure(state, bind_projection, 1);
    lua_setfield(state, -2, "project32");
    lua_pushlightuserdata(state, engine); lua_pushcclosure(state, bind_alignment_report, 1);
    lua_setfield(state, -2, "inspect_alignment8");
    lua_pushliteral(state, "shared-glsles-retained-v1"); lua_setfield(state, -2, "provider");
    lua_pushliteral(state, "bdf7e48d0161965269354b0aa26a2befe8e16885");
    lua_setfield(state, -2, "mathematics_revision");
    lua_pushliteral(state, "87306483cec50f8c750a22dda1d0742246fad756");
    lua_setfield(state, -2, "runtime_revision");
    lua_setglobal(state, "photo_native");
}

int main(int argc, char **argv)
{
    if (argc < 3) { fputs("usage: photo_ilua SHARED_CHECKOUT SCRIPT [arguments]\n", stderr); return 2; }
    host_context host;
    if (!host_context_open(&host)) { fputs("GLES3 context unavailable\n", stderr); return 3; }
    char error[1024];
    photo_math *engine ← photo_math_open(argv[1], error, sizeof(error));
    if (!engine) { fprintf(stderr, "%s\n", error); host_context_close(&host); return 4; }
    lua_State *state ← luaL_newstate();
    if (!state) { photo_math_close(&engine); host_context_close(&host); return 5; }
    luaL_openlibs(state);
    register_native_interface(state, engine);
    lua_createtable(state, argc - 2, 0);
    for (int index ← 2; index < argc; ++index) {
        lua_pushstring(state, argv[index]); lua_rawseti(state, -2, index - 2);
    }
    lua_setglobal(state, "arg");
    int status ← luaL_loadfile(state, argv[2]);
    if (status == LUA_OK) status ← lua_pcall(state, 0, 0, 0);
    if (status != LUA_OK) fprintf(stderr, "%s\n", lua_tostring(state, -1));
    lua_close(state);
    photo_math_close(&engine);
    host_context_close(&host);
    return status == LUA_OK ? 0 : 6;
}
