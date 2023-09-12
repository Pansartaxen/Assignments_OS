#include <iostream>
#include "fs.h"
#include <string>
#include <vector>

int curr_dir_block = ROOT_BLOCK;

FS::FS()
{
    std::cout << "FS::FS()... Creating file system\n";
}

FS::~FS()
{
}

int
FS::format()
{
    std::cout << "FS::formatting..()\n";

    int nr_blocks = disk.get_no_blocks();
    // Marking root directory and block one EOF
    fat[0] = FAT_EOF;
    fat[1] = FAT_EOF;

    // Marking rest of blocks as free
    for (int i = 2; i < nr_blocks; i++)
    {
        fat[i] = FAT_FREE;
    }

    std::string name = "/";

    // Creating root directory block
    // IN PROGRESS
    dir_entry root_blk[BLOCK_SIZE];
    for(int i = 0; i < BLOCK_SIZE/sizeof(dir_entry); i++){
        root_blk[i].size = 0;
        root_blk[i].first_blk = 0;
        root_blk[i].type = 0;
        root_blk[i].access_rights = 0;
        strcpy(root_blk->file_name, name.c_str());
    }

    // Writing data to root directory and block one
    disk.write(ROOT_BLOCK, (uint8_t*)root_blk);
    disk.write(FAT_BLOCK, (uint8_t*)fat);

    // Saving current dir
    int curr_dir_block = ROOT_BLOCK;

    return 0;
}

int FS::create(std::string filepath)
{
    // check filename size
    if (filepath.length() > 55)
    {
        std::cout << "The filename is too long." << std::endl;
        return 1;
    }

    // extract filename from filepath
    std::string filename = filepath_to_filename(filepath);

    // remove filename from filepath
    erase_filename(&filepath);

    // find final block of filepath
    int final_block = find_final_block(curr_dir_block, filepath);

    // check if file already exists in current directory
    if (check_file_exists(filename, curr_dir_block))
    {
        std::cout << "A file with that name already exists." << std::endl;
        return 1;
    }

    // create directory entry
    dir_entry new_entry[BLOCK_SIZE];
    disk.read(final_block, (uint8_t*)new_entry);

    // find an empty directory block to write to
    int empty_block_id = empty_dirrectory_id(new_entry);
    dir_entry *empty_block = new_entry + empty_block_id;

    if (empty_block_id == -1)
    {
        std::cout << "No space left." << std::endl;
        return 1;
    }

    // get user input for file contents
    std::string user_input = "", line;
    int line_count = 0;

    // read lines of input from the user until an empty line is entered
    while (std::getline(std::cin, line))
    {
        if (line.empty())
            break;

        user_input += line;
    }

    // convert user input to a C-style string
    const char *data = user_input.c_str();

    // get size of user input in bytes
    int data_size = user_input.length() + 1;

    // initialize block pointers
    int first_block = -1, cur_block = -1, prev_block = -1;

    // write user input to disk, one block at a time
    while (data_size > 0)
    {
        // find a free block to write to
        cur_block = find_free_block();

        if (cur_block == -1)
        {
            std::cout << "ERROR: No free blocks left." << std::endl;
            return 1;
        }

        // write data to current block
        disk.write(cur_block, (uint8_t*)data);

        // update FAT to mark current block as end of file
        fat[cur_block] = FAT_EOF;

        // update data pointer and size
        data_size -= BLOCK_SIZE;
        data += BLOCK_SIZE;

        // set first block pointer if not already set
        if (first_block < 0)
            first_block = cur_block;

        // update previous block pointer to point to current block
        if (prev_block > 0)
            fat[prev_block] = cur_block;

        // set previous block pointer to current block
        prev_block = cur_block;
    }

    // fill in directory entry for new file
    strcpy(empty_block->file_name, filename.c_str());
    empty_block->size = (uint32_t)(user_input.length() + 1);
    empty_block->access_rights = READ | WRITE;
    empty_block->first_blk = first_block;
    empty_block->type = TYPE_FILE;

    // write directory and FAT to disk
    disk.write(final_block, (uint8_t*)new_entry);
    disk.write(FAT_BLOCK, (uint8_t*)fat);

    // return success
    return 0;
}


