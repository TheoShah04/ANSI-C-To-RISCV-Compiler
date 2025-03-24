int fakeputs(char *x){
    char *y = x;
    if(x == y){
        return 1;
    }
    return 0;
}

int g()
{
    return fakeputs("wibble");
}
