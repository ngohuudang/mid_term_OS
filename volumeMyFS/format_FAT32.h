#pragma once
#include <stdint.h>
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <iostream>
#include <vector>
#include <map>
#include <queue>
#define _CRT_SECURE_NO_WARNINGS
#define BYTE_PER_SEC 512
#define EOF 268435455
#pragma warning(disable : 4996)
using namespace std;
struct FAT32
{
    BYTE BS_JmpBoot[3];		    // 0-2		Lenh nhay qua vung thong so FAT32
    BYTE BS_OEMName[8];			// 3-A		version
    BYTE BPB_BytsPerSec[2];		// B-C	    so Byte tren moi sector
    BYTE BPB_SecPerClus[1];		// D		so Sector tren moi cluster          S_C
    BYTE BPB_RsvdSecCnt[2];		// E-F  	sectors thuoc bootsector            S_B
    BYTE BPB_NumFATs[1];		// 10		so bang FAT                         N_F
    BYTE BPB_ClusSavePass[2];	// 11-12	cluster luu password volume
    BYTE BPB_TotSec[2];			// 13-14	khong su dung
    BYTE BPB_Media[1];			// 15		loai thiet bi
    BYTE BPB_SecPerFAT[2];		// 16-17	khong su dung
    BYTE BPB_SecPerTrk[2];		// 18-19	so Sector tren moi track
    BYTE BPB_NumHeads[2];		// 1A-1B	so luong dau doc
    BYTE BPB_HiddSec[4];        // 1C-1F    khong su dung
    BYTE BPB_TotSec32[4];       // 20-23    kick thuoc volume                   S_V
    BYTE BPB_FATSz32[4];        // 24-27    kick thuoc moi bang FAT             S_F
    BYTE BPB_ExtFlags[2];       // 28-29    khong su dung    
    BYTE BPB_FSVer[2];          // 2A-2B    khong su dung
    BYTE BPB_RootClus[4];       // 2C-2F    cluster bat dau cua RDET
    BYTE BPB_FSInfo[2];         // 30-31    khong su dung
    BYTE BPB_BkBootSec[2];      // 32-33    khong su dung
    BYTE BPB_Reserved[12];      // 34-3F    khong su dung
    BYTE BS_DrvNum[1];          // 40       khong su dung
    BYTE BS_Reserved[1];        // 41       khong su dung
    BYTE BS_BootSig[1];         // 42       khong su dung
    BYTE BS_VolID[4];           // 43-46    khong su dung
    BYTE BS_VolLab[11];         // 47-51    volume label
    BYTE BS_FilSysType[8];      // 52-59    la chuoi FAT32
    BYTE BS_BootCode32[420];    // 5A-1FD   khong su dung
    BYTE BS_BootSign[2];        // 1FE-1FF  dau hieu ket thuc bootsector
};

int calculate_sector_of_file(string fileName);

bool checkPassword_file(string password, string file, int offset, LPCWSTR fileName);

bool checkPassword_volume(string password, LPCWSTR fileName);

bool copy_file_from_volume(string fileSource, string fileDes, FAT32 bootSector);

bool copy_file_to_volume(string fileSource, LPCWSTR fileDes, FAT32 bootSector);

BYTE* copy_sector(BYTE* buffer);

int count_empty_cluster(BYTE* buffer, unsigned int n);

FAT32 create_bootSector(unsigned int n);

BYTE* create_main_entry_file(string fileName, int mode, int cluster_begin, int fileSize);

void create_MyFS_with_byte_zero(string myFile, unsigned int size);

BYTE* create_offsets_zero(unsigned int n);

BYTE* dec_to_hex_with_little_endian(unsigned int num, int n_byte);

int find_end_file(BYTE* buffer,BYTE end);

int get_and_update_clus_RDET(int S_B, int S_F, int N_F, int Clus_RDET, int S_C, LPCWSTR fileName);

