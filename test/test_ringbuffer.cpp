#include <stdint.h>
#include <iostream>
#include <bitset>
#include <string.h>

#define RING_BUF_SIZE 3
char ringbuf[RING_BUF_SIZE][128];
int head = 0;
int ring_counter = 0;

void ringbuf_push(char *str) {
    strcpy(ringbuf[head], str);
    head = (head + 1) % RING_BUF_SIZE;
    if (ring_counter != RING_BUF_SIZE)
    {
        ring_counter++;
    }
}

void ringbuf_print() {
    int start = 0;
    if (ring_counter == RING_BUF_SIZE) {start = head % RING_BUF_SIZE;}
    for (int i = 0; i < ring_counter; i++)
    {
        puts(ringbuf[start]);
        start = (start + 1) % RING_BUF_SIZE;
    }
}


int main() {

   std::cout << "====== Test1: empty ======" << std::endl;
   ringbuf_print();

   std::cout << "====== Test2: 2 item ======" << std::endl;
   ringbuf_push("1-hello");
   ringbuf_push("2-hello world!");
   ringbuf_print();

   std::cout << "====== Test1: 3 with buffuer already full ======" << std::endl;
   ringbuf_push("3-full");
   ringbuf_push("4-wow!");
   ringbuf_push("5-bingo!");
   ringbuf_push("6-bye!");
   ringbuf_print();

   return 0;
}