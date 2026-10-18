/** @file @brief Private, seekable session file removed automatically when closed. */
#ifndef PE_PLATFORM_TEMP_FILE_H
#define PE_PLATFORM_TEMP_FILE_H
#include <stdio.h>
/** @brief Open a binary update stream in the OS temporary area. NULL on failure. */
FILE *editor_temporary_file(void);
#endif
