/*
    KindlePDFViewer
    Copyright (C) 2011 Hans-Werner Hilse <hilse@web.de>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <string.h>
#include <fcntl.h>
#include <stdio.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "blitbuffer.h"
#include "drawcontext.h"
#include "koptcontext.h"
#include "pdf.h"
#include "mupdfimg.h"
#include "djvu.h"
#include "pic.h"
#include "cre.h"
#include "einkfb.h"
#include "input.h"
#include "ft.h"
#include "util.h"
#include "lua_gettext.h"

#include "lfs.h"

lua_State *L;

/* from luajit-2.0/src/luajit.c : */
static int traceback(lua_State *L)
{
  if (!lua_isstring(L, 1)) { /* Non-string error object? Try metamethod. */
    if (lua_isnoneornil(L, 1) ||
	!luaL_callmeta(L, 1, "__tostring") ||
	!lua_isstring(L, -1))
      return 1;  /* Return non-string error object. */
    lua_remove(L, 1);  /* Replace object by result of __tostring metamethod. */
  }
  lua_getfield(L, LUA_GLOBALSINDEX, "debug");
  if (!lua_istable(L, -1)) {
    lua_pop(L, 1);
    return 1;
  }
  lua_getfield(L, -1, "traceback");
  if (!lua_isfunction(L, -1)) {
    lua_pop(L, 2);
    return 1;
  }
  lua_pushvalue(L, 1);  /* Push error message. */
  lua_pushinteger(L, 2);  /* Skip this function and debug.traceback(). */
  lua_call(L, 2, 1);  /* Call debug.traceback(). */
  return 1;
}

/* from luajit-2.0/src/luajit.c : */
static int docall(lua_State *L, int narg, int clear)
{
  int status;
  int base = lua_gettop(L) - narg;  /* function index */
  lua_pushcfunction(L, traceback);  /* push traceback function */
  lua_insert(L, base);  /* put it under chunk and args */
  status = lua_pcall(L, narg, (clear ? 0 : LUA_MULTRET), base);
  lua_remove(L, base);  /* remove traceback function */
  /* force a complete garbage collection in case of errors */
  if (status != 0) lua_gc(L, LUA_GCCOLLECT, 0);
  return status;
}

int main(int argc, char **argv) {
	int i;

	if(argc < 2) {
		fprintf(stderr, "needs config file as first argument.\n");
		return -1;
	}

	/* set up Lua state */
	L = lua_open();
	if(L) {
		luaL_openlibs(L);

		luaopen_blitbuffer(L);
		luaopen_drawcontext(L);
		luaopen_koptcontext(L);
		luaopen_einkfb(L);
		luaopen_kobolight(L);
		luaopen_pdf(L);
		luaopen_djvu(L);
		luaopen_pic(L);
		luaopen_cre(L);
		luaopen_input(L);
		luaopen_util(L);
		luaopen_ft(L);
		luaopen_mupdfimg(L);
		luaopen_luagettext(L);

		luaopen_lfs(L);

		lua_newtable(L);
		for(i=2; i < argc; i++) {
			lua_pushstring(L, argv[i]);
			lua_rawseti(L, -2, i-1);
		}
		lua_setglobal(L, "ARGV");

		if(luaL_loadfile(L, argv[1]) || docall(L, 0, 1)) {
			fprintf(stderr, "lua config error: %s\n", lua_tostring(L, -1));
			lua_close(L);
			L=NULL;
			return -1;
		}
	}

	return 0;
}

