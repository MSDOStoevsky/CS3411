#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef struct
{
    off_t starts_at; /* File's starting byte in the package file */
    off_t file_size; /* Original File size in bytes */
    char file_name[256];
} file_data;

int main(int argc, char *argv[])
{
    int index, pkg_in, num_of_files, fd;
    file_data data;

    char buffer [50];
    int *filebuffer;
    int metadata[1];
    
    const int magic_num = 918237;
    /* 8 to account for first two ints of file */
    int bytes_r = 8;

    /* if more than two cin arg is supplied */
    if (argc > 3)
    {
        sprintf(buffer, "Usage: upack <file> package | upack package");
        puts(buffer);
        exit(EXIT_FAILURE);
    }
    else if(argc == 1)
    {
        sprintf(buffer, "Usage: upack <file> package | upack package");
        puts(buffer);
        exit(EXIT_FAILURE);
    }
    else if(argc == 3)
    {
        pkg_in = open(argv[argc - 1], O_RDONLY);
        if (pkg_in < 0)
        {
            perror("open()");
            exit(EXIT_FAILURE);
        }

        /* read first line */
        if (read(pkg_in, metadata, 4) < 0)
        {
            perror("read()");
            exit(EXIT_FAILURE);
        }
        /* validate file */
        if (metadata[0] != magic_num)
        {
            sprintf(buffer, "Error: file not valid");
            puts(buffer);
            exit(EXIT_FAILURE);
        }

        /*get number of files */
        if (read(pkg_in, metadata, 4) < 0)
        {
            perror("read()");
            exit(EXIT_FAILURE);
        }
        num_of_files = metadata[0];

        for(index = 0; index < num_of_files; index++)
        {
            read(pkg_in, &data, sizeof(file_data));
            if (strcmp(data.file_name, argv[argc - 2]) == 0)
            {   
                fd = open(data.file_name, O_RDWR|O_CREAT, 0644);
                if (fd < 0)
                {
                    perror("open()");
                    exit(EXIT_FAILURE);
                }   
                
                /* from data.starts_at to data.file_size write to fd */
                lseek(pkg_in, data.starts_at, SEEK_SET);
                filebuffer = malloc(data.file_size);
                if(read(pkg_in, filebuffer, data.file_size ) < 0)
                {
                    perror("read()");
                    exit(EXIT_FAILURE);
                }
                
                if(write(fd, filebuffer, data.file_size ) < 0)
                {
                    perror("write()");
                    exit(EXIT_FAILURE);
                }
                free(filebuffer);
                if(close(fd) < 0)
                {
                    perror("write()");
                    exit(EXIT_FAILURE);
                }
                if(close(pkg_in) < 0)
                {
                    perror("write()");
                    exit(EXIT_FAILURE);
                }
                return 0;
            }

        }
        sprintf(buffer, "Error: file not found");
        puts(buffer);
    }
    /* upack all */
    else
    {
        pkg_in = open(argv[argc - 1], O_RDONLY);
        if (pkg_in < 0)
        {
            perror("open()");
            exit(EXIT_FAILURE);
        }


        /*read first line */
        if (read(pkg_in, metadata, 4) < 0)
        {
            perror("read()");
            exit(EXIT_FAILURE);
        }

        /* validate file */
        if (metadata[0] != magic_num)
        {
            sprintf(buffer, "Error: file not valid");
            puts(buffer);
            exit(EXIT_FAILURE);
        }


        if (read(pkg_in, metadata, 4) < 0)
        {
            perror("read()");
            exit(EXIT_FAILURE);
        }
        num_of_files = metadata[0];

        for(index = 0; index < num_of_files; index++)
        {
            bytes_r += read(pkg_in, &data, sizeof(file_data));
            fd = open(data.file_name, O_RDWR|O_CREAT, 0644);
            if (fd < 0)
            {
                perror("open()");
                exit(EXIT_FAILURE);
            }   
            
            /* from data.starts_at to data.file_size write to fd */
            lseek(pkg_in, data.starts_at, SEEK_SET);
            filebuffer = malloc(data.file_size);
            if(read(pkg_in, filebuffer, data.file_size ) < 0)
            {
                perror("read()");
                exit(EXIT_FAILURE);
            }
            
            if(write(fd, filebuffer, data.file_size ) < 0)
            {
                perror("write()");
                exit(EXIT_FAILURE);
            }

            /* after writing move ci back to struct reader */
            lseek(pkg_in, bytes_r, SEEK_SET);
        }
        free(filebuffer);
        if(close(fd) < 0)
        {
            perror("write()");
            exit(EXIT_FAILURE);
        }
        if(close(pkg_in) < 0)
        {
            perror("write()");
            exit(EXIT_FAILURE);
        }
    }
    

    
    return 0;
}