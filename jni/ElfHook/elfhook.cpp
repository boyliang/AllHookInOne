/*
 * elfhook.cpp
 *
 *  Created on: 2014年10月9日
 *      Author: boyliang
 */

#include <unistd.h>
#include <stdio.h>
#include <sys/mman.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <sys/syscall.h>

#include "common.h"
#include "elfutils.h"
#include "elfio.h"


#define PAGE_START(addr) (~(getpagesize() - 1) & (addr))

static int modifyMemAccess(void *addr, int prots){
	void *page_start_addr = (void *)PAGE_START((uint32_t)addr);
	return mprotect(page_start_addr, getpagesize(), prots);
}

static int clearCache(void *addr, size_t len){
	void *end = (uint8_t *)addr + len;
	syscall(0xf0002, addr, end);
}

static int replaceFunc(void *addr, void *replace_func, void **old_func){
	int res = 0;

	if(*(void **)addr == replace_func){
		LOGW("addr %p had been replace.", addr);
		goto fails;
	}

	if(!*old_func){
		*old_func = *(void **)addr;
	}

	if(modifyMemAccess((void *)addr, PROT_EXEC|PROT_READ|PROT_WRITE)){
		LOGE("[-] modifymemAccess fails, error %s.", strerror(errno));
		res = 1;
		goto fails;
	}

	*(void **)addr = replace_func;
	clearCache(addr, getpagesize());
	LOGI("[+] old_func is %p, replace_func is %p, new_func %p.", *old_func, replace_func, *(uint32_t *)addr);

	fails:
	return res;
}

#define R_ARM_ABS32 0x02
#define R_ARM_GLOB_DAT 0x15
#define R_ARM_JUMP_SLOT 0x16

int elfHook(const char *soname, const char *symbol, void *replace_func, void **old_func){
	assert(old_func);
	assert(replace_func);
	assert(symbol);

	ElfHandle* handle = openElfBySoname(soname);
	ElfInfo info;

	getElfInfoBySegmentView(info, handle);

	Elf32_Sym *sym = NULL;
	int symidx = 0;

	findSymByName(info, symbol, &sym, &symidx);

	if(!sym){
		LOGE("[-] Could not find symbol %s", symbol);
		goto fails;
	}else{
		LOGI("[+] sym %p, symidx %d.", sym, symidx);
	}

	for (int i = 0; i < info.relpltsz; i++) {
		Elf32_Rel& rel = info.relplt[i];
		if (ELF32_R_SYM(rel.r_info) == symidx && ELF32_R_TYPE(rel.r_info) == R_ARM_JUMP_SLOT) {

			void *addr = (void *) (info.elf_base + rel.r_offset);
			if (replaceFunc(addr, replace_func, old_func))
				goto fails;

			//only once
			break;
		}
	}

	for (int i = 0; i < info.reldynsz; i++) {
		Elf32_Rel& rel = info.reldyn[i];
		if (ELF32_R_SYM(rel.r_info) == symidx &&
				(ELF32_R_TYPE(rel.r_info) == R_ARM_ABS32
						|| ELF32_R_TYPE(rel.r_info) == R_ARM_GLOB_DAT)) {

			void *addr = (void *) (info.elf_base + rel.r_offset);
			if (replaceFunc(addr, replace_func, old_func))
				goto fails;
		}
	}

	fails:
	closeElfBySoname(handle);
	return 0;
}


