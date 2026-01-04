#include <stdio.h>
#include <stdlib.h>

#define MAX_TREE_HT 256

// ---------------- NODE ----------------
typedef struct Node {
    unsigned char data;
    unsigned freq;
    struct Node *left, *right;
} Node;

// ---------------- MIN HEAP ----------------
typedef struct {
    unsigned size;
    Node **array;
} MinHeap;

// ---------------- NODE UTILS ----------------
Node* newNode(unsigned char data, unsigned freq) {
    Node* node = (Node*)malloc(sizeof(Node));
    node->data = data;
    node->freq = freq;
    node->left = node->right = NULL;
    return node;
}

// ---------------- HEAP UTILS ----------------
MinHeap* createMinHeap() {
    MinHeap* heap = (MinHeap*)malloc(sizeof(MinHeap));
    heap->size = 0;
    heap->array = (Node**)malloc(256 * sizeof(Node*));
    return heap;
}

void swap(Node** a, Node** b) {
    Node* t = *a;
    *a = *b;
    *b = t;
}

void minHeapify(MinHeap* heap, int i) {
    int smallest = i;
    int l = 2 * i + 1;
    int r = 2 * i + 2;

    if (l < heap->size && heap->array[l]->freq < heap->array[smallest]->freq)
        smallest = l;
    if (r < heap->size && heap->array[r]->freq < heap->array[smallest]->freq)
        smallest = r;

    if (smallest != i) {
        swap(&heap->array[i], &heap->array[smallest]);
        minHeapify(heap, smallest);
    }
}

Node* extractMin(MinHeap* heap) {
    Node* temp = heap->array[0];
    heap->array[0] = heap->array[--heap->size];
    minHeapify(heap, 0);
    return temp;
}

void insertMinHeap(MinHeap* heap, Node* node) {
    int i = heap->size++;
    while (i && node->freq < heap->array[(i - 1) / 2]->freq) {
        heap->array[i] = heap->array[(i - 1) / 2];
        i = (i - 1) / 2;
    }
    heap->array[i] = node;
}

// ---------------- BUILD HUFFMAN TREE ----------------
Node* buildHuffmanTree(int freq[]) {
    MinHeap* heap = createMinHeap();

    for (int i = 0; i < 256; i++)
        if (freq[i])
            heap->array[heap->size++] = newNode((unsigned char)i, freq[i]);

    for (int i = (heap->size - 2) / 2; i >= 0; i--)
        minHeapify(heap, i);

    while (heap->size > 1) {
        Node* left = extractMin(heap);
        Node* right = extractMin(heap);
        Node* top = newNode(0, left->freq + right->freq);
        top->left = left;
        top->right = right;
        insertMinHeap(heap, top);
    }

    return extractMin(heap);
}

// ---------------- BIT READ ----------------
int readBit(FILE* in, unsigned char* buffer, int* bitPos) {
    if (*bitPos == 8) {
        if (fread(buffer, 1, 1, in) != 1)
            return -1;
        *bitPos = 0;
    }
    int bit = (*buffer >> (7 - *bitPos)) & 1;
    (*bitPos)++;
    return bit;
}

// ---------------- DECOMPRESSION ----------------
void decompressCLI(char* inputFile, char* outputFile) {
    FILE* in = fopen(inputFile, "rb");
    if (!in) {
        perror("Input file open failed");
        exit(1);
    }

    FILE* out = fopen(outputFile, "wb");
    if (!out) {
        perror("Output file open failed");
        exit(1);
    }

    int freq[256];
    fread(freq, sizeof(int), 256, in);

    Node* root = buildHuffmanTree(freq);
    Node* curr = root;

    unsigned char buffer = 0;
    int bitPos = 8;

    int totalChars = 0;
    for (int i = 0; i < 256; i++)
        totalChars += freq[i];

    while (totalChars > 0) {
        int bit = readBit(in, &buffer, &bitPos);
        if (bit == -1)
            break;

        curr = bit ? curr->right : curr->left;

        if (!curr->left && !curr->right) {
            fputc(curr->data, out);
            curr = root;
            totalChars--;
        }
    }

    fclose(in);
    fclose(out);
    printf("Decompression successful\n");
}

// ---------------- MAIN ----------------
int main(int argc, char* argv[]) {
    if (argc != 3) {
        printf("Usage: huffman_decompress <input.huff> <output_file>\n");
        return 1;
    }

    decompressCLI(argv[1], argv[2]);
    return 0;
}
