/** @file
 * @brief Session dispatch for the document's heterogeneous history commands. */
#ifndef PE_VECTOR_COMMAND_CODEC_H
#define PE_VECTOR_COMMAND_CODEC_H
#include "editors/vector/history/history.h"
#include <stdio.h>
bool vector_command_write(const VectorCommand *command, FILE *file);
bool vector_command_read(VectorCommand *command, FILE *file);
#endif
