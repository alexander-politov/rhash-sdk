/* win_utils.h utility functions for Windows */
#ifndef WIN_UTILS_H
#define WIN_UTILS_H

/* windows only definitions */
#ifdef _WIN32
#include "common_func.h"

#ifdef __cplusplus
extern "C" {
#endif

/* encoding conversion functions */
wchar_t* c2w(const char* str, int try_no);
char* w2c(const wchar_t* wstr);
char* win_to_utf8(const char* str);
#define win_is_utf8() (opt.flags & OPT_UTF8)
#define WIN_DEFAULT_ENCODING -1
char* wchar_to_cstr(const wchar_t* wstr, int codepage, int* failed);

void expand_wildcards(struct vector_t* vect, wchar_t* filepath);

/* file functions */
FILE* win_fopen_ex(const char* path, const char* mode, int exclusive);

#define fopen(path, mode) win_fopen_ex(path, mode, 0)
#define win_fopen_bin(path, mode) win_fopen_ex(path, mode, 1)
int win_stat(const char* path, struct rsh_stat_struct *buffer);
int can_open_exclusive(const char* path);
void win32_set_filesize64(const char* path, uint64_t *pSize);
wchar_t* make_pathw(const wchar_t* dir_path, size_t dir_len, wchar_t* filename);

void set_benchmark_cpu_affinity(void);
void setup_console(void);
void restore_console(void);

/* readdir structures and functions */
#define DIR WIN_DIR
#define dirent win_dirent
#define opendir  win_opendir
#define readdir  win_readdir
#define closedir win_closedir

/* dirent struct for windows to traverse directory content */
struct win_dirent {
	char*     d_name;   /* file name */
	wchar_t*  d_wname;  /* file name in Unicode (UTF-16) */
	int       d_isdir;  /* non-zero if file is a directory */
};

struct WIN_DIR_t;
typedef struct WIN_DIR_t WIN_DIR;

WIN_DIR* win_opendir(const char*);
WIN_DIR* win_wopendir(const wchar_t*);
struct win_dirent* win_readdir(WIN_DIR*);
void win_closedir(WIN_DIR*);

#ifdef __cplusplus
}
#endif

#endif /* _WIN32 */

#endif /* WIN_UTILS_H */
