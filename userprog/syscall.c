#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/loader.h"
#include "userprog/gdt.h"
#include "threads/flags.h"
#include "intrinsic.h"


void syscall_entry (void);
void syscall_handler (struct intr_frame *);
void check_address(void* addr);

/* System call.
 *
 * Previously system call services was handled by the interrupt handler
 * (e.g. int 0x80 in l-inux). However, in x86-64, the manufacturer supplies
 * efficient path for requesting the system call, the `syscall` instruction.
 *
 * The syscall instruction works by reading the values from the the Model
 * Specific Register (MSR). For the details, see the manual. */

#define MSR_STAR 0xc0000081         /* Segment selector msr */
#define MSR_LSTAR 0xc0000082        /* Long mode SYSCALL target */
#define MSR_SYSCALL_MASK 0xc0000084 /* Mask for the eflags */

void
syscall_init (void) {
	write_msr(MSR_STAR, ((uint64_t)SEL_UCSEG - 0x10) << 48  |
			((uint64_t)SEL_KCSEG) << 32);
	write_msr(MSR_LSTAR, (uint64_t) syscall_entry);

	/* The interrupt service rountine should not serve any interrupts
	 * until the syscall_entry swaps the userland stack to the kernel
	 * mode stack. Therefore, we masked the FLAG_FL. */
	write_msr(MSR_SYSCALL_MASK,
			FLAG_IF | FLAG_TF | FLAG_DF | FLAG_IOPL | FLAG_AC | FLAG_NT);
}

/* The main system call interface */
//시스템 콜이 호출될 때 관리하는 syscall_handler 구현하기.
//ex) halt(), exit(), create(), remove() 등
//syscall_handler를 호출할 때 인자 값 수에 맞게 인자 넣어주기.
//순서는 rdi, rsi, rdx 순서대로 인자 값을 넣어주는 것이고 특별한 값이 있는게 아니다.
void
syscall_handler (struct intr_frame *f UNUSED) {
	// TODO: Your implementation goes here.
	int syscall_n = f->R.rax;
	switch (syscall_n)
	{
	case SYS_HALT:
		halt();
		break;
	case SYS_EXIT:
		exit(f->R.rdi);
		break;
	case SYS_FORK:
		f->R.rax = fork(f->R.rdi, f);
		break;
	case SYS_EXEC:
		f->R.rax = exec(f->R.rdi);
		break;
	case SYS_WAIT:
		f->R.rax = wait(f->R.rdi);
		break;
	case SYS_CREATE:
		f->R.rax = create(f->R.rdi, f->R.rsi);
		break;
	case SYS_REMOVE:
		f->R.rax = remove(f->R.rdi);
		break;
	case SYS_OPEN:
		f->R.rax = open(f->R.rdi);
		break;
	case SYS_FILESIZE:
		f->R.rax = filesize(f->R.rdi);
		break;
	case SYS_READ:
		f->R.rax = read(f->R.rdi, f->R.rsi, f->R.rdx);
		break;
	case SYS_WRITE:
		f->R.rax = write(f->R.rdi, f->R.rsi, f->R.rdx);
		break;
	case SYS_SEEK:
		seek(f->R.rdi, f->R.rsi);
		break;
	case SYS_TELL:
		f->R.rax = tell(f->R.rdi);
		break;
	case SYS_CLOSE:
		close(f->R.rdi);
	}


	printf ("system call!\n");
	thread_exit ();
}


//함수 호출 시 핀토스를 종료시키는 시스템 콜
void halt(void){
	power_off();
}


//현재 실행중인 프로세스만 종료시키는 시스템 콜
void exit(int status){
	struct thread* t = thread_current();
	t->exit_status = status;                   // 종료 코드 저장 
    printf("%s: exit(%d)\n", t->name, status); // 종료 메시지 출력
	//부모 프로세스가 wait() 시스템 콜을 통해 **자식의 종료 상태(exit code)**를 받아야 하기 때문
	//따라서 exit()는 단순히 종료만 하는 게 아니라, 자식 프로세스의 상태를 부모가 볼 수 있도록 기록해야 함
	/*wait(pid) 시스템 콜의 목적 : 부모 프로세스가 자식 프로세스의 종료를 기다리고, 
	자식이 exit(status)로 종료할 때 전달한 status 값을 가져오기 위함입니다. */
	thread_exit();
}




//파일을 생성하는 시스템 콜
bool 
create(const char *file, unsigned initial_size) 
{
    check_address(file);

    return filesys_create(file, initial_size);
}



//파일을 삭제하는 시스템 콜
bool 
remove(const char *file) 
{
    check_address(file);

    return filesys_remove(file);
}





void check_address(void* addr)
{
	if(addr == NULL) //null 포인터
		exit(-1);
	if(!is_user_vaddr(addr)) //매핑되지 않은 가상 메모리를 가리키는 포인터
		exit(-1);
	if(pml4_get_page(thread_current() -> pml4, addr) == NULL) //커널 가상 주소 공간을 가리키는 포인터터
		exit(-1);
}