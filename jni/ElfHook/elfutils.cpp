/*
 * elfutils.cpp
 *
 *  Created on: 2014年10月7日
 *      Author: boyliang
 */

#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stddef.h>

#include "common.h"
#include "elfutils.h"

static inline Elf32_Shdr *findSectionByName(ElfInfo &info, const char *sname){
	Elf32_Shdr *target = NULL;
	Elf32_Shdr *shdr = info.shdr;
	for(int i=0; i<info.ehdr->e_shnum; i++){
		const char *name = (const char *)(shdr[i].sh_name + info.shstr);
		if(!strncmp(name, sname, strlen(sname))){
			target = (Elf32_Shdr *)(shdr + i);
			break;
		}
	}

	return target;
}

static inline Elf32_Phdr *findSegmentByType(ElfInfo &info, const Elf32_Word type){
	Elf32_Phdr *target = NULL;
	Elf32_Phdr *phdr = info.phdr;

	for(int i=0; i<info.ehdr->e_phnum; i++){
		if(phdr[i].p_type == type){
			target = phdr + i;
			break;
		}
	}

	return target;
}


#define SAFE_SET_VALUE(t, v) if(t) *(t) = (v)
template<class T>
static inline void getSectionInfo(ElfInfo &info, const char *name, Elf32_Word *pSize, Elf32_Shdr **ppShdr, T *data){
	Elf32_Shdr *_shdr = findSectionByName(info, name);

	if(_shdr){
		SAFE_SET_VALUE(pSize, _shdr->sh_size / _shdr->sh_entsize);
		SAFE_SET_VALUE(data, reinterpret_cast<T>(info.elf_base + _shdr->sh_offset));
	}else{
		LOGE("[-] Could not found section %s\n", name);
		exit(-1);
	}

	SAFE_SET_VALUE(ppShdr, _shdr);
}

template<class T>
static void getSegmentInfo(ElfInfo &info, const Elf32_Word type, Elf32_Phdr **ppPhdr, Elf32_Word *pSize, T *data){
	Elf32_Phdr *_phdr = findSegmentByType(info, type);

	if(_phdr){

		if(info.handle->fromfile){ //文件读取
			SAFE_SET_VALUE(data, reinterpret_cast<T>(info.elf_base + _phdr->p_offset));
			SAFE_SET_VALUE(pSize, _phdr->p_filesz);
		}else{ //从内存读取
			SAFE_SET_VALUE(data, reinterpret_cast<T>(info.elf_base + _phdr->p_vaddr));
			SAFE_SET_VALUE(pSize, _phdr->p_memsz);
		}

	}else{
		LOGE("[-] Could not found segment type is %d\n", type);
		exit(-1);
	}

	SAFE_SET_VALUE(ppPhdr, _phdr);
}

unsigned elf_hash(const char *name) {
	const unsigned char *tmp = (const unsigned char *) name;
	unsigned h = 0, g;

	while (*tmp) {
		h = (h << 4) + *tmp++;
		g = h & 0xf0000000;
		h ^= g;
		h ^= g >> 24;
	}
	return h;
}

void getElfInfoBySectionView(ElfInfo &info, const ElfHandle *handle){

	info.handle = handle;
	info.elf_base = (uint8_t *) handle->base;
	info.ehdr = reinterpret_cast<Elf32_Ehdr *>(info.elf_base);
	info.shdr = reinterpret_cast<Elf32_Shdr *>(info.elf_base + info.ehdr->e_shoff);
	info.phdr = reinterpret_cast<Elf32_Phdr *>(info.elf_base + info.ehdr->e_phoff);

	Elf32_Shdr *shstr = (Elf32_Shdr *)(info.shdr + info.ehdr->e_shstrndx);
	info.shstr = reinterpret_cast<char *>(info.elf_base + shstr->sh_offset);

	getSectionInfo(info, ".dynstr", NULL, NULL, &info.symstr);
	getSectionInfo(info, ".dynamic", &info.dynsz, NULL, &info.dyn);
	getSectionInfo(info, ".dynsym", &info.symsz, NULL, &info.sym);
	getSectionInfo(info, ".rel.dyn", &info.reldynsz, NULL, &info.reldyn);
	getSectionInfo(info, ".rel.plt", &info.relpltsz, NULL, &info.relplt);

	Elf32_Shdr *hash = findSectionByName(info, ".hash");
	if(hash){
		uint32_t *rawdata = reinterpret_cast<uint32_t *>(info.elf_base + hash->sh_offset);
		info.nbucket = rawdata[0];
		info.nchain = rawdata[1];
		info.bucket = rawdata + 2;
		info.chain = info.bucket + info.nbucket;
	}
}

