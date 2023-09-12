#include <stdio.h>
#include <stdlib.h>

struct page_table_struct{
    int page_number;
};

struct all_pages_struct{
    int *pages_arr;
    int occupied;
    int size;
};

int check_next_occurance(struct all_pages_struct *all_pages, int start, int cur_page, int page_size){
    //Goes through all comming pages needed and returns the first occurance
    for(int i = start + 1; i < all_pages->occupied; i++){
        if(cur_page == all_pages->pages_arr[i] / page_size){
            return i - start;
        }
    }
    return (all_pages->occupied)-1;
}

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

    struct all_pages_struct *pages_struct; //Struct for reading all adresses so we can "see in to the future later"
    pages_struct = malloc(sizeof(struct all_pages_struct));
    pages_struct->pages_arr = malloc(10 * sizeof(int));
    pages_struct->occupied = 0; //Keeps track of loaded elements
    pages_struct->size = 10; //Current size of the array with all the data

    struct page_table_struct* page_table = malloc(no_phys_pages * sizeof(struct page_table_struct)); //Struct for the page table
    for(int i = 0; i < no_phys_pages; i++){
        page_table[i].page_number = -99; //Sets start value to avoid an unwanted page table hit from random grabage when initilizing the struct
    }

    if (file_ptr == NULL){
        printf("Error opening file\n");
        exit(1);
    }

    printf("\n*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*\n");
    printf("No physical pages = %d, page_table size = %d\n", no_phys_pages, page_size);
    printf("Reading memory trace from %s     ", argv[3]);

    int index = 0;

    while (fscanf(file_ptr, "%d", &buf) == 1) {
        memory_read++;
        index = pages_struct->occupied; //Index for the pages array
        if (pages_struct->occupied == pages_struct->size){
            //If the amount in the array equals its size
            pages_struct->size *= 2; //Dubble the size
            pages_struct->pages_arr = realloc(pages_struct->pages_arr, pages_struct->size * sizeof(int)); //Reallocates the array with the updated size
        }
        pages_struct->pages_arr[index] = buf; //Adds the loaded adress to the array
        pages_struct->occupied++;
    }
    int pages = pages_struct->occupied;
    int hit = 0;

    for (int i=0; i < pages; i++) {
        hit = 0;
        int cur_page = (int)((pages_struct->pages_arr[i])/page_size); //Gets the current page number
        for (int k=0; k < no_phys_pages; k++) {
            if (cur_page == page_table[k].page_number) {
                //If the current page is in the page table
                hit = 1;
            }
        }
        int furthest_index = 0;
        int furthest_occurrence = 0;
        if (hit == 0) {
            pagefaults += 1; //Increase the amount of page faults
            for (int l=0; l < no_phys_pages; l++) {
                if (check_next_occurance(pages_struct, i, page_table[l].page_number, page_size) > furthest_occurrence) {
                    //If the current page occurs further than the previous furthest
                    furthest_occurrence = check_next_occurance(pages_struct, i, page_table[l].page_number, page_size);
                    furthest_index = l; //Saves the index of the page which is the one used furthest away in time
                }
            }
            page_table[furthest_index].page_number = cur_page;
        }
    }

    printf("Read %d memory references\nResult: %d pagefaults\n", memory_read, pagefaults);
    printf("*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*\n");
    fclose(file_ptr); //Closes the file
    free(page_table);
    free(pages_struct);
    return 0;
}