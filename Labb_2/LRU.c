#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]){
    if (argc < 4){
        printf("Too few arguments\n");
        exit(1);
    }
    //atoi converts string to int
    int no_phys_pages = atoi(argv[1]); //FIrst argument: number of pages
    int page_size = atoi(argv[2]); //Second argument: page size
    FILE *file_ptr = fopen(argv[3],"r");  //Third argument: name of the file to read from
    int pagefaults = 0; //Counter for amount of page foults
    int memory_read = 0; //Counter for amount of times memory is read
    int buf; //Buffer for reading in to
    int page_miss;
    int cur_page;
    int page_table[no_phys_pages]; //Array for the page table

    if (file_ptr == NULL){
        printf("Error opening file\n");
        exit(1);
    }

    printf("\n*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*\n");
    printf("No physical pages = %d, page size = %d\n", no_phys_pages, page_size);
    printf("Reading memory trace from %s     ", argv[3]);

    while (fscanf(file_ptr, "%d", &buf) == 1) {
        memory_read++;
        page_miss = 1;
        cur_page = (int)(buf/page_size)*page_size;

        for (int i=0; i < no_phys_pages; i++) {
            if (page_table[i] == cur_page) { //If the page is in the page table
                page_miss = 0;
                int temp = page_table[i];
                //Shift all pages to the left
                for (int j = i; j < no_phys_pages - 1; j++) {
                    page_table[j] = page_table[j + 1];
                }
                page_table[no_phys_pages - 1] = temp; //Put the page at the end
                break;
            }
        }
        if (page_miss) {
            pagefaults = pagefaults + 1; //Increase counter of page misses
            //Shift all pages to the left and writes over the first element since it is the LRU
            for (int j = 0; j < no_phys_pages - 1; j++) {
                page_table[j] = page_table[j + 1];
            }
            page_table[no_phys_pages - 1] = cur_page; //Puts the current page at the end
        }
    }

    printf("Read %d memory references\nResult: %d pagefaults\n", memory_read, pagefaults);
    printf("*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*\n");
    fclose(file_ptr);
    return 0;
}