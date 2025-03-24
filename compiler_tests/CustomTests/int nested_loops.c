int nested_loops() {
    int sum = 0;
    int i, j;

    for (i = 0; i < 3; i++) {
        for (j = 0; j < 3; j++) {
            sum += i * j;
        }
    }

    return sum;
}
