int f() {
    int i, sum = 0;

    for (i = 1; i <= 10; i++) {
        if (i == 5) {
            continue;
        }
        if (i == 8) {
            break;
        }
        sum += i;
    }


    if (sum == 23) {
        return 1;
    }
    return 0;
}
