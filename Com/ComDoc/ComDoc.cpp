#include <iostream>
#include <fstream>

#include <math.h>
#include <tchar.h>
#include <cassert>
#include <string>
#include <iomanip>
#include <vector>
#include <list>
#include <algorithm>

#include "ComDoc.h"

using namespace std;

struct DirectEntry 
{
	string strDirName;
	vector<int> vSidList;
}stDirectEntry;

/* declare const */

 int	SectorSize = 0;
 int	miniSectorSize = 0;
 ULONG  MaxMiniStreamSize =0; /** short stream 的最大长度。用来判断使用的sat表式那种类型，小于在ssat中 */
/* declare function */
bool DumpDocHeader(PDocHeader pHeader);
bool IfReadFile(ifstream &inStream,unsigned char * buf,unsigned int iReadOffest,size_t size);
int GetOffestFremSid(SECT sid);
bool ProcessDirEntry(PDirectoryEntry pDirEntry,vector<SECT> & slist,vector<int> & sslist,vector<DirectEntry >& FatOfDirEntry,int );
SECT ReadMastList(ifstream &inStream,vector<SECT>&vMsat,SECT sid,size_t size);

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
	MaxMiniStreamSize = pHeaderSec->_ulMiniSectorCutoff;
	/**********************************************************************/
	/* 处理msat                                                                      */
	/************************************************************************/
	unsigned long  iMastSize = 0;

	vector<SECT> vSatList;  /** 用于存储sat链表*/
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
				SECT * pListOfMast = (SECT *)SecBuf;
				int i = 0;
				for (i;i<128;i++)
				{
					if (pListOfMast[i] != 0xFFFFFFFF )
					{
						vSatList.push_back(pListOfMast[i]);
					}

				}
				
				delete []SecBuf;
			}
		
		}
	}
	else
	{
		/** Mast 大于 109个扇区的情况 */
		SECT *begin = pHeaderSec->_sectFat;
		SECT *end = begin + 109;
		vector<SECT> vTempMsat(begin,end);
	
		SECT NextMsatSid;
		SECT  sid = pHeaderSec->_sectDifStart;
		for (int i = 0; i < pHeaderSec->_csectDif;i++)
		{
			
			NextMsatSid = ReadMastList(inStream,vTempMsat,sid,SectorSize);
			sid = NextMsatSid;
		}

		vTempMsat.push_back(sid);
		/*
		 * 打印输出MastList
		 */
		vector<SECT> vMsatList(vTempMsat.begin(),vTempMsat.end());
		cout<<"MastLiat: ";
		cout<<hex<<showbase;
		copy(vMsatList.begin(),vMsatList.end(),ostream_iterator<SECT>(cout," "));
		int pos = 0,
			length = vMsatList.size() ;
		for (pos ; pos < length ;++pos)
		{
			if ((vMsatList[pos]) == ENDOFCHAIN)
			{
				break;
			}
			
 			if ((vMsatList[pos] ) ==  0xFFFFFFFF)
 			{
 				continue;
 			}
 			else
			{
	
 				BYTE  *SecBuf = new BYTE[SectorSize];
 				IfReadFile(inStream,SecBuf,GetOffestFremSid((vMsatList[pos] )),SectorSize);
 				SECT * pListOfMast = (SECT *)SecBuf;
 				int i = 0;
 				for (i;i<128;i++)
 				{
					if (pListOfMast[i] != 0xFFFFFFFF )
					{
						vSatList.push_back(pListOfMast[i]);
					}
					
 				}
 				
 				delete []SecBuf;
 			}
 		}
		
	}

	cout<<"the sat size "<<(vSatList.size()*512+512)<<endl;
	inStream.seekg(0,ios::end);
	long filesize = inStream.tellg();
	cout<<"the file size :"<<filesize<<endl;

	/************************************************************************/
	/* 处理ssat                                                                     */
	/************************************************************************/
	
	/*
	 * 读取存取ssat短链分配表占用的扇区链，
	 */
	vector<int>	vSsatFat;
	vSsatFat.push_back(pHeaderSec->_sectMiniFatStart);
	int index = vSsatFat[0];
	while(vSatList[index] != ENDOFCHAIN )
	{
		index = vSatList[index];
		vSsatFat.push_back(index);

	}
	
	vSsatFat.push_back(vSatList[index]);
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
				//cout<<vSatList.front()<<" \t"<<vSatList.back()<<'\t';
				
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
	while(vSatList[index] != ENDOFCHAIN )
	{
		index = vSatList[index];
		vDirFat.push_back(index);
		
	}
	
	vDirFat.push_back(vSatList[index]);

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
	vector<DirectoryEntry> lDirList;
	for (i=0; i<(vDirFat.size()-1) ;i++)
	{
		BYTE  *SecBuf = new BYTE[SectorSize];
		IfReadFile(inStream,SecBuf,GetOffestFremSid(vDirFat[i]),SectorSize);
		PDirectoryEntry pDirEntry = PDirectoryEntry(SecBuf);
		for (int j = 0;j<4;j++)
		{
				DirectoryEntry  tempDirEntry = pDirEntry[j];
				lDirList.push_back(tempDirEntry);

		}
	
	
		
		delete []SecBuf;
	}

	vector< DirectEntry> vFatOfDirEntry;
	
	/*
	 *processing  DirectoryEntry list
	 */
	for(i = 0;i<lDirList.size();i++)
	{
		ProcessDirEntry(&lDirList[i],vSatList,vSsatList,vFatOfDirEntry,i)	;
		memset(&stDirectEntry,0,sizeof(stDirectEntry));
	}
	


	delete []lpHeaderBuf;

	inStream.close();
	return 0;

}

