#include "../lib/qdos.h"

int main()
{
    fdesc_t ifile= fopen("counter.txt", 'w');
    if (ifile < 0)
    {
        print("Unable to open file&n");
        return -1;
    }

    char tstr[2]= {'\0','\0'};
    fseek(ifile, 0);
    print("Counter is at ");
    tstr[0]= fread(ifile);
    tstr[0]++;
    print(tstr);
    print("&n");
    fseek(ifile, 0);
    fwrite(ifile, tstr[0]);
    fwrite(ifile, '\0');
    flushfs();

    fclose(ifile);
}
