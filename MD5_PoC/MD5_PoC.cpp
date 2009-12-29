/*****************************************************************************
 *	Very simple MD5 cracker using NVIDIA GPU                                 *
 *	Written as proof-of-concept and demonstration,                           *
 *	Specially for Troopers 2008                                              *
 *	                                                                         *
 *	(c) Copyright 2008, Andrey Belenko, ElcomSoft Co. Ltd.                   *
 *****************************************************************************/

// MD5_PoC.cpp

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <cutil_inline.h>
//#include <cuda_runtime.h>
typedef unsigned long long __int64;
extern "C" void RunKernel_MD5( int grid, unsigned long *pdwResult );

//char szChars[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
char szChars[] = "abcdefghijklmnopqrstuvwxyz";
int nPasswordLen = 8;

// Put here MD5 hash of password you want to recover
// MD5("password") = 5F4DCC3B5AA765D61D8327DEB882CF99
unsigned char bHash[16] = { 0x5F, 0x4D, 0xCC, 0x3B, 0x5A, 0xA7, 0x65, 0xD6, 0x1D, 0x83, 0x27, 0xDE, 0xB8, 0x82, 0xCF, 0x99 };

int main(int argc, char* argv[])
{
	printf( "==== MD5 Password Cracker (GPU PoC) ====\n" );

	int deviceCount = 0;

	cudaError rc;
	rc = cudaGetDeviceCount( &deviceCount );
	if( rc != cudaSuccess )
	{
		printf( " ! cudaGetDeviceCount() failed: %s\n", cudaGetErrorString( rc ) );
		return 0;
	}

	if( deviceCount == 1 )
	{
		cudaDeviceProp prop;
		cudaGetDeviceProperties( &prop, 0 );

		if( prop.major < 1 )
			deviceCount = 0;
	}

	if( deviceCount == 0 )
	{
		printf( " ! No compatible device found. Check your hardware and drivers.\n" );
		return 0;
	}
	else
	{
		printf( " + Number of CUDA devices present: %d\n", deviceCount );
	}

	char szCharset[256] = { 0 };
	unsigned int nPassword[32] = { 0 };
	unsigned int nCharset[256] = { 0 };
	void *pdResult = NULL;
	int nResult = -1;
	int t, ctr = 0;

	int nCharsetLen = strlen( szChars );
	strcpy( szCharset, szChars );

	for( int i = 0; i < nCharsetLen; i++ )
	{
		nCharset[i] = szCharset[i];
	}

	rc = cudaMalloc( &pdResult, 4 );
	if( rc != cudaSuccess )
	{
		printf( " ! cudaMalloc() failed: %s\n", cudaGetErrorString( rc ) );
		return 0;
	}

	rc = cudaMemcpy( pdResult, &nResult, 4, cudaMemcpyHostToDevice );
	if( rc != cudaSuccess )
	{
		printf( " ! cudaMemcpy() failed: %s\n", cudaGetErrorString( rc ) );
		return 0;
	}

	rc = cudaMemcpyToSymbol( "nPassword", nPassword, 32*4 );
	if( rc != cudaSuccess )
	{
		printf( " ! cudaMemcpyToSymbol() failed: %s\n", cudaGetErrorString( rc ) );
		return 0;
	}
	rc = cudaMemcpyToSymbol( "nPasswordLen", &nPasswordLen, 4 );
	if( rc != cudaSuccess )
	{
		printf( " ! cudaMemcpyToSymbol() failed: %s\n", cudaGetErrorString( rc ) );
		return 0;
	}

	rc = cudaMemcpyToSymbol( "cCharset", nCharset, 256*4 );
	if( rc != cudaSuccess )
	{
		printf( " ! cudaMemcpyToSymbol() failed: %s\n", cudaGetErrorString( rc ) );
		return 0;
	}
	rc = cudaMemcpyToSymbol( "nCharsetLen", &nCharsetLen, 4 );
	if( rc != cudaSuccess )
	{
		printf( " ! cudaMemcpyToSymbol() failed: %s\n", cudaGetErrorString( rc ) );
		return 0;
	}

	rc = cudaMemcpyToSymbol( "bHash", bHash, 16 );
	if( rc != cudaSuccess )
	{
		printf( " ! cudaMemcpyToSymbol() failed: %s\n", cudaGetErrorString( rc ) );
		return 0;
	}

	time_t start;
	unsigned long long  pwd_count = 0;

	start = time( NULL );

	while(1)
	{
		RunKernel_MD5( 8192, (unsigned long*)pdResult );
		rc = cudaThreadSynchronize();
		if( rc != cudaSuccess )
		{
			printf( " ! cudaThreadSynchronize() failed: %s\n", cudaGetErrorString( rc ) );
			break;
		}

		cudaMemcpy( &nResult, pdResult, 4, cudaMemcpyDeviceToHost );

		if( nResult != -1 )
		{
			t = nResult;
		}
		else
		{
			t = 8192*256;
		}

		pwd_count += t;
		
		for( int i = 0; i < nPasswordLen; i++ )
		{
			t = t + nPassword[i];
			nPassword[i] = t % nCharsetLen;
			t = t / nCharsetLen;
		}

		rc = cudaMemcpyToSymbol( "nPassword", nPassword, 32*4 );
		if( rc != cudaSuccess )
		{
			printf( " ! cudaMemcpyToSymbol() failed: %s\n", cudaGetErrorString( rc ) );
			break;
		}

		ctr++;

		if( ctr > 100 )
		{
			char szPassword[256] = { 0 };
			for( int i = 0; i < nPasswordLen; i++ )
				szPassword[i] = szCharset[ nPassword[i] ];

			printf( " + Current password: \"%s\"", szPassword );

			time_t cur = time( NULL );

			if( cur - start > 10 )
			{
				printf( ", avg. speed: %.2fM p/s", ((double)pwd_count)/(cur-start)/1e6 );
			}


			printf( "\r" );

			ctr = 0;
		}

		if( nResult != -1 )
		{
			char szPassword[256] = { 0 };
			for( int i = 0; i < nPasswordLen; i++ )
				szPassword[i] = szCharset[ nPassword[i] ];

			printf( " + Password for " );

			for( int i = 0; i < 16; i++ )
				printf( "%02x", bHash[i] );

			printf( " is \"%s\"\n", szPassword );

			break;
		}
	};

	cudaFree( pdResult );

	return 0;
}