bool ProcessDirEntry(PDirectoryEntry pDirEntry,vector<SECT> & slist,vector<int> & sslist,vector<DirectEntry >& vFatOfDirEntry,int i)
{
	string DirName;
	unsigned int index;
	
	if (pDirEntry->_cb == 0)
	{
		return false;
	}else
	{
		char buf[256] ={0,0};
		wcstombs(buf,(wchar_t *)pDirEntry->_ab,(size_t)pDirEntry->_cb);
		DirName = buf;
		stDirectEntry.strDirName = DirName;
		/************************************************************************/
		/* 是否是短流                                                                     */
		/************************************************************************/
		if((pDirEntry->_ulSize < MaxMiniStreamSize)&&(pDirEntry->_mse != STGTY_ROOT)) 
		{
			stDirectEntry.vSidList.push_back(pDirEntry->_sectStart);
			index = pDirEntry->_sectStart;
			while(sslist[index] != ENDOFCHAIN)
			{
				index=sslist[index];
				stDirectEntry.vSidList.push_back(index);
			}
			stDirectEntry.vSidList.push_back(sslist[index]);

		}else
		{
			stDirectEntry.vSidList.push_back(pDirEntry->_sectStart);
			index = pDirEntry->_sectStart;
			while(slist[index] != ENDOFCHAIN)
			{
				index=slist[index];
				stDirectEntry.vSidList.push_back(index);
			}
			stDirectEntry.vSidList.push_back(slist[index]);
		}
		cout<<DirName<<'\t';
		for (int j =0; j < stDirectEntry.vSidList.size();j++)
		{
			cout<<" \" \" <<DirName<<-FAT["<<j<<"] == ";
			cout<<hex<<stDirectEntry.vSidList[j]<<'\t';
		}
		cout<<endl;
		vFatOfDirEntry.push_back(stDirectEntry);
	
		
	}
	
	return true;

}

SECT ReadMastList(ifstream &inStream,vector<SECT> &vMsat,SECT sid,size_t size)
{
	SECT tempSid = 0;
	BYTE  * MsatBuf = new BYTE [512];
	IfReadFile(inStream,MsatBuf,GetOffestFremSid(sid),size);
	SECT * temSidList = (SECT *)MsatBuf;
	if (temSidList[127] != ENDOFCHAIN)
	{
		vMsat.insert(vMsat.end(),temSidList,(temSidList+127));
		tempSid = temSidList[127];
	}else{
		vMsat.insert(vMsat.end(),temSidList,(temSidList+127));
		tempSid = temSidList[127];
	}
	
	delete []MsatBuf;
	return tempSid;
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
	for (int i = 0;i<109;i++)
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

