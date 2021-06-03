#include <unistd.h>
#include <stdio.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>

#define EOL 1
#define ARG 2
#define AMPERSAND 3
#define SEMICOLON 4
#define PIPE 5

#define MAXARG    512
#define MAXBUF    512

#define FOREGROUND  0
#define BACKGROUND  1

static char inputBuffer[MAXBUF], tokenBuffer[MAXBUF];
static char *ptr = inputBuffer, *tok = tokenBuffer;
static char special[] = {' ','\t','&',';','\n','\0'};
char *args[10][MAXARG + 1];
int runPipe(int num, int where);
int runCommand(char **cline, int where);
int fatal(char *s)
{
	perror(s);
	exit(1);
}

//사용자의 입력을 받는다.
int input(char *prompt){
	char temp;
	int count = 0;
	printf("%s>", prompt);
	tok = tokenBuffer;
	ptr = inputBuffer;
	//받은 문자열이 EOF가 아닐 경우까지 Loop를 실행한다.
	while( (temp = getchar()) != EOF){
		//사용자의 입력이 최대 버퍼의 크기를 넘지 않는 경우
		if(count < MAXBUF)
			inputBuffer[count++] = temp; // 문자를 저장한다.
		
		//사용자의 입력이 최대 버퍼의 크기를 넘지 않으면서 종료된 경우
		if(temp == '\n' && count < MAXBUF){
			inputBuffer[count++] = '\0';// 마지막 글자에 NULL 문자를 넣어준다.
			return count; // 입력된 문자열의 길이를 반환한다.
		}
		else if(temp == '\n'){
			printf("Warning: input line is too long\n");
			count = 0;
			printf("%s>", prompt);
		}
	}
	//받은 문자열이 EOF 이므로, EOF를 반환한다.
	return EOF;
}

//입력된 문자가, 특수한 옵션인지 판단한다.
int inarg(char c){
    char *temp;
	
    for(temp = special; *temp; temp++)
    {
        if(c == *temp)
            return 0;
    }
    return 1;
}

int getToken (char **outptr)
{
    int type;
    int c;

    *outptr = tok;

    while(*ptr == ' ' || *ptr == '\t')
		ptr++;

    *tok++ = *ptr;

	/* 버퍼내의 토큰에 따라 유형 변수를 지정한다.(pipe 추가) */
	switch(*ptr++){
    case '"':
    case '\n'://개행 문자인 경우
		type = EOL;
        break;
    case '&':// &인 경우
        type = AMPERSAND;
        break;
    case ';'://;인 경우
        type = SEMICOLON;
        break;
	case '|'://파이프일 경우
        type = PIPE;
        break;
    default://추가 Argument인 경우
        type = ARG;
        /* 유효한 보통문자들을 계속 읽는다. */
        while (inarg (*ptr))
    			*tok++ = *ptr++;
	}
	
	*tok++ = '\0';
    return type;
}

/* 입력줄을 처리한다. */
int procline(void)
{
	char *arg[MAXARG + 1];//명령어를 저장하는 배열
	int toktype;// 명령의 토큰 유형 
	int narg; // 지금까지의 인수 개수
	int typeGround; // 동작 위치 구분(BackGround, ForeGround)
	int typeToken;// pipe 이외의 타입 구분
	int num = 0;// 파이프 개수
	
	//변수들의 초기화를 진행한다.
	for (int i=0; i<10; i++)
	{
		for(int j=0; j<MAXARG+1; j++)
			args[i][j]=0;
	}
	typeToken=0;
	narg = 0;
	
	//무한 반복문
	while(1)
	{
    //입력된 토큰을 저장하고 tokType을 받아온다
		switch(toktype = getToken(&arg[narg]))
		{
		case ARG: //argument입력이 maxarg보다 적을 경우 인수 개수를 증가
			if(narg < MAXARG)
				narg++;
			break;
      
		case EOL:
		case SEMICOLON:
		//';', '\n'인 경우
			typeGround = FOREGROUND;
		case AMPERSAND:
		//토큰의 종류가 '\n', ';', '&' 중 하나인 경우
			if(toktype == AMPERSAND)
				typeGround = BACKGROUND;

			if(narg != 0)
			{
				int m = 1;
				int fd1, fd2;
				pid_t pid;
				char **temp;
				arg[narg] = NULL;
				temp = arg;
				while ((m <= narg) && ((strcmp (temp[m-1], "<") * strcmp (temp[m-1], ">")) != 0))
					m++;
				
				if (strcmp(arg[0], "cd") == 0)
				{
					if (narg != 1)
					{
						if (chdir(arg[1]) == -1)
							printf("Not exist\n");
					}else{
						chdir(getenv("HOME"));
					}
				}
				//파이프 명령어인 경우 runPipe를 실행한다.
				else if(typeToken == PIPE)
				{
					for(int i=0; i<narg; i++) // 명령어 수만큼 Loop를 돌린다.
						args[num][i] = arg[i]; // 현재 파이프 인덱스에 명령어를 넣고 배열에 저장한다.
					runPipe(num, typeGround);// 현재 파이프 인덱스와 동작 환경의 정보를 전달한다.
				}else{//파이프 명령이 이난경우
					if (strcmp ("exit", *arg ) == 0 || strcmp("logout", *arg) == 0)//종료 문자의 경우 종료한다.
						exit(1);
					runCommand(arg, typeGround); //명령을 실행한다.
				}
			}
			if((toktype == EOL)) // EOL인 경우
				return -1;
			narg = 0;
			break;

			case PIPE: // '|'인 경우
				for (int i = 0; i < narg; i++)
					args[num][i] = arg[i]; // 현재 arg에 저장된 명령어를 args에 저장한다.
				args[num][narg] = NULL; // 마지막 명령어에 NULL을 넣어준다.
				num++; //파이프의 개수를 증가시킨다.
				narg = 0;//초기화
				typeToken = toktype;//현재 토큰의 정보를 저장한다.
				break;
		}
	}
}

