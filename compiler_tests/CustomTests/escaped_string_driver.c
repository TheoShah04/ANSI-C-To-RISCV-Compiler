char f(char *x);

int main() {
    return !(f("\\helloworld \" ") == '"');
}

