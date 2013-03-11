/* win_utils.c - Windows-specific utility functions */
#ifdef _WIN32

#include <windows.h>
#include <share.h> /* for _SH_DENYWR */
#include <sys/stat.h>
#include <fcntl.h> /* for _O_RDONLY, _O_BINARY */
#include <io.h> /* for isatty */
#include <assert.h>
#include <errno.h>
#include <locale.h>

#include "common_func.h"
#include "parse_cmdline.h"
#include "rhash_main.h"
#include "win_utils.h"

/**
 * Convert a c-string to wide character string using given codepage
 *
 * @param str the string to convert
 * @param codepage the codepage to use
 * @return converted string on success, NULL on fail
 */
static wchar_t* cstr_to_wchar(const char* str, int codepage)
{
	wchar_t* buf;
	int size = MultiByteToWideChar(codepage, MB_ERR_INVALID_CHARS, str, -1, NULL, 0);
	if(size == 0) return NULL; /* conversion failed */

	buf = (wchar_t*)rsh_malloc(size * sizeof(wchar_t));
	MultiByteToWideChar(codepage, 0, str, -1, buf, size);
	return buf;
}

/**
 * Convert c-string to wide string using primary or secondary codepage.
 *
 * @param str the C-string to convert
 * @param try_no 0 for primary codepage, 1 for a secondary one
 * @return converted wide string on success, NULL on error
 */
wchar_t* c2w(const char* str, int try_no)
{
	int is_utf = (try_no == (opt.flags & OPT_UTF8 ? 0 : 1));
	int codepage = (is_utf ? CP_UTF8 : (opt.flags & OPT_OEM) ? CP_OEMCP : CP_ACP);
	return cstr_to_wchar(str, codepage);
}

/**
 * Convert a UTF8-encoded string to wide string.
 *
 * @param str the UTF8-encoded string to convert
 * @return wide string on success, NULL on error
 */
static wchar_t* utf8_to_wchar(const char* utf8_str)
{
	return cstr_to_wchar(utf8_str, CP_UTF8);
}

/**
 * Convert a wide character string to c-string using given codepage.
 * Optionally set a flag if conversion failed.
 *
 * @param wstr the wide string to convert
 * @param codepage the codepage to use
 * @param failed pointer to the flag, to on failed conversion, can be NULL
 * @return converted string on success, NULL on fail
 */
char* wchar_to_cstr(const wchar_t* wstr, int codepage, int* failed)
{
	int size;
	char *buf;
	BOOL bUsedDefChar, *lpUsedDefaultChar;
	if(codepage == -1) {
		codepage = (opt.flags & OPT_UTF8 ? CP_UTF8 : (opt.flags & OPT_OEM) ? CP_OEMCP : CP_ACP);
	}
	/* note: lpUsedDefaultChar must be NULL for CP_UTF8, otrherwise WideCharToMultiByte() will fail */
	lpUsedDefaultChar = (failed && codepage != CP_UTF8 ? &bUsedDefChar : NULL);

	size = WideCharToMultiByte(codepage, 0, wstr, -1, 0, 0, 0, 0);
	if(size == 0) return NULL; /* conversion failed */
	buf = (char*)rsh_malloc(size);
	WideCharToMultiByte(codepage, 0, wstr, -1, buf, size, 0, lpUsedDefaultChar);
	if(failed) *failed = (lpUsedDefaultChar && *lpUsedDefaultChar);
	return buf;
}

/**
 * Convert wide string to multi-byte c-string using codepage specified
 * by command line options.
 *
 * @param wstr the wide string to convert
 * @return c-string on success, NULL on fail
 */
char* w2c(const wchar_t* wstr)
{
	return wchar_to_cstr(wstr, -1, NULL);
}

/**
 * Convert given C-string from encoding specified by
 * command line options to utf8.
 *
 * @param str the string to convert
 * @return converted string on success, NULL on fail
 */
char* win_to_utf8(const char* str)
{
	char* res;
	wchar_t* buf;

	assert((opt.flags & (OPT_UTF8 | OPT_OEM | OPT_ANSI)) != 0);
	if(opt.flags & OPT_UTF8) return rsh_strdup(str);

	if((buf = c2w(str, 0)) == NULL) return NULL;
	res = wchar_to_cstr(buf, CP_UTF8, NULL);
	free(buf);
	return res;
}

/**
 * Open file path given in the current encoding, using desired shared access.
 *
 * @param path file path
 * @param mode string specifying file opening mode
 * @param exclusive non-zero to prohibit write access to the file
 * @return file descriptor on success, NULL on error
 */
