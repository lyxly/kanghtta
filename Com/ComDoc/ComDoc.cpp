#include <iostream>
#include <fstream>

#include <math.h>
#include <tchar.h>
#include <cassert>
#include <string>
#include <iomanip>
#include <vector>
#include <list>

#include "ComDoc.h"

using namespace std;

/* declare const */

 int	SectorSize = 0;
 int	miniSectorSize = 0;

/* declare function */
bool DumpDocHeader(PDocHeader pHeader);
bool IfReadFile(ifstream &inStream,unsigned char * buf,unsigned int iReadOffest,size_t size);
int GetOffestFremSid(SECT sid);


int main(int argc,char *argv[])
{
	cout<<"Enter the name of the input file :\n";
	string	 inputFileName;
	getline(cin,inputFileName);
	
	BYTE * lpHeaderBuf  = new BYTE[512];

	memset(lpHeaderBuf,0,512);

	ifstream	inStream;
	inStream.open(inputFileName.data(),ios::binary|ios::in);
	assert(inStream.is_open());
	inStream.read((char*)lpHeaderBuf,512);
	PDocHeader   pHeaderSec = (PDocHeader)lpHeaderBuf;
	
	cout<<"open the "<<inputFileName<<" file is successful\n "<<endl;
	DumpDocHeader(pHeaderSec);

	/************************************************************************/
	/* 处理msat                                                                      */
	/************************************************************************/
	unsigned long  iMastSize = 0;
	vector<int> vMastList;
	if ((pHeaderSec->_sectDifStart == ENDOFCHAIN)&&(pHeaderSec->_csectDif == 0))
	{
		for(iMastSize = 1;iMastSize <= 109 ;iMastSize ++)
		{
			if ((( pHeaderSec->_sectFat[iMastSize-1] ) ==  0xFFFFFFFF))
			{
				break;
			}
			else
			{
				
				BYTE  *SecBuf = new BYTE[SectorSize];
				IfReadFile(inStream,SecBuf,GetOffestFremSid((pHeaderSec->_sectFat[iMastSize - 1] )),SectorSize);
				int * pListOfMast = (int *)SecBuf;
				int i = 0;
				while( pListOfMast[i]!= FREESECT )
				{
					vMastList.push_back(pListOfMast[i]);
					//cout<<vMastList.front()<<" \t"<<vMastList.back()<<'\t';
					
					cout<<"MAST["<<i<<"] == ";
					cout<<vMastList[i]<<'\t';
					if (vMastList[vMastList.size()-1] == ENDOFCHAIN)
					{
						cout<<endl;
					}
					i++;
				}
				
				delete []SecBuf;
			}
		
		}
	}
	else
	{
		/** Mast 大于 109个扇区的情况 */
	}

	/************************************************************************/
	/* 处理ssat                                                                     */
	/************************************************************************/
	
	/*
	 * 读取存取ssat短链分配表占用的扇区链，
	 */
	vector<int>	vSsatFat;
	vSsatFat.push_back(pHeaderSec->_sectMiniFatStart);
	int index = vSsatFat[0];
	while(vMastList[index] != ENDOFCHAIN )
	{
		index = vMastList[index];
		vSsatFat.push_back(index);

	}
	
	vSsatFat.push_back(vMastList[index]);
	int i = 0; /** 循环计数器*/
	for (i; i < vSsatFat.size();i++)
	{
		cout<<"S-FAT["<<i<<"] == ";
		cout<<hex<<vSsatFat[i]<<'\t';
	}
	cout<<endl;

	/*
	 * 读取ssat链，即用于记录short stream 的链
	 * pHeaderSec->_csectMiniFat 指出short stream 占几个扇区
	 */
	vector<int> vSsatList;

	for (i = 1; i <= pHeaderSec->_csectMiniFat; i++)
	{
		
			BYTE  *SecBuf = new BYTE[SectorSize];
			IfReadFile(inStream,SecBuf,GetOffestFremSid(vSsatFat[i-1]),SectorSize);
			int * pListOfMast = (int *)SecBuf;
			int i = 0;
			while( pListOfMast[i]!= FREESECT )
			{
				vSsatList.push_back(pListOfMast[i]);
				//cout<<vMastList.front()<<" \t"<<vMastList.back()<<'\t';
				
				cout<<"SSAT["<<i<<"] == ";
				cout<<vSsatList[i]<<'\t';
				if (vSsatList[vSsatList.size()-1] == ENDOFCHAIN)
				{
					cout<<endl;
				}
				i++;
			}
			
			delete []SecBuf;
		
	}
	
	/************************************************************************/
	/* Process Directory                                                                     */
	/************************************************************************/
	
	/*
	 * 读取存取directory 的扇区sid链 
	 */
	vector<int>	vDirFat;
	vDirFat.push_back(pHeaderSec->_sectDirStart);
	index = vDirFat[0];
	while(vMastList[index] != ENDOFCHAIN )
	{
		index = vMastList[index];
		vDirFat.push_back(index);
		
	}
	
	vDirFat.push_back(vMastList[index]);

	for (i =0; i < vDirFat.size();i++)
	{
		cout<<"Director-FAT["<<i<<"] == ";
		cout<<hex<<vDirFat[i]<<'\t';
	}
	cout<<endl;
	cout<<vDirFat.size()<<endl;

	/*
	 *分析并处理directory目录
	 */
	list<DirectoryEntry> lDirList;
	for (i=0; i<(vDirFat.size()-1) ;i++)
	{
		BYTE  *SecBuf = new BYTE[SectorSize];
		IfReadFile(inStream,SecBuf,GetOffestFremSid(vDirFat[i]),SectorSize);
		PDirectoryEntry pDirEntry = PDirectoryEntry(SecBuf);
		lDirList.push_back(pDirEntry[0]);
		lDirList.push_back(pDirEntry[1]);
		lDirList.push_back(pDirEntry[2]);
		lDirList.push_back(pDirEntry[3]);
		delete []SecBuf;
	}

	/*
	 *processing  DirectoryEntry list
	 */




	delete []lpHeaderBuf;

	inStream.close();
	return 0;
}





