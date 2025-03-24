int test_binary_expressions() {
    int a = 5, b = 3;
    float x = 5.5f, y = 2.2f;
    double p = 10.1, q = 3.3;
    char c1 = 'A', c2 = 'B';

    int char_sum = c1 + c2;
    int char_diff = c1 - c2;
    int char_mult = c1 * c2;
    int char_div = c1 / c2;

    int char_bit_and = c1 & c2;
    int char_bit_or = c1 | c2;
    int char_bit_xor = c1 ^ c2;

    int char_eq = (c1 == c2);
    int char_neq = (c1 != c2);
    int char_lt = (c1 < c2);
    int char_gt = (c1 > c2);
    int char_lte = (c1 <= c2);
    int char_gte = (c1 >= c2);

    int char_logical_and = c1 && c2;
    int char_logical_or = c1 || c2;

    int total =
        char_sum + char_diff + char_mult + char_div +
        char_bit_and + char_bit_or + char_bit_xor +
        char_eq + char_neq + char_lt + char_gt + char_lte + char_gte +
        char_logical_and + char_logical_or;

    return total;
}