FILE* win_fopen_ex(const char* path, const char* mode, int exclusive)
{
	FILE* fd = 0;
	int i;
	wchar_t* wmode = utf8_to_wchar(mode);
	assert(wmode != NULL);

	/* try two code pages */
	for(i = 0; i < 2; i++) {
		wchar_t* wpath = c2w(path, i);
		if(wpath == NULL) continue;
		fd = _wfsopen(wpath, wmode, (exclusive ? _SH_DENYWR : _SH_DENYNO));
		free(wpath);
		if(fd || errno != ENOENT) break;
	}
	free(wmode);
	return fd;
}

/**
 * stat() a file with encoding support.
 *
 * @param path the file path
 * @param buffer pointer to the buffer to store file properties to
 * @return 0 on success, -1 on error
 */
int win_stat(const char* path, struct rsh_stat_struct *buffer)
{
	int i, res = -1;
	for(i = 0; i < 2; i++) {
		wchar_t* wpath = c2w(path, i);
		if(wpath == NULL) continue;
		res = clib_wstat(wpath, buffer);
		free(wpath);
		if(res == 0 || errno != ENOENT) break;
	}
	return res;
}

/**
 * Get 64-bit file size and store it into given 64-bit integer.
 *
 * @param path file path
 * @param pSize pointer to a 64-bit integer to store file size into
 */