int FS::cat(std::string filepath)
{
    // Get filename and directory path
    std::string filename = filepath_to_filename(filepath);
    erase_filename(&filepath);

    // Find the final block of the directory path
    int final_block = find_final_block(curr_dir_block, filepath);
    
    // Find the index of the file in the directory
    int file_index = find_file_id(filename, final_block);

    // Return an error if the final block or file is not found
    if (file_index == -1)
    {
        std::cout << "ERROR: No such file or directory." << std::endl;
        return 1;
    }

    // Read the directory block
    dir_entry block[BLOCK_SIZE];
    disk.read(final_block, (uint8_t*)block);

    // Search for the file in the directory block
    for(int i = 0; i < BLOCK_SIZE / sizeof(dir_entry); i++){
        if((block[i].access_rights & READ) != 0 && std::string(block[i].file_name) == filename){
            file_index = i;
            break;
        }
    }

    // Get the directory entry for the file
    dir_entry file = block[file_index];

    // Return an error if the file is not actually a file
    if(file.type != TYPE_FILE){
        std::cout << "ERROR: Not a file." << std::endl;
        return 1;
    }

    // Return an error if the file does not have read access
    if((file.access_rights & READ) == 0){
        std::cout << "ERROR: No read access." << std::endl;
        return 1;
    }

    // Print the contents of the file
    int block_id = file.first_blk;
    char *current_block = (char*)block;

    while(block_id != FAT_EOF){
        disk.read(block_id, (uint8_t*)current_block);
        std::cout << current_block;
        block_id = fat[block_id];
    }
    std::cout << std::endl;

    return 0;
}


int
FS::ls()
{
    // Load current directory
    dir_entry current_block[BLOCK_SIZE];
    disk.read(curr_dir_block, (uint8_t*)current_block);

    std::cout << "name     type    access rights    size\n";
    
    for (int i = 0; i < BLOCK_SIZE/sizeof(dir_entry); i++){
        std::string print = "";

        // Read the next directory entry from the root block
        dir_entry entry = current_block[i];

        // If the entry is empty, we've reached the end of the directory
        if (entry.first_blk == 0)
            continue;

        // Print the name and type of the file or directory
        if (entry.type == TYPE_FILE)
        {
            std::cout << entry.file_name << "\t " << "file";
            std::cout << "\t " << ((entry.access_rights & READ) ? "r" : "-");
            std::cout << ((entry.access_rights & WRITE) ? "w" : "-");
            std::cout << ((entry.access_rights & EXECUTE) ? "x" : "-");
            std::cout << "\t\t  " << entry.size << std::endl;
        }
        else if (entry.type == TYPE_DIR)
        {
            std::cout << entry.file_name << "\t " << "dir" << "\t " << "-" << std::endl;

        }

    
    }

    return 0;
}

