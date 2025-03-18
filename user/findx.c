#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

int my_strlen(const char *s) {
    int i = 0;
    while(s[i])
        i++;
    return i;
}

int match(const char *name, const char *pattern) {
    int plen = my_strlen(pattern);
    if(plen == 0)
        return 0;
    if(pattern[0]=='*' && pattern[plen-1]=='*'){
        if(plen <= 2)
            return 1;
        int sublen = plen - 2;
        int name_len = my_strlen(name);
        for(int i = 0; i <= name_len - sublen; i++){
            int j;
            for(j = 0; j < sublen; j++){
                if(name[i+j] != pattern[1+j])
                    break;
            }
            if(j == sublen)
                return 1;
        }
        return 0;
    }
    else if(pattern[0]=='*'){
        if(plen == 1)
            return 1;
        int sublen = plen - 1;
        int name_len = my_strlen(name);
        if(name_len < sublen)
            return 0;
        for(int i = 0; i < sublen; i++){
            if(name[name_len - sublen + i] != pattern[1+i])
                return 0;
        }
        return 1;
    }
    else if(pattern[plen-1]=='*'){
        if(plen == 1)
            return 1;
        int sublen = plen - 1;
        for(int i = 0; i < sublen; i++){
            if(name[i] != pattern[i])
                return 0;
        }
        return 1;
    }
    else {
        return (strcmp(name, pattern)==0);
    }
}