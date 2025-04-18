#include "../lib/qdos.h"

int main()
{
    fdesc_t ifile= fopen("test.txt", 'r');
    if (ifile < 0)
    {
        print("Unable to open file&n");
        return -1;
    }
    char tstr[2]= {'\0','\0'};
    while (ftell(ifile))
    {
        tstr[0]= fread(ifile);
        print(tstr);
    }
}