int FS::cp(std::string srcPath, std::string dstPath)
{
    // Extract the file name from the source path
    std::string fileNameFromSourcePath = filepath_to_filename(srcPath);

    // Remove the file name from the source path to get the directory
    erase_filename(&srcPath);

    // Find the block of the source file in the current directory
    int blkOfSrc = find_final_block(curr_dir_block, srcPath);

    // Find the file ID of the source file in the directory block
    int srcFileID = find_file_id(fileNameFromSourcePath, blkOfSrc);

    // If source file doesn't exist, display an error and return
    if (srcFileID == -1) {
        std::cout << "File \"" << fileNameFromSourcePath << "\" does not exist.\n";
        return 1;
    }

    // Read the source directory block and get the source file entry
    dir_entry sourceBlock[BLOCK_SIZE];
    disk.read(blkOfSrc, (uint8_t*)sourceBlock);
    dir_entry* srcFileEntry = sourceBlock + srcFileID;

    // If the source is a directory, display an error and return
    if (srcFileEntry->type == TYPE_DIR) {
        std::cout << "Cannot copy directory\n";
        return 1;
    }

    // Check if the user has permission to read the source file
    if ((srcFileEntry->access_rights & READ) == 0) {
        std::cout << "Invalid access rights, you do not have permission to read this file.\n";
        return 1;
    }

    // Initialize variables for the destination file
    std::string nameOfCopiedFile;
    int blkIDofDst = -1;

    // Check if the destination path is a directory
    if (dstPath.find("/") != std::string::npos) {
        // Find the block of the destination path
        blkIDofDst = find_final_block(curr_dir_block, dstPath);

        // If destination path is invalid, display an error and return
        if (blkIDofDst == -1) {
            std::cout << "Invalid path for destination file\n";
            return 1;
        }

        // Check if the destination directory is the same as the source directory
        if (blkIDofDst == blkOfSrc) {
            std::cout << "Destination sub-directory is the same directory as the source directory.\n";
            return 1;
        }

        // Set the name of the copied file
        nameOfCopiedFile = fileNameFromSourcePath;
    } else {
        // Check if the destination file already exists in the current directory
        int doesDstFileExist = find_file_id(dstPath, curr_dir_block);

        // If the destination file exists, handle accordingly
        if (doesDstFileExist != -1) {
            // Read the destination directory block and get the destination file entry
            dir_entry destBlock[BLOCK_SIZE];
            disk.read(curr_dir_block, (uint8_t*)destBlock);
            dir_entry* dstFileEntry = destBlock + doesDstFileExist;

            // If the destination is a file, display an error and return
            if (dstFileEntry->type == TYPE_FILE) {
                std::cout << "A file with name " << dstFileEntry->file_name << " already exists.\n";
                return 1;
            }

            // Set the name of the copied file and the destination block ID
            nameOfCopiedFile = fileNameFromSourcePath;
            blkIDofDst = dstFileEntry->first_blk;
        } else {
            // Set the name of the copied file and the destination block ID
            nameOfCopiedFile = dstPath;
            blkIDofDst = blkOfSrc;
        }
    }

    // Check if a file with the same name already exists in the destination directory
    if (find_file_id(nameOfCopiedFile, blkIDofDst) != -1) {
        std::cout << "File with name " << nameOfCopiedFile << " already exists in destination sub-directory, aborting\n";
        return 1;
    }

    // Read the destination directory block
    dir_entry destBlock[BLOCK_SIZE];
    disk.read(blkIDofDst, (uint8_t*)destBlock);

    // Find an empty entry in the destination directory
    int freeEntryID = empty_dirrectory_id(destBlock);
    if (freeEntryID == -1) {
        std::cout << "No free space in directory to copy file." << "\n";
        return 1;
    }

    // Get a pointer to the free entry in the destination directory
    dir_entry* dstEntry = destBlock + freeEntryID;

    // Copy file information from source to destination entry
    strcpy(dstEntry->file_name, nameOfCopiedFile.c_str());
    dstEntry->size = sourceBlock[srcFileID].size;
    dstEntry->type = sourceBlock[srcFileID].type;
    dstEntry->access_rights = sourceBlock[srcFileID].access_rights;

    // Initialize variables for block copying
    uint8_t bufferForBlock[BLOCK_SIZE];
    int srcBlockID = sourceBlock[srcFileID].first_blk;
    int dstBlockID = -1;

    // Copy data blocks from source to destination
    while (srcBlockID != FAT_EOF) {
        // Find a free block for copying
        int freeBlockID = find_free_block();

        // Update FAT for file allocation
        fat[freeBlockID] = FAT_EOF;
        if (dstBlockID == -1)
            dstEntry->first_blk = freeBlockID;
        else
            fat[dstBlockID] = freeBlockID;
        dstBlockID = freeBlockID;

        // Check for unaccounted-for errors in block IDs
        if (dstBlockID <= 1) {
            std::cout << "Unaccounted-for error: dstBlockID <= 1 (" << dstBlockID << "). This may indicate that there's no more space on the disk.\n";
            return 1;
        }

        // Read data from source block and write to destination block
        disk.read(srcBlockID, bufferForBlock);
        disk.write(dstBlockID, bufferForBlock);

        // Move to the next source block
        srcBlockID = fat[srcBlockID];
    }

    // Write updated destination directory block and FAT to disk
    disk.write(blkIDofDst, (uint8_t*)destBlock);
    disk.write(FAT_BLOCK, (uint8_t*)fat);

    // Return success status
    return 0;
}


