/*
 * elfio.cpp
 *
 *  Created on: 2014年10月8日
 *      Author: boyliang
 */

#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include "common.h"
#include "elfio.h"

ElfHandle *openElfByFile(const char *path) {
	void *base = NULL;
	int fd = open(path, O_RDWR);
	if (fd < 0) {
		LOGE("[-] open %s fails.\n", path);
		exit(-1);
	}

	struct stat fs;
	fstat(fd, &fs);

	base = mmap(NULL, fs.st_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
	if (base == MAP_FAILED) {
		LOGE("[-] mmap fails.\n");
		exit(-1);
	}
	close(fd);

	ElfHandle *handle = (ElfHandle *) malloc(sizeof(ElfHandle));
	handle->base = base;
	handle->space_size = fs.st_size;
	handle->fromfile = true;

	return handle;
}

void closeElfByFile(ElfHandle *handle) {
	if (handle) {
		if (handle->base && handle->space_size > 0) {
			msync(handle->base, handle->space_size, MS_SYNC);
			munmap(handle->base, handle->space_size);
			free(handle);
		}
	}
}

/**
 * 查找soname的基址，如果为NULL，则为当前进程基址
 */
static void *findLibBase(const char *soname) {
	FILE *fd = fopen("/proc/self/maps", "r");
	char line[256];
	void *base = 0;

	while (fgets(line, sizeof(line), fd) != NULL) {
		if (soname == NULL || strstr(line, soname)) {
			line[8] = '\0';
			base = (void *) strtoul(line, NULL, 16);
			break;
		}
	}

	fclose(fd);
	return base;
}

/**
 * 从给定的so中获取基址
 */
ElfHandle *openElfBySoname(const char *soname){
	void *base = findLibBase(soname);
	if(!base){
		LOGE("[-] could find %s. \n", soname);
		exit(-1);
	}

	ElfHandle *handle = (ElfHandle *) malloc(sizeof(ElfHandle));
	handle->base = base;
	handle->space_size = -1;
	handle->fromfile = false;

	return handle;
}

/**
 * 释放资源
 */
void closeElfBySoname(ElfHandle *handle){
	//only free the base
	free(handle);
}