FAT32 get_bootSector(string fileName);

int get_clus_i_in_sector(BYTE* buffer, string fileName_entry);

int get_clus_in_FAT(int offset, LPCWSTR fileName);

int get_fileSize(string fileName);

int get_fileSize_in_volume(BYTE* buffer, string fileName_entry);

void get_info(FAT32 bootSector, int& S_B, int& S_F, int& N_F, int& Clus_Begin_RDET, int& S_C);

int get_mode_file(string file, int offset, LPCWSTR fileName);

string getPassword_file(string file, int offset, LPCWSTR fileName);

string getPassword_volume(int offset, LPCWSTR fileName);

queue<string> get_path(string fileName);

pair<int, int> get_startPos_and_offset_file(string file, int offset, LPCWSTR fileName);

string hex_file_to_string(BYTE* byte, int start);

string hex_sector_to_string(BYTE* buffer, int start, int end);

void init_FAT(int S_B, int S_F, int N_F, LPCWSTR fileName);

bool is_empty_entry(BYTE* buffer);

bool is_empty_buffer(BYTE* buffer,int starPos,BYTE empty);

bool is_entry_in_sector(BYTE* buffer, string fileName_entry);


bool is_exsist_file_normal(string fileName);

bool is_in_list_file(vector<string>file_used, string fileName);

void lock_files(vector<string>file_used, int offset, LPCWSTR fileName);

void lock_or_unlock_sector(string password, int offset, LPCWSTR fileName, int start, int end);

void listed_file(int offset, LPCWSTR fileName);

BYTE* readBlock(int offset, LPCWSTR fileName);

BYTE* readFile_normal(int offset, string fileName);

bool remove_file(string fileSource, FAT32 bootSector, LPCWSTR fileName);

BYTE* replace_byte(BYTE* buffer, int cluster_index, int clus_of_file, int mode);

int reverseByte(BYTE* byte, unsigned int count);

bool savePassword_file(string password, int offset, int startPos, LPCWSTR fileName);

bool savePassword_volume(string password, int offset, LPCWSTR fileName);

void set_mode_file(string fileSource, int mode, int offset, LPCWSTR fileName);

bool setPassword_file(string password, string file, int offset, LPCWSTR fileName);

bool setPassword_volume(string password, LPCWSTR fileName, FAT32 bootSector);

BYTE* string_to_hex(string name);

void update_FAT(int S_B, int offset, int cluster_index, int clus_of_file, LPCWSTR fileName);

bool update_zero_in_FAT(int offset, int clus_index_in_RDET, LPCWSTR fileName);

bool writeBlock(int offset, BYTE* buffer, LPCWSTR fileName);

bool writeBlock_tail(int offset, BYTE* buffer, LPCWSTR fileName);

bool write_boolSector(FAT32 bootSector, LPCWSTR fileName);

bool write_entry_to_volume(int offset, BYTE* entry, LPCWSTR fileName);

bool writeFile_normal(int offset, BYTE* buffer, string fileName);

bool write_tail_file(int offset, BYTE* buffer, string fileName);

BYTE* xor_password(BYTE* buffer, string password, int start, int end);

//
//bool is_clus_i_in_sec(BYTE* buffer, int cluster_index);

void print_info_bootSector(FAT32 bootSector);
//vector<int> get_cluster_index(int S_B, int S_F, int N_F, int Clus_Begin_RDET, int S_C, LPCWSTR fileName);
//bool is_in_array(vector<int >a, int n);
//
//vector<int> get_size_of_file(int offset, vector<int> cluster_index, LPCWSTR fileName);
//void restore_byte_in_RDET(int offset, LPCWSTR fileName, vector<int> cluster_index);
void print_buffer(BYTE* buffer);
//
//void test();
//void print_sector_in_offset(int offset, LPCWSTR fileName);
//BYTE* string_to_hex(string name);
//
//string get_fileName_from_path(string myFile);
