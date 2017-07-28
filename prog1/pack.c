#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <libgen.h>
#include <linux/limits.h>

typedef struct
{
    off_t starts_at; /* File's starting byte in the package file */
    off_t file_size; /* Original File size in bytes */
    char file_name[256];
} file_data;

int main(int argc, char *argv[])
{

    /*** delcarations ***/
    int index, pkg_out, fd, struct_space, md_space, file_space, file_size;
    file_data data;
    int sizeof_fd = sizeof(file_data);

    
    int *filebuffer;
    char path_arg [PATH_MAX];
    char buffer [50];
    int bytes_w = 8;
    const int magic_num = 918237;

    /* number of files excluding package and argv[0] */
    unsigned int num_files = argc - 2;

    /* if only one cin arg is supplied */
    if (argc <= 2)
    {
        sprintf(buffer, "Usage: pack <file1> <file2> ... <filen> package");
        write(STDOUT_FILENO, buffer, 50);
        exit(EXIT_FAILURE);
    }

    /* check for dupes */
    strcpy(path_arg, argv[1]);
    for (index = 2; index <= num_files; index++)
    {
        if(strcmp(path_arg, argv[index]) == 0)
        {
            sprintf(buffer, "Error: Duplicate path detected");
            write(STDERR_FILENO, buffer, 50);
            exit(EXIT_FAILURE);
        }
        strcpy(path_arg, argv[index]);
    }

    /* create package file */
    pkg_out = open(argv[num_files + 1], O_RDWR|O_CREAT, 0644);
    
    if(write(pkg_out, &magic_num, 4) < 0)
    {
        perror("magic_num write()");
        exit(EXIT_FAILURE);
    }
    if(write(pkg_out, &num_files, 4) < 0)
    {
        perror("num_files write()");
        exit(EXIT_FAILURE);
    }

    /* store amount of space for structs + */
    struct_space = sizeof_fd * num_files;
    md_space = struct_space + 8;

    file_space = md_space;

    /* add structs + files */
    for(index = 1 ; index <= num_files;  index++)
    {
        fd = open(argv[index],O_RDONLY);
        if(fd < 0)
        {
            perror("open()");
            exit(EXIT_FAILURE);
        }

        /* get size */
        file_size = lseek(fd, 0, SEEK_END);
        if(lseek(fd, 0, SEEK_SET) < 0)
        {
            perror("lseek()");
            exit(EXIT_FAILURE);
        }

        /* initialize struct info */
        data.file_size = file_size;
        data.starts_at = file_space;
        strcpy(data.file_name, basename(argv[index]));

        
        filebuffer = malloc(file_size);
        if(read(fd, filebuffer, file_size) < 0)
        {
            perror("read()");
            exit(EXIT_FAILURE);
        }

	    if(write(pkg_out, &data, sizeof_fd) < 0)
        {
            perror("write()");
            exit(EXIT_FAILURE);
        }
        bytes_w += sizeof_fd;
        
        /* write struct and file contents */
        if(lseek(pkg_out, file_space, SEEK_SET) < 0)
        {
            perror("lseek()");
            exit(EXIT_FAILURE);
        }
	    if(write(pkg_out, filebuffer, data.file_size) < 0)
        {
            perror("write()");
            exit(EXIT_FAILURE);
        }
        if(lseek(pkg_out, bytes_w, SEEK_SET) < 0)
        {
            perror("lseek()");
            exit(EXIT_FAILURE);
        }

        
        /* set position for next file to be added */
        file_space += file_size;
    }
    free(filebuffer);
    if(close(pkg_out) < 0)
    {
        perror("close()");
        exit(EXIT_FAILURE);
    }
    return 0;
}
