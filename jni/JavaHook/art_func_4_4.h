/*
 * art_func_4_4.h
 *
 *  Created on: 2015年4月8日
 *      Author: boyliang
 */

#ifndef ART_FUNC_4_4_H_
#define ART_FUNC_4_4_H_

#include "art_object_4_4.h"

namespace art{

namespace mirror{
	class ArtMethod;
};

void ConvertUtf16ToModifiedUtf8(char* utf8_out, const uint16_t* utf16_in, size_t char_count);

extern "C" void artInterpreterToCompiledCodeBridge(Thread* self, MethodHelper& mh, const CodeItem* code_item, ShadowFrame* shadow_frame, JValue* result);

namespace interpreter{

extern "C" void artInterpreterToInterpreterBridge(Thread* self, MethodHelper& mh,const CodeItem* code_item, ShadowFrame* shadow_frame, JValue* result);

};

extern "C" void art_quick_to_interpreter_bridge(mirror::ArtMethod*);
extern "C" void art_portable_to_interpreter_bridge(mirror::ArtMethod*);

};


#endif /* ART_FUNC_4_4_H_ */
