#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <random>

struct ListElem {
    struct ListElem* next;
};

ListElem* createCircularList(size_t listElems);

int main(int argc, const char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s <list size in KB>\n", argv[0]);
        exit(1);
    }

    // Create circular list of specified size
    size_t listKBs = atol(argv[1]);
    size_t listElems = listKBs * 1024ul / sizeof(ListElem);
    ListElem* head = createCircularList(listElems);

    // Traverse circular list
    ListElem* cur = head;

    // We want to do this, but the compiler's optimizer sees this code has no
    // effect and removes it...
    // while (true) {
    //     cur = cur->next;
    // }

    // Here's a version that confuses the optimizer enough and performs
    // multiple traversals per iteration
	for (int i = 0; i < 1000ul * 12 * 1024 / listKBs; i++) {
		cur = head;
		int j = 0;
    	while (j < listElems) j++, cur = cur->next;// printf("%d\n", cur-head);
	}
    printf("Cur: %p\n", cur);
    return 0;
}

void swap(ListElem &a, ListElem &b) {
    ListElem* tmp1 = a.next;
	ListElem* tmp2 = b.next;
    b.next = b.next->next;
    a.next = tmp2;
    a.next->next = tmp1;
}

ListElem* createCircularList(size_t elems) {
    ListElem* listArray = (ListElem*) calloc(elems, sizeof(ListElem));

    std::mt19937 gen(0xB1C5A11D1E);
    std::uniform_int_distribution<uint32_t> d(0, 1<<30);

    // Initialize with contiguous, circular refs and random data
    for (uint32_t i = 0; i < elems; i++) {
        listArray[i].next = &listArray[(i + 1) % elems];
    }
    //listArray[elems-1].next = NULL;
    // Shuffle randomly (Fisher-Yates shuffle)	
    for (uint32_t i = elems-2; i > 0; i--) {
        uint32_t j = d(gen) % i;  // j is in [0,...,i)
        swap(listArray[i], listArray[j]);
    }
    return listArray;
}

