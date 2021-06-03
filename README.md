### 문제

다양한 System Call을 사용하여, Myshell Program을 작성하라

#### 구현 기능

1. stdin command parsing 및 fork/exec 를 이용한 command 실행
2. background process 제공 ( & ) 즉, command 뒤에 &를 붙이면, background 로 실행되어야 한다. 
3. signal 처리. terminal 에서 발생하는 SIGINT, SIGQUIT 에 대한 처리
4. pipe 지원 ( | )

### 가정

+ 주석(#)을 이용하지 않는다
+ 실행한 MyShell을 종료할 경우 exit, logout 명령을 이용하여 종료한다.

### 구현방법

#### fork

새로운 프로세스를 하나 생성한다.

fork를 실행한 부모 코드의 경우, 자식의 프로세스 ID를 반환하고

생성된 프로세스인 자식의 경우, 0을 반환하는 함수이다.

```c
#include <stdio.h> 
#include <unistd.h> 
#include <string.h> 
#include <stdlib.h>
#include <stdlib.h>

int main(int argc, char * argv[]){
    pid_t pid;
    
    printf("Calling fork...\n");
    
    pid = fork();
    if(pid == -1)
        printf("ERROR\n");
	else if(pid == 0)
        printf("Child\n");
	else
        printf("Parent\n");
    
    return 0;
}
```

#### execvp

execvp 함수는 실행가능한 file의 실행코드를 현재 프로세스에 적재하고 기존의 실행코드와 교체하여 새로운 기능으로 실행합니다.

위의 결과로 이전에 실행되고 있던 프로그램의 기능은 사라지고 새로움 프로그램을 메모리에 loading 하여 처음부터 실행합니다.

```c
#include <stdio.h> 
#include <unistd.h> 
#include <string.h> 
#include <stdlib.h>

int main(int argc, char *argv[]) { 
    char **new_argv; char command[] = "ls"; 
    int idx; new_argv = (char **)malloc(sizeof(char *) * (argc + 1));
    new_argv[0] = command;
    for(idx = 1; idx < argc; idx++) 
    { 
        new_argv[idx] = argv[idx]; 
    } 
    new_argv[argc] = NULL; 
    if(execvp("ls", new_argv) == -1) 
    {
        fprintf(stderr, "ERROR\n"); 
        return 1; 
    }
    printf("ERROR\n"); 
    return 0; 
}
```

exec 계열의 execvp와 fork를 사용하여, 명령이 동작하는 MyShell을 만들 수 있다.

execvp를 사용할 경우, 프로세스의 기능이 execvp에 전달된 프로그램의 기능으로 변경되어, 기존에 존재하던 프로그램의 기능을 더이상 찾을 수 없다.

fork를 이용한 경우, 새로운 프로세스를 생성하였고, 그곳에서 execvp가 작동하기 때문에, 기존에 동작하던, MyShell은 그대로 동작하면서 새롭게 생성된 자식 프로세스에서는 execvp를 이용한 명령 동작이 가능한 것이다.

또한, execvp의 경우 cd와 exit를 지원하지 않기 때문에 이의 경우 각각에 맞는 처리를 해주어야 한다,

#### wait

한 프로세스로 하여금, 다른 프로세스가 종료될 떄까지 대기하는 기능을 가지고 있다.

위의 기능을 이용하여, 기본적인 프로세스 구현을 만든다고 하더라도, 기존에 실행되고 있는 프로세스가 종료될 때까지 기다려야 한다.

따라서 wait을 이용하여, 기존에 실행중인 프로세스가 동작할 떄까지 대기한다.

#### signal

인터럽트 키를 누르는 경우와 같이, 현재 동작하는 프로세스와 어떤 상호작용이 필요하다.

이러한 경우에 현재 동작 중인 프로세스에 어떤 신호를 보내게 되고 이러한 것에 대한 처리를 signal함수를 이용하여 할 수 있다.

```c
#include <signal.h>
void ( *signal (int sig, void(*func)(int)) )(int);
```

여기서 sig는 들어오는 signal의 정보(SIGABRT : 비정상 종료, SIGINT 대화식 어텐션...)

func의 경우 대처와 관련된 정보(.SIG_DFL: 기본 처리(default), SIG_IGN:무시(ignore))

이를 이용하여, 기능 3에서 제시하고 있는 foreground와 background에대한 각각의 신호에 따라 처리를 결정할 수 있다.

#### pipe

pipe를 통해서 다른 프로세스와의 상호작용을 할 수 있다.

> \> ls | grep c # 현재 디렉토리에 있는 정보 중, c를 포함한 정보를 출력하라,

dup2를 이용하여 현재 프로세스가 받아들일 표준 출력을 전달하거나, 표준입력을 전달받을 수 있다.

### 실행환경

+ OS
  + Ubuntu 20.04.1 LTS (GNU/Linux 4.4.0-18362-Microsoft x86_64)
  + WSL
+ Compiler
  + gcc
+ Language
  + C

### 실행

#### 기본적인 실행과 간단한 명령어 처리

#### 다양한 특수 기호 처리

그냥 ls를 진행할 경우 현재 디렉토리에 존재하는 파일들을 보여주지만 ;을 이용하여 빈 디렉토리에서 실행 되므로 아무런 출력이 이루어지지 않습니다.

#### 파이프

기능 4의 파이프 3개를 확인합니다

+ 기본 ls
+ ls에서 M이 포함된 내용
+ ls에서 M이 포함된 내용에서 c가 포함된 내용

### 느낀점

시스템 프로그래밍을 수강하고, UNIX 시스템 프로그래밍 교재를 실습하면서 Unix(Linux)에서 어떻게 프로세스나 프로그램이 동작하는지를 알 수 있었습니다.

메모리에 직접적으로 접근할 수 있다고 들어왔던 C를 실제로 직접적으로 이용하는 경우는 단순히 포인터 정도에 불과했지만 실습을 계속 진행하면서 이를 실제로 직접 싨습해 볼 수 있었던 것 같습니다.

기말 과제를 진행하면서, 다양한 예외 상황이나, 기능들에 대해서 실습하고 그 결과를 확인하면서 아직 수강하지 않은 운영체제 과목을 수강하면서 조금 더 자세한 내용을 수강하고 싶습니다.