int
FS::mv(std::string sourcepath, std::string destpath) {

    std::string src_filename = filepath_to_filename(sourcepath);
    int src_dir = find_final_block(curr_dir_block, sourcepath);

    // Make sure the source file exists
    int src_id = find_file_id(src_filename, src_dir);
    if(src_id == -1){
        std::cout << "File: " << src_filename << " does not exist.\n";
        return 1;
    }

    // Load the current directory
    dir_entry blk[BLOCK_SIZE];
    disk.read(src_dir, (uint8_t*)blk);
    dir_entry* src_file = blk + src_id;
    if(src_file->type == TYPE_DIR){
        std::cout << "Error: Source is directory\n";
        return 1;
    }

    int dest_id = find_file_id(destpath, curr_dir_block);
    if(destpath.find('/') == -1 && destpath != ".." && dest_id == -1) {

        if(destpath.length() >= 55){
            std::cout << "Filename too long. The name of a file can be at most be 56 characters long\n";
            return 1;
        }


        std::strcpy(src_file->file_name, destpath.c_str());
        disk.write(curr_dir_block, (uint8_t*)blk);

    } else {

        // Check if it's a valid path
        int dir_final_block = find_final_block(curr_dir_block, destpath);
        if(dir_final_block== -1){
            std::cout << "Invalid path for destination file\n";
            return 1;
        }

        // Load the block of the destination directory
        dir_entry n_blk[BLOCK_SIZE];
        disk.read(dir_final_block, (uint8_t*)n_blk);

        if(find_file_id(src_filename, dir_final_block) != -1){
            std::cout << "Sourcefile already exists in the given directory.\n";
            return 1;
        }

        // Find an empty dir_entry in destination sub-directory
        int empty_dir_entry = empty_dirrectory_id(n_blk);
        if(empty_dir_entry == -1){
            std::cout << "No free space for file in destination sub-directory\n";
            return 1;
        }

        // Get pointers to the source and destination file entries
        dir_entry *source_entry = blk + src_id;
        dir_entry *dest_entry = n_blk + empty_dir_entry;

        // Update the desination entry with the source entry data
        strcpy(dest_entry->file_name,  source_entry->file_name);
        dest_entry->size             = source_entry->size;
        dest_entry->first_blk        = source_entry->first_blk;
        dest_entry->type             = source_entry->type;
        dest_entry->access_rights    = source_entry->access_rights;

        // Set source entry as empty
        source_entry->size = 0;
        source_entry->first_blk = 0;

        // Write new data to disk
        disk.write(curr_dir_block, (uint8_t*)blk);
        disk.write(dir_final_block, (uint8_t*)n_blk);
    }

    return 0;
}

int FS::rm(std::string filepath)
{
    // Extract the filename from the filepath
    std::string filename = filepath_to_filename(filepath);

    // Remove the filename from the filepath to get the directory
    erase_filename(&filepath);

    // Find the block of the source directory
    int src_dir = find_final_block(ROOT_BLOCK, filepath);

    // If source directory doesn't exist, display an error and return
    if (src_dir == -1) {
        std::cout << "Error: Directory " << filepath << " not found" << std::endl;
        return 1;
    }

    // Find the file ID of the file in the source directory
    int src_file_id = find_file_id(filename, src_dir);

    // If source file doesn't exist, display an error and return
    if (src_file_id == -1) {
        std::cout << "Error: File " << filename << " not found" << std::endl;
        return 1;
    }

    // Read the source directory block and get the source file entry
    dir_entry src[BLOCK_SIZE];
    disk.read(src_dir, (uint8_t*)src);
    dir_entry* src_file = src + src_file_id;

    // Initialize a null name for clearing entry
    char null_name[56] = {0};

    // Check if the source file is a regular file
    if (src_file->type == TYPE_FILE) {
        int next_index = src_file->first_blk, index = 0;

        // Clear data blocks and update FAT for the file
        while (next_index != FAT_EOF) {
            index = next_index;
            next_index = fat[next_index];
            fat[index] = FAT_FREE;
        }

        // Reset file attributes
        src_file->first_blk = 0;
        src_file->size = 0;
        strcpy(src_file->file_name, "");
    } else {
        // Check if the source directory is empty
        dir_entry dir_entry[BLOCK_SIZE];
        disk.read(src_file->first_blk, (uint8_t*)dir_entry);

        for (int i = 1; i < BLOCK_SIZE / sizeof(dir_entry); i++) {
            if (dir_entry[i].size != FAT_FREE || dir_entry[i].first_blk != 0) {
                std::cout << "Error: Directory not empty" << std::endl;
                return 1;
            }
        }

        // Update FAT and reset directory attributes
        fat[src_file->first_blk] = FAT_FREE;
        src_file->first_blk = 0;
        src_file->size = 0;
        src_file->access_rights = 0;
    }

    // Write updated source directory entry and FAT to disk
    disk.write(src_dir, (uint8_t*)src_file);
    disk.write(FAT_BLOCK, (uint8_t*)fat);

    // Return success status
    return 0;
}


