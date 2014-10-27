/*
 * elfhook.h
 *
 *  Created on: 2014年10月25日
 *      Author: boyliang
 */

#ifndef ELFHOOK_H_
#define ELFHOOK_H_


/**
 *
 *if soname is NULL, then only found the current process's .rel.plt and .rel.dyn section
 */
int elfHook(const char *soname, const char *symbol, void *replace_func, void **old_func);


#endif /* ELFHOOK_H_ */
