int test_binary_expressions() {
    int a = 5, b = 3;
    float x = 5.5f, y = 2.2f;
    double p = 10.1, q = 3.3;
    char c1 = 'A', c2 = 'B';

    double double_sum = p + q;
    double double_diff = p - q;
    double double_mult = p * q;
    double double_div = p / q;

    int double_eq = (p == q);
    int double_neq = (p != q);
    int double_lt = (p < q);
    int double_gt = (p > q);
    int double_lte = (p <= q);
    int double_gte = (p >= q);

    int double_logical_and = p && q;
    int double_logical_or = p || q;
    int total =
    double_eq + double_neq + double_lt + double_gt + double_lte + double_gte +
    double_logical_and + double_logical_or;
    return total;
}
