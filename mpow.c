#include <Python.h>
#include <time.h>

#ifdef HAVE_CL_CL_H
#include <CL/cl.h>
#elif HAVE_OPENCL_OPENCL_H
#include <OpenCL/opencl.h>
#else
#include <omp.h>
#include "b2b/blake2.h"
#endif

#if defined(HAVE_CL_CL_H) || defined(HAVE_OPENCL_OPENCL_H)
// this is the variable opencl_program in nano-node/nano/node/openclwork.cpp
const char *opencl_program = R"%%%(
enum blake2b_constant
{
	BLAKE2B_BLOCKBYTES = 128,
	BLAKE2B_OUTBYTES   = 64,
	BLAKE2B_KEYBYTES   = 64,
	BLAKE2B_SALTBYTES  = 16,
	BLAKE2B_PERSONALBYTES = 16
};
typedef struct __blake2b_param
{
	uchar  digest_length; // 1
	uchar  key_length;    // 2
	uchar  fanout;        // 3
	uchar  depth;         // 4
	uint leaf_length;   // 8
	ulong node_offset;   // 16
	uchar  node_depth;    // 17
	uchar  inner_length;  // 18
	uchar  reserved[14];  // 32
	uchar  salt[BLAKE2B_SALTBYTES]; // 48
	uchar  personal[BLAKE2B_PERSONALBYTES];  // 64
} blake2b_param;
typedef struct __blake2b_state
{
	ulong h[8];
	ulong t[2];
	ulong f[2];
	uchar  buf[2 * BLAKE2B_BLOCKBYTES];
	size_t   buflen;
	uchar  last_node;
} blake2b_state;
__constant static const ulong blake2b_IV[8] =
{
	0x6a09e667f3bcc908UL, 0xbb67ae8584caa73bUL,
	0x3c6ef372fe94f82bUL, 0xa54ff53a5f1d36f1UL,
	0x510e527fade682d1UL, 0x9b05688c2b3e6c1fUL,
	0x1f83d9abfb41bd6bUL, 0x5be0cd19137e2179UL
};
__constant static const uchar blake2b_sigma[12][16] =
{
  {  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15 } ,
  { 14, 10,  4,  8,  9, 15, 13,  6,  1, 12,  0,  2, 11,  7,  5,  3 } ,
  { 11,  8, 12,  0,  5,  2, 15, 13, 10, 14,  3,  6,  7,  1,  9,  4 } ,
  {  7,  9,  3,  1, 13, 12, 11, 14,  2,  6,  5, 10,  4,  0, 15,  8 } ,
  {  9,  0,  5,  7,  2,  4, 10, 15, 14,  1, 11, 12,  6,  8,  3, 13 } ,
  {  2, 12,  6, 10,  0, 11,  8,  3,  4, 13,  7,  5, 15, 14,  1,  9 } ,
  { 12,  5,  1, 15, 14, 13,  4, 10,  0,  7,  6,  3,  9,  2,  8, 11 } ,
  { 13, 11,  7, 14, 12,  1,  3,  9,  5,  0, 15,  4,  8,  6,  2, 10 } ,
  {  6, 15, 14,  9, 11,  3,  0,  8, 12,  2, 13,  7,  1,  4, 10,  5 } ,
  { 10,  2,  8,  4,  7,  6,  1,  5, 15, 11,  9, 14,  3, 12, 13 , 0 } ,
  {  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15 } ,
  { 14, 10,  4,  8,  9, 15, 13,  6,  1, 12,  0,  2, 11,  7,  5,  3 }
};
static inline int blake2b_set_lastnode( blake2b_state *S )
{
  S->f[1] = ~0UL;
  return 0;
}
/* Some helper functions, not necessarily useful */
static inline int blake2b_set_lastblock( blake2b_state *S )
{
  if( S->last_node ) blake2b_set_lastnode( S );
  S->f[0] = ~0UL;
  return 0;
}
static inline int blake2b_increment_counter( blake2b_state *S, const ulong inc )
{
  S->t[0] += inc;
  S->t[1] += ( S->t[0] < inc );
  return 0;
}
static inline ulong load64( const void *src )
{
#if defined(__ENDIAN_LITTLE__)
  return *( ulong * )( src );
#else
  const uchar *p = ( uchar * )src;
  ulong w = *p++;
  w |= ( ulong )( *p++ ) <<  8;
  w |= ( ulong )( *p++ ) << 16;
  w |= ( ulong )( *p++ ) << 24;
  w |= ( ulong )( *p++ ) << 32;
  w |= ( ulong )( *p++ ) << 40;
  w |= ( ulong )( *p++ ) << 48;
  w |= ( ulong )( *p++ ) << 56;
  return w;
#endif
}
static inline void store32( void *dst, uint w )
{
#if defined(__ENDIAN_LITTLE__)
  *( uint * )( dst ) = w;
#else
  uchar *p = ( uchar * )dst;
  *p++ = ( uchar )w; w >>= 8;
  *p++ = ( uchar )w; w >>= 8;
  *p++ = ( uchar )w; w >>= 8;
  *p++ = ( uchar )w;
#endif
}
static inline void store64( void *dst, ulong w )
{
#if defined(__ENDIAN_LITTLE__)
  *( ulong * )( dst ) = w;
#else
  uchar *p = ( uchar * )dst;
  *p++ = ( uchar )w; w >>= 8;
  *p++ = ( uchar )w; w >>= 8;
  *p++ = ( uchar )w; w >>= 8;
  *p++ = ( uchar )w; w >>= 8;
  *p++ = ( uchar )w; w >>= 8;
  *p++ = ( uchar )w; w >>= 8;
  *p++ = ( uchar )w; w >>= 8;
  *p++ = ( uchar )w;
#endif
}
static inline ulong rotr64( const ulong w, const unsigned c )
{
  return ( w >> c ) | ( w << ( 64 - c ) );
}
static void ucharset (void * dest_a, int val, size_t count)
{
	uchar * dest = (uchar *)dest_a;
	for (size_t i = 0; i < count; ++i)
	{
		*dest++ = val;
	}
}
/* init xors IV with input parameter block */
static inline int blake2b_init_param( blake2b_state *S, const blake2b_param *P )
{
  uchar *p, *h;
  __constant uchar *v;
  v = ( __constant uchar * )( blake2b_IV );
  h = ( uchar * )( S->h );
  p = ( uchar * )( P );
  /* IV XOR ParamBlock */
  ucharset( S, 0, sizeof( blake2b_state ) );
  for( int i = 0; i < BLAKE2B_OUTBYTES; ++i ) h[i] = v[i] ^ p[i];
  return 0;
}
static inline int blake2b_init( blake2b_state *S, const uchar outlen )
{
  blake2b_param P[1];
  if ( ( !outlen ) || ( outlen > BLAKE2B_OUTBYTES ) ) return -1;
  P->digest_length = outlen;
  P->key_length    = 0;
  P->fanout        = 1;
  P->depth         = 1;
  store32( &P->leaf_length, 0 );
  store64( &P->node_offset, 0 );
  P->node_depth    = 0;
  P->inner_length  = 0;
  ucharset( P->reserved, 0, sizeof( P->reserved ) );
  ucharset( P->salt,     0, sizeof( P->salt ) );
  ucharset( P->personal, 0, sizeof( P->personal ) );
  return blake2b_init_param( S, P );
}
static int blake2b_compress( blake2b_state *S, __private const uchar block[BLAKE2B_BLOCKBYTES] )
{
  ulong m[16];
  ulong v[16];
  int i;
  for( i = 0; i < 16; ++i )
	m[i] = load64( block + i * sizeof( m[i] ) );
  for( i = 0; i < 8; ++i )
	v[i] = S->h[i];
  v[ 8] = blake2b_IV[0];
  v[ 9] = blake2b_IV[1];
  v[10] = blake2b_IV[2];
  v[11] = blake2b_IV[3];
  v[12] = S->t[0] ^ blake2b_IV[4];
  v[13] = S->t[1] ^ blake2b_IV[5];
  v[14] = S->f[0] ^ blake2b_IV[6];
  v[15] = S->f[1] ^ blake2b_IV[7];
#define G(r,i,a,b,c,d) \
  do { \
	a = a + b + m[blake2b_sigma[r][2*i+0]]; \
	d = rotr64(d ^ a, 32); \
	c = c + d; \
	b = rotr64(b ^ c, 24); \
	a = a + b + m[blake2b_sigma[r][2*i+1]]; \
	d = rotr64(d ^ a, 16); \
	c = c + d; \
	b = rotr64(b ^ c, 63); \
  } while(0)
#define ROUND(r)  \
  do { \
	G(r,0,v[ 0],v[ 4],v[ 8],v[12]); \
	G(r,1,v[ 1],v[ 5],v[ 9],v[13]); \
	G(r,2,v[ 2],v[ 6],v[10],v[14]); \
	G(r,3,v[ 3],v[ 7],v[11],v[15]); \
	G(r,4,v[ 0],v[ 5],v[10],v[15]); \
	G(r,5,v[ 1],v[ 6],v[11],v[12]); \
	G(r,6,v[ 2],v[ 7],v[ 8],v[13]); \
	G(r,7,v[ 3],v[ 4],v[ 9],v[14]); \
  } while(0)
  ROUND( 0 );
  ROUND( 1 );
  ROUND( 2 );
  ROUND( 3 );
  ROUND( 4 );
  ROUND( 5 );
  ROUND( 6 );
  ROUND( 7 );
  ROUND( 8 );
  ROUND( 9 );
  ROUND( 10 );
  ROUND( 11 );
  for( i = 0; i < 8; ++i )
	S->h[i] = S->h[i] ^ v[i] ^ v[i + 8];
#undef G
#undef ROUND
  return 0;
}
static void ucharcpy (uchar * dst, uchar const * src, size_t count)
{
	for (size_t i = 0; i < count; ++i)
	{
		*dst++ = *src++;
	}
}
void printstate (blake2b_state * S)
{
	printf ("%lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu ", S->h[0], S->h[1], S->h[2], S->h[3], S->h[4], S->h[5], S->h[6], S->h[7], S->t[0], S->t[1], S->f[0], S->f[1]);
	for (int i = 0; i < 256; ++i)
	{
		printf ("%02x", S->buf[i]);
	}
	printf (" %lu %02x\n", S->buflen, S->last_node);
}
/* inlen now in bytes */
static int blake2b_update( blake2b_state *S, const uchar *in, ulong inlen )
{
  while( inlen > 0 )
  {
	size_t left = S->buflen;
	size_t fill = 2 * BLAKE2B_BLOCKBYTES - left;
	if( inlen > fill )
	{
	  ucharcpy( S->buf + left, in, fill ); // Fill buffer
	  S->buflen += fill;
	  blake2b_increment_counter( S, BLAKE2B_BLOCKBYTES );
	  blake2b_compress( S, S->buf ); // Compress
	  ucharcpy( S->buf, S->buf + BLAKE2B_BLOCKBYTES, BLAKE2B_BLOCKBYTES ); // Shift buffer left
	  S->buflen -= BLAKE2B_BLOCKBYTES;
	  in += fill;
	  inlen -= fill;
	}
	else // inlen <= fill
	{
	  ucharcpy( S->buf + left, in, inlen );
	  S->buflen += inlen; // Be lazy, do not compress
	  in += inlen;
	  inlen -= inlen;
	}
  }
  return 0;
}
/* Is this correct? */
static int blake2b_final( blake2b_state *S, uchar *out, uchar outlen )
{
  uchar buffer[BLAKE2B_OUTBYTES];
  if( S->buflen > BLAKE2B_BLOCKBYTES )
  {
	blake2b_increment_counter( S, BLAKE2B_BLOCKBYTES );
	blake2b_compress( S, S->buf );
	S->buflen -= BLAKE2B_BLOCKBYTES;
	ucharcpy( S->buf, S->buf + BLAKE2B_BLOCKBYTES, S->buflen );
  }
  //blake2b_increment_counter( S, S->buflen );
  ulong inc = (ulong)S->buflen;
  S->t[0] += inc;
//  if ( S->t[0] < inc )
//    S->t[1] += 1;
  // This seems to crash the opencl compiler though fortunately this is calculating size and we don't do things bigger than 2^32
	
  blake2b_set_lastblock( S );
  ucharset( S->buf + S->buflen, 0, 2 * BLAKE2B_BLOCKBYTES - S->buflen ); /* Padding */
  blake2b_compress( S, S->buf );
  for( int i = 0; i < 8; ++i ) /* Output full hash to temp buffer */
	store64( buffer + sizeof( S->h[i] ) * i, S->h[i] );
  ucharcpy( out, buffer, outlen );
  return 0;
}
static void ucharcpyglb (uchar * dst, __global uchar const * src, size_t count)
{
	for (size_t i = 0; i < count; ++i)
	{
		*dst = *src;
		++dst;
		++src;
	}
}
	