int
FS::append(std::string filepath1, std::string filepath2)
{
    // Check if the source file exists
    int src_final_block = find_final_block(ROOT_BLOCK, filepath1);
    if (src_final_block == -1) {
        std::cout << "Error: Source file not found" << std::endl;
        return -1;
    }

    // Read source block
    dir_entry src[BLOCK_SIZE];
    disk.read(src_final_block, (uint8_t*)src);
    std::string src_filename = filepath_to_filename(filepath1);
    int src_file_id = find_file_id(src_filename, src_final_block);
    dir_entry* src_file = src + src_file_id;

    // Check type
    if (src_file->type == TYPE_DIR) {
        std::cout << "Error: Can't append type directory" << std::endl;
        return -1;
    }

    // Check if the destination file exists
    int dest_final_block = find_final_block(ROOT_BLOCK, filepath2);
    if (dest_final_block == -1) {
        std::cout << "Error: Destination file not found" << std::endl;
        return -1;
    }

    // Read destination block
    dir_entry dest[BLOCK_SIZE];
    disk.read(dest_final_block, (uint8_t*)dest);
    std::string dest_filename = filepath_to_filename(filepath2);
    int dest_file_id = find_file_id(dest_filename, dest_final_block);
    dir_entry* dest_file = dest + dest_file_id;

    // Check type
    if (dest_file->type == TYPE_DIR) {
        std::cout << "Error: Can't append to type directory" << std::endl;
        return -1;
    }

    // Check access rights
    if((src_file->access_rights & READ) == 0){
        std::cout << "Invalid access rights, you do not have permission to read the source file" << "\n";
        return 1;
    }

    if((dest_file->access_rights & WRITE) == 0){
        std::cout << "Invalid access rights, you do not have permission to write to the destination file " << "\n";
        return 1;
    }
    

    // Find the last block of the destination file
    int src_blk = src_file->first_blk;
    int curr_blk = dest_file->first_blk;
    while (fat[curr_blk] != FAT_EOF) {
        curr_blk = fat[curr_blk];
    }
    
    uint8_t block[BLOCK_SIZE*2];
    int blocks_needed = src_file->size + (dest_file->size % BLOCK_SIZE);
    int next_blk;
    int block_pos = 0;

    disk.read(curr_blk, block);
    block_pos = (dest_file->size % BLOCK_SIZE);
    block[block_pos - 1] = '\n';

    disk.read(src_blk, block+block_pos);

    if(blocks_needed > BLOCK_SIZE)
    {
        block_pos += BLOCK_SIZE;
    } 
    else
    {
        block_pos += (src_file->size % BLOCK_SIZE);
    }


    // Read and write the data blocks of the source file to the end of the destination file
    for (int i = 0; i < blocks_needed; i++) {
        if (i < blocks_needed - 1) {
                disk.write(curr_blk, block);
                fat[curr_blk] = FAT_EOF;
                for(int i = 0; i < BLOCK_SIZE; i++)
                {
                    block[i] = block[BLOCK_SIZE + i];
                }
                block_pos -= BLOCK_SIZE;
                blocks_needed -= BLOCK_SIZE;
                src_blk = fat[src_blk];
                if(src_blk != FAT_EOF)
                {
                    disk.read(src_blk, block + block_pos);
                    if(fat[src_blk == FAT_EOF])
                    {
                        block_pos += (src_file->size%BLOCK_SIZE);
                    }
                    else 
                    {
                        block_pos += BLOCK_SIZE;
                    }
                }

                next_blk = find_free_block();
                fat[curr_blk] = next_blk;
                curr_blk = next_blk;
        } else {
           disk.write(curr_blk, block);
           break;
        }
    }
    fat[curr_blk] = FAT_EOF;

    // Update destination file size
    dest_file->size += src_file->size;

    // Write destination block back to disk
    disk.write(dest_final_block, (uint8_t*)dest);

    return 0;

}
int FS::mkdir(std::string dirpath)
{
    // Extract the directory name from the provided path
    std::string name = filepath_to_filename(dirpath);

    // Remove parent directories if path contains ".."
    for (int i = 0; i < dirpath.size() - 1; i++) {
        if (dirpath[i] == '.' && dirpath[i+1] == '.') {
            int j = i - 2;
            while (j >= 0 && dirpath[j] != '/') {
                j--;
            }
            dirpath.erase(j, i + 2 - j);
            i = j;
        }
    }

    // Erase the filename from the path to get the parent directory's path
    erase_filename(&dirpath);

    // Check if a file or directory with the same name already exists
    if (check_file_exists(name, curr_dir_block)) {
        std::cout << "Error: File or directory '" << name << "' already exists." << std::endl;
        return -1;
    }

    // Find the final block of the parent directory
    int final_block = find_final_block(curr_dir_block, dirpath);
    if (final_block == -1) {
        std::cout << "Error: Directory not found" << std::endl;
        return -1;
    }

    // Find a free block for the new directory
    int new_block = find_free_block();
    if (new_block == -1) {
        std::cout << "Error: No free blocks available" << std::endl;
        return -1;
    }

    // Read the parent directory's block
    dir_entry dir[BLOCK_SIZE];
    disk.read(final_block, (uint8_t*)dir);

    // Find an empty directory entry within the parent directory's block
    int dir_id = empty_dirrectory_id(dir);
    if (dir_id == -1) {
        std::cout << "Error: No memory left" << std::endl;
        return -1;
    }

    // Create a new directory entry within the parent directory
    dir_entry *new_dir = dir + dir_id;
    new_dir->type = TYPE_DIR;
    new_dir->size = 1;
    new_dir->first_blk = new_block;
    new_dir->access_rights = WRITE | READ | EXECUTE;
    strcpy(new_dir->file_name, name.c_str());

    // Initialize the new directory's block with empty entries
    dir_entry new_dir_entry[BLOCK_SIZE];
    disk.read(new_block, (uint8_t*)new_dir_entry);
    for (int i = 0; i < BLOCK_SIZE / sizeof(dir_entry); i++) {
        new_dir_entry[i].first_blk = 0;
        new_dir_entry[i].size = 0;
    }

    // Initialize the ".." entry within the new directory
    dir_entry* root_dir = new_dir_entry;
    root_dir->type = TYPE_DIR;
    root_dir->size = 1;
    root_dir->first_blk = final_block;
    root_dir->access_rights = WRITE | READ | EXECUTE;
    root_dir->file_name[0] = '.';
    root_dir->file_name[1] = '.';
    root_dir->file_name[2] = '\0';

    // Mark the new block as the end of the file allocation table
    fat[new_block] = FAT_EOF;

    // Write changes back to disk for parent directory, FAT, and new directory
    disk.write(final_block, (uint8_t*)dir);
    disk.write(FAT_BLOCK, (uint8_t*)fat);
    disk.write(new_block, (uint8_t*)new_dir_entry);

    return 0;
}

