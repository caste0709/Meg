#include <iostream>
#include "arbol.h"

void print_bool(bool cond) {
    if (cond) {
        std::cout << "True\n";
    } else {
        std::cout << "False\n";
    }
}

int main() {
    BPlusTree<int> bpt(5);  // max 4 claves por nodo

    bpt.insert(10);
    std::cout << "Después de insertar 10:\n";
    bpt.bpt_print();
    std::cout << "----\n";

    bpt.insert(27);
    std::cout << "Después de insertar 27:\n";
    bpt.bpt_print();
    std::cout << "----\n";

    bpt.insert(29);
    std::cout << "Después de insertar 29:\n";
    bpt.bpt_print();
    std::cout << "----\n";

    bpt.insert(17);
    std::cout << "Después de insertar 17:\n";
    bpt.bpt_print();
    std::cout << "----\n";

    bpt.insert(16);
    std::cout << "Después de insertar 17:\n";
    bpt.bpt_print();
    std::cout << "----\n";
}

