#include<stdlib.h>
#include<stdio.h>
#include<unistd.h>
#include<string.h>
#include<fcntl.h>
#include<omp.h>
#include<blake2.h>

char *strrev(char *str)
{
      char *p1, *p2;

      if (! str || ! *str)
            return str;
      for (p1 = str, p2 = str + strlen(str) - 1; p2 > p1; ++p1, --p2)
      {
            *p1 ^= *p2;
            *p2 ^= *p1;
            *p1 ^= *p2;
      }
      return str;
}

char *bin2hex(const unsigned char *bin, size_t len)
{
    char   *out;
    size_t  i;

    if (bin == NULL || len == 0)
        return NULL;

    out = malloc(len*2+1);
    for (i=0; i<len; i++) {
        out[i*2]   = "0123456789abcdef"[bin[i] >> 4];
        out[i*2+1] = "0123456789abcdef"[bin[i] & 0x0F];
    }
    out[len*2] = '\0';

    return out;
}

int hexchr2bin(const char hex, char *out)
{
    if (out == NULL)
        return 0;

    if (hex >= '0' && hex <= '9') {
        *out = hex - '0';
    } else if (hex >= 'A' && hex <= 'F') {
        *out = hex - 'A' + 10;
    } else if (hex >= 'a' && hex <= 'f') {
        *out = hex - 'a' + 10;
    } else {
        return 0;
    }

    return 1;
}

size_t hex2bin(const char *hex, unsigned char **out)
{
    size_t len;
    char   b1;
    char   b2;
    size_t i;

    if (hex == NULL || *hex == '\0' || out == NULL)
        return 0;

    len = strlen(hex);
    if (len % 2 != 0)
        return 0;
    len /= 2;

    *out = malloc(len);
    memset(*out, 'A', len);
    for (i=0; i<len; i++) {
        if (!hexchr2bin(hex[i*2], &b1) || !hexchr2bin(hex[i*2+1], &b2)) {
            return 0;
        }
        (*out)[i] = (b1 << 4) | b2;
    }
    return len;
}

int main(int argc, char *argv[]){
	unsigned char *str;
	int i=0;

    if(argc>= 2){
		hex2bin(argv[1], &str);

		#pragma omp parallel
		do{
			blake2b_state b2b;

			unsigned char *r_str, *b2b_b, *b2b_h;
			r_str = malloc(9);
			b2b_b = malloc(9);

			int fd = open("/dev/urandom", O_RDONLY);
			read(fd, r_str, 8);
			r_str[8]='\0';
			close(fd);
			for(int j=0;j<256;j++){
				r_str[7]=(r_str[7]+j)%256;

				blake2b_init(&b2b, 8);
				blake2b_update(&b2b, r_str, 8);
				blake2b_update(&b2b, str, 32);
				blake2b_final( &b2b, b2b_b, 8);
				b2b_b[8]='\0';

				strrev(b2b_b);

				b2b_h=bin2hex(b2b_b, 8);

				if(strcmp( b2b_h , "ffffffc000000000")>0){
					free(b2b_h);
					i++;
					strrev(r_str);
					printf("%s\n",bin2hex(r_str, 8));
				}
				free(b2b_h);
			}
			free(r_str);
			free(b2b_b);
		}while(i==0);

	}
    return 0;
}
