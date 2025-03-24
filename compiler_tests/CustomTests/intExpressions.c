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
    int total =
    int_sum + int_diff + int_mult + int_div + int_mod +
        int_bit_and + int_bit_or + int_bit_xor + int_shift_left + int_shift_right +
        int_eq + int_neq + int_lt + int_gt + int_lte + int_gte +
        int_logical_and + int_logical_or;
    return total;
}
