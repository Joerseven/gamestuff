//
// Created by jdhyd on 12/3/2023.
//

#ifndef CSC8503_LUTILS_H
#define CSC8503_LUTILS_H

#include <lua.hpp>

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

inline int setIntField(lua_State* L,  const char* key, int value) {
    if (!lua_istable(L, -1)) {
        luaL_error(L, "element on stack is not a table!");
    }
    lua_pushstring(L, key);
    lua_pushnumber(L, value);
    lua_settable(L, -3);
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





#endif //CSC8503_LUTILS_H
