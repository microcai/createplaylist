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

int glob(const char* pattern, int flags, void* unused, glob_t* pglob)
{
	std::string path;
	int err = 0;
	size_t len;
	int pattern_len = strlen(pattern);
	WIN32_FIND_DATAW finddata = {0};

	std::wstring wpattern;
	int wchar_pattern_length = MultiByteToWideChar(CP_UTF8, 0, pattern, pattern_len, 0, 0);
	wpattern.resize(wchar_pattern_length);
	MultiByteToWideChar(CP_UTF8, 0, pattern, pattern_len, &wpattern[0], wchar_pattern_length);

	pglob->gl_pathc = 0;

	path = pattern;
	len = path.length();

	while (len > 0 && path[len - 1] != '/' && path[len - 1] != '\\')
	{
		len--;
	}
	path.resize(len);

	HANDLE hfindfile = FindFirstFileExW(wpattern.c_str(), FindExInfoBasic, &finddata, FindExSearchNameMatch, nullptr, 0);

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

			std::string cFileName;
			auto cFileNameW_len = lstrlenW(finddata.cFileName);
			int u8char_length = WideCharToMultiByte(CP_ACP, 0, finddata.cFileName, cFileNameW_len, 0, 0, 0, 0);
			cFileName.resize(u8char_length);
			WideCharToMultiByte(CP_ACP, 0, finddata.cFileName, cFileNameW_len, &cFileName[0], u8char_length, 0, 0);

			pglob->gl_pathv.push_back( path + cFileName );
		}
		while (!err && FindNextFileW(hfindfile, &finddata));

		FindClose(hfindfile);
	}
	else
	{
		err = -ENOENT;
	}

	return err;
}

void globfree(glob_t* pglob)
{
	if (pglob)
	{
		pglob->gl_pathv.clear();
	}
}
