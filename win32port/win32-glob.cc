/* libSoX minimal glob for MS-Windows: (c) 2009 SoX contributors
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>

#include "win32-glob.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include "nowide/convert.hpp"

struct auto_handle
{
	HANDLE handle;
	operator HANDLE () { return handle; }
	auto_handle(HANDLE handle) : handle(handle) {}
	~auto_handle() { FindClose(handle); }
};

int glob(const char* pattern, int flags, void* unused, glob_t* pglob)
{
	std::string path;
	size_t len;
	WIN32_FIND_DATAW finddata = {0};

	pglob->gl_pathc = 0;

	path = pattern;
	len = path.length();

	while (len > 0 && path[len - 1] != '/' && path[len - 1] != '\\')
	{
		len--;
	}
	path.resize(len);

	auto_handle hfindfile = FindFirstFileExW(nowide::widen(pattern).c_str(), FindExInfoBasic, &finddata, FindExSearchNameMatch, nullptr, 0);

	if (!pattern || flags != (flags & GLOB_FLAGS) || unused || !pglob)
	{
		errno = EINVAL;
		return EINVAL;
	}

	if (hfindfile != INVALID_HANDLE_VALUE)
	{
		do
		{
			pglob->gl_pathc ++;
			pglob->gl_pathv.push_back( path + nowide::narrow(finddata.cFileName) );
		}
		while (FindNextFileW(hfindfile, &finddata));
	}
	else
	{
		return -ENOENT;
	}
	return 0;
}

void globfree(glob_t* pglob)
{
}
