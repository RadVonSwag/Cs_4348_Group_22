#include<stdio.h>
#include<string.h>
#include<fcntl.h>
#include<errno.h>
#include<unistd.h>
#include<stdlib.h>
#include<time.h>

#define BLOCK_SIZE 1024
#define INODE_SIZE 64

typedef struct {
    int isize;
    int fsize;
    int nfree;
    unsigned int free[200];
    char flock;
    char ilock;
    char fmod;
    unsigned int time;
    unsigned short ninode;
    unsigned short inode[100];
} superblock_type;

typedef struct {
    unsigned short flags;
    unsigned short nlinks;
    unsigned int uid;
    unsigned int gid;
    unsigned int size0;
    unsigned int size1;
    unsigned int addr[9];
    unsigned int actime;
    unsigned int modtime;
} inode_type;

typedef struct { 
    unsigned int inode; 
    char filename[28];
} dir_type;  //32 Bytes long

superblock_type superBlock;
inode_type root;

int fd;

int open_fs(char *file_name){
    fd = open(file_name, O_RDWR | O_CREAT, 0600);

    if(fd == -1){
        return -1;
    }
    else{
        return fd;
    }
}

// Function to write inode
void inode_writer(int inum, inode_type inode){
   lseek(fd,2*BLOCK_SIZE+(inum-1)*INODE_SIZE,SEEK_SET); 
    write(fd,&inode,sizeof(inode));
}


int get_free_block(){
    if(superBlock.nfree == 0)
        return -1;
    return superBlock.free[superBlock.nfree--];
}

int add_free_block(unsigned int num){
    if(superBlock.nfree < 200){
        superBlock.free[superBlock.nfree++] = num;
        return 1;
    }
    return -1;
}

// Function to read inodes
inode_type inode_reader(int inum, inode_type inode){
   lseek(fd,2*BLOCK_SIZE+(inum-1)*INODE_SIZE,SEEK_SET); 
    read(fd, &inode, sizeof(inode));
    return inode;
}

// Function to write inode number after filling some fileds
void fill_an_inode_and_write(int inum){
inode_type root;
int i;

    root.flags |= 1 << 15; //Root is allocated
    root.flags |= 1 <<14; //It is a directory
    root.actime = time(NULL);
    root.modtime = time(NULL);

    root.size0 = 0;
    root.size1 = 2 * sizeof(dir_type);
    root.addr[0] = 2 + superBlock.free[superBlock.nfree--];
    for (i=1;i<9;i++) root.addr[i]=-1;//all other addr elements are null so setto -1
    inode_writer(inum, root);
}

void initfs(char *file_name , int n1, int n2){
    int fd = open_fs(file_name);

    if(fd != -1){
        superBlock.isize = n2;
        superBlock.fsize = n1;
        superBlock.ninode = 0;
        if((n1 - n2 - 2) >= 200){ // in the case that there is enough blocks to fill free[]
            superBlock.nfree = 199;
            int i , j = 0;
            for(i=199; i > 0; i--){
                superBlock.free[i] = 2 + n2 + j;
                j++; // this for loop gives the next available
                // free block to the free array, 
                // from free[199] down to free[1]
            }
        }
        else{
            superBlock.nfree = n1 - n2 - 2;
            int i , j = 0;
            for(i=superBlock.nfree; i > 0; i--){
                superBlock.free[i] = 2 + n2 + j;
                j++;
            }
        }

        //write superblock
        lseek(fd, BLOCK_SIZE, SEEK_SET);
        write(fd, &superBlock, 1);

        //init root inode
        inode_type inode1;
        fill_an_inode_and_write(1);
        inode1 = inode_reader(1,inode1);

        //write inodes
        lseek(fd, (2 * BLOCK_SIZE), SEEK_SET);
        for (int i = 0; i < n2; i++ ) {
            write(fd, &superBlock.inode[i], INODE_SIZE);
        } 

        // seeks to point after root and superblock and inodes
        lseek(fd, ((n2 * INODE_SIZE) + (2 * BLOCK_SIZE)), SEEK_SET); 
        write(fd, " ", 1);
    }
    else {
        printf("ERROR: File open failed.");
    }

    //close the file
    //fclose(fd); getting segmentation faults at the moment.

}

//printfs for debugging
void printfs() {
    printf("Superblock Info\n");
    printf("\tnumber of inodes: %d\n", superBlock.isize);
    printf("\tnumber of blocks: %d\n", superBlock.fsize);
    printf("\tsize of block: %d\n", BLOCK_SIZE);
    printf("\tnfree: %d", superBlock.nfree);
    //etc

    printf("\nInode Info\n...");

}

// q function to quit the program.
void q() {
    printf("\nClosing file system...\n");
    exit(0);
}


// The main function
int main(){
    initfs("Test_fs.txt", 500, 16);
    q();
}
