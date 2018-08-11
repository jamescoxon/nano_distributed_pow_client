#include<stdlib.h>
#include<stdio.h>
#include<unistd.h>
#include<string.h>
#include<fcntl.h>
#include<omp.h>
#include<blake2.h>

#ifdef HAVE_OPENCL_CL_H
#include<OpenCL/opencl.h>
#elif HAVE_CL_CL_H
#include<CL/opencl.h>
//~ #include<CL/cl_intel.h>
#endif


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

unsigned char* work_value(unsigned char *r_str, unsigned char *str){
	unsigned char *b2b_b = malloc(9);;
	b2b_b[8]='\0';
	blake2b_state b2b;
	blake2b_init(&b2b, 8);
	blake2b_update(&b2b, r_str, 8);
	blake2b_update(&b2b, str, 32);
	blake2b_final( &b2b, b2b_b, 8);

	strrev(b2b_b);

	return b2b_b;
}

char *pow_omp(char *str){
	unsigned char *work;
	int i=0;
	#pragma omp parallel
	while(i==0){
		unsigned char *r_str = malloc(9), *b2b_b, *b2b_h;
		int fd = open("/dev/urandom", O_RDONLY);
		blake2b_state b2b;

		r_str[8]='\0';

		read(fd, r_str, 8);
		close(fd);
		for(int j=0;j<256&&i==0;j++){
			r_str[7]=(r_str[7]+j)%256;
			b2b_b=work_value(r_str, str);
			b2b_h=bin2hex(b2b_b, 8);

			if(strcmp( b2b_h , "ffffffc000000000")>0){
				strrev(r_str);
				#pragma omp atomic write
				work=bin2hex(r_str, 8);
				i++;
			}
			free(b2b_b);
			free(b2b_h);
		}
		free(r_str);
	}
    return work;
}

const char *pow_generate(char *hash){
	unsigned char *str, *work;

	hex2bin(hash, &str);

	#if defined(HAVE_OPENCL_CL_H) || defined(HAVE_CL_CL_H)
		cl_mem d_rand, d_work, d_str;
		cl_platform_id cpPlatform;        // OpenCL platform
		cl_uint num;
		int err=0, i=0;

		clGetPlatformIDs(1, &cpPlatform, &num);
		if(num>0){ //use omp if there is no gpu
			char *opencl_program, *workb=malloc(9), *r_str = malloc(9);
			const size_t work_size = 256; // find optimal value
			size_t length;

			for(i=0;i<9;i++) workb[i]='\0';
			r_str[8]='\0';
			i=0;

			FILE * f = fopen ("mpow.cl", "rb");

			if(f){
				fseek (f, 0, SEEK_END);
				length = ftell (f);
				fseek (f, 0, SEEK_SET);
				opencl_program = malloc(length);
				if (opencl_program) fread (opencl_program, 1, length, f);
				fclose (f);
			}

			cl_device_id device_id; // device ID
			err = clGetDeviceIDs(cpPlatform, CL_DEVICE_TYPE_GPU, 1, &device_id, NULL);
			cl_context context = clCreateContext(0, 1, &device_id, NULL, NULL, &err); // context
			cl_command_queue queue = clCreateCommandQueueWithProperties(context, device_id, 0, &err); //queue

			cl_program program = clCreateProgramWithSource(context, 1, &opencl_program, &length, &err); // program

			d_rand = clCreateBuffer(context, CL_MEM_READ_WRITE|CL_MEM_COPY_HOST_PTR, 8, r_str, &err);
			d_work = clCreateBuffer(context, CL_MEM_READ_WRITE|CL_MEM_COPY_HOST_PTR, 8, workb, &err);
			d_str = clCreateBuffer(context, CL_MEM_READ_WRITE|CL_MEM_COPY_HOST_PTR, 32, str, &err);

			clBuildProgram(program, 0, NULL, NULL, NULL, NULL); //build

			cl_kernel kernel = clCreateKernel(program, "raiblocks_work", &err); // kernel

			err=clSetKernelArg(kernel, 0, sizeof(d_rand), &d_rand);
			err=clSetKernelArg(kernel, 1, sizeof(d_work), &d_work);
			err=clSetKernelArg(kernel, 2, sizeof(d_str), &d_str);

			while(i==0){
				int fd = open("/dev/urandom", O_RDONLY);
				read(fd, r_str, 8);
				close(fd);

				err=clEnqueueWriteBuffer(queue, d_rand, CL_TRUE, 0, 8, r_str, 0, NULL, NULL );
				err=clEnqueueWriteBuffer(queue, d_str, CL_TRUE, 0, 32, str, 0, NULL, NULL );

				err=clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &work_size, &work_size, 0, NULL, NULL);

				err=clEnqueueReadBuffer(queue, d_work, CL_TRUE, 0, 8, workb, 0, NULL, NULL );

				err=clFinish(queue);

				if(strcmp( workb , "\0")>0){
					strrev(workb);
					work=bin2hex(workb,8);
					i++;
				}
			}

			clReleaseMemObject(d_rand);
			clReleaseMemObject(d_work);
			clReleaseMemObject(d_str);
			clReleaseKernel(kernel);
			clReleaseProgram(program);
			clReleaseCommandQueue(queue);
			clReleaseContext(context);
		}
		else work=pow_omp(str);
	#else
		work=pow_omp(str);
	#endif
	return work;
}

int main(int argc, char *argv[]){
    if(argc> 1) printf("%s\n", pow_generate(argv[1]));
}
