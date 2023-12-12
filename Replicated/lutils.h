//
// Created by jdhyd on 12/3/2023.
//

#ifndef CSC8503_LUTILS_H
#define CSC8503_LUTILS_H

#include <lua.hpp>
#include <stb/stb_image.h>
#include <cstring>

inline void stackDump (lua_State *L) {
    int i;
    int top = lua_gettop(L);
    for (i = 1; i <= top; i++) {  /* repeat for each level */
        int t = lua_type(L, i);
        switch (t) {

            case LUA_TSTRING:  /* strings */
                printf("`%s'", lua_tostring(L, i));
                break;

            case LUA_TBOOLEAN:  /* booleans */
                printf(lua_toboolean(L, i) ? "true" : "false");
                break;

            case LUA_TNUMBER:  /* numbers */
                printf("%g", lua_tonumber(L, i));
                break;

            default:  /* other values */
                printf("%s", lua_typename(L, t));
                break;

        }
        printf("  ");  /* put a separator */
    }
    printf("\n");  /* end the listing */
}

inline int getIntField(lua_State* L, const char* key) {
    lua_pushstring(L, key);
    lua_gettable(L, -2);
    if (!lua_isnumber(L, -1)) {
        luaL_error(L, "field is not an integer");
    }
    auto result = (int)lua_tonumber(L, -1);
    lua_pop(L, 1);
    return result;
}

inline double getNumberField(lua_State* L, const char* key) {
    lua_pushstring(L, key);
    lua_gettable(L, -2);
    if (!lua_isnumber(L, -1)) {
        luaL_error(L, "field is not an number");
    }
    auto result = lua_tonumber(L, -1);
    lua_pop(L, 1);
    return result;
}

inline bool getBool(lua_State* L, const char* key) {
    lua_pushstring(L, key);
    lua_gettable(L, -2);
    if (!lua_isboolean(L, -1)) {
        luaL_error(L, "field is not a boolean");
    }
    auto result = lua_toboolean(L, -1);
    lua_pop(L, 1);
    return result;
}

inline const char* getStringField(lua_State* L, const char* key) {
    lua_pushstring(L, key);
    lua_gettable(L, -2);
    if (!lua_isstring(L, -1)) {
        luaL_error(L, "field is not a string");
    }
    auto result = lua_tostring(L, -1);
    lua_pop(L, 1);
    return result;
}

inline Vector3 getVec3Field(lua_State* L, const char* key) {
    Vector3 v;
    lua_pushstring(L, key);
    lua_gettable(L, -2);
    // -1 table -2 table

    v.x = (float)getNumberField(L, "x");
    v.y = (float)getNumberField(L, "y");
    v.z = (float)getNumberField(L, "z");

    // Pop the vector table
    lua_pop(L, 1);

    return v;
}

inline Vector3 getVec3Field(lua_State* L, int key) {
    Vector3 v;
    lua_pushinteger(L, key);
    lua_gettable(L, -2);
    // -1 table -2 table

    v.x = (float)getNumberField(L, "x");
    v.y = (float)getNumberField(L, "y");
    v.z = (float)getNumberField(L, "z");

    // Pop the vector table
    lua_pop(L, 1);

    return v;
}

inline Vector4 getVec4Field(lua_State* L, const char* key) {
    Vector4 v;
    lua_pushstring(L, key);
    lua_gettable(L, -2);

    v.x = (float)getNumberField(L, "x");
    v.y = (float)getNumberField(L, "y");
    v.z = (float)getNumberField(L, "z");
    v.w = (float)getNumberField(L, "w");

    lua_pop(L, 1);

    return v;
}



inline void setIntField(lua_State* L,  const char* key, int value) {
    if (!lua_istable(L, -1)) {
        luaL_error(L, "element on stack is not a table!");
    }
    lua_pushstring(L, key);
    lua_pushnumber(L, value);
    lua_settable(L, -3);
}

inline void setFloatField(lua_State* L, const char* key, float value) {
    if (!lua_istable(L, -1)) {
        luaL_error(L, "element on stack is not a table!");
    }
    lua_pushstring(L, key);
    lua_pushnumber(L, value);
    lua_settable(L, -3);
}

inline int setVec3Field(lua_State* L, const char* key, const Vector3& value) {
    if (!lua_istable(L ,-1)) {
        luaL_error(L, "element on stack not a table");
    }
    lua_getglobal(L, "Vector3");
    lua_getfield(L, -1, "new");
    lua_getglobal(L, "Vector3");
    lua_pushnumber(L, value.x);
    lua_pushnumber(L, value.y);
    lua_pushnumber(L, value.z);

    auto result = lua_pcall(L, 4, 1, 0);
    if (result) {
        std::cerr << lua_tostring(L, 1) << std::endl;
    }


}

inline int getTableSize(lua_State* L) {
    if (!lua_istable(L, -1)) {
        luaL_error(L, "Aint a table on top of the stack");
    }

    lua_len(L, -1);
    auto length = (int)lua_tonumber(L, -1);
    lua_pop(L, 1);
    return length;
}

int LoadLevelFromImage(lua_State *L) {
    auto filename = luaL_checkstring(L, 1);
    auto mapRatio = luaL_checknumber(L, 2);

    auto location = std::string(ASSETROOTLOCATION) + std::string("/Data/") + std::string(filename);

    int width, height, channels;
    unsigned char *data = stbi_load(
            location.c_str(),
            &width, &height, &channels, 0);

    lua_createtable(L, height * width, 0);

    if (!data) {
        std::cerr << "Failed to load file" << std::endl;
    }

    int tableCounter = 1;

    auto offsetX = (float)height * 0.5 * mapRatio;
    auto offsetZ = (float)width * 0.5 * mapRatio;

    for (int z =0; z < height*channels; z+=channels) {
        for (int x = 0; x < width*channels; x+=channels) {
            int offset = (z * width) + x;
            auto r = (unsigned int)data[offset];
            auto g = (unsigned int)data[offset+1];
            auto b = (unsigned int)data[offset+2];
            lua_newtable(L);
            setIntField(L, "r", r);
            setIntField(L, "g", g);
            setIntField(L, "b", b);
            setFloatField(L, "x", x * mapRatio);// * 0.25 * mapRatio - offsetX);
            setFloatField(L, "z", z * mapRatio);// * 0.25 * mapRatio - offsetZ);
            lua_rawseti(L, -2, tableCounter++);
        }
    }

    return 1;
}

inline void RegisterFunctions(lua_State* L) {
    lua_register(L, "LoadLevelFromImage", LoadLevelFromImage);
}



#endif //CSC8503_LUTILS_H
