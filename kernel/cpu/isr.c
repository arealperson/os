#include <system.h>
#include <isr.h>
#include <debug.h>
#include <syscall.h>
#include <process.h>
#include <signal.h>
#include <scheduler.h>
#include <fpu.h>

extern void isr0 (void);
extern void isr1 (void);
extern void isr2 (void);
extern void isr3 (void);
extern void isr4 (void);
extern void isr5 (void);
extern void isr6 (void);
extern void isr7 (void);
extern void isr8 (void);
extern void isr9 (void);
extern void isr10(void);
extern void isr11(void);
extern void isr12(void);
extern void isr13(void);
extern void isr14(void);
extern void isr15(void);
extern void isr16(void);
extern void isr17(void);
extern void isr18(void);
extern void isr19(void);
extern void isr20(void);
extern void isr21(void);
extern void isr22(void);
extern void isr23(void);
extern void isr24(void);
extern void isr25(void);
extern void isr26(void);
extern void isr27(void);
extern void isr28(void);
extern void isr29(void);
extern void isr30(void);
extern void isr31(void);
extern void isr128(void);


#define SAVE_PROCESS_STATE \
	{\
		current_process->stat.rax = regs->rax;\
		current_process->stat.rdx = regs->rdx;\
		current_process->stat.rcx = regs->rcx;\
		current_process->stat.rbx = regs->rbx;\
		current_process->stat.rsp = regs->rsp;\
		current_process->stat.rbp = regs->rbp;\
		current_process->stat.rsi = regs->rsi;\
		current_process->stat.rdi = regs->rdi;\
		current_process->stat.r8  = regs->r8;\
		current_process->stat.r9  = regs->r9;\
		current_process->stat.r10 = regs->r10;\
		current_process->stat.r11 = regs->r11;\
		current_process->stat.r12 = regs->r12;\
		current_process->stat.r13 = regs->r13;\
		current_process->stat.r14 = regs->r14;\
		current_process->stat.r15 = regs->r15;\
		current_process->stat.rip = regs->rip;\
		current_process->stat.rflags = regs->rflags;\
	}

regs_t r;

void syscall_int(regs_t *regs)
{
	/* 	NOTE : This memcpy can be optimized if we assume strict following of
		GCC calling conventions and store only registers that shouldn't change.
	*/
	memcpy(&current_process->stat, regs, sizeof(regs_t));

	/*
	current_process->stat.rax = regs->rax;
	current_process->stat.rdx = regs->rdx;
	current_process->stat.rcx = regs->rcx;
	current_process->stat.rbx = regs->rbx;
	current_process->stat.rsp = regs->rsp;
	current_process->stat.rbp = regs->rbp;
	current_process->stat.rsi = regs->rsi;
	current_process->stat.rdi = regs->rdi;
	current_process->stat.r8  = regs->r8;
	current_process->stat.r9  = regs->r9;
	current_process->stat.r10 = regs->r10;
	current_process->stat.r11 = regs->r11;
	current_process->stat.r12 = regs->r12;
	current_process->stat.r13 = regs->r13;
	current_process->stat.r14 = regs->r14;
	current_process->stat.r15 = regs->r15;
	current_process->stat.rip = regs->rip;
	current_process->stat.rflags = regs->rflags;
	*/
	void (*f)(uint64_t, uint64_t, uint64_t) = sys_calls[regs->rax];
	f(regs->rbx, regs->rcx, regs->rdx);
	return;
	/*uint8_t *str = msg;
	uint8_t *v = (uint8_t*)0xFFFFFFFFC00B8000;
	while(*str)
	{
		*v = *str++;
		v += 2;
	}*/
}


static const char *msg[32] = {
	"Division by zero",				/* 0 */
	"Debug",
	"Non-maskable interrupt",
	"Breakpoint",
	"Detected overflow",
	"Out-of-bounds",				/* 5 */
	"Invalid opcode",
	"No coprocessor",
	"Double fault",
	"Coprocessor segment overrun",
	"Bad TSS",						/* 10 */
	"Segment not present",
	"Stack fault",
	"General protection fault",
	"Page fault",
	"Unknown interrupt",			/* 15 */
	"Coprocessor fault",
	"Alignment check",
	"Machine check",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved"
};