void win32_set_filesize64(const char* path, uint64_t *pSize)
{
	int try_no;
	for(try_no = 0; try_no < 2; try_no++) {
		HANDLE hFile;
		DWORD fileSizeLow, fileSizeHigh;

		wchar_t* wpath = c2w(path, try_no);
		if(wpath == NULL) continue;
		hFile = CreateFileW(wpath, 0, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		free(wpath);
		if(hFile == INVALID_HANDLE_VALUE) {
			UINT error = GetLastError();
			if(error != ERROR_FILE_NOT_FOUND && error != ERROR_PATH_NOT_FOUND) break;
		} else {
			fileSizeLow = GetFileSize(hFile, &fileSizeHigh);

			/* it is strange but the correct method to check for error */
			if(fileSizeLow == 0xffffffff && GetLastError() != 0) {
				/* printf("error code = %u: %s\n", GetLastError(), filepath); */
			} else {
				*pSize = fileSizeLow + ( (uint64_t)fileSizeHigh << 32 );
			}
			CloseHandle(hFile);
			break;
		}
	}
}

/**
 * Check if given file can be opened with exclusive write access.
 *
 * @param path path to the file
 * @return 1 if file can be opened, 0 otherwise
 */
int can_open_exclusive(const char* path)
{
	int i, res = 0;
	for(i = 0; i < 2 && res == 0; i++) {
		int fd;
		wchar_t* wpath = c2w(path, i);
		if(wpath == NULL) continue;
		fd = _wsopen(wpath, _O_RDONLY | _O_BINARY, _SH_DENYWR, 0);
		if(fd >= 0) {
			res = 1;
			_close(fd);
		}
		free(wpath);
	}
	return res;
}

/**
 * Concatenate directory path with filename, unicode version.
 *
 * @param dir_path directory path
 * @param dir_len length of directory path in characters
 * @param filename the file name to append to the directory
 * @return concatenated path
 */
wchar_t* make_pathw(const wchar_t* dir_path, size_t dir_len, wchar_t* filename)
{
	wchar_t* res;
	size_t len;

	if(dir_path == 0) dir_len = 0;
	else {
		/* remove leading path separators from filename */
		while(IS_PATH_SEPARATOR_W(*filename)) filename++;

		if(dir_len == (size_t)-1) dir_len = wcslen(dir_path);
	}
	len = wcslen(filename);

	res = (wchar_t*)rsh_malloc((dir_len + len + 2) * sizeof(wchar_t));
	if(dir_len > 0) {
		memcpy(res, dir_path, dir_len * sizeof(wchar_t));
		res[dir_len++] = (wchar_t)SYS_PATH_SEPARATOR;
	}

	/* append filename */
	memcpy(res + dir_len, filename, (len + 1) * sizeof(wchar_t));
	return res;
}

/**
 * Expand wildcards in the given filepath and store results into vector.
 * If no wildcards are found then just the filepath is stored.
 * Note: only wildcards in the last filename of the path are expanded.
 *
 * @param vect the vector to receive wide-strings with file paths
 * @param filepath the filepath to process
 */
void expand_wildcards(vector_t* vect, wchar_t* filepath)
{
	int added = 0;
	size_t len = wcslen(filepath);
	size_t index = wcscspn(filepath, L"*?");

	/* if a wildcard has been found without a directory separator after it */
	if(index < len && wcscspn(filepath + index, L"/\\") >= (len - index))
	{
		wchar_t* parent;
		WIN32_FIND_DATAW d;
		HANDLE h;

		/* find directory separator */
		for(; index > 0 && !IS_PATH_SEPARATOR(filepath[index]); index--);
		parent = (IS_PATH_SEPARATOR(filepath[index]) ? filepath : 0);

		h = FindFirstFileW(filepath, &d);
		if(INVALID_HANDLE_VALUE != h) {
			do {
				wchar_t* wpath;
				char* cstr;
				int failed;
				if((0 == wcscmp(d.cFileName, L".")) || (0 == wcscmp(d.cFileName, L".."))) continue;
				if(NULL == (wpath = make_pathw(parent, index + 1, d.cFileName))) continue;
				cstr = wchar_to_cstr(wpath, WIN_DEFAULT_ENCODING, &failed);
				/* note: just quietly skip unconvertible file names */
				if(!cstr || failed) {
					free(cstr);
					free(wpath);
					continue;
				}
				rsh_vector_add_ptr(vect, cstr);
				added++;
			} while(FindNextFileW(h, &d));
			FindClose(h);
		}
	}

	if(added == 0) {
		wchar_t* wpath = make_pathw(0, 0, filepath);
		char* cstr = w2c(wpath);
		if(cstr) rsh_vector_add_ptr(vect, cstr);
	}
}

/* functions to setup/restore console */

/**
 * Prepare console on program initialization: change console font codepage
 * according to program options and hide cursor.
 */
void setup_console(void)
{
	HANDLE hOut;
	CONSOLE_CURSOR_INFO cci;

	int cp = (opt.flags&OPT_UTF8 ? CP_UTF8 : opt.flags&OPT_ANSI ? GetACP() : GetOEMCP());
	rhash_data.saved_console_codepage = -1;
	/* note: we are using numbers 1 = _fileno(stdout), 2 = _fileno(stderr) */
	/* cause _fileno() is undefined,  when compiling as strict ansi C. */
	if(cp > 0 && IsValidCodePage(cp) && (isatty(1) || isatty(2)) )
	{
		rhash_data.saved_console_codepage = GetConsoleOutputCP();
		SetConsoleOutputCP(cp);
		setlocale(LC_CTYPE, opt.flags&OPT_UTF8 ? "C" :
			opt.flags&OPT_ANSI ? ".ACP" : ".OCP");
		rsh_exit = rhash_exit;
	}

	if((opt.flags & OPT_PERCENTS) != 0) {
		hOut = GetStdHandle(STD_ERROR_HANDLE);
		if(hOut != INVALID_HANDLE_VALUE) {
			/* store current cursor size and visibility flag */
			GetConsoleCursorInfo(hOut, &cci);
			rhash_data.saved_cursor_size = (cci.bVisible ? cci.dwSize : 0);

			/* now hide cursor */
			cci.bVisible = 0;
			SetConsoleCursorInfo(hOut, &cci); /* hide cursor */
		}
	}
}

/**
 * Restore console on program exit.
 */
void restore_console(void)
{
	HANDLE hOut;
	CONSOLE_CURSOR_INFO cci;

	if(rhash_data.saved_console_codepage > 0) {
		SetConsoleOutputCP(rhash_data.saved_console_codepage);
	}

	hOut = GetStdHandle(STD_ERROR_HANDLE);
	if(hOut != INVALID_HANDLE_VALUE && rhash_data.saved_cursor_size) {
		/* restore cursor size and visibility */
		cci.dwSize = rhash_data.saved_cursor_size;
		cci.bVisible = 1;
		SetConsoleCursorInfo(hOut, &cci);
	}
}

/****************************************************************************
 *                           Directory functions                            *
 ****************************************************************************/
struct WIN_DIR_t
{
	WIN32_FIND_DATAW findFileData;
	HANDLE hFind;
	struct win_dirent dir;
	int state; /* 0 - not started, -1 - ended, >=0 file index */
};

/**
 * Open directory iterator for reading the directory content.
 *
 * @param dir_path directory path
 * @return pointer to directory stream. On error, NULL is returned,
 *         and errno is set appropriately.
 */
WIN_DIR* win_opendir(const char* dir_path)
{
	WIN_DIR* d;
	wchar_t* wpath;

	/* append '\*' to the dir_path */
	size_t len = strlen(dir_path);
	char *path = (char*)malloc(len + 3);
	if(!path) return NULL; /* failed, malloc also set errno = ENOMEM */
	strcpy(path, dir_path);
	strcpy(path + len, "\\*");

	d = (WIN_DIR*)malloc(sizeof(WIN_DIR));
	if(!d) {
		free(path);
		return NULL;
	}
	memset(d, 0, sizeof(WIN_DIR));

	wpath = c2w(path, 0);
	d->hFind = (wpath != NULL ?
		FindFirstFileW(wpath, &d->findFileData) : INVALID_HANDLE_VALUE);
	free(wpath);
	
	if(d->hFind == INVALID_HANDLE_VALUE && GetLastError() != ERROR_ACCESS_DENIED) {
		wpath = c2w(path, 1); /* try to use secondary codepage */
		if(wpath) {
			d->hFind = FindFirstFileW(wpath, &d->findFileData);
			free(wpath);
		}
	}
	free(path);
	
	if(d->hFind == INVALID_HANDLE_VALUE && GetLastError() == ERROR_ACCESS_DENIED) {
		free(d);
		errno = EACCES;
		return NULL;
	}

	d->state = (d->hFind == INVALID_HANDLE_VALUE ? -1 : 0);
	d->dir.d_name = NULL;
	return d;
}

/**
 * Open a directory for reading its content.
 * For simplicity the function supposes that dir_path points to an
 * existing directory and doesn't check for this error.
 * The Unicode version of the function.
 *
 * @param dir_path directory path
 * @return pointer to directory iterator
 */
WIN_DIR* win_wopendir(const wchar_t* dir_path)
{
	WIN_DIR* d;

	/* append '\*' to the dir_path */
	wchar_t *wpath = make_pathw(dir_path, (size_t)-1, L"*");
	d = (WIN_DIR*)rsh_malloc(sizeof(WIN_DIR));

	d->hFind = FindFirstFileW(wpath, &d->findFileData);
	free(wpath);
	if(d->hFind == INVALID_HANDLE_VALUE && GetLastError() == ERROR_ACCESS_DENIED) {
		free(d);
		errno = EACCES;
		return NULL;
	}

	/* note: we suppose if INVALID_HANDLE_VALUE was returned, then the file listing is empty */
	d->state = (d->hFind == INVALID_HANDLE_VALUE ? -1 : 0);
	d->dir.d_name = NULL;
	return d;
}

/**
 * Close a directory iterator.
 *
 * @param d pointer to the directory iterator
 */
void win_closedir(WIN_DIR* d)
{
	if(d->hFind != INVALID_HANDLE_VALUE) {
		FindClose(d->hFind);
	}
	free(d->dir.d_name);
	free(d);
}

/**
 * Read a directory content.
 *
 * @param d pointer to the directory iterator
 * @return directory entry or NULL if no entries left
 */
struct win_dirent* win_readdir(WIN_DIR* d)
{
	char* filename;
	int failed;

	if(d->state == -1) return NULL;
	if(d->dir.d_name != NULL) {
		free(d->dir.d_name);
		d->dir.d_name = NULL;
	}

	for(;;) {
		if(d->state > 0) {
			if( !FindNextFileW(d->hFind, &d->findFileData) ) {
				/* the directory listing has ended */
				d->state = -1;
				return NULL;
			}
		}
		d->state++;

		if(d->findFileData.cFileName[0] == L'.' &&
			(d->findFileData.cFileName[1] == 0 ||
			(d->findFileData.cFileName[1] == L'.' &&
			d->findFileData.cFileName[2] == 0))) {
				/* simplified implementation, skips '.' and '..' names */
				continue;
		}

		d->dir.d_name = filename = wchar_to_cstr(d->findFileData.cFileName, WIN_DEFAULT_ENCODING, &failed);

		if(filename && !failed) {
			d->dir.d_wname = d->findFileData.cFileName;
			d->dir.d_isdir = (0 != (d->findFileData.dwFileAttributes &
				FILE_ATTRIBUTE_DIRECTORY));
			return &d->dir;
		}
		/* quietly skip an invalid filename and repeat the search */
		if(filename) {
			free(filename);
			d->dir.d_name = NULL;
		}
	}
}

#endif /* _WIN32 */
