#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/time.h>
#include <termios.h>

#define BOARD_Y 20
#define BOARD_X 20

#define START_Y 2
#define START_X 2
#define END_Y (BOARD_Y + 1)
#define END_X (BOARD_X + 1)
static const int display_y = END_Y + 5;
static const int display_x = 5;

#define WALL_C "█"
#define BODY "㉮"
#define UP "△"
#define RIGHT "▷"
#define LEFT "◁"
#define DOWN "▽"
#define ITEM_C "Ó"

#define MAX_SIZE (BOARD_Y * BOARD_X)

#define EMPTY 0
#define WALL 1
#define WORM 2
#define ITEM 3

#define PROGRESS -1
#define DEATH 1
#define CLEAR 2

typedef enum { U, R, L, D } Direction;
static const int dy[] = { -1, 0, 0, 1 };
static const int dx[] = { 0, 1, -1, 0 };

static int FIN = PROGRESS;
static int move_cnt = 0;
static int item_time = 0;

//Customize Game Info
static const int start_y = 1;
static const int start_x = 1;
static Direction flag = R;
static const int item_response = 10;
static const int g_time = 100000 * 1.5;

typedef struct deque {
    int arr[MAX_SIZE];
    int front, rear;
} Deque;

static const int ITEM_TIME = 5;
int g[BOARD_Y + 2][BOARD_X + 2];

Deque y_dq, x_dq;


void init_deque(Deque* dq) {
    dq->front = -1;
    dq->rear = -1;
}

int is_empty(Deque* dq) {
    return (dq->front == -1);
}

int is_full(Deque* dq) {
    return ((dq->rear + 1) % MAX_SIZE == dq->front);
}

void add_front(Deque* dq, int item) {
    if (is_full(dq)) {
        //printf("Deque is full.\n");
        return;
    }
    if (dq->front == -1) {
        dq->front = 0;
        dq->rear = 0;
    }
    else if (dq->front == 0)
        dq->front = MAX_SIZE - 1;
    else
        dq->front--;
    dq->arr[dq->front] = item;
}

void add_rear(Deque* dq, int item) {
    if (is_full(dq)) {
        //printf("Deque is full.\n");
        return;
    }
    if (dq->front == -1) {
        dq->front = 0;
        dq->rear = 0;
    }
    else if (dq->rear == MAX_SIZE - 1)
        dq->rear = 0;
    else
        dq->rear++;
    dq->arr[dq->rear] = item;
}

int delete_front(Deque* dq) {
    int item;
    if (is_empty(dq)) {
        //printf("Deque is empty.\n");
        //exit(1);
        return (-1);
    }
    item = dq->arr[dq->front];
    if (dq->front == dq->rear)
        dq->front = dq->rear = -1;
    else if (dq->front == MAX_SIZE - 1)
        dq->front = 0;
    else
        dq->front++;
    return item;
}

int delete_rear(Deque* dq) {
    int item;
    if (is_empty(dq)) {
        //printf("Deque is empty.\n");
        //exit(1);
        return (-1);
    }
    item = dq->arr[dq->rear];
    if (dq->front == dq->rear)
        dq->front = dq->rear = -1;
    else if (dq->rear == 0)
        dq->rear = MAX_SIZE - 1;
    else
        dq->rear--;
    return item;
}

int get_front(Deque* dq) {
    if (is_empty(dq)) {
        //printf("Deque is empty.\n");
        //exit(1);
        return (-1);
    }
    return dq->arr[dq->front];
}

int get_rear(Deque* dq) {
    if (is_empty(dq)) {
        //printf("Deque is empty.\n");
        //exit(1);
        return (-1);
    }
    return dq->arr[dq->rear];
}

void gotoxy(int x, int y) {
    printf("%c[%d;%df", 0x1B, y, x);
}

void make_map() {
	for (int i = START_Y; i <= END_Y; ++i) {
		gotoxy(2, i);
		for (int j = START_X; j <= END_X; ++j) {
			putchar('o');	
		}
	}
}

bool isDead(int y, int x) {
	if (g[y][x] == WALL || g[y][x] == WORM)
		return (true);
	return (false);
}

void moveOneBlock() {
	int y = get_front(&y_dq);
	int x = get_front(&x_dq);
	int ny = y + dy[flag];
	int nx = x + dx[flag];
	
	gotoxy(display_x + 7, display_y + 1);
	printf("%d", move_cnt); 

	if (isDead(ny, nx)) {
		FIN = DEATH;
		return ;
	}
	
	if (g[ny][nx] == ITEM) {
		add_front(&y_dq, ny);
		add_front(&x_dq, nx);
		g[ny][nx] = WORM;

		gotoxy(nx + 1, ny + 1);
		if (flag == U)
			printf(UP);
		else if (flag == D)
			printf(DOWN);
		else if (flag == R)
			printf(RIGHT);
		else if (flag == L)
			printf(LEFT);	
	}
	else {
		add_front(&y_dq, ny);
                add_front(&x_dq, nx);
		g[ny][nx] = WORM;

		gotoxy(nx + 1, ny + 1);
                if (flag == U)
                        printf(UP);
                else if (flag == D)
                        printf(DOWN);
                else if (flag == R) 
			printf(RIGHT);
                else if (flag == L)
                        printf(LEFT);


		gotoxy(get_rear(&x_dq) + 1, get_rear(&y_dq) + 1);
                putchar(' ');

		g[get_rear(&y_dq)][get_rear(&x_dq)] = EMPTY;
		delete_rear(&y_dq);
		delete_rear(&x_dq);
	}
}

