/* Open a file. */

#include <syscall.h>
#include "tests/lib.h"
#include "tests/main.h"

void
test_main (void) 
{
  int handle = open ("sample.txt"); 
  //이 open을 호출한 것은 user program이기 때문에 바로 우리가 구현한 userprog/syscall.c의 open이 불리는 것이 아니고,
  //lib/user/syscall.h의 open이 불리게 된다.
  if (handle < 2)
    fail ("open() returned %d", handle);
}