uint32_t fork_skip_page = 0;
void interrupt(regs_t *regs)
{
	extern uint64_t int_num;
	extern uint64_t err_num;
	debug("rip -> %lx\n", regs->rip);
	debug("Recieved interrupt [%d] [%x] : %s\n", (uint32_t)int_num, err_num, msg[int_num]);

#if _LAZYALLOC_
	if(int_num == 14) // Page fault
	{		
		uint64_t addr;
		asm("mov %%cr2, %%rax; movq %%rax, %0;"::"g"(addr));
		if(forking || addr <= current_process->size * 0x1000 
			&& addr >= current_process->addr
			&& regs->rip <= current_process->size * 0x1000
			&& regs->rip >= current_process->addr
		)
		{
			if(forking)
			{
				debug("forking hack %lx\n", regs->r10);
				fork_skip_page = 1;
				regs->rip = regs->r10;
				return;
			}
			debug("Lazy alloc %lx\n", addr);
			map_mem_user(current_process->pdpt, (uint64_t*)(addr/0x1000*0x1000), 0x1000);
			kernel_idle();
		}
	}
#endif

#if 1
	if(regs->rip == -1)	// Returned from signal handler
	{
		void *stack = (void*)regs->rsp;
		memcpy(&current_process->stat, stack, sizeof(stat_t));
		debug("%lx\n", current_process->stat.rip);
		kernel_idle();
	}
	if(regs->rip < 0x7FC0000000) // exception generated by user process
	{
		switch((uint32_t)int_num)
		{
			case 0:
				signal_send(current_process, SIGFPE);
				break;
			case 6:
			case 7:
				if(fpu_state)
				{
					signal_send(current_process, SIGILL);
					break;
				}
				trap_fpu();
				kernel_idle();
				break;
			case 13:
			case 14:
				signal_send(current_process, SIGSEGV);
				break;
			case 16:
				signal_send(current_process, SIGFPE);
				break;
			default:
				signal_send(current_process, SIGILL);
				break;
		}
		kernel_idle();
	}
#endif	
	//dump_stack();
	debug("Kernel exception\n"); // That's bad
	for(;;);
}

void isr_install(void) {	
	idt_set_gate(0x00,  isr0,  0x08, 0x8E);
	idt_set_gate(0x01,  isr1,  0x08, 0x8E);
	idt_set_gate(0x02,  isr2,  0x08, 0x8E);
	idt_set_gate(0x03,  isr3,  0x08, 0x8E);
	idt_set_gate(0x04,  isr4,  0x08, 0x8E);
	idt_set_gate(0x05,  isr5,  0x08, 0x8E);
	idt_set_gate(0x06,  isr6,  0x08, 0x8E);
	idt_set_gate(0x07,  isr7,  0x08, 0x8E);
	idt_set_gate(0x08,  isr8,  0x08, 0x8E);
	idt_set_gate(0x09,  isr9,  0x08, 0x8E);
	idt_set_gate(0x0A, isr10, 0x08, 0x8E);
	idt_set_gate(0x0B, isr11, 0x08, 0x8E);
	idt_set_gate(0x0C, isr12, 0x08, 0x8E);
	idt_set_gate(0x0D, isr13, 0x08, 0x8E);
	idt_set_gate(0x0E, isr14, 0x08, 0x8E);
	idt_set_gate(0x0F, isr15, 0x08, 0x8E);
	idt_set_gate(0x10, isr16, 0x08, 0x8E);
	idt_set_gate(0x11, isr17, 0x08, 0x8E);
	idt_set_gate(0x12, isr18, 0x08, 0x8E);
	idt_set_gate(0x13, isr19, 0x08, 0x8E);
	idt_set_gate(0x14, isr20, 0x08, 0x8E);
	idt_set_gate(0x15, isr21, 0x08, 0x8E);
	idt_set_gate(0x16, isr22, 0x08, 0x8E);
	idt_set_gate(0x17, isr23, 0x08, 0x8E);
	idt_set_gate(0x18, isr24, 0x08, 0x8E);
	idt_set_gate(0x19, isr25, 0x08, 0x8E);
	idt_set_gate(0x1A, isr26, 0x08, 0x8E);
	idt_set_gate(0x1B, isr27, 0x08, 0x8E);
	idt_set_gate(0x1C, isr28, 0x08, 0x8E);
	idt_set_gate(0x1D, isr29, 0x08, 0x8E);
	idt_set_gate(0x1E, isr30, 0x08, 0x8E);
	idt_set_gate(0x1F, isr31, 0x08, 0x8E);
	idt_set_gate(0x80, isr128, 0x08, 0xEE);
}