int input_arrow() {
	fd_set read_fds;
	FD_ZERO(&read_fds);
	FD_SET(STDIN_FILENO, &read_fds);

	struct timeval tv;
    	tv.tv_sec = 0;
   	tv.tv_usec = g_time;

	int ret = select(STDIN_FILENO + 1, &read_fds, NULL, NULL, &tv);

	if (ret == -1) {
        	//perror("select");
        	return 1;
    	}
	else if (ret == 0) {
        //printf("No input received within 1 second.\n");
        	return 0;
    	}
	else {
		char c;
            	read(STDIN_FILENO, &c, 1);

		if (c == '\033') {  // 화살표 키 입력인 경우
                	  struct timeval tv;
char seq[3];
                	if (read(STDIN_FILENO, &seq, 3) == -1) {
                    		//perror("read");
                    		return 1;
                	}
                	if (seq[0] == '[' && seq[2] == '\0') {
                    		switch (seq[1]) {
                        		case 'A':  // 위쪽 방향키
                            			//printf("Up arrow key pressed.\n");
                            			flag = U;
						break;
                        		case 'B':  // 아래쪽 방향키
                            			flag = D;
						//printf("Down arrow key pressed.\n");
                            			break;
                        		case 'C':  // 오른쪽 방향키
               					flag = R;
						//printf("Right arrow key pressed.\n");
                            			break;
                        		case 'D':  // 왼쪽 방향키
						flag = L;
                            			//printf("Left arrow key pressed.\n");
                           	 		break;
                    		}
                	}
         	}
		else {  // 다른 키 입력이면
            		//printf("Non-arrow key pressed: %c\n", c);
        	}
	}
	fflush(stdin);
}

void process_item() {
	int r_y, r_x;
	
	item_time = (rand() + clock()) % item_response + 1;
	while (true) {
		r_y = (rand() + clock()) % BOARD_Y + 1;
		r_x = (rand() + clock()) % BOARD_X + 1;

		if (g[r_y][r_x] == 0)
			break;
	}
	g[r_y][r_x] = ITEM;
	gotoxy(r_x + 1, r_y + 1);
	printf(ITEM_C);
}

void game() {
	while (true) {
		++move_cnt;
		--item_time;
		input_arrow();
		moveOneBlock();
		if (item_time <= 0) 
			process_item();

		fflush(stdout);
		
		if (FIN == DEATH)
			return ;
	}
}

void init() {
	for (int i = 0; i <= END_Y; ++i)
		if (i == 0 || i == END_Y)
			for (int j = 0; j <= END_X; ++j)
				g[i][j] = WALL;
		else
			g[i][0] = g[i][END_X] = WALL;
	
	init_deque(&y_dq);
	init_deque(&x_dq);

	add_rear(&y_dq, start_y);
	add_rear(&x_dq, start_x);
	gotoxy(start_x + 1, start_y + 1);
	if (flag == U)
        	printf(UP);
        else if (flag == D)
                printf(DOWN);
        else if (flag == R)
                printf(RIGHT);
        else if (flag == L)
                printf(LEFT);
}

void print_map() {
        system("clear");
	gotoxy(0, 0);
	for (int i = 0; i <= BOARD_Y + 2; ++i) {
                if (i == 0 || i == BOARD_Y + 2) {
                        for (int j = 0; j < BOARD_X + 2; ++j)
                                printf(WALL_C);
                }
                else {
                        printf(WALL_C);
                        gotoxy(BOARD_X + 2, i);
                        printf(WALL_C);
                        putchar('\n');
                }
        }
	gotoxy(display_x, display_y);
	printf("[ GAME INFO ]");
	gotoxy(display_x, display_y + 1);
	printf("move: ");
        gotoxy(BOARD_X + 3, 0);
        putchar(' ');
	fflush(stdout);
}

void select_game_mode() {
	
}

int main() {
   	struct termios term;
    	struct timeval start, end;

	gettimeofday(&start, NULL);

	tcgetattr(STDIN_FILENO, &term);
    	term.c_lflag &= ~(ICANON | ECHO);
    	tcsetattr(STDIN_FILENO, TCSANOW, &term);
	printf("\033[?25l");

	init();
	print_map();

	game();

	gotoxy(0, BOARD_Y + 2);
	putchar('\n');

	term.c_lflag |= ICANON;
        term.c_lflag |= ECHO;
        tcsetattr(STDIN_FILENO, TCSANOW, &term);
	printf("\033[?25h");

	gettimeofday(&end, NULL);

	long seconds = end.tv_sec - start.tv_sec;  // 초 계산
  	long microseconds = end.tv_usec - start.tv_usec;  // 마이크로초 계산
    	double elapsed = seconds + microseconds * 1e-6;  // 실행 시간 계산
    	int minutes = elapsed / 60;  // 분 단위 계산
    	elapsed -= minutes * 60;  // 분 단위를 제외한 초 단위 계산

	gotoxy(display_x, display_y + 2);
	printf("TIME: ");
	if (minutes > 0)	printf("%d min %.6f sec", minutes, elapsed);
	else			printf("%.6f sec", elapsed);
//	printf("Elapsed time: %d minutes %.6f seconds\n", minutes, elapsed);
	putchar('\n');
}
