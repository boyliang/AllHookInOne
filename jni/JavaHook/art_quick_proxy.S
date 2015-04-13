

.macro ENTRY name
    .thumb_func
    .type \name, #function
    .global \name
    /* Cache alignment for function entry */
    .balign 16
\name:
    .cfi_startproc
    .fnstart
.endm

.macro END name
    .fnend
    .cfi_endproc
    .size \name, .-\name
.endm

/*
 * Art Quick Dispatcher.
 * On entry:
 *   r0 = method pointer
 *   r1 = arg1
 *   r2 = arg2
 *   r3 = arg3
 *   [sp] = method pointer
 *   [sp + 4] = addr of thiz
 *   [sp + 8] = addr of arg1
 *   [sp + 12] = addr of arg2
 *   [sp + 16] = addr of arg3
 * and so on
 */
	.extern artQuickToDispatcher
ENTRY art_quick_dispatcher
	push 	{r4, r5, lr}		   @ sp - 12
	mov 	r0, r0				   @ pass r0 to method
    str		r1, [sp, #(12 + 4)]
    str		r2, [sp, #(12 + 8)]
    str		r3, [sp, #(12 + 12)]
    mov		r1, r9				   @ pass r1 to thread
    add 	r2, sp, #(12 + 4)	   @ pass r2 to args array
    add		r3, sp, #12			   @ pass r3 to old SP
    blx     artQuickToDispatcher   @ (Method* method, Thread*, u4 **, u4 **)
    pop    	{r4, r5, pc}		   @ return on success, r0 and r1 hold the result
END art_quick_dispatcher

/*
 *
 * Art Quick Call Entrypoint
 * On entry:
 *  r0 = method pointer
 *  r1 = thread pointer
 *  r2 = args arrays pointer
 *  r3 = old_sp
 *  [sp] = entrypoint
 */
ENTRY art_quick_call_entrypoint
	push	{r4, r5, lr}		   @ sp - 12
	sub		sp, #(40 + 20)		   @ sp - 40 - 20
	str		r0, [sp, #(40 + 0)]	   @ var_40_0 = method_pointer
	str		r1, [sp, #(40 + 4)]	   @ var_40_4 = thread_pointer
	str		r2, [sp, #(40 + 8)]	   @ var_40_8 = args_array
	str		r3, [sp, #(40 + 12)]   @ var_40_12 = old_sp
	mov		r0, sp
	mov		r1, r3
	ldr		r2, =40
	blx		memcpy				   @ memcpy(dest, src, size_of_byte)
	ldr		r0, [sp, #(40 + 0)]	   @ restore method to r0
	ldr		r1, [sp, #(40 + 4)]
	mov		r9, r1				   @ restore thread to r9
	ldr		r5, [sp, #(40 + 8)]	   @ pass r5 to args_array
	ldr		r1, [r5]			   @ restore arg1
	ldr		r2, [r5, #4]		   @ restore arg2
	ldr		r3, [r5, #8]		   @ restore arg3
	ldr		r5, [sp, #(40 + 20 + 12)] @ pass ip to entrypoint
	blx		r5
	add		sp, #(40 + 20)
	pop		{r4, r5, pc}		   @ return on success, r0 and r1 hold the result
END art_quick_call_entrypoint