// wait를 선택사항으로 하여 명령을 수행한다.
int runCommand(char **cline, int where)
{
	pid_t pid;
	int status;
	//fork하여 자식 프로세스를 생성한다.
	switch (pid = fork()) 
	{
		case -1://오류
			fatal ("smallsh");
			return (-1);
		//자식인 경우 0을 반환받는다.
		//자식 프로세스인 경우
		case 0:
			if(where == BACKGROUND) // background에서 작동하고 있을 때
         {
            signal(SIGINT, SIG_IGN); // 프로세스를 멈추는 인터럽트를 무시한다.
            signal(SIGQUIT, SIG_IGN); // 코어 덤프 하는 인터럽틀를 무시한다.
         }else{
            signal(SIGINT, SIG_DFL); // 프로세스를 멈추는 인터럽트를 무시하지 않는다.
            signal(SIGQUIT,SIG_DFL); // 코어 덤프 하는 인터럽트를 무시하지 않는다.
         }
			execvp(*cline,cline);
			fatal(*cline);
			exit(1);
		//부모의 경우 자식 프로세스의 ID를 받는다.
		default:
			signal(SIGINT, SIG_IGN);
			signal(SIGQUIT, SIG_DFL);
    }
    //백그라운드 실행인 경우
    if(where == BACKGROUND)
    {
		//실행 중인 프로세스 id를 출력한다.
		printf ("[Process id %d]\n", pid);
		return (0);
    }
    
      // fork로 생성한 프로세스가 종료가 될때까지 기다린다.
    if (waitpid(pid,&status,0) == -1)
		return (-1);
    else
		return (status);
}

int runPipe(int num, int where)
{
	pid_t pid[num+1]; // 프로세스 id를 저장하는 배열이다.
    int p[num][2]; //pipe filee descriptor를 저장하는 배열이다
    int status; // 현재 상태를 저장한다.

    //파이프 개수만큼 파이프를 생성한다.
    for(int i = 0; i < num; i++)
    {
      if(pipe(p[i])==-1)
        fatal("pipe call in join");
    }
    
	//파이프 수만큼 실행한다.
    for(int i = 0; i <= num; i++)
    {
		//fork하여 자식 프로세스를 생성한다.
        switch (pid[i] = fork()){
        case -1: //오류
              fatal("Children failed");
		//자식인 경우 0을 반환받는다.
		//자식 프로세스인 경우
        case 0:
			/*
			signal(signal , func)
			
			signal:
			SIGINT 프로세스를 멈추는 인터럽트를
			SIGQUIT 코어 덤프 하는 인터럽트(터미널 종료)
			
			func:
			SIG_IGN 무시한다.
			SIG_DFL 신호에 대한 기본 처리를 진행한다.
			

			*/
              if(where == BACKGROUND) // 백그라운드 실행의 경우
              {
                 signal (SIGINT,SIG_IGN);
                 signal (SIGQUIT,SIG_IGN);
              }
              else
              {
                 signal (SIGINT, SIG_DFL);
                 signal (SIGQUIT, SIG_DFL);
              }
              if(i==0)//첫번째 파이프의 경우
              {
                dup2(p[i][1],1); //표준 출력을 전달한다.
                for(int j=0; j < num; j++) //pipe file descriptor를 close하여 자원을 절약한다.
                {
                    close(p[j][1]);
                    close(p[j][0]);
                }
                execvp(args[i][0], args[i]); // args에 기술된 명령어를 실행한다.
                //execvp가 실행된 경우 아래의 작성된 명령은 실행되지 않는다.
				//execvp가 실행되고 복귀 했다면, 문제가 발생한 것이다.
				fatal(args[i][0]); 
             }
             else if (i == num)//마지막 파이프의 경우
             {
                dup2(p[i-1][0],0); // 표준입력을 전달받는다.
                for(int j = 0;j < num; j++)
                {   
                    close(p[j][0]);
                    close(p[j][1]);
                }
                execvp(args[num][0], args[num]);
                fatal(args[num][0]);
            }
            else
            {
               dup2(p[i-1][0],0); // 표준 입력을 파이프로부터 전달받는다.
               dup2(p[i][1],1);//표준 출력을 다음 파이프로 전달한다.
               for(int j = 0; j < num; j++)
               {
                   close(p[j][0]);
                   close(p[j][1]);
               }
               execvp(args[i][0], args[i]);
               fatal(args[i][0]);
            }
         }
     }

    for(int j = 0; j < num; j++)
    {
      close(p[j][0]);
      close(p[j][1]);
    }
 
 
    if (where == BACKGROUND)
    {
       for(int j = 0; j <= num; j++)
       {
            if (pid[j]>0)
                printf("[Process id %d]\n", pid[j]);
            else
                 sleep(1);
       }
       return(0);
	}
	while(waitpid(pid[num], &status, WNOHANG) == 0) // fork로 생성한 프로세스가 종료가 될때까지 기다린다.
		sleep(1);
	return(0);
}

int main()
{
	static struct sigaction act;
	while (input("prompt") != EOF) // 사용자의 입력이 EOF 가 아닐때까지 반복한다.
	{
		procline();
	}
}