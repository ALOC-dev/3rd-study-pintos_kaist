#include <syscall.h>

int main (int, char *[]);
void _start (int argc, char *argv[]);

//[Argument Passing]pintos c lib의 유저프로그램 시작 포인트
//_start는 main함수를 감싸고 있는 함수, main은 리턴되면서 exit()을 호출
void
_start (int argc, char *argv[]) {
	exit (main (argc, argv));
}
