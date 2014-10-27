/*
 * elfutils.h
 *
 *  Created on: 2014年10月7日
 *      Author: boyliang
 */

#ifndef ELFUTILS_H_
#define ELFUTILS_H_

#include <elf.h>
#include <stdint.h>
#include <stddef.h>

#include "elfio.h"

/**
 * elf关键信息
 */
struct ElfInfo{
	const ElfHandle *handle;

	uint8_t *elf_base;

	Elf32_Ehdr *ehdr;
	Elf32_Phdr *phdr;
	Elf32_Shdr *shdr;

	Elf32_Dyn *dyn;
	Elf32_Word dynsz;

	Elf32_Sym *sym;
	Elf32_Word symsz;

	Elf32_Rel *relplt;
	Elf32_Word relpltsz;
	Elf32_Rel *reldyn;
	Elf32_Word reldynsz;

	uint32_t nbucket;
	uint32_t nchain;

	uint32_t *bucket;
	uint32_t *chain;

	const char *shstr;
	const char *symstr;
};

/**
 * 符号hash函数
 */
unsigned elf_hash(const char *name);

/**
 * 从section视图获取info
 */
void getElfInfoBySectionView(ElfInfo &info, const ElfHandle *handle);

/**
 * 从segment视图获取info
 */
void getElfInfoBySegmentView(ElfInfo &info, const ElfHandle *handle);


/**
 * 根据符号名寻找Sym
 */
void findSymByName(ElfInfo &info, const char *symbol, Elf32_Sym **sym, int *symidx);

/**
 * 打印section信息
 */
void printSections(ElfInfo &info);


/**
 * 打印segment信息
 */
void printSegments(ElfInfo &info);

/**
 * 打印dynamic信息
 */
void printfDynamics(ElfInfo &info);

/**
 * 打印所有符号信息
 */
void printfSymbols(ElfInfo &info);

/**
 * 打印重定位信息
 */
void printfRelInfo(ElfInfo &info);


#endif /* ELFUTILS_H_ */