int FS::cd(std::string dirpath)
{
    // Declare an array of dir_entry objects named block, with a size of BLOCK_SIZE
    dir_entry block[BLOCK_SIZE];

    // Read the contents of the current directory block from the disk into the block array
    disk.read(curr_dir_block, (uint8_t*)block);

    // Find the final block corresponding to the given dirpath within the current directory
    int final_block = find_final_block(curr_dir_block, dirpath);

    // If the final_block is -1, the given dirpath is not a directory
    if(final_block == -1){
        std::cout << "Path: " << dirpath << " is not a directory.\n";
        return 1;
    }

    int src_id = find_file_id(filepath_to_filename(dirpath), final_block);

    // If src_id is not -1, the dirpath is a file, not a directory
    if(src_id != -1){
        std::cout << "\"" <<dirpath << "\" is a file.\n";
        return 1;
    }

    // Update the current directory block to the final_block
    curr_dir_block = final_block;

    // Return 0 to indicate successful change of directory
    return 0;
}


int 
FS::pwd() {

    std::string path = "";
    int currentBlock = curr_dir_block;

    // check if at root directory
    if (currentBlock == ROOT_BLOCK) {
        std::cout << "/" << std::endl;
        return 0;
    }

    dir_entry currentDir[BLOCK_SIZE];
    do {
        // read current directory block
        disk.read(currentBlock, (uint8_t*)currentDir);

        // read parent directory block
        dir_entry parent = currentDir[0];
        dir_entry parentDir[BLOCK_SIZE];
        disk.read(parent.first_blk, (uint8_t*)parentDir);

        // search for current directory in parent
        for (int i = 0; i < BLOCK_SIZE / sizeof(dir_entry); i++) {
            if (!visible(parentDir + i))
                continue;

            if (parentDir[i].first_blk == currentBlock) {
                // insert directory name at the start of path
                path.insert(0, parentDir[i].file_name);
            }
        }

        // update current block to parent block
        path.insert(0, "/");
        currentBlock = parent.first_blk;
    } while (currentBlock != ROOT_BLOCK);

    std::cout << path << std::endl;
    return 0;

}

