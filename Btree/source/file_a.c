#include <stdio_ext.h>
#include <stdio.h>
#include <stdbool.h>
#include <errno.h>
#include <malloc.h>

#include <file_a.h>

struct file_a{
	FILE* file;
	bool locked;
};

typedef struct file_a file_a;

FILE* file_file(file_a* file){ return file->file; }

file_a* file_create(const char* name, const char* mode)
{
	if(!name || !mode)
		return (file_a*)0;

	file_a* f = (file_a*)malloc(sizeof(file_a));
	f->locked = false;
	f->file = fopen(name, mode);

	if(!f->file)
	{
		if(errno == ENOENT)
			printf("File %s was not found.\n", name);
		else
			perror("file_create::fopen");

		errno = 0;
		free(f);
		return (file_a*)0;
	}

	return f;
}

void file_destroy(file_a** _f)
{
	file_a* f = *_f;
	if(f)
	{
		if(f->locked)
		{
			funlockfile(f->file);
			f->locked = false;
		}

		if(f->file)
		{
			fclose(f->file);
			f->file = (FILE*)0;
		}
		
		free(f);
		f = (file_a*)0;
	}
}

void file_lock(file_a* f)
{
	if(f->locked)
		return;

	__fsetlocking(f->file, FSETLOCKING_BYCALLER);
	flockfile(f->file);
	f->locked = true;
}

void file_unlock(file_a* f)
{
	if(f->locked)
		funlockfile(f->file);

	f->locked = false;
}

int file_seek(file_a* f, off_t offset)
{
	return fseeko(f->file, offset, SEEK_SET);
}

int file_seek_relative(file_a* f, off_t offset, int from)
{
	return fseeko(f->file, offset, from);
}

int file_size(file_a* f)
{
	off_t old = ftello(f->file);
	fseeko(f->file, 0, SEEK_END);
	off_t size = ftello(f->file);
	fseeko(f->file, old, SEEK_SET);
	return (int)size;
}

size_t file_read(file_a* f, char* data, size_t size)
{
	return fread(data, size, 1, f->file);
}

size_t file_write(file_a* f, const char* data, size_t size)
{
	return fwrite(data, size, 1, f->file);
}

off_t file_tell(file_a* f)
{
	return ftello(f->file);
}

int file_get_delim(file_a* f, char* buff, int len, char delim)
{
	if(!f || !buff || len < 1 || !delim)
		return -1;

	char chr = '\0';
	for(int i = 0; i < len-1; i++)
	{
		if(feof(f->file))
		{
			buff[i] = '\0';
			return i;
		}

		if(fread(&chr, sizeof(char), 1, f->file))
		{
			if(chr == delim)
			{
				buff[i] = '\0';
				return i;
			}

			buff[i] = chr;
		}
	}

	buff[len-1] = '\0';
	return len-1;
}
