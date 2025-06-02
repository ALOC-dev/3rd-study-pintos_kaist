#include "userprog/syscall.h"
#include "filesys/filesys.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/loader.h"
#include "userprog/gdt.h"
#include "threads/flags.h"
#include "intrinsic.h"
#ifndef STDIN_FILENO
#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#endif
#define FDCOUNT_LIMIT 128

void syscall_entry (void);
void syscall_handler (struct intr_frame *);
void check_address(void* addr);

/* System call.
 *
 * Previously system call services was handled by the interrupt handler
 * (e.g. int 0x80 in linux). However, in x86-64, the manufacturer supplies
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
/* The main system call interface */
void syscall_handler (struct intr_frame *f UNUSED) {

	/* rax = 시스템 콜 넘버 */
	int sys_number = f->R.rax;
	// TODO: Your implementation goes here.
	switch (sys_number)
	{
		case SYS_HALT:
			halt();
			break;
		case SYS_EXIT:
			exit(f->R.rdi);
			break;
		case SYS_FORK:
			fork(f->R.rdi);
			break;
		case SYS_WAIT:
			wait(f->R.rdi);
			break;
		case SYS_CREATE:
			create(f->R.rdi, f->R.rsi);
			break;
		case SYS_REMOVE:
			remove(f->R.rdi);
			break;
		case SYS_OPEN:
			open(f->R.rdi);
			break;
		case SYS_FILESIZE:
			filesize(f->R.rdi);
			break;
		case SYS_READ:
			read(f->R.rdi,f->R.rsi, f->R.rdx);
			break;
		case SYS_WRITE:
			write(f->R.rdi,f->R.rsi, f->R.rdx);
			break;
		case SYS_SEEK:
			seek(f->R.rdi, f->R.rsi);
			break;
		case SYS_TELL:
			tell(f->R.rdi);
			break;
		case SYS_CLOSE:
			close(f->R.rdi);
			break;
		default:
			thread_exit();
	}

	printf ("system call!\n");
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


int write(int fd, const void *buffer, unsigned size) {
    check_address(buffer);
    struct file *f = process_get_file(fd);
    /* 실행된 후 쓰여진 바이트 수를 저장하는 변수 */
    int bytes_written = 0;

    lock_acquire(&filesys_lock);

    if (fd == STDOUT_FILENO) {
        /* 쓰기가 표준 출력인 경우, 버퍼의 내용을 화면에 출력하고 쓰여진 바이트 수를 저장 */
        putbuf(buffer, size);
        bytes_written = size;
    } else if (fd == STDIN_FILENO) {
        /* 쓰기가 표준 입력인 경우, 파일 시스템 잠금 해제 후 -1 반환 */
        lock_release(&filesys_lock);
        return -1;
    } else if (fd >= 2) {
        if (f == NULL) {
            /* 쓰기가 파일에 대한 것인데 파일이 없는 경우, 파일 시스템 잠금 해제 후 -1 반환 */
            lock_release(&filesys_lock);
            return -1;
        }
        /* 파일에 버퍼의 내용을 쓰고 쓰여진 바이트 수를 저장 */
        bytes_written = file_write(f, buffer, size);
    }

    lock_release(&filesys_lock);

    return bytes_written;
}


int open (const char *file) {
    check_address(file);  // 주소 유효성 검사

    struct file *f = filesys_open(file);  // 파일 시스템에서 파일 열기

    if (file == NULL) {
        return -1;  // 파일 열기 실패 시 -1 반환
    }

    int fd = process_add_file(file);  // 파일을 프로세스에 추가하고 파일 디스크립터 얻기

    if (fd == -1) {
        file_close(file);  // 파일 디스크립터 추가 실패 시 열었던 파일 닫기
    }

    return fd;  // 파일 디스크립터 반환
}

// 파일 객체에 대한 파일 디스크립터를 생성하는 함수
int process_add_file(struct file *f) {
    struct thread *t = thread_current();  // 현재 실행 중인 스레드 구조체
    struct file **fdt = t->fdt;  // 현재 스레드의 파일 디스크립터 테이블
    int fd = t->fdidx;  // 현재 스레드의 파일 디스크립터 인덱스

    // 파일 디스크립터 테이블에서 비어있는 위치를 찾아 파일을 추가한다.
    while (t->fdt[fd] != NULL && fd < FDCOUNT_LIMIT) {
        fd++;
    }
    if (fd >= FDCOUNT_LIMIT) {
        // 파일 디스크립터 테이블이 가득찬 경우, -1을 반환한다.
        return -1;
    }

    t->fdidx = fd;  // 다음 파일에 할당될 파일 디스크립터 인덱스 갱신
    fdt[fd] = f;  // 파일 객체를 파일 디스크립터 테이블에 추가

    return fd;  // 할당된 파일 디스크립터 반환
}

// 파일 디스크립터를 사용하여 파일의 크기를 가져오는 함수
int filesize(int fd) {
    // 주어진 파일 디스크립터로부터 파일 객체를 가져옴
    struct file *f = process_get_file(fd);

    // 파일 객체가 NULL인 경우, 즉 파일을 찾을 수 없는 경우 -1을 반환
    if (f == NULL) {
        return -1;
    }

    // 파일 객체의 크기를 가져와서 반환
    file_length(f);
}

// 주어진 파일 디스크립터를 사용하여 스레드의 파일 테이블에서 파일 객체를 찾아 반환하는 함수
struct file *process_get_file(int fd) {
    // 파일 디스크립터가 유효한 범위를 벗어나면 NULL을 반환
    if (fd < 0 || fd >= FDCOUNT_LIMIT) {
        return NULL;
    }

    // 현재 실행 중인 스레드의 정보를 가져옴
    struct thread *t = thread_current();
    // 현재 스레드의 파일 테이블을 가져옴
    struct file **fdt = t->fdt;

    // 파일 테이블에서 주어진 파일 디스크립터에 해당하는 파일 객체를 가져옴
    struct file *file = fdt[fd];

    // 찾은 파일 객체를 반환
    return file;
}


// 주어진 파일 디스크립터를 사용하여 파일로부터 데이터를 읽어오는 함수
int read(int fd, void *buffer, unsigned size) {
    // 주어진 buffer의 주소 유효성을 확인
    check_address(buffer);
    check_address((char*)buffer + size - 1);

    // buffer를 unsigned char 포인터로 캐스팅하여 사용하기 위한 변수
    unsigned char *buf = buffer;
    int bytes_written;

    // 주어진 파일 디스크립터로부터 파일 객체를 가져옴
    struct file *f = process_get_file(fd);

    // 파일 객체가 NULL인 경우, 즉 파일을 찾을 수 없는 경우 -1을 반환
    if (f == NULL) {
        return -1;
    }

    if (fd == STDIN_FILENO) {
        // 파일 디스크립터가 표준 입력인 경우, 키보드 입력을 받아 buffer에 저장
        char key;
        for (int bytes_written = 0; bytes_written < size; bytes_written++) {
            key = input_getc();
            *buf++ = key;
            if (key == '\0') {
                break;
            }
        }
    } else if (fd == STDOUT_FILENO) {
        // 파일 디스크립터가 표준 출력인 경우, 읽기 작업을 지원하지 않으므로 -1을 반환
        return -1;
    } else {
        // 일반 파일인 경우, 파일을 읽어와서 buffer에 저장하고 읽은 바이트 수를 반환
        lock_acquire(&filesys_lock);
        bytes_written = file_read(f, buffer, size);
        lock_release(&filesys_lock);
    }

    return bytes_written;
}

// 주어진 파일 디스크립터를 사용하여 파일 내에서 지정된 위치로 이동하는 함수
void seek(int fd, unsigned position) {
    // 파일 디스크립터가 표준 입력 또는 표준 출력인 경우 함수 종료
    if (fd < 2) {
        return;
    }

    // 주어진 파일 디스크립터로부터 파일 객체를 가져옴
    struct file *f = process_get_file(fd);

    // 파일 객체의 주소 유효성을 확인
    check_address(f);

    // 파일 객체가 NULL인 경우 함수 종료
    if (f == NULL) {
        return;
    }

    // 파일 객체의 위치를 주어진 position으로 이동
    file_seek(f, position);
}



// 주어진 파일 디스크립터를 사용하여 파일 내 현재 커서의 위치를 반환하는 함수
unsigned tell(int fd) {
    // 파일 디스크립터가 표준 입력 또는 표준 출력인 경우 함수 종료
    if (fd < 2) {
        return;
    }

    // 파일 디스크립터가 유효하지 않은 경우 함수 종료
    if (fd < 0 || fd >= FDCOUNT_LIMIT) {
        return;
    }

    // 주어진 파일 디스크립터로부터 파일 객체를 가져옴
    struct file *f = process_get_file(fd);

    // 파일 객체의 주소 유효성을 확인
    check_address(f);

    // 파일 객체가 NULL인 경우 함수 종료
    if (f == NULL) {
        return;
    }

    // 파일 객체의 현재 커서 위치를 반환
    return file_tell(f);
}


// 주어진 파일 디스크립터를 사용하여 열린 파일을 닫는 함수
void close(int fd) {
    // 파일 디스크립터가 표준 입력 또는 표준 출력인 경우 함수 종료
    if (fd < 2) {
        return;
    }

    // 주어진 파일 디스크립터로부터 파일 객체를 가져옴
    struct file *f = process_get_file(fd);

    // 파일 객체의 주소 유효성을 확인
    check_address(f);

    // 파일 객체가 NULL인 경우 함수 종료
    if (f == NULL) {
        return;
    }

    // 파일 디스크립터의 범위를 확인하여 파일 테이블 엔트리를 NULL로 설정하여 파일을 닫음
    if (fd < 0 || fd >= FDCOUNT_LIMIT) {
        return;
    }
    
    thread_current()->fdt[fd] = NULL;
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