int FS::chmod(std::string accessrights, std::string filepath)
{
    // Extract the filename from the given filepath
    std::string src_filename = filepath_to_filename(filepath);

    // Remove the filename to get the path to the parent directory
    erase_filename(&filepath);
    
    // Find the block of the parent directory of the target file
    int file_directory_block = find_final_block(curr_dir_block, filepath);

    // Ensure that the target destination file exists
    int file_index = find_file_id(src_filename, file_directory_block);
    if (file_index == -1) {
        std::cout << "Error: " << src_filename << " does not exist." << std::endl;
        return 1;
    }

    // Convert the new access rights string to an integer
    int new_access_rights = std::stoi(accessrights);

    // Validate the new access rights value
    if (new_access_rights < 0 || new_access_rights > 7) {
        std::cout << "Access rights are not valid" << std::endl;
        return 1;
    }

    // Read the parent directory's block
    dir_entry blk[BLOCK_SIZE];
    disk.read(file_directory_block, (uint8_t*)blk);

    // Locate the entry for the target file within the parent directory
    dir_entry* file_entry = blk + file_index;

    // Update the access rights of the target file
    file_entry->access_rights = new_access_rights;

    // Write the updated directory block back to disk
    disk.write(file_directory_block, (uint8_t*)blk);

    return 0;
}


std::string
FS::filepath_to_filename(std::string filepath)
{
    std::string filename = filepath;
    int i = filename.rfind('/');
    if(i != -1){
        filename.erase(0, i+1);
    }
    return filename;
}

void
FS::erase_filename(std::string *filepath)
{
    int index_last_slash = filepath->rfind('/');

    if(index_last_slash != -1) {
        filepath->erase(index_last_slash, filepath->length()); 
        
        if(filepath->empty())
            filepath->insert(0, "/");
    } else {
        filepath->erase(0, filepath->length());
    }
    
}

