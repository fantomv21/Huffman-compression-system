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

// ---------------- BIT BUFFER ----------------
typedef struct {
    unsigned char buffer;
    int bitCount;
} BitBuffer;

// ---------------- FILE SIZE ----------------
long getFileSize(FILE *fp) {
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    rewind(fp);
    return size;
}

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
    Node* t = *a; *a = *b; *b = t;
}

void minHeapify(MinHeap* heap, int i) {
    int smallest = i;
    int l = 2*i + 1;
    int r = 2*i + 2;

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
    while (i && node->freq < heap->array[(i-1)/2]->freq) {
        heap->array[i] = heap->array[(i-1)/2];
        i = (i-1)/2;
    }
    heap->array[i] = node;
}

// ---------------- HUFFMAN TREE ----------------
Node* buildHuffmanTree(int freq[]) {
    MinHeap* heap = createMinHeap();

    for (int i = 0; i < 256; i++)
        if (freq[i])
            heap->array[heap->size++] = newNode(i, freq[i]);

    for (int i = (heap->size - 2) / 2; i >= 0; i--)
        minHeapify(heap, i);

    while (heap->size > 1) {
        Node* left = extractMin(heap);
        Node* right = extractMin(heap);
        Node* top = newNode('$', left->freq + right->freq);
        top->left = left;
        top->right = right;
        insertMinHeap(heap, top);
    }
    return extractMin(heap);
}

// ---------------- CODE GENERATION ----------------
void generateCodes(Node* root, int arr[], int top, char* codes[]) {
    if (root->left) {
        arr[top] = 0;
        generateCodes(root->left, arr, top + 1, codes);
    }
    if (root->right) {
        arr[top] = 1;
        generateCodes(root->right, arr, top + 1, codes);
    }
    if (!root->left && !root->right) {
        codes[root->data] = (char*)malloc(top + 1);
        for (int i = 0; i < top; i++)
            codes[root->data][i] = arr[i] + '0';
        codes[root->data][top] = '\0';
    }
}

// ---------------- BIT WRITE ----------------
void writeBit(FILE* out, BitBuffer* bb, int bit) {
    bb->buffer = (bb->buffer << 1) | bit;
    bb->bitCount++;

    if (bb->bitCount == 8) {
        fwrite(&bb->buffer, 1, 1, out);
        bb->buffer = 0;
        bb->bitCount = 0;
    }
}

// ---------------- COMPRESSION ----------------
void compressCLI(char *inFile, char *outFile) {
    FILE *in = fopen(inFile, "rb");
    FILE *out = fopen(outFile, "wb");
    if (!in || !out) {
        printf("File error\n");
        exit(1);
    }

    long originalSize = getFileSize(in);

    int freq[256] = {0};
    int c;
    while ((c = fgetc(in)) != EOF)
        freq[c]++;

    Node* root = buildHuffmanTree(freq);

    char* codes[256] = {0};
    int arr[MAX_TREE_HT];
    generateCodes(root, arr, 0, codes);

    fwrite(freq, sizeof(int), 256, out);

    rewind(in);
    BitBuffer bb = {0, 0};

    while ((c = fgetc(in)) != EOF) {
        char* code = codes[c];
        for (int i = 0; code[i]; i++)
            writeBit(out, &bb, code[i] - '0');
    }

    if (bb.bitCount) {
        bb.buffer <<= (8 - bb.bitCount);
        fwrite(&bb.buffer, 1, 1, out);
    }

    fclose(in);
    fclose(out);

    FILE *cf = fopen(outFile, "rb");
    long compressedSize = getFileSize(cf);
    fclose(cf);

    printf("Original size   : %ld bytes\n", originalSize);
    printf("Compressed size : %ld bytes\n", compressedSize);
    printf("Compression ratio : %.2f : 1\n",
           (double)originalSize / compressedSize);
}

// ---------------- MAIN ----------------
int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: huffman_compress <input_file> <output_file>\n");
        return 1;
    }
    compressCLI(argv[1], argv[2]);
    return 0;
}
