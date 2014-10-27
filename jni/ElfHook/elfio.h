/*
 * elfio.h
 *
 *  Created on: 2014年10月8日
 *      Author: boyliang
 */

#ifndef ELFIO_H_
#define ELFIO_H_

struct ElfHandle {
	void *base;
	size_t space_size;
	bool fromfile;
};

/**
 * 从文件中打开ELF
 */
ElfHandle *openElfByFile(const char *path);

/**
 * 释放资源
 */
void closeElfByFile(ElfHandle *handle);

/**
 * 从给定的so中获取基址, 如果soname为NULL，则表示当前进程自身
 */
ElfHandle *openElfBySoname(const char *soname);

/**
 * 释放资源
 */
void closeElfBySoname(ElfHandle *handle);


#endif /* ELFIO_H_ */
