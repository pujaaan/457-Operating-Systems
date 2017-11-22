#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>
#if defined(__APPLE__)
#  define COMMON_DIGEST_FOR_OPENSSL
#  include <CommonCrypto/CommonDigest.h>
#  define SHA1 CC_SHA1
#else
#  include <openssl/md5.h>
#endif


//http://stackoverflow.com/questions/8236/how-do-you-determine-the-size-of-a-file-in-c
//http://stackoverflow.com/questions/1129499/how-to-get-the-size-of-a-dir-programatically-in-linux
//http://stackoverflow.com/questions/21618260/how-to-get-total-size-of-subdirectories-in-c
//http://stackoverflow.com/questions/8436841/how-to-recursively-list-directories-in-c-on-linux


// Calculates the md5
char *str2md5(const char *str, int length) {
    int n;
    MD5_CTX c;
    unsigned char digest[16];
    char *out = (char*)malloc(33);

    MD5_Init(&c);

    while (length > 0) {
        if (length > 512) {
            MD5_Update(&c, str, 512);
        } else {
            MD5_Update(&c, str, length);
        }
        length -= 512;
        str += 512;
    }

    MD5_Final(digest, &c);

    for (n = 0; n < 16; ++n) {
        snprintf(&(out[n*2]), 16*2, "%02x", (unsigned int)digest[n]);
    }

    return out;
}

// Calculates the size of the directory

long goDir(char *dirname)
{
    DIR *dir = opendir(dirname);
    if (dir == 0)
        return 0;

    struct dirent *dit;
    struct stat st;
    long size = 0;
    long total_size = 0;
    char filePath[NAME_MAX];

    while ((dit = readdir(dir)) != NULL)
    {
        if ( (strcmp(dit->d_name, ".") == 0) || (strcmp(dit->d_name, "..") == 0) )
            continue;

        sprintf(filePath, "%s/%s", dirname, dit->d_name);
        if (lstat(filePath, &st) != 0)
            continue;
        size = st.st_size;

        if (S_ISDIR(st.st_mode))
        {
            long dir_size = goDir(filePath) + size;
            total_size += dir_size;
        }
        else
        {
            total_size += size;
        }
    }
    return total_size;
}

// Scans directory
// Prints the directory and the type
// Calls the function which gives the size and the md5

void listdir(const char *name, int level)
{
	struct dirent **namelist;
	int n;
	n = scandir(name, &namelist, NULL, alphasort);
	struct stat st;
	
	int orderCount = 2;
	if (n < 0)
		return;
	else {
		while (n--) {

			if(n > 1){
				if (namelist[orderCount]->d_type == DT_DIR) {
					char path[1024];
					int len = snprintf(path, sizeof(path)-1, "%s/%s", name, namelist[orderCount]->d_name);
					path[len] = 0;

					if (strcmp(namelist[orderCount]->d_name, ".") == 0 || strcmp(namelist[orderCount]->d_name, "..") == 0)
						continue;
					for (int i = 0; i < level*3; i++){
						printf("-");
					}
					printf("| %s (directory - ", namelist[orderCount]->d_name);
					int size = goDir(path);
					char *md5Sum = str2md5(namelist[orderCount]->d_name, strlen(namelist[orderCount]->d_name));
					printf("%d %s) \n", size, md5Sum);
					orderCount++;
					listdir(path, level + 1);
					free(namelist[orderCount]);
				}
			else if (namelist[orderCount]->d_type == DT_LNK) {
				printf("%*s| %s (symbolic link - points to /?/? absolute path: /?/?)\n", level*3, "", namelist[orderCount]->d_name);
				
			}
			else{
				struct stat st;
				stat(namelist[orderCount], &st);
				int size = st.st_size;
				char *md5Sum = str2md5(namelist[orderCount]->d_name, strlen(namelist[orderCount]->d_name));
					printf("%*s| %s (regular file - %d - %s)\n", level*3, "", namelist[orderCount]->d_name, size, md5Sum);
				}
			}
			
		}
		free(namelist);

	}
} 

// Main file calling the function which prints the gls

int main(int argc, char *argv[])
{
  if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <Directory>\n", argv[0]);
        return 1;
    }
	listdir(argv[1], 0);
    return 0;

}

