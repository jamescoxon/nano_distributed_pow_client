#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef HAVE_CL_CL_H
#include <CL/cl.h>
#elif HAVE_OPENCL_OPENCL_H
#include <OpenCL/opencl.h>
#else
#include <blake2.h>
#include <omp.h>
#endif

#define WORK_SIZE 1024 * 1024

static uint64_t s[16];
static int p;

uint64_t xorshift1024star(void) {  // raiblocks/rai/node/xorshift.hpp
  const uint64_t s0 = s[p++];
  uint64_t s1 = s[p &= 15];
  s1 ^= s1 << 31;         // a
  s1 ^= s1 >> 11;         // b
  s1 ^= s0 ^ (s0 >> 30);  // c
  s[p] = s1;
  return s1 * (uint64_t)1181783497276652981;
}

void swapLong(uint64_t *X) {
  uint64_t x = *X;
  x = (x & 0x00000000FFFFFFFF) << 32 | (x & 0xFFFFFFFF00000000) >> 32;
  x = (x & 0x0000FFFF0000FFFF) << 16 | (x & 0xFFFF0000FFFF0000) >> 16;
  x = (x & 0x00FF00FF00FF00FF) << 8 | (x & 0xFF00FF00FF00FF00) >> 8;
}

int hexchr2bin(char hex, uint8_t *out) {
  if (out == NULL) return 0;

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

size_t hex2bin(char *hex, uint8_t **out) {
  size_t len, i;
  uint8_t b1, b2;

  if (hex == NULL || *hex == '\0' || out == NULL) return 0;

  len = strlen(hex);
  if (len % 2 != 0) return 0;
  len /= 2;

  *out = malloc(len);
  memset(*out, 'A', len);
  for (i = 0; i < len; i++) {
    if (!hexchr2bin(hex[i * 2], &b1) || !hexchr2bin(hex[i * 2 + 1], &b2))
      return 0;
    (*out)[i] = (b1 << 4) | b2;
  }
  return len;
}

#if !defined(HAVE_OPENCL_OPENCL_H) && !defined(HAVE_CL_CL_H)
void pow_omp(uint8_t *str, char *work) {
  int i = 0;
  uint64_t r_str = 0;
  while (i == 0) {
    r_str = xorshift1024star();
#pragma omp parallel
#pragma omp for
    for (int j = 0; j < WORK_SIZE; j++) {
      uint64_t r_str_l = r_str + j, b2b_b = 0;
      char b2b_h[17];
      blake2b_state b2b;

      blake2b_init(&b2b, 8);
      blake2b_update(&b2b, (uint8_t *)&r_str_l, 8);
      blake2b_update(&b2b, str, 32);
      blake2b_final(&b2b, (uint8_t *)&b2b_b, 8);

      swapLong(&b2b_b);
      sprintf(b2b_h, "%016lx", b2b_b);

      if (strcmp(b2b_h, "ffffffc000000000") > 0) {
        swapLong(&r_str_l);
#pragma omp atomic write
        i = sprintf(work, "%016lx", r_str_l);
#pragma omp cancel for
      }
#pragma omp cancellation point for
    }
  }
}
#endif

char *pow_generate(char *hash) {
  char *work = malloc(17);
  uint8_t *str;

  hex2bin(hash, &str);

  srand(time(NULL));
  for (int i = 0; i < 16; i++)
    for (int j = 0; j < 4; j++) ((uint16_t *)&s[i])[j] = rand();

#if defined(HAVE_OPENCL_OPENCL_H) || defined(HAVE_CL_CL_H)
  cl_platform_id cpPlatform;
  int err;
  cl_uint num;

  err = clGetPlatformIDs(1, &cpPlatform, &num);
  if (err != CL_SUCCESS) {
    printf("clGetPlatformIDs failed with error code %d\n", err);
    goto FAIL;
  } else if (num == 0) {
    printf("clGetPlatformIDs failed to find a gpu device\n");
    goto FAIL;
  } else {
    int i = 0;
    char *opencl_program;
    size_t length;
    const size_t work_size = WORK_SIZE;  // default value from nano
    uint64_t workb = 0, r_str = 0;
    cl_mem d_rand, d_work, d_str;
    cl_device_id device_id;
    cl_program program;

    err = clGetDeviceIDs(cpPlatform, CL_DEVICE_TYPE_GPU, 1, &device_id, NULL);
    if (err != CL_SUCCESS) {
      printf("clGetDeviceIDs failed with error code %d\n", err);
      goto FAIL;
    }

    cl_context context = clCreateContext(0, 1, &device_id, NULL, NULL, &err);
    if (err != CL_SUCCESS) {
      printf("clCreateContext failed with error code %d\n", err);
      goto FAIL;
    }

#ifndef __APPLE__
    cl_command_queue queue =
        clCreateCommandQueueWithProperties(context, device_id, 0, &err);
    if (err != CL_SUCCESS) {
      printf("clCreateCommandQueueWithProperties failed with error code %d\n",
             err);
      goto FAIL;
    }
#else
    cl_command_queue queue = clCreateCommandQueue(context, device_id, 0, &err);
    if (err != CL_SUCCESS) {
      printf("clCreateCommandQueue failed with error code %d\n", err);
      goto FAIL;
    }
#endif

    FILE *f = fopen("mpow.cl", "rb");

    fseek(f, 0, SEEK_END);
    length = ftell(f);
    rewind(f);
    opencl_program = malloc(length);
    if (opencl_program) fread(opencl_program, 1, length, f);
    fclose(f);

    program = clCreateProgramWithSource(
        context, 1, (const char **)&opencl_program, &length, &err);
    if (err != CL_SUCCESS) {
      printf("clCreateProgramWithSource failed with error code %d\n", err);
      goto FAIL;
    }

    err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
    if (err != CL_SUCCESS) {
      printf("clBuildProgram failed with error code %d\n", err);
      goto FAIL;
    }

    d_rand = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
                            8, &r_str, &err);
    if (err != CL_SUCCESS) {
      printf("clCreateBuffer failed with error code %d\n", err);
      goto FAIL;
    }

    d_work = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
                            8, &workb, &err);
    if (err != CL_SUCCESS) {
      printf("clCreateBuffer failed with error code %d\n", err);
      goto FAIL;
    }

    d_str = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
                           32, str, &err);
    if (err != CL_SUCCESS) {
      printf("clCreateBuffer failed with error code %d\n", err);
      goto FAIL;
    }

    cl_kernel kernel = clCreateKernel(program, "raiblocks_work", &err);
    if (err != CL_SUCCESS) {
      printf("clCreateKernel failed with error code %d\n", err);
      goto FAIL;
    }

    err = clSetKernelArg(kernel, 0, sizeof(d_rand), &d_rand);
    if (err != CL_SUCCESS) {
      printf("clSetKernelArg failed with error code %d\n", err);
      goto FAIL;
    }

    err = clSetKernelArg(kernel, 1, sizeof(d_work), &d_work);
    if (err != CL_SUCCESS) {
      printf("clSetKernelArg failed with error code %d\n", err);
      goto FAIL;
    }

    err = clSetKernelArg(kernel, 2, sizeof(d_str), &d_str);
    if (err != CL_SUCCESS) {
      printf("clSetKernelArg failed with error code %d\n", err);
      goto FAIL;
    }

    while (i == 0) {
      r_str = xorshift1024star();

      err = clEnqueueWriteBuffer(queue, d_rand, CL_FALSE, 0, 8, &r_str, 0, NULL,
                                 NULL);
      if (err != CL_SUCCESS) {
        printf("clEnqueueWriteBuffer failed with error code %d\n", err);
        goto FAIL;
      }

      err = clEnqueueWriteBuffer(queue, d_str, CL_FALSE, 0, 32, str, 0, NULL,
                                 NULL);
      if (err != CL_SUCCESS) {
        printf("clEnqueueWriteBuffer failed with error code %d\n", err);
        goto FAIL;
      }

      err = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &work_size, NULL, 0,
                                   NULL, NULL);
      if (err != CL_SUCCESS) {
        printf("clEnqueueNDRangeKernel failed with error code %d\n", err);
        goto FAIL;
      }

      err = clEnqueueReadBuffer(queue, d_work, CL_FALSE, 0, 8, &workb, 0, NULL,
                                NULL);
      if (err != CL_SUCCESS) {
        printf("clEnqueueReadBuffer failed with error code %d\n", err);
        goto FAIL;
      }

      err = clFinish(queue);
      if (err != CL_SUCCESS) {
        printf("clFinish failed with error code %d\n", err);
        goto FAIL;
      }

      if (workb != 0) {
        swapLong(&workb);
        i = sprintf(work, "%016lx", workb);
      }
    }

    free(opencl_program);
    err = clReleaseMemObject(d_rand);
    if (err != CL_SUCCESS) {
      printf("clReleaseMemObject failed with error code %d\n", err);
      goto FAIL;
    }
    err = clReleaseMemObject(d_work);
    if (err != CL_SUCCESS) {
      printf("clReleaseMemObject failed with error code %d\n", err);
      goto FAIL;
    }
    err = clReleaseMemObject(d_str);
    if (err != CL_SUCCESS) {
      printf("clReleaseMemObject failed with error code %d\n", err);
      goto FAIL;
    }
    err = clReleaseKernel(kernel);
    if (err != CL_SUCCESS) {
      printf("clReleaseKernel failed with error code %d\n", err);
      goto FAIL;
    }
    err = clReleaseProgram(program);
    if (err != CL_SUCCESS) {
      printf("clReleaseProgram failed with error code %d\n", err);
      goto FAIL;
    }
    err = clReleaseCommandQueue(queue);
    if (err != CL_SUCCESS) {
      printf("clReleaseCommandQueue failed with error code %d\n", err);
      goto FAIL;
    }
    err = clReleaseContext(context);
    if (err != CL_SUCCESS) {
      printf("clReleaseContext failed with error code %d\n", err);
      goto FAIL;
    }
  }
#else
  pow_omp(str, work);
#endif
  return work;
#if defined(HAVE_OPENCL_OPENCL_H) || defined(HAVE_CL_CL_H)
FAIL:
  return NULL;
#endif
}

int main(int argc, char *argv[]) {
  if (argc > 1) {
    char *work;
    work = pow_generate(argv[1]);
    printf("%s\n", work);
    free(work);
  }
}
