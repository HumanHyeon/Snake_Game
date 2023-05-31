#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
#include <windows.h>
#include <conio.h>
#pragma warning(disable:4996)

#define FINGER_C "☞"
#define ITEM_C "o"
#define BODY_C "X"
#define UP_C "O"
#define DOWN_C "O"
#define LEFT_C "O"
#define RIGHT_C "O"

#define MAX 100
#define MAX_SIZE (MAX * MAX + 1)

#define DISPLAY_Y 5
#define DISPLAY_X (MAP_X + 5)

typedef struct deque {
    int arr[MAX_SIZE];
    int front, rear;
} Deque;
typedef enum { U, D, L, R } Direction;

//게임 조작 변수
static int MAP_Y = 20;
static int MAP_X = 20;
static int INTERVAL = 100;
static int ITEM_RESPONSE = 10;

static int START_Y = 1;
static int START_X = 1;
Direction flag = R;

//게임 내 변수
#define EMPTY 0
#define WALL 1
#define WORM 2
#define ITEM 3

#define PROGRESS 0
#define DEATH 1

#define REGULAR 0
#define CUSTOM 1
#define BBS 2

static const int dy[] = { -1, 1, 0, 0 };
static const int dx[] = { 0, 0, -1, 1 };

clock_t start_time;
clock_t end_time;

Deque y_dq;
Deque x_dq;

int status = PROGRESS;
char nickname[20];

int map[MAX + 2][MAX + 2];
int item_time;
int move_cnt = 0;
int block_cnt = 0;

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
        return (-1);
    }
    return dq->arr[dq->front];
}

int get_rear(Deque* dq) {
    if (is_empty(dq)) {
        return (-1);
    }
    return dq->arr[dq->rear];
}

void gotoxy(int x, int y) {
    COORD pos = { x, y };
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);
}

int input_arrow() {
    DWORD        mode;
    INPUT_RECORD in_rec;
    DWORD        num_read;

    HANDLE h_input = GetStdHandle(STD_INPUT_HANDLE);
    if (h_input == INVALID_HANDLE_VALUE) {
        return -1;
    }

    // 입력 모드를 확인하고, 이벤트를 읽을 수 있도록 설정합니다.
    if (!GetConsoleMode(h_input, &mode)) {
        return -1;
    }
    SetConsoleMode(h_input, mode & ~(ENABLE_MOUSE_INPUT | ENABLE_INSERT_MODE | ENABLE_QUICK_EDIT_MODE));

    // 1초 동안만 키 이벤트를 대기합니다.
    DWORD start_time = GetTickCount64();
    bool input_received = false;
    while (!input_received) {
        DWORD elapsed_time = GetTickCount64() - start_time;
        if (elapsed_time > INTERVAL) {
            break;
        }

        DWORD result = WaitForSingleObject(h_input, INTERVAL - elapsed_time);
        if (result == WAIT_OBJECT_0) {
            // 키 이벤트를 읽어옵니다.
            if (!ReadConsoleInput(h_input, &in_rec, 1, &num_read)) {
                return -1;
            }

            if (in_rec.EventType == KEY_EVENT && in_rec.Event.KeyEvent.bKeyDown) {
                switch (in_rec.Event.KeyEvent.wVirtualKeyCode) {
                case VK_UP:
                    flag = U;
                    break;
                case VK_DOWN:
                    flag = D;
                    break;
                case VK_RIGHT:
                    flag = R;
                    break;
                case VK_LEFT:
                    flag = L;
                    break;
                default:
                    break;
                }
                input_received = true;
            }
        }
        else if (result == WAIT_FAILED) {
            return -1;
        }
    }

    // 입력 모드를 원래대로 돌려놓습니다.
    //SetConsoleMode(h_input, mode);
    fflush(stdin);

    return 0;
}

void generate_item() {
    int r_y, r_x;

    item_time = (rand() + clock()) % ITEM_RESPONSE + 1;
    while (true) {
        r_y = (rand() + clock()) % (MAP_Y - 1) + 1;
        r_x = (rand() + clock()) % (MAP_X - 1) + 1;

        if (map[r_y][r_x] == EMPTY)
            break;
    }

    map[r_y][r_x] = ITEM;
    gotoxy(r_x, r_y);
    printf(ITEM_C);
}

void print_map() {
    for (int y = 0; y <= MAP_Y + 1; ++y) {
        if (y == 0 || y == MAP_Y + 1) {
            if (y == 0) {
                gotoxy(0, 0);
                printf("┌");
                for (int x = 1; x <= MAP_X; ++x)
                    printf("─");
                printf("┐");
            }
            else {
                gotoxy(0, MAP_Y + 1);
                printf("└");
                for (int x = 1; x <= MAP_X; ++x)
                    printf("─");
                printf("┘");
            }
        }
        else {
            gotoxy(0, y);
            printf("│");
            gotoxy(MAP_X + 1, y);
            printf("│");
        }
    }
    putchar('\n');
    fflush(stdout);
}

void print_head(int y, int x) {
    gotoxy(x, y);
    if (flag == U)  printf(UP_C);
    else if (flag == D) printf(DOWN_C);
    else if (flag == L) printf(LEFT_C);
    else if (flag == R) printf(RIGHT_C);
}

