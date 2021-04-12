// TODO: need a '_start' routine to set-up the stack and data section
void main(void) {
    int a = 6;
    int c = 7;

    for (int i=0; i<10; ++i) {
        c = a+i;
        a = c+i;
    }

    for(;;);
}