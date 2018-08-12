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
//~ #include<CL/opencl.h>
#include<CL/cl_intel.h>
#endif

#if defined(HAVE_OPENCL_CL_H) || defined(HAVE_CL_CL_H)
// this variable has the same definition as in raiblocks/rai/node/openclwork.cpp
#endif

void swapLong(uint64_t *X){
	uint64_t x = *X;
	x = (x & 0x00000000FFFFFFFF) << 32 | (x & 0xFFFFFFFF00000000) >> 32;
	x = (x & 0x0000FFFF0000FFFF) << 16 | (x & 0xFFFF0000FFFF0000) >> 16;
	x = (x & 0x00FF00FF00FF00FF) << 8  | (x & 0xFF00FF00FF00FF00) >> 8;
}

int hexchr2bin(char hex, uint8_t *out)
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

size_t hex2bin(char *hex, uint8_t **out)
{
    size_t len, i;
    uint8_t b1, b2;

    if (hex == NULL || *hex == '\0' || out == NULL)
        return 0;

    len = strlen(hex);
    if (len % 2 != 0)
        return 0;
    len /= 2;

    *out = malloc(len);
    memset(*out, 'A', len);
    for (i=0; i<len; i++) {
        if (!hexchr2bin(hex[i*2], &b1) || !hexchr2bin(hex[i*2+1], &b2)) return 0;
        (*out)[i] = (b1 << 4) | b2;
    }
    return len;
}

void pow_omp(uint8_t *str, char *work){
	int i=0;
	#pragma omp parallel
	while(i==0){
		uint64_t r_str=0, b2b_b=0;
		char b2b_h[17];
		blake2b_state b2b;
		int fd = open("/dev/urandom", O_RDONLY);

		read(fd, &r_str, 8);
		close(fd);
		for(int j=0;j<256&&i==0;j++){
			r_str+=j;

			blake2b_init(&b2b, 8);
			blake2b_update(&b2b, (uint8_t *)&r_str, 8);
			blake2b_update(&b2b, str, 32);
			blake2b_final(&b2b, (uint8_t *)&b2b_b, 8);

			swapLong(&b2b_b);
			sprintf(b2b_h, "%016lx", b2b_b);

			if(strcmp( b2b_h , "ffffffc000000000")>0){
				swapLong(&r_str);
				#pragma omp atomic write
				i=sprintf(work, "%016lx", r_str);
			}
		}
	}
}

char *pow_generate(char *hash){
	char *work=malloc(17);
	uint8_t *str;

	hex2bin(hash, &str);

#if defined(HAVE_OPENCL_CL_H) || defined(HAVE_CL_CL_H)
	cl_platform_id cpPlatform; // OpenCL platform
	cl_uint num;

	clGetPlatformIDs(1, &cpPlatform, &num);
	if(num>0){ //use omp if there is no gpu
		int i=0;
		char *opencl_program;
		size_t length;
		const size_t work_size = 10000; // find optimal value
		uint64_t workb=0, r_str=0;
		FILE *f;
		cl_mem d_rand, d_work, d_str;
		cl_device_id device_id; // device ID
		cl_program program;

		clGetDeviceIDs(cpPlatform, CL_DEVICE_TYPE_GPU, 1, &device_id, NULL);
		cl_context context = clCreateContext(0, 1, &device_id, NULL, NULL, NULL); // context
		cl_command_queue queue = clCreateCommandQueueWithProperties(context, device_id, 0, NULL); //queue

		f = fopen ("mpow.cl.bin", "rb");

		if(f){
			fseek (f, 0, SEEK_END);
			length = ftell (f);
			rewind(f);
			opencl_program = malloc(length);
			if (opencl_program) fread (opencl_program, 1, length, f);
			fclose (f);
			program = clCreateProgramWithBinary(context, 1, &device_id, &length, (const unsigned char **)&opencl_program, NULL, NULL); // program
		}
		else{
			f = fopen ("mpow.cl", "rb");

			fseek (f, 0, SEEK_END);
			length = ftell (f);
			rewind(f);
			opencl_program = malloc(length);
			if (opencl_program) fread (opencl_program, 1, length, f);
			fclose (f);

			program = clCreateProgramWithSource(context, 1, (const char **)&opencl_program, &length, NULL); // program
			clBuildProgram(program, 0, NULL, NULL, NULL, NULL); //build

			clGetProgramInfo(program, CL_PROGRAM_BINARY_SIZES, sizeof(size_t), &length, NULL);

			char **programBinary = malloc(sizeof(char *));
			programBinary[0] = malloc(length);

			clGetProgramInfo(program, CL_PROGRAM_BINARIES, 1, programBinary, NULL);

            f = fopen("mpow.cl.bin", "wb");
            fwrite(programBinary[0], 1, length, f);
            fclose(f);

			free(programBinary[0]);
			free(programBinary);
		}


		d_rand = clCreateBuffer(context, CL_MEM_READ_WRITE|CL_MEM_COPY_HOST_PTR, 8, &r_str, NULL);
		d_work = clCreateBuffer(context, CL_MEM_READ_WRITE|CL_MEM_COPY_HOST_PTR, 8, &workb, NULL);
		d_str = clCreateBuffer(context, CL_MEM_READ_WRITE|CL_MEM_COPY_HOST_PTR, 32, str, NULL);

		cl_kernel kernel = clCreateKernel(program, "raiblocks_work", &i); // kernel

		clSetKernelArg(kernel, 0, sizeof(d_rand), &d_rand);
		clSetKernelArg(kernel, 1, sizeof(d_work), &d_work);
		clSetKernelArg(kernel, 2, sizeof(d_str), &d_str);

		while(i==0){
			int fd = open("/dev/urandom", O_RDONLY);
			read(fd, &r_str, 8);
			close(fd);

			clEnqueueWriteBuffer(queue, d_rand, CL_TRUE, 0, 8, &r_str, 0, NULL, NULL );
			clEnqueueWriteBuffer(queue, d_str, CL_TRUE, 0, 32, str, 0, NULL, NULL );

			clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &work_size, NULL, 0, NULL, NULL);

			clEnqueueReadBuffer(queue, d_work, CL_TRUE, 0, 8, &workb, 0, NULL, NULL );

			clFinish(queue);

			if(workb!=0){
				swapLong(&workb);
				i=sprintf(work, "%016lx", workb);
			}
		}

		free(opencl_program);
		clReleaseMemObject(d_rand);
		clReleaseMemObject(d_work);
		clReleaseMemObject(d_str);
		clReleaseKernel(kernel);
		clReleaseProgram(program);
		clReleaseCommandQueue(queue);
		clReleaseContext(context);
	}
	else pow_omp(str, work);
#else
	pow_omp(str, work);
#endif
	return work;
}

int main(int argc, char *argv[]){
    if(argc> 1) printf("%s\n", pow_generate(argv[1]));
}
