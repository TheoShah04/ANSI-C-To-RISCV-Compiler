int test_binary_expressions() {
    int a = 5, b = 3;
    float x = 5.5f, y = 2.2f;
    double p = 10.1, q = 3.3;
    char c1 = 'A', c2 = 'B';

    float float_sum = x + y;
    float float_diff = x - y;
    float float_mult = x * y;
    float float_div = x / y;

    int float_eq = (x == y);
    int float_neq = (x != y);
    int float_lt = (x < y);
    int float_gt = (x > y);
    int float_lte = (x <= y);
    int float_gte = (x >= y);

    int float_logical_and = x && y;
    int float_logical_or = x || y;
    int total =
    float_eq + float_neq + float_lt + float_gt + float_lte + float_gte +
        float_logical_and + float_logical_or;
    return total;
}
