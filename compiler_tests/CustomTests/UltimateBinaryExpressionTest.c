int test_binary_expressions() {
    int a = 5, b = 3;
    float x = 5.5f, y = 2.2f;
    double p = 10.1, q = 3.3;
    char c1 = 'A', c2 = 'B';
    int int_sum = a + b;
    int int_diff = a - b;
    int int_mult = a * b;
    int int_div = a / b;
    int int_mod = a % b;

    int int_bit_and = a & b;
    int int_bit_or = a | b;
    int int_bit_xor = a ^ b;
    int int_shift_left = a << 1;
    int int_shift_right = a >> 1;

    int int_eq = (a == b);
    int int_neq = (a != b);
    int int_lt = (a < b);
    int int_gt = (a > b);
    int int_lte = (a <= b);
    int int_gte = (a >= b);

    int int_logical_and = a && b;
    int int_logical_or = a || b;

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
        int_sum + int_diff + int_mult + int_div + int_mod +
        int_bit_and + int_bit_or + int_bit_xor + int_shift_left + int_shift_right +
        int_eq + int_neq + int_lt + int_gt + int_lte + int_gte +
        int_logical_and + int_logical_or +

        float_eq + float_neq + float_lt + float_gt + float_lte + float_gte +
        float_logical_and + float_logical_or +

        double_eq + double_neq + double_lt + double_gt + double_lte + double_gte +
        double_logical_and + double_logical_or +

        char_sum + char_diff + char_mult + char_div +
        char_bit_and + char_bit_or + char_bit_xor +
        char_eq + char_neq + char_lt + char_gt + char_lte + char_gte +
        char_logical_and + char_logical_or;

    return total;
}
