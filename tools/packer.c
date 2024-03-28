#include "miniz.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[])
{
    if (argc < 3) {
        printf("Usage: %s zip_archive file1 [file2 ...]\n", argv[0]);
        return 1;
    }

    // Open the zip file for writing
    mz_bool status;
    mz_zip_archive zip_archive;
    memset(&zip_archive, 0, sizeof(zip_archive));
    printf("Creating a zip archive named %s.\n", argv[1]);
    status = mz_zip_writer_init_file(&zip_archive, argv[1], 0);
    if (!status) {
        printf("Error: could not open zip file for writing.\n%s\n", mz_error(status));
        return 1;
    }

    // Add files to the zip archive
    for (int i = 2; i < argc; ++i)
      mz_zip_writer_add_file(&zip_archive, argv[i], argv[i], NULL, 0, MZ_DEFAULT_COMPRESSION);

    // Close the zip file
    mz_zip_writer_finalize_archive(&zip_archive);
    mz_zip_writer_end(&zip_archive);

    return 0;
}