int FS::find_final_block(int curr_dir, std::string filepath)
{
    // Check if the filepath corresponds to the root directory
    if (filepath == "/")
        return ROOT_BLOCK;

    // If filepath is empty, return the current directory block
    if (filepath.empty())
        return curr_dir;

    // If filepath starts with a '/', remove it and set curr_dir to the root directory
    if (filepath[0] == '/')
    {
        filepath.erase(0, 1);
        curr_dir = ROOT_BLOCK;
    }

    // Buffer to store the directory name being processed
    std::string dir_buffer;

    // Loop until the entire filepath is processed
    while (filepath.length() > 0)
    {
        // Find the index of the next '/' character in filepath
        int slash_index = filepath.find('/');

        // Extract the directory name from filepath based on the slash index
        if (slash_index != -1)
        {
            dir_buffer = filepath.substr(0, slash_index);
            filepath = "";
        }
        else
        {
            dir_buffer = filepath;
            filepath.erase(0, filepath.length());
        }

        // Declare an array of dir_entry objects named dir to hold directory contents
        dir_entry dir[BLOCK_SIZE];

        // Read the contents of the current directory block into the dir array
        disk.read(curr_dir, (uint8_t*)dir);

        // Flag to track whether the next directory was found
        int next_found = 0;

        // Loop through the entries in the dir array to find the next directory
        for (int i = 0; i < BLOCK_SIZE / sizeof(dir_entry); i++)
        {
            // Skip invisible entries (neither files nor non-empty directories)
            if (!visible(dir + i))
                continue;

            // Check if the directory name in dir_buffer matches the current entry
            if (dir_buffer == dir[i].file_name)
            {
                // If the entry is a directory, update curr_dir to its first block
                if (dir[i].type == TYPE_DIR)
                    curr_dir = dir[i].first_blk;

                // Mark that the next directory was found
                next_found = 1;
                break;
            }
        }

        // If the next directory was not found, return -1 to indicate failure
        if (!next_found)
            return -1;
    }

    // Return the final block corresponding to the given filepath
    return curr_dir;
}


bool
FS::check_file_exists(std::string filename, int dir_block)
{
    // Read the directory block from the disk
    dir_entry buff[BLOCK_SIZE];
    disk.read(dir_block, (uint8_t*)buff);

    // Iterate through the entries in the directory
    for (int i = 0; i < BLOCK_SIZE; i += sizeof(dir_entry))
    {
        // Read the next directory entry from the block
        dir_entry entry = buff[i];

        // If the entry is empty, we've reached the end of the directory
        if (entry.first_blk == 0)
        {
            break;
        }

        // If the file name matches the entry name, return true
        if (filename == entry.file_name)
        {
            return true;
        }
        // If the entry is a subdirectory, search the subdirectory
        else if (entry.type == TYPE_DIR)
        {
            if (check_file_exists(filename, entry.first_blk))
            {
                return true;
            }
        }
    }

    // If the file was not found, return false
    return false;
}

int
FS::find_free_block()
{
    for(int i = 2; i < disk.get_no_blocks(); i++)
        if(fat[i] == FAT_FREE)
            return i;

    return -1;
}

int
FS::empty_dirrectory_id(dir_entry* block)
{
    // go through file and find first empty block
    for(int i = 0; i < BLOCK_SIZE / sizeof(dir_entry); i++)
    {
        if(!visible(block + i))
            return i;
    }
    return -1;
}

std::string 
FS::filepath_to_dirpath(std::string filepath) {
    size_t last_slash_pos = filepath.find_last_of("/");
    if (last_slash_pos == std::string::npos) {
        return "/";
    }
    return filepath.substr(0, last_slash_pos);
}

int
FS::find_file_id(std::string filename, int dir_block)
{
    if (filename.empty())
        return -1;
    
    if(filename.at(0) == '/'){
        filename = filename.erase(0, 1);
        return find_file_id(filename, ROOT_BLOCK);
    }

    // Read the directory block from the disk
    dir_entry buff[BLOCK_SIZE];
    disk.read(dir_block, (uint8_t*)buff);

    for (int i = 0; i < BLOCK_SIZE / sizeof(dir_entry); i++) {
        if(visible(buff + i) && std::string(buff[i].file_name) == filename)
            return i;
    }

    // If the file was not found, return false
    return -1;
}

bool
FS::visible(dir_entry* file)
{
    return  (file->type == TYPE_FILE && file->size >  0) ||
            (file->type == TYPE_DIR  && file->size != 0);
}
