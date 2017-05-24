/*
    To better understand how multifile stuff works, I'm going to practice.
    gcc main.c a.c b.c -o main
*/
#include "a.h"
#include "b.h"
#include "structs.h"

int main(void){
    func_a1();
    func_a2();
    func_b1();
    func_b2();
}