void getElfInfoBySegmentView(ElfInfo &info, const ElfHandle *handle){

	info.handle = handle;
	info.elf_base = (uint8_t *) handle->base;
	info.ehdr = reinterpret_cast<Elf32_Ehdr *>(info.elf_base);

	// may be wrong
	info.shdr = reinterpret_cast<Elf32_Shdr *>(info.elf_base + info.ehdr->e_shoff);
	info.phdr = reinterpret_cast<Elf32_Phdr *>(info.elf_base + info.ehdr->e_phoff);

	info.shstr = NULL;

	Elf32_Phdr *dynamic = NULL;
	Elf32_Word size = 0;

	getSegmentInfo(info, PT_DYNAMIC, &dynamic, &size, &info.dyn);
	if(!dynamic){
		LOGE("[-] could't find PT_DYNAMIC segment");
		exit(-1);
	}
	info.dynsz = size / sizeof(Elf32_Dyn);

	Elf32_Dyn *dyn = info.dyn;
	for(int i=0; i<info.dynsz; i++, dyn++){

		switch(dyn->d_tag){

		case DT_SYMTAB:
			info.sym = reinterpret_cast<Elf32_Sym *>(info.elf_base + dyn->d_un.d_ptr);
			break;

		case DT_STRTAB:
			info.symstr = reinterpret_cast<const char *>(info.elf_base + dyn->d_un.d_ptr);
			break;

		case DT_REL:
			info.reldyn = reinterpret_cast<Elf32_Rel *>(info.elf_base + dyn->d_un.d_ptr);
			break;

		case DT_RELSZ:
			info.reldynsz = dyn->d_un.d_val / sizeof(Elf32_Rel);
			break;

		case DT_JMPREL:
			info.relplt = reinterpret_cast<Elf32_Rel *>(info.elf_base + dyn->d_un.d_ptr);
			break;

		case DT_PLTRELSZ:
			info.relpltsz = dyn->d_un.d_val / sizeof(Elf32_Rel);
			break;

		case DT_HASH:
			uint32_t *rawdata = reinterpret_cast<uint32_t *>(info.elf_base + dyn->d_un.d_ptr);
			info.nbucket = rawdata[0];
			info.nchain = rawdata[1];
			info.bucket = rawdata + 2;
			info.chain = info.bucket + info.nbucket;

			info.symsz = info.nchain;
			break;
		}
	}

}

void findSymByName(ElfInfo &info, const char *symbol, Elf32_Sym **sym, int *symidx) {
	Elf32_Sym *target = NULL;

	unsigned hash = elf_hash(symbol);
	uint32_t index = info.bucket[hash % info.nbucket];

	if (!strcmp(info.symstr + info.sym[index].st_name, symbol)) {
		target = info.sym + index;
	}

	if (!target) {
		do {
			index = info.chain[index];
			if (!strcmp(info.symstr + info.sym[index].st_name, symbol)) {
				target = info.sym + index;
				break;
			}

		} while (index != 0);
	}

	if(target){
		SAFE_SET_VALUE(sym, target);
		SAFE_SET_VALUE(symidx, index);
	}
}

void printSections(ElfInfo &info){
	Elf32_Half shnum = info.ehdr->e_shnum;
	Elf32_Shdr *shdr = info.shdr;

	LOGI("Sections: \n");
	for(int i=0; i<shnum; i++, shdr++){
		const char *name = shdr->sh_name == 0 || !info.shstr ? "UNKOWN" :  (const char *)(shdr->sh_name + info.shstr);
		LOGI("[%.2d] %-20s 0x%.8x\n", i, name, shdr->sh_addr);
	}
}

void printSegments(ElfInfo &info){
	Elf32_Phdr *phdr = info.phdr;
	Elf32_Half phnum = info.ehdr->e_phnum;

	LOGI("Segments: \n");
	for(int i=0; i<phnum; i++){
		LOGI("[%.2d] %-20d 0x%-.8x 0x%-.8x %-8d %-8d\n", i,
				phdr[i].p_type, phdr[i].p_vaddr,
				phdr[i].p_paddr, phdr[i].p_filesz,
				phdr[i].p_memsz);
	}
}

void printfDynamics(ElfInfo &info){
	Elf32_Dyn *dyn = info.dyn;

	LOGI(".dynamic section info:\n");
	const char *type = NULL;

	for(int i=0; i<info.dynsz; i++){
		switch(dyn[i].d_tag){
		case DT_INIT:
			type = "DT_INIT";
			break;
		case DT_FINI:
			type = "DT_FINI";
			break;
		case DT_NEEDED:
			type = "DT_NEEDED";
			break;
		case DT_SYMTAB:
			type = "DT_SYMTAB";
			break;
		case DT_SYMENT:
			type = "DT_SYMENT";
			break;
		case DT_NULL:
			type = "DT_NULL";
			break;
		case DT_STRTAB:
			type= "DT_STRTAB";
			break;
		case DT_REL:
			type = "DT_REL";
			break;
		case DT_SONAME:
			type = "DT_SONAME";
			break;
		case DT_HASH:
			type = "DT_HASH";
			break;
		default:
			type = NULL;
			break;
		}

		// we only printf that we need.
		if(type){
			LOGI("[%.2d] %-10s 0x%-.8x 0x%-.8x\n", i, type,  dyn[i].d_tag, dyn[i].d_un.d_val);
		}

		if(dyn[i].d_tag == DT_NULL){
			break;
		}
	}
}

void printfSymbols(ElfInfo &info){
	Elf32_Sym *sym = info.sym;

	LOGI("dynsym section info:\n");
	for(int i=0; i<info.symsz; i++){
		LOGI("[%2d] %-20s\n", i, sym[i].st_name + info.symstr);
	}
}


void printfRelInfo(ElfInfo &info){
	Elf32_Rel* rels[] = {info.reldyn, info.relplt};
	Elf32_Word resszs[] = {info.reldynsz, info.relpltsz};

	Elf32_Sym *sym = info.sym;

	LOGI("rel section info:\n");
	for(int i=0; i<sizeof(rels)/sizeof(rels[0]); i++){
		Elf32_Rel *rel = rels[i];
		Elf32_Word relsz = resszs[i];

		for(int j=0; j<relsz; j++){
		const char *name = sym[ELF32_R_SYM(rel[j].r_info)].st_name + info.symstr;
		LOGI("[%.2d-%.4d] 0x%-.8x 0x%-.8x %-10s\n", i, j, rel[j].r_offset, rel[j].r_info, name);
		}
	}
}