void move_one_block() {
    int y = get_front(&y_dq);
    int x = get_front(&x_dq);
    int ny = y + dy[flag];
    int nx = x + dx[flag];

    if (map[ny][nx] == WALL || map[ny][nx] == WORM) {
        status = DEATH;
        return;
    }

    if (map[ny][nx] == ITEM) {
        gotoxy(x, y);
        printf(BODY_C);

        ++block_cnt;
        print_head(ny, nx);
        map[ny][nx] = WORM;
        add_front(&y_dq, ny);
        add_front(&x_dq, nx);
    }
    else {
        int ty = get_rear(&y_dq);
        int tx = get_rear(&x_dq);

        gotoxy(get_front(&x_dq), get_front(&y_dq));
        printf(BODY_C);

        gotoxy(tx, ty);
        map[ty][tx] = EMPTY;
        putchar(' ');
        delete_rear(&y_dq);
        delete_rear(&x_dq);


        print_head(ny, nx);
        map[ny][nx] = WORM;
        add_front(&y_dq, ny);
        add_front(&x_dq, nx);
    }
    fflush(stdout);
}

void game() {
    gotoxy(DISPLAY_X, DISPLAY_Y);
    printf("[ 게 임 정 보 ]");
    gotoxy(DISPLAY_X, DISPLAY_Y + 1);
    printf("MOVE: ");
    gotoxy(DISPLAY_X, DISPLAY_Y + 2);
    printf("BLOCK: ");
    gotoxy(DISPLAY_X, DISPLAY_Y + 3);
    printf("TIME: ");

    start_time = clock();

    while (true) {
        clock_t middle_time = clock();
        double elapsed_time = (double)(middle_time - start_time) / CLOCKS_PER_SEC;
        int minutes = (int)(elapsed_time / 60);
        int seconds = (int)(elapsed_time - minutes * 60);

        gotoxy(DISPLAY_X + 5, DISPLAY_Y + 3);
        printf("%2d min %02d sec", minutes, seconds);

        ++move_cnt;
        --item_time;

        input_arrow();
        move_one_block();
        if (status == DEATH)    break;;
        if (item_time <= 0)
            generate_item();


        gotoxy(DISPLAY_X + 5, DISPLAY_Y + 1);
        printf("%d", move_cnt);
        gotoxy(DISPLAY_X + 6, DISPLAY_Y + 2);
        printf("%d", block_cnt);
    }
}

void game_setting() {
    HANDLE console_handle = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cursor_info;
    cursor_info.dwSize = 1;
    cursor_info.bVisible = FALSE;
    SetConsoleCursorInfo(console_handle, &cursor_info);

    CONSOLE_FONT_INFOEX font_info = { 0 };
    font_info.cbSize = sizeof(font_info);
    GetCurrentConsoleFontEx(console_handle, FALSE, &font_info);
    font_info.dwFontSize.X = 22;
    font_info.dwFontSize.Y = 22;
    SetCurrentConsoleFontEx(console_handle, FALSE, &font_info);

    SMALL_RECT windowSize = { 0, 0, MAP_X + 30, MAP_Y + 1 };
    SetConsoleWindowInfo(console_handle, TRUE, &windowSize);
}

void init() {
    srand((unsigned int)time(NULL));

    init_deque(&y_dq);
    init_deque(&x_dq);

    for (int y = 0; y <= MAP_Y + 1; ++y) {
        if (y == 0 || y == MAP_Y + 1)
            for (int x = 0; x <= MAP_X + 1; ++x)
                map[y][x] = WALL;
        else
            map[y][0] = map[y][MAP_X + 1] = WALL;
    }

    add_front(&y_dq, START_Y);
    add_front(&x_dq, START_X);
    map[START_Y][START_X] = WORM;
    print_head(START_Y, START_X);
    block_cnt = 1;
}

void regular() {

}

void customize() {
    printf("속도(매우빠름1~매우느림100): ");
    scanf("%d", &INTERVAL);
    INTERVAL *= 10;
    printf("아이템 리스폰 간격(1초부터 N초 랜덤, 1~10): ");
    scanf("%d", &ITEM_RESPONSE);
    printf("맵 크기 Y(1~100): ");
    scanf("%d", &MAP_Y);
    printf("맵 크기 X(1~100): ");
    scanf("%d", &MAP_X);

    system("cls");
    print_map();
    init();
    game_setting();
    game();
}

void run_connect() {
    HANDLE console_handle = GetStdHandle(STD_OUTPUT_HANDLE);
    SMALL_RECT windowSize = { 0, 0, MAP_X + 30, MAP_Y + 1 };
    SetConsoleWindowInfo(console_handle, TRUE, &windowSize);

    while (true) {
        printf(FINGER_C);
        printf(" 닉네임 입력: ");
        scanf("%s", nickname);
       
        if (strlen(nickname) > 1 && strlen(nickname) <= 10)
            break;
    }
    printf("%s\n", nickname);

    
}

int main() {
//    run_connect();
    customize();
    game();
    while (true) {
        input_arrow();
        generate_item();
    }


    end_time = clock();
    double elapsed_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;
    int minutes = (int)(elapsed_time / 60);
    int seconds = (int)(elapsed_time - minutes * 60);

    gotoxy(DISPLAY_X + 5, DISPLAY_Y + 3);
    printf("%2d min %02d sec", minutes, seconds);
}