__kernel void nano_work (__global ulong const * attempt, __global ulong * result_a, __global uchar const * item_a, __global ulong const * difficulty_a)
{
	int const thread = get_global_id (0);
	uchar item_l [32];
	ucharcpyglb (item_l, item_a, 32);
	ulong attempt_l = *attempt + thread;
	blake2b_state state;
	blake2b_init (&state, sizeof (ulong));
	blake2b_update (&state, (uchar *) &attempt_l, sizeof (ulong));
	blake2b_update (&state, item_l, 32);
	ulong result;
	blake2b_final (&state, (uchar *) &result, sizeof (result));
	if (result >= *difficulty_a)
	{
		*result_a = attempt_l;
	}
}
)%%%";
#endif

static uint64_t s[16];
static int p;

uint64_t xorshift1024star(void) {  // nano-node/nano/node/xorshift.hpp
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

static PyObject *generate(PyObject *self, PyObject *args) {
  int i, j;
  uint8_t *str;
  uint64_t difficulty = 0, workb = 0, r_str = 0;
  const size_t work_size = 1024 * 1024;  // default value from nano

  if (!PyArg_ParseTuple(args, "y#K", &str, &i, &difficulty)) return NULL;

  srand(time(NULL));
  for (i = 0; i < 16; i++)
    for (j = 0; j < 4; j++) ((uint16_t *)&s[i])[j] = rand();

#if defined(HAVE_CL_CL_H) || defined(HAVE_OPENCL_OPENCL_H)
  int err;
  cl_uint num;
  cl_platform_id cpPlatform;

  err = clGetPlatformIDs(1, &cpPlatform, &num);
  if (err != CL_SUCCESS) {
    printf("clGetPlatformIDs failed with error code %d\n", err);
    goto FAIL;
  } else if (num == 0) {
    printf("clGetPlatformIDs failed to find a gpu device\n");
    goto FAIL;
  } else {
    size_t length = strlen(opencl_program);
    cl_mem d_rand, d_work, d_str, d_difficulty;
    cl_device_id device_id;
    cl_context context;
    cl_command_queue queue;
    cl_program program;
    cl_kernel kernel;

    err = clGetDeviceIDs(cpPlatform, CL_DEVICE_TYPE_GPU, 1, &device_id, NULL);
    if (err != CL_SUCCESS) {
      printf("clGetDeviceIDs failed with error code %d\n", err);
      goto FAIL;
    }

    context = clCreateContext(0, 1, &device_id, NULL, NULL, &err);
    if (err != CL_SUCCESS) {
      printf("clCreateContext failed with error code %d\n", err);
      goto FAIL;
    }

#ifndef __APPLE__
    queue = clCreateCommandQueueWithProperties(context, device_id, 0, &err);
    if (err != CL_SUCCESS) {
      printf("clCreateCommandQueueWithProperties failed with error code %d\n",
             err);
      goto FAIL;
    }
#else
    queue = clCreateCommandQueue(context, device_id, 0, &err);
    if (err != CL_SUCCESS) {
      printf("clCreateCommandQueue failed with error code %d\n", err);
      goto FAIL;
    }
#endif

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

    d_difficulty =
        clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, 8,
                       &difficulty, &err);
    if (err != CL_SUCCESS) {
      printf("clCreateBuffer failed with error code %d\n", err);
      goto FAIL;
    }

    kernel = clCreateKernel(program, "nano_work", &err);
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

    err = clSetKernelArg(kernel, 3, sizeof(d_difficulty), &d_difficulty);
    if (err != CL_SUCCESS) {
      printf("clSetKernelArg failed with error code %d\n", err);
      goto FAIL;
    }

    err =
        clEnqueueWriteBuffer(queue, d_str, CL_FALSE, 0, 32, str, 0, NULL, NULL);
    if (err != CL_SUCCESS) {
      printf("clEnqueueWriteBuffer failed with error code %d\n", err);
      goto FAIL;
    }

    err = clEnqueueWriteBuffer(queue, d_difficulty, CL_FALSE, 0, 8, &difficulty,
                               0, NULL, NULL);
    if (err != CL_SUCCESS) {
      printf("clEnqueueWriteBuffer failed with error code %d\n", err);
      goto FAIL;
    }

    while (workb == 0) {
      r_str = xorshift1024star();

      err = clEnqueueWriteBuffer(queue, d_rand, CL_FALSE, 0, 8, &r_str, 0, NULL,
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
    }

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
    err = clReleaseMemObject(d_difficulty);
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
FAIL:
#else
  while (workb == 0) {
    r_str = xorshift1024star();

#pragma omp parallel
#pragma omp for
    for (i = 0; i < work_size; i++) {
#ifdef USE_VISUAL_C
      if (workb == 0) {
#endif
      uint64_t r_str_l = r_str + i, b2b_b = 0;
      blake2b_state b2b;

      blake2b_init(&b2b, 8);
      blake2b_update(&b2b, (uint8_t *)&r_str_l, 8);
      blake2b_update(&b2b, str, 32);
      blake2b_final(&b2b, (uint8_t *)&b2b_b, 8);

      swapLong(&b2b_b);

#ifdef USE_VISUAL_C
        if (b2b_b >= difficulty) {
#pragma omp critical
          workb = r_str_l;
        }
      }
#else
      if (b2b_b >= difficulty) {
#pragma omp atomic write
        workb = r_str_l;
#pragma omp cancel for
      }
#pragma omp cancellation point for
#endif
    }
  }
#endif
  swapLong(&workb);
  return Py_BuildValue("K", workb);
}

static PyMethodDef generate_method[] = {
    {"generate", generate, METH_VARARGS, NULL}, {NULL, NULL, 0, NULL}};

static struct PyModuleDef work_module = {PyModuleDef_HEAD_INIT, "work", NULL,
                                         -1, generate_method};

PyMODINIT_FUNC PyInit_mpow(void) {
  PyObject *m = PyModule_Create(&work_module);
  if (m == NULL) return NULL;
  return m;
}