bool IfReadFile(ifstream &inStream,unsigned char * buf,unsigned int iReadOffest,size_t size)
{
	inStream.seekg(iReadOffest,ios::beg);
	inStream.read(( char* )buf,size);
	return true;	
	
}

int GetOffestFremSid(SECT sid)
{
	return sid*SectorSize+512;
}


bool DumpDocHeader(PDocHeader pHeader)
{
	cout<<"\t The comdoc flag is\t{ ";
	for (int i = 0; i < 8; i++)
	{
		cout<<hex<<(int)pHeader->_abSig[i]<<" ";
		if (i!=7)
		{
			cout<<',';
		}
	}
	cout<<'}'<<endl;
	if (pHeader->_uByteOrder == 0xFFFE)
	{
		cout<<"\t The file Byte order is Little-Endian"<<endl;
	}
	cout.setf(ios::dec,ios::basefield);
	SectorSize = (int)pow((double)2,(int)pHeader->_uSectorShift);

	cout<<"\t Size of a Sector in the compound document file is "<<SectorSize<<endl;
	miniSectorSize = (int)pow((double)2,(int)pHeader->_uMiniSectorShift);
	cout<<"\t Size of a short-sector in the short-stream container stream is "<<miniSectorSize<<endl;
	cout<<"\t Total number of sectors used for the sector allocation table is "<<(DWORD)pHeader->_csectFat<<endl;
	cout<<"\t SecID of first sector of the directory stream is "<<(ULONG)pHeader->_sectDirStart<<endl;
	cout<<"\t Minimum size of a standard stream  is "<<(ULONG)pHeader->_ulMiniSectorCutoff<<endl;
	cout<<"\t SecID of first sector of the short-sector allocation table is "<<(ULONG)pHeader->_sectMiniFatStart<<endl;
	cout<<"\t Total number of sectors used for the short-sector allocation table is "<<(ULONG)pHeader->_csectMiniFat<<endl;
	cout<<"\t SecID of first sector of the master sector allocation table is "<<pHeader->_sectDifStart<<endl;
	cout<<"\t Total number of sectors used for the master sector allocation table is "<<(ULONG)pHeader->_csectDif<<endl;
	cout<<"\t First part of the master sector allocation table containing 109 SecIDs  is "<<endl;
	cout<<"\t {\t";
	for (i = 0;i<109;i++)
	{
		if ( pHeader->_sectFat[i] != -1 )
		{
			cout<<pHeader->_sectFat[i]<<'\t';
			if (i/20 !=0)
			{
				cout<<endl;
			}
		}
		else
		{
			break;
		}	
	}
	cout<<'}'<<endl;
	return true;
}