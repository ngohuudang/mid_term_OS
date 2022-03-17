#include "format_FAT32.h"
//tinh xem kick thuoc file chiem bao nhieu sector
int calculate_sector_of_file(string fileName) {
	int offset = 0;
	BYTE* buffer;
	while (1) {
		buffer = readFile_normal(offset, fileName);
		if (buffer[0] == BYTE(205) && buffer[1] == BYTE(205)) {
			break;
		}
		offset += BYTE_PER_SEC;
	}
	return offset / BYTE_PER_SEC;
}
//kiem tra password nhap vao co dung khong
bool checkPassword_file(string password, string file, int offset, LPCWSTR fileName) {
	BYTE* buffer;
	pair<int, int> startPos_offset = get_startPos_and_offset_file(file, offset, fileName);
	buffer = readBlock(startPos_offset.second, fileName);
	int start = startPos_offset.first + 16;		//bat dau tu dong thu 2 cua entry
	BYTE* new_buffer = xor_password(buffer, password, start, start + 48);//xor dong 2,3,4 cua entry
	string pass_file = hex_sector_to_string(new_buffer, start + 17, start + 48);
	if (pass_file == password)
		return true;
	return false;
}
//kiem tra password nhap vao co dung khong
bool checkPassword_volume(string password, LPCWSTR fileName) {
	BYTE* buffer;
	bool flag;
	buffer = readBlock(0, fileName);
	BYTE* new_buffer = xor_password(buffer, password, 0, BYTE_PER_SEC);
	BYTE clus_pass[2];
	clus_pass[0] = new_buffer[17];
	clus_pass[1] = new_buffer[18];
	if (reverseByte(clus_pass, 2) != 2)				//cluster save password la 2
		return false;
	BYTE sb[2];
	sb[0] = new_buffer[14];
	sb[1] = new_buffer[15];
	int S_B = reverseByte(sb, 2);
	BYTE sf[4];
	sf[0] = new_buffer[36];
	sf[1] = new_buffer[37];
	sf[2] = new_buffer[38];
	sf[3] = new_buffer[39];
	int S_F = reverseByte(sf, 4);
	BYTE nf[1];
	nf[0] = new_buffer[16];
	int N_F = reverseByte(nf, 1);
	string pass_sec = getPassword_volume((S_B + S_F * N_F) * BYTE_PER_SEC, fileName);
	if (password == pass_sec)
		return true;
	return false;
}
//copy file tu volume ra ngoai
bool copy_file_from_volume(string fileSource, string fileDes, FAT32 bootSector) {
	queue<string> list_path = get_path(fileSource);
	string fileName_volume = list_path.front();//lay ten volume: MyFS.dat
	list_path.pop();//loai bo ten volume
	BYTE* buffer;
	int S_B;
	int S_F;
	int N_F;
	int Clus_Begin_RDET;
	int S_C;
	bool flag;
	int clus_index = 0;
	int offset = 0;
	int fileSize = 0;
	get_info(bootSector, S_B, S_F, N_F, Clus_Begin_RDET, S_C);
	//get cluster index and filesize of file
	while (1) {
		buffer = readFile_normal((S_B + S_F * N_F + (Clus_Begin_RDET - 2) * S_C) * BYTE_PER_SEC + offset,
			fileName_volume);
		if (buffer[0] == BYTE(0))
			return false;
		clus_index = get_clus_i_in_sector(buffer, list_path.back());
		fileSize = get_fileSize_in_volume(buffer, list_path.back());
		if (clus_index > 0 && fileSize>0)
			break;
		offset += BYTE_PER_SEC;
	}
	// write content file to file des
	offset = 0;
	for (int i = 0; i < fileSize / BYTE_PER_SEC; i++) {
		//file name la kieu string nen dung ham doc file binh thuong(FILE*f)
		buffer = readFile_normal((S_B + S_F * N_F + (clus_index - 2) * S_C) * BYTE_PER_SEC + offset,
			fileName_volume);
		flag = writeFile_normal(offset, buffer, fileDes);
		offset += BYTE_PER_SEC;
	}
	buffer = readFile_normal((S_B + S_F * N_F + (clus_index - 2) * S_C) * BYTE_PER_SEC + offset,
		fileName_volume);
	flag = write_tail_file(offset, buffer, fileDes);
	return true;
}
//copy file tu ngoai vao volume
bool copy_file_to_volume(string fileSource, LPCWSTR fileDes, FAT32 bootSector) {
	BYTE* buffer;
	int S_B;
	int S_F;
	int N_F;
	int Clus_Begin_RDET;
	int S_C;
	bool flag;
	get_info(bootSector, S_B, S_F, N_F, Clus_Begin_RDET, S_C);//lay thong tin bootsector
	int sec_of_file = calculate_sector_of_file(fileSource);//so sector ma file chiem
	int clus_of_file = (sec_of_file / S_C) + 1;//so cluster ma file chiem
	int index = -1;
	int offset = 0;
	int offset_RDET;
	int cluster_empty=0;
	//get offset RDET
	offset_RDET =get_and_update_clus_RDET(S_B, S_F, N_F, Clus_Begin_RDET, S_C, fileDes);
	//update FAT
	int clus_i_begin=-1;
	int offset_i_begin = 0;
	for (int i = 0; i < S_F; i++) {
		int n;
		buffer = readBlock(S_B * BYTE_PER_SEC+ offset, fileDes);
		for (int j = 0; j < BYTE_PER_SEC; j += 4) {
			if (buffer[j] == BYTE(0)) {
				// tim khu vuc trong(EOF) du lon de chua file
				n= count_empty_cluster(buffer, j);
				if (clus_i_begin == -1) {
					clus_i_begin = j;			//vi tri bat dau cap nhat FAT
					offset_i_begin = offset;	//sector chua clus_i_begin
				}
				cluster_empty += n;			
				if (cluster_empty >= clus_of_file) {
					index = clus_i_begin;
					break;
				}
				j += n * 4;
				if (j != 512)
					clus_i_begin = -1;//reset lai vi khong du cluster
			}
		}
		if (index != -1)
			break;
		offset += BYTE_PER_SEC;
	}
	if (index == -1) {	//kich thuoc khong du luu file
		return false;
	}
	update_FAT(S_B, offset_i_begin, index / 4, clus_of_file, fileDes);
	//ghi thong tin file vao RDET
	queue<string> file_path = get_path(fileSource);
	int clus_index_in_RDET = (offset_i_begin + index) / 4;//vi tri cluter ma noi dung file se duoc ghi
	string name = file_path.back();//lay file
	int fileSize = get_fileSize(fileSource);//lay kick thuoc file
	BYTE* main_entry = create_main_entry_file(name, 2, clus_index_in_RDET, fileSize);//tao 1 main_entry
	BYTE* entry = create_offsets_zero(64);//gan entry =0
	for (int i = 0; i < 32; i++) {
		entry[i] = main_entry[i];
	}
	entry[32] = BYTE(229);				// E5
	flag = write_entry_to_volume(offset_RDET,entry,fileDes);
	// data area
	offset = 0;
	for (int i = 0; i < fileSize / BYTE_PER_SEC; i++) {
		buffer = readFile_normal(offset, fileSource);	//doc noi dung file source
		flag = writeBlock((S_B + S_F * N_F + (clus_index_in_RDET - 2) * S_C) * BYTE_PER_SEC + offset,
			buffer,
			fileDes);
		if (!flag)
			return false;
		offset += BYTE_PER_SEC;
	}
	buffer = readFile_normal(offset, fileSource);	//doc noi dung file source
	flag = writeBlock_tail((S_B + S_F * N_F + (clus_index_in_RDET - 2) * S_C) * BYTE_PER_SEC + offset,
		buffer,
		fileDes);
	if (!flag)
		return false;
	return true;
}
//copy sector
BYTE* copy_sector(BYTE* buffer) {
	BYTE* new_buffer = new BYTE[BYTE_PER_SEC];
	for (int i = 0; i < BYTE_PER_SEC; i++) {
		new_buffer[i] = buffer[i];
	}
	return new_buffer;
}
//dem so cluter trong sector cua FAT
int count_empty_cluster(BYTE* buffer, unsigned int n) {
	int count = 0;
	for (int i = n ; i < BYTE_PER_SEC; i += 4) {
		if (buffer[i] == BYTE(0))
			count++;
		else
			break;
	}
	return count;
}
//tao bootsector
//trong cau truc file FAT32 dung thiet ke volume MyFS.dat cp nhung Byte khong dung toi
//thay vi gan =0 ta gan gia tri rac(doc bootsector tu 1 volume khac co cau trau FAT32 
//roi chinh sua cac gia tri can thiet)
//muc dich la khi set password(xor bootsector voi password),se kho doan ra duoc password file
FAT32 create_bootSector(unsigned int n) {
	FAT32  bootSector;
	FILE* f = fopen("bootsector.dat", "rb");
	if (f == NULL)
	{
		printf("Could not open device.\n");
		return FAT32();
	}
	fread(&bootSector, sizeof(FAT32), 1, f);
	fclose(f);
	int S_F = (1024 * 1024 * n - 1024) / 532992;	//chung minh cong thuc o phan bao cao do an
	BYTE* byte_sf = dec_to_hex_with_little_endian(S_F, 4);
	for (int i = 0; i < 4; i++)
		bootSector.BPB_FATSz32[i] = byte_sf[i];
	bootSector.BPB_RsvdSecCnt[0] = BYTE(2);			//size of bootsector area
	bootSector.BPB_RsvdSecCnt[1] = BYTE(0);
	bootSector.BPB_RootClus[0] = BYTE(3);	//RDET start =3
	bootSector.BPB_ClusSavePass[0] = BYTE(2);	//cluster save password =2
	bootSector.BPB_NumFATs[0] = BYTE(1);	//number FAT =1
	return bootSector;

}
//tao 1 entry chinh luu ten file,ten mo rong,kick thuc,cluster bat dau
BYTE* create_main_entry_file(string fileName, int mode, int cluster_begin, int fileSize) {
	BYTE* entry = create_offsets_zero(32);
	if (mode == 4) {			//directory se chi co ten file,khong co ten mo rong
		BYTE* name = string_to_hex(fileName);
		for (int i = 0; i < fileName.length(); i++) {
			entry[i] = name[i];
		}
		for (int i = fileName.length(); i < 11; i++) {
			entry[i] = BYTE(32);
		}
	}
	else {//khong phai directory se co ten file mo rong
		int n = fileName.find_first_of(".");
		BYTE* name = string_to_hex(fileName.substr(0, n));
		BYTE* tail_file = string_to_hex(fileName.substr(n + 1, fileName.length() - n - 1));
		BYTE* size = dec_to_hex_with_little_endian(fileSize, 4);
		for (int i = 0; i < n; i++) {//save n ki tu ten vao n byte dau
			entry[i] = name[i];
		}
		for (int i = n; i < 8; i++) {//save khang trang vao 8-n byte con lai
			entry[i] = BYTE(32);	//khoang trang
		}
		for (int i = 8; i < 8 + fileName.length() - n - 1; i++) {//save  duoi file mo rong
			entry[i] = tail_file[i - 8];
		}
		for (int i = 8 + fileName.length() - n - 1; i < 11; i++) {//save khoang trang vao 3-n(duoi_file)
			entry[i] = BYTE(32);
		}
		//save kich thuoc file
		entry[28] = size[0];
		entry[29] = size[1];
		entry[30] = size[2];
		entry[31] = size[3];
	}
	//save mode file(0->6)
	entry[12] = BYTE(mode);
	BYTE* clus = dec_to_hex_with_little_endian(cluster_begin, 2);
	//save cluster chua noi dung file
	entry[26] = clus[0];
	entry[27] = clus[1];
	return entry;
}
//tao 1 volume trong voi tat ca cac byte =0
void create_MyFS_with_byte_zero(string myFile, unsigned int size) {
	FILE* f = fopen(myFile.c_str(), "wb+");
	BYTE buffer[BYTE_PER_SEC];
	for (int i = 0; i < BYTE_PER_SEC; i++) {
		buffer[i] = BYTE(0);
	}

	for (int i = 0; i < (size * 1048576) / 512; i++) {	//1 MB=1048576B
		fwrite(buffer, BYTE_PER_SEC, 1, f);
	}
	fclose(f);
}
//tao n byte 0
BYTE* create_offsets_zero(unsigned int n) {
	BYTE* offsets = new BYTE[n];
	for (int i = 0; i < n; i++) {
		offsets[i] = BYTE(0);
	}
	return offsets;
}
//chuyen 1 so he 10 sang hexa va luu theo dang little endian 
BYTE* dec_to_hex_with_little_endian(unsigned int num, int n_byte) {
	BYTE* byte = create_offsets_zero(n_byte);
	while (1) {
		unsigned int dec = num;
		int n = 0;
		while (dec > 256) {
			dec = dec >> 8;
			n++;
		}
		byte[n] = BYTE(dec);
		num = num - (dec << (8 * n));
		if (n == 0)
			break;
	}
	return byte;
}
//trong cluster luu noi dung file,khi thay offset =0x00 la ket thuc ghi file
int find_end_file(BYTE* buffer,BYTE end) {		//find 0x00
	for (int i = 0; i < BYTE_PER_SEC; i++) {
		if (buffer[i] == end) {
			if(is_empty_buffer(buffer,i, end))
				return i;
		}
	}
	return BYTE_PER_SEC;
}
//kiem tra xem RDET full hay chua
//chua full thi tra ve offset cua sector con trong
//full roi thi kiem 1 cluster khac lam RDET va ghi cluster do vao entry cuoi cua RDET truoc do
int get_and_update_clus_RDET(int S_B, int S_F, int N_F, int Clus_RDET, int S_C, LPCWSTR fileName) {
	BYTE* buffer;
	int index = -1;
	bool flag;
	int offset = (S_B + S_F * N_F + (Clus_RDET - 2) * S_C) * BYTE_PER_SEC;
	for (int i = 0; i < 8; i++) {
		buffer = readBlock(offset, fileName);
		if (buffer[0] == BYTE(0))
			return offset;
		offset += BYTE_PER_SEC;
	}
	//truong hop sector thu 8 cua volume co noi dung
	for (int i = 0; i < BYTE_PER_SEC - 64; i += 64) {	//entry cuoi de ghi dau hieu RDET full
		if (buffer[i] == BYTE(0))
			return offset;
	}
	int offset_FAT = 0;
	if (buffer[448] == BYTE(0)) {		//byte dau tien cua entry cuoi trong RDET
		BYTE* buffer_FAT;
		while (1) {
			buffer_FAT = readBlock(S_B * BYTE_PER_SEC + offset_FAT, fileName);
			for (int i = 0; i < BYTE_PER_SEC; i += 4) {
				if (buffer_FAT[i] == BYTE(0)) {
					// tim khu vuc trong(EOF) du lon de luu next_RDET
					if (count_empty_cluster(buffer_FAT, i) >= 1) {
						index = i;
						break;
					}
				}
			}
			if (index != -1)
				break;
			offset_FAT += BYTE_PER_SEC;
		}
		update_FAT(S_B, offset_FAT, index / 4, 1, fileName);
		//update last sector in RDET
		buffer[448] = BYTE(229);	//gan 0xE5 vao dau entry
		BYTE* clus_next_RDET = dec_to_hex_with_little_endian(index / 4, 2);
		buffer[474] = clus_next_RDET[0];
		buffer[475] = clus_next_RDET[1];
		flag = writeBlock(offset, buffer, fileName);// cap nhat lai sector cuoi cua RDET
	}
	BYTE b[2];
	b[0] = buffer[474];
	b[1] = buffer[475];
	int cluster_next_of_RDET = reverseByte(b, 2);
	return get_and_update_clus_RDET(S_B, S_F, N_F, cluster_next_of_RDET, S_C, fileName);
}
//lay thong tin bootsector
FAT32 get_bootSector(string fileName) {
	FILE* f;
	FAT32 bootSector;
	f = fopen(fileName.c_str(), "rb");
	if (f == NULL)
	{
		printf("Could not open device.\n");
		exit(EXIT_FAILURE);
	}
	fread(&bootSector, sizeof(FAT32), 1, f);
	return bootSector;
}
// lay vi tri cluter chua noi dung file
int get_clus_i_in_sector(BYTE* buffer, string fileName_entry) {
	for (int i = 0; i < BYTE_PER_SEC; i += 32) {
		if (fileName_entry == hex_file_to_string(buffer, i)) {
			BYTE clus[2];// trong entry chinh,byte thu 26,27 tinh tu dau entry se luu vi tri cluster
			clus[0] = buffer[i + 26];
			clus[1] = buffer[i + 27];
			return reverseByte(clus, 2);
		}
	}
	return -1;
}
//lay so cluster dang su dung
int get_clus_in_FAT(int offset, LPCWSTR fileName) {
	BYTE* buffer;
	int n_cluster = 0;
	while (1) {
		buffer = readBlock(offset, fileName);
		if (buffer[0] == BYTE(0))
			break;
		n_cluster += 128;//cong truoc 128.het vong lap se tru lai
		offset += BYTE_PER_SEC;
	}
	n_cluster -= 128;	//truong hop sector khong dung het 512 byte de luu cluster index
	buffer = readBlock(offset, fileName);
	for (int i = 0; i < BYTE_PER_SEC; i += 4) {
		if (buffer[i] == BYTE(0))
			break;
		n_cluster++;
	}
	return n_cluster;
}
//lay kick thuoc file tu ben ngoai
int get_fileSize(string fileName) {
	int offset = 0;
	BYTE* buffer;
	//doc tung sector(block)
	while (1) {
		buffer = readFile_normal(offset, fileName);
		if (buffer[0] == BYTE(205) && buffer[1] == BYTE(205)) {
			break;
		}
		offset += BYTE_PER_SEC;
	}
	int n = 0;
	offset -= BYTE_PER_SEC;//day la sector chua phan ket thuc file(kick thuoc se <=512B)
	buffer = readFile_normal(offset, fileName);
	for (int i = 0; i < BYTE_PER_SEC; i++) {
		if (buffer[i] == BYTE(205)) {//dau hieu ket thuc file doc tu ngoai vao
			break;
		}
		n++;
	}
	return n + offset;
}

int get_fileSize_in_volume(BYTE* buffer, string fileName_entry) {
	for (int i = 0; i < BYTE_PER_SEC; i += 32) {
		if (fileName_entry == hex_file_to_string(buffer, i)) {
			BYTE byte[4];	//kich thuoc o 4 byte cuoi cua entry
			int n = 0;
			byte[n++] = buffer[i + 28];
			byte[n++] = buffer[i + 29];
			byte[n++] = buffer[i + 30];
			byte[n++] = buffer[i + 31];
			return reverseByte(byte, 4);
		}
	}
	return -1;
}
//lay thong tin 
//sectors thuoc bootsector	S_B
//kick thuoc moi bang FAT	S_F
//so bang FAT				N_F
//so sector tren moi cluter	S_C
//cluter chua RDET			Clus_Begin_RDET
void get_info(FAT32 bootSector, int& S_B, int& S_F, int& N_F, int& Clus_Begin_RDET, int& S_C) {
	S_B = reverseByte(bootSector.BPB_RsvdSecCnt, 2);
	S_F = reverseByte(bootSector.BPB_FATSz32, 4);
	N_F = reverseByte(bootSector.BPB_NumFATs, 1);
	Clus_Begin_RDET = reverseByte(bootSector.BPB_RootClus, 4);
	S_C = reverseByte(bootSector.BPB_SecPerClus, 1);
}
//lay thuoc tinh file
int get_mode_file(string fileSource, int offset, LPCWSTR fileName) {
	queue<string> list_path = get_path(fileSource);
	string file = list_path.back();
	BYTE* buffer;
	pair<int, int> startPos_offset = get_startPos_and_offset_file(file, offset, fileName);
	buffer = readBlock(startPos_offset.second, fileName);
	BYTE b[1];
	b[0] = buffer[startPos_offset.first + 11];
	return reverseByte(b, 1);
}
//lay password file o trong volume
string getPassword_file(string file, int offset, LPCWSTR fileName) {
	BYTE* buffer;
	bool flag;
	pair<int, int> startPos_offset = get_startPos_and_offset_file(file, offset, fileName);
	buffer = readBlock(startPos_offset.second, fileName);
	int start = startPos_offset.first + 33;		//bat dau tu dong thu 3 offset 1 cua entry
	string pass_file = hex_sector_to_string(buffer, start, start + 31);
	return pass_file;
}
//lay password volume o trong volume MyFS.dat
string getPassword_volume(int offset, LPCWSTR fileName) {
	BYTE* buffer;
	buffer = readBlock(offset, fileName);
	string pass_sec = hex_sector_to_string(buffer, 0, 32);
	return pass_sec;
}
//tach(split) duong dan ra tung phan
queue<string> get_path(string fileName) {
	queue<string> list_name;
	while (1) {
		int n = fileName.find_first_of("\\");
		if (n == -1) {
			list_name.push(fileName);
			break;
		}
		string name = fileName.substr(0, n);
		list_name.push(name);
		fileName = fileName.substr(n + 1, fileName.length() - n - 1);
	}
	return list_name;
}
//lay vi tri bat dau cua entry va sector chua thong tin file trong volume 
pair<int, int> get_startPos_and_offset_file(string fileSource, int offset, LPCWSTR fileName) {
	BYTE* buffer;
	string name;
	int new_offset = offset;
	int startPos = 0;
	bool flag;
	queue<string> list_path = get_path(fileSource);
	string file = list_path.back();//lay ten file
	while (1) {
		buffer = readBlock(new_offset, fileName);
		if (buffer[0] == BYTE(0))
			break;
		for (int i = 0; i < BYTE_PER_SEC; i += 64) {
			if (buffer[i] != BYTE(229)) {
				name = hex_file_to_string(buffer, i);
				if (name == file) {
					return pair<int, int>(i, new_offset);
				}
			}
		}
		new_offset += BYTE_PER_SEC;
	}
	return pair<int, int>(0, 0);
}
//chuyen byte luu ten file,ten file mo rong thanh chuoi tu vi tri start
string hex_file_to_string(BYTE* byte, int start) {
	string name = "";
	for (int i = start; i < start + 8; i++) {
		if (byte[i] == BYTE(32))
			break;
		name += byte[i];
	}
	if (byte[start + 8] == BYTE(32))
		return name;
	name += ".";
	for (int i = start + 8; i < start + 11; i++) {
		if (byte[i] != BYTE(32)) {
			name += byte[i];
		}
	}
	return name;
}
//chuyen offset o sector thanh chuoi tu vi tri start toi end
string hex_sector_to_string(BYTE* buffer, int start, int end) {
	string str = "";
	for (int i = start; i < end; i++) {
		if (buffer[i] == BYTE(0))
			return str;
		str += buffer[i];
	}
	return str;
}
//khoi tao bao FAT voi cluster 0,1,2,3 se duoc khoi tao san
//cluter 0,1 khong su dung,cluster 2 luu pass,cluter 3 luu RDET
void init_FAT(int S_B, int S_F, int N_F, LPCWSTR fileName) {
	bool flag;
	BYTE* buffer = create_offsets_zero(512);
	BYTE* start = dec_to_hex_with_little_endian(268435448, 4);		//0xOFFFFFF8
	BYTE* next_start = dec_to_hex_with_little_endian(4294967295, 4);	//0xFFFFFFFF
	BYTE* clus_pass = dec_to_hex_with_little_endian(EOF, 4);		//0x0FFFFFFF
	BYTE* clus_REDET = dec_to_hex_with_little_endian(EOF, 4);		//0x0FFFFFFF
	for (int i = 0; i < 4; i++) {
		buffer[i] = start[i];
		buffer[i + 4] = next_start[i];
		buffer[i + 8] = clus_pass[i];
		buffer[i + 12] = clus_REDET[i];
	}
	for (int i = 0; i < N_F; i++) {
		flag = writeBlock((S_B + N_F * i) * BYTE_PER_SEC, buffer, fileName);
	}
}
//kiem tra entry co trong khong
bool is_empty_entry(BYTE* buffer) {
	for (int i = 0; i < BYTE_PER_SEC; i += 64) {
		if (buffer[i] == BYTE(0))
			return true;
	}
	return false;
}

//kiem tra sector co trong khong
bool is_empty_buffer(BYTE* buffer,int starPos,BYTE empty) {
	for (int i = starPos; i < BYTE_PER_SEC; i++) {
		if (buffer[i] != empty)
			return false;
	}
	return true;
}
//kiem tra entry co nam trong sector hay khong bang cach so sanh ten file
bool is_entry_in_sector(BYTE* buffer, string fileName_entry) {
	for (int i = 0; i < BYTE_PER_SEC; i += 32) {
		if (fileName_entry == hex_file_to_string(buffer, i))
			return true;
	}
	return false;
}
//kiem tra file co ton tai hay khong roi moi copy ra ngoai
bool is_exsist_file_normal(string fileName) {
	FILE* f = fopen(fileName.c_str(), "rb");
	if (f)
		return true;
	return false;
}
//kiem tra file co nam trong list file hay khong
bool is_in_list_file(vector<string>file_used, string fileName) {
	for (int i = 0; i < file_used.size(); i++) {
		if (fileName == file_used[i])
			return true;
	}
	return false;
}
//khoa lai cac file da su dung
void lock_files(vector<string>file_used, int offset, LPCWSTR fileName) {
	pair<int, int>starPos_offset;
	string password;
	for (int i = 0; i < file_used.size(); i++) {
		password = getPassword_file(file_used[i], offset, fileName);
		starPos_offset = get_startPos_and_offset_file(file_used[i], offset, fileName);
		if (password.length() > 0)//nhung file co password se dc gan lai thuoc tinh khoa
			set_mode_file(file_used[i], 6, starPos_offset.second, fileName);
		lock_or_unlock_sector(password, starPos_offset.second, fileName,
			starPos_offset.first + 16, starPos_offset.first + 64);
	}
}
//khoa hoac mo khoa sector tu vi tru start toi end bang cach XOR voi password
void lock_or_unlock_sector(string password, int offset, LPCWSTR fileName, int start, int end) {
	BYTE* buffer;
	bool flag;
	buffer = readBlock(offset, fileName);
	BYTE* new_buffer = xor_password(buffer, password, start, end);
	flag = writeBlock(offset, new_buffer, fileName);
}
//liet ke danh sach cai file,file nao co mat khau se khong thay kick thuoc file
void listed_file(int offset, LPCWSTR fileName) {
	BYTE* buffer;
	map<string, int> list;
	while (1) {
		buffer = readBlock(offset, fileName);
		if (buffer[0] == BYTE(0))
			break;
		for (int i = 0; i < BYTE_PER_SEC; i += 64) {
			if (buffer[i] == BYTE(0))
				break;
			if (buffer[i] == BYTE(229))		//file da xoa 0xE5
				continue;
			if (buffer[i + 26] != BYTE(0)) {
				for (int j = i; j < i + 8; j++) {
					if (buffer[j] == BYTE(32))
						break;
					cout << buffer[j];
				}
				if (buffer[i + 8] == BYTE(32)) {
					break;
				}
				cout << ".";
				for (int j = i + 8; j < i + 11; j++) {
					cout << buffer[j];
				}
				if (buffer[i + 11] == BYTE(6)) {
					cout << endl;
					continue;
				}
				cout << "\t\t";
				BYTE byte[4];
				int n = 0;
				byte[n++] = buffer[i + 28];
				byte[n++] = buffer[i + 29];
				byte[n++] = buffer[i + 30];
				byte[n++] = buffer[i + 31];
				cout << reverseByte(byte, 4) << " B" << endl;
			}
		}
		offset += BYTE_PER_SEC;
	}


}
//doc 1 sector trong volume
BYTE* readBlock(int offset, LPCWSTR fileName) {
	DWORD bytesRead;
	HANDLE hFile;
	bool flag;
	BYTE* buffer = new BYTE[BYTE_PER_SEC];
	hFile = CreateFile(fileName,
		GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		0,
		NULL);
	SetFilePointer(hFile, offset, NULL, FILE_BEGIN);
	flag = ReadFile(hFile, buffer, BYTE_PER_SEC, &bytesRead, NULL);
	CloseHandle(hFile);
	if(flag)
		return buffer;
	return NULL;
}
//doc 1 sector trong volume bang kieu doc file tong thuong (FILE*f)
BYTE* readFile_normal(int offset, string fileName) {
	FILE* f = fopen(fileName.c_str(), "rb");
	fseek(f, offset, SEEK_SET);
	BYTE* buffer = new BYTE[BYTE_PER_SEC];
	fread(buffer, BYTE_PER_SEC, 1, f);
	fclose(f);
	return buffer;
}
//xoa file
bool remove_file(string fileSource, FAT32 bootSector, LPCWSTR fileName) {
	queue<string> list_path = get_path(fileSource);
	string file = list_path.back();
	BYTE* buffer;
	int S_B;
	int S_F;
	int N_F;
	int Clus_Begin_RDET;
	int S_C;
	bool flag;
	get_info(bootSector, S_B, S_F, N_F, Clus_Begin_RDET, S_C);
	pair<int, int> startPos_offset = get_startPos_and_offset_file(file,
		(S_B + S_F * N_F + (Clus_Begin_RDET - 2) * S_C) * BYTE_PER_SEC,
		fileName);
	buffer = readBlock(startPos_offset.second, fileName);
	buffer[startPos_offset.first] = BYTE(229);	// gan 0xE5 cho dau entry
	BYTE b[2];
	b[0] = buffer[startPos_offset.first + 26];
	b[1] = buffer[startPos_offset.first + 27];
	int clus_index = reverseByte(b, 2);//lay vi tri cluster chua file
	for (int i = 0; i < N_F; i++) {
		//cho cac cac byte quan ly cluter index = 0
		flag = update_zero_in_FAT((S_B + i * S_F) * BYTE_PER_SEC, clus_index, fileName);
		if (!flag)
			return false;
	}
	flag = writeBlock(startPos_offset.second, buffer, fileName);
	return flag;
}
//cap nhat lai bang FAT theo cluter index
//cluter of file la so cluster chua file
//dung de quy de luu
BYTE* replace_byte(BYTE* buffer, int cluster_index_of_file, int clus_of_file,int mode) {
	int cluster_index = cluster_index_of_file % 128;
	if (clus_of_file == 1) {
		if (mode == 0) {
			buffer[4 * cluster_index + 3] = BYTE(15);	//0F
			buffer[4 * cluster_index + 2] = BYTE(255);	//FF
			buffer[4 * cluster_index + 1] = BYTE(255);	//FF
			buffer[4 * cluster_index] = BYTE(255);		//FF
			return buffer;
		}
		int n = (cluster_index_of_file + 1) / (256 * 256 * 256);//+1 de luu 4byte tiep theo
		buffer[4 * cluster_index + 3] = BYTE(n);
		int n_mode = (cluster_index_of_file + 1) % (256 * 256 * 256);
		n = n_mode / (256 * 256);
		buffer[4 * cluster_index + 2] = BYTE(n);
		n_mode = n_mode % (256 * 256);
		n = n_mode / 256;
		buffer[4 * cluster_index + 1] = BYTE(n);
		n_mode = n_mode % 256;
		buffer[4 * cluster_index] = BYTE(n_mode);
		return buffer;
	}
	//cso cluster chua file >=2
	int n = (cluster_index_of_file + 1) / (256 * 256 * 256);//+1 de luu 4byte tiep theo
	buffer[4 * cluster_index + 3] = BYTE(n);
	int n_mode = (cluster_index_of_file + 1) % (256 * 256 * 256);
	n = n_mode / (256 * 256);
	buffer[4 * cluster_index + 2] = BYTE(n);
	n_mode = n_mode % (256 * 256);
	n = n_mode / 256;
	buffer[4 * cluster_index + 1] = BYTE(n);
	n_mode = n_mode % 256;
	buffer[4 * cluster_index] = BYTE(n_mode);
	//print_buffer(buffer);
	return replace_byte(buffer, cluster_index_of_file + 1, clus_of_file - 1, mode);
}

//chuyen hexa sang decimal va can dao byte vi volume luu theo little endian
int reverseByte(BYTE* byte, unsigned int count)
{
	int result = 0;
	for (int i = count - 1; i >= 0; i--)
		result = (result << 8) | byte[i];

	return result;
}
//luu password cua file vao volume
bool savePassword_file(string password, int offset, int startPos, LPCWSTR fileName) {
	bool flag;
	BYTE* byte_pass = string_to_hex(password);//chuyen password than byte
	BYTE* buffer = readBlock(offset, fileName);
	//luu vao startPos+33.vi tri thu 1 cua entry phu(vi tri 0 se chua byte 0xE5)
	for (int i = startPos + 33; i < password.length() + startPos + 33; i++) {
		buffer[i] = byte_pass[i - startPos - 33];
	}
	if (password.length() == 0)
		buffer[startPos + 11] = BYTE(2);
	int n = password.length() + startPos + 33;
	while (n % 32 != 0) {			//cac byte con lai cua entry =0x00
		buffer[n] = BYTE(0);
		n++;
	}
	flag = writeBlock(offset, buffer, fileName);
	return flag;
}
//luu password cua volume vao volume
bool savePassword_volume(string password, int offset, LPCWSTR fileName) {
	bool flag;
	BYTE* byte_pass = string_to_hex(password); //chuyen password than byte
	BYTE* buffer = create_offsets_zero(BYTE_PER_SEC);//tao buffer moi
	//luu password vao buffer moi
	for (int i = 0; i < password.length(); i++) {
		buffer[i] = byte_pass[i];
	}
	//ghi de vao sector chua password
	flag = writeBlock(offset, buffer, fileName);
	return flag;
}
//thay doi thuoc tinh cua file
void set_mode_file(string fileSource, int mode, int offset, LPCWSTR fileName) {
	queue<string> list_path = get_path(fileSource);
	string file = list_path.back();
	BYTE* buffer;
	pair<int, int> startPos_offset = get_startPos_and_offset_file(file, offset, fileName);
	buffer = readBlock(startPos_offset.second, fileName);
	buffer[startPos_offset.first + 11] = BYTE(mode);
	bool flag = writeBlock(startPos_offset.second, buffer, fileName);
}
//them,thay doi password cua file
bool setPassword_file(string password, string file, int offset, LPCWSTR fileName) {
	bool flag;
	pair<int, int> startPos_offset = get_startPos_and_offset_file(file, offset, fileName);
	flag = savePassword_file(password, startPos_offset.second, startPos_offset.first, fileName);
	return flag;
}
// them,thay doi password cua volume
bool setPassword_volume(string password, LPCWSTR fileName, FAT32 bootSector) {

	int S_B;
	int S_F;
	int N_F;
	int Clus_Begin_RDET;
	int S_C;
	bool flag;
	int clus_index = 0;
	int offset = 0;
	get_info(bootSector, S_B, S_F, N_F, Clus_Begin_RDET, S_C);
	flag = savePassword_volume(password, (S_B + S_F * N_F) * BYTE_PER_SEC, fileName);
	return flag;
}
//chuyen chuoi thanh hexa
BYTE* string_to_hex(string name) {
	BYTE* byte_name = new BYTE[name.length()];
	for (int i = 0; i < name.length(); i++) {
		byte_name[i] = BYTE(int(name[i]));
	}
	return byte_name;
}
//cap nhat cac cluter vua thay doi trong FAT
void update_FAT(int S_B, int offset, int cluster_index, int clus_of_file, LPCWSTR fileName) {
	bool flag;
	BYTE* buffer;
	//int n_cluster = get_clus_in_FAT(S_B * BYTE_PER_SEC, fileName);
	int n_empty_clus;
	BYTE* new_buffer;
	while (1) {
		buffer = readBlock(S_B * BYTE_PER_SEC + offset, fileName);
		n_empty_clus = 128 - (cluster_index % 128);
		if (clus_of_file > n_empty_clus) {
			new_buffer = replace_byte(buffer, cluster_index, n_empty_clus,1);
			flag = writeBlock(S_B * BYTE_PER_SEC + offset, new_buffer, fileName);
		}
		else {
			new_buffer = replace_byte(buffer, cluster_index, clus_of_file,0);
			flag = writeBlock(S_B * BYTE_PER_SEC + offset, new_buffer, fileName);
			break;
		}
		clus_of_file -= n_empty_clus;
		cluster_index += n_empty_clus;
		offset += BYTE_PER_SEC;
	}
}
//cap nhat cac cluter chua cac file bi xoa trong FAT
bool update_zero_in_FAT(int offset, int clus_index_in_RDET, LPCWSTR fileName) {
	bool flag;
	bool is_EOF = false;
	BYTE* buffer;
	int new_offset = offset + (clus_index_in_RDET / 128) * BYTE_PER_SEC;	//offset chua clus_index
	int sec_index = clus_index_in_RDET % 128;
	while (1) {
		buffer = readBlock(new_offset, fileName);
		if (buffer == NULL)
			return false;
		for (int i = sec_index * 4; i < BYTE_PER_SEC; i += 4) {
			if (buffer[i] == BYTE(255) and buffer[i + 1] == BYTE(255) and buffer[i + 2] == BYTE(255) and buffer[i + 3] == BYTE(15)) {
				buffer[i] = BYTE(0);
				buffer[i + 1] = BYTE(0);
				buffer[i + 2] = BYTE(0);
				buffer[i + 3] = BYTE(0);
				is_EOF = true;
				break;
			}
			buffer[i] = BYTE(0);
			buffer[i + 1] = BYTE(0);
			buffer[i + 2] = BYTE(0);
			buffer[i + 3] = BYTE(0);
		}
		flag = writeBlock(new_offset, buffer, fileName);
		new_offset += BYTE_PER_SEC;
		sec_index = 0;
		if (is_EOF)
			break;
	}
	return flag;
}

bool writeBlock(int offset, BYTE* buffer, LPCWSTR fileName) {
	DWORD bytesRead;
	HANDLE hFile;
	bool flag;
	hFile = CreateFile(fileName,
		GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		0,
		NULL);
	SetFilePointer(hFile, offset, NULL, FILE_BEGIN);
	flag = WriteFile(hFile, buffer, BYTE_PER_SEC, &bytesRead, NULL);
	CloseHandle(hFile);
	return flag;
}
//ghi sector vao cuoi
bool writeBlock_tail(int offset, BYTE* buffer, LPCWSTR fileName) {
	DWORD bytesRead;
	HANDLE hFile;
	bool flag;
	hFile = CreateFile(fileName,
		GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		0,
		NULL);
	SetFilePointer(hFile, offset, NULL, FILE_BEGIN);
	int i = 0;
	for (i; i < BYTE_PER_SEC; i++) {
		if (buffer[i] == BYTE(205)) {
			if (is_empty_buffer(buffer, i, BYTE(205))) {
				for(int j=i;j< BYTE_PER_SEC;j++)
					buffer[j] = BYTE(0);
				break;
			}
		}
	}
	flag = WriteFile(hFile, buffer, BYTE_PER_SEC, &bytesRead, NULL);
	CloseHandle(hFile);
	return flag;
}
//ghi bootsSector
bool write_boolSector(FAT32 bootSector, LPCWSTR fileName) {
	DWORD bytesRead;
	HANDLE hFile;
	hFile = CreateFile(fileName,
		GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		0,
		NULL);
	SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
	bool flag = WriteFile(hFile, &bootSector, sizeof(bootSector), &bytesRead, NULL);
	CloseHandle(hFile);
	return flag;
}
//ghi entry vao volume
bool write_entry_to_volume(int offset, BYTE* entry, LPCWSTR fileName) {
	bool flag;
	BYTE* buffer;
	//tim entry trong de luu entry moi
	while (1) {
		buffer = readBlock(offset, fileName);
		if (is_empty_entry(buffer))
			break;
		offset += BYTE_PER_SEC;
	}
	for (int i = 0; i < BYTE_PER_SEC; i += 64) {
		if (buffer[i] == BYTE(0)) {
			for (int j = i; j < i + 64; j++) {
				buffer[j] = entry[j - i];
			}
			break;
		}
	}
	flag = writeBlock(offset, buffer, fileName);
	return flag;
}
//ghi file  theo kieu binh thuong FILE*f
bool writeFile_normal(int offset, BYTE* buffer, string fileName) {
	FILE* f = fopen(fileName.c_str(), "ab+");
	fseek(f, offset, SEEK_SET);
	fwrite(buffer, 1, BYTE_PER_SEC, f);
	fclose(f);
	return 1;
}
//ghi vao cuoi file,khi copy file tu volume ra ngoai se ghi vao cuoi file
bool write_tail_file(int offset, BYTE* buffer, string fileName) {
	FILE* f = fopen(fileName.c_str(), "ab+");
	fseek(f, offset, SEEK_SET);
	int n = find_end_file(buffer,BYTE(0));
	fwrite(buffer, 1, n, f);
	fclose(f);
	return 1;
}
//ma hoa thong tin bang cac xor sector voi passpwrd tu vi tri start toi end
BYTE* xor_password(BYTE* buffer, string password, int start, int end) {
	BYTE* pass = string_to_hex(password);
	BYTE* new_buffer = copy_sector(buffer);
	int len = password.length();
	if (len == 0)
		return new_buffer;
	for (int i = start; i < end; i += len) {
		int n = len < (end - i) ? len : (end - i);
		for (int j = i; j < i + n; j++) {
			new_buffer[j] = buffer[j] xor pass[j - i];
		}
	}
	return new_buffer;
}

//bool is_clus_i_in_sec(BYTE* buffer, int cluster_index) {
//	for (int i = 0; i < BYTE_PER_SEC; i += 64) {
//		BYTE byte[2];
//		byte[0] = buffer[i + 26];
//		byte[1] = buffer[i + 27];
//		if (reverseByte(byte, 2) == cluster_index)
//			return true;
//	}
//	return false;
//}

//vector<int> get_size_of_file(int offset, vector<int> cluster_index, LPCWSTR fileName) {
//	vector<int> size_of_file;
//	BYTE* buffer;
//	while (1) {
//		buffer = readBlock(offset, fileName);
//		if (buffer[0] == BYTE(0))
//			break;
//		for (int i = 0; i < 512; i += 32) {
//			if (buffer[i] == BYTE(0))
//				break;
//			if (buffer[i + 26] != BYTE(0)) {
//				BYTE clus[2];
//				clus[0] = buffer[i + 26];
//				clus[1] = buffer[i + 27];
//				int cluster_i = reverseByte(clus, 2);
//				if (is_in_array(cluster_index, cluster_i)) {
//					BYTE size[4];
//					size[0] = buffer[i + 28];
//					size[1] = buffer[i + 29];
//					size[2] = buffer[i + 30];
//					size[3] = buffer[i + 31];
//					size_of_file.push_back(reverseByte(size, 4));
//				}
//			}
//		}
//	}
//	return size_of_file;
//}
//void restore_byte_in_RDET(int offset, LPCWSTR fileName, vector<int> cluster_index) {
//	bool flag;
//	BYTE* buffer=create_offsets_zero(BYTE_PER_SEC);
//	BYTE* buffer_bootSector;
//	buffer_bootSector = readBlock(0, fileName);
//	if (!writeBlock(0, buffer, fileName)) {
//		cout << "error" << endl;
//		return;
//	}
//	while (1) {
//		buffer = readBlock(offset, fileName);
//		if (buffer[0] == BYTE(0))
//			break;
//		for (int i = 0; i < BYTE_PER_SEC; i += 32) {
//			if (buffer[i] == BYTE(0))
//				break;
//			if (buffer[i + 26] != BYTE(0)) {
//				BYTE clus[2];
//				clus[0] = buffer[i + 26];
//				clus[1] = buffer[i + 27];
//				int cluster = reverseByte(clus, 2);
//				for (int j = 0; j < cluster_index.size(); j++) {
//					if (cluster == cluster_index[j]) {
//						if (buffer[i] == BYTE(229)) {
//							buffer[i] = BYTE(70);	//1 ki tu khac e5,cho 70 de ra ki tu "F" nhu ban dau
//						}
//					}
//				}
//			}
//		}
//		flag = writeBlock(offset, buffer, fileName);
//		offset += BYTE_PER_SEC;
//	}
//	flag = writeBlock(0, buffer_bootSector, fileName);
//}

void print_buffer(BYTE* buffer) {
	for (int i = 0; i < BYTE_PER_SEC; i+=16) {
		for (int j = i; j < i + 16; j++) {
			printf("%02x ", buffer[j]);
		}
		printf("\n");
	}
}

//void test() {
//	BYTE* buffer;
//	bool flag;
//	LPCWSTR fileName = L"bootSector.dat";
//	string password = "dang123";
//	buffer = readBlock(0, fileName);
//	//print_buffer(buffer);
//	BYTE* new_buffer = xor_password(buffer, password,0,BYTE_PER_SEC);
//	//cout << "boot sector xor password" << endl;
//	//print_buffer(new_buffer);
//	//cout << "===============================================" << endl;
//	//flag = writeBlock(0, new_buffer, fileName);
//	//buffer = readBlock(0, fileName);
//	BYTE* new2_buffer = xor_password(new_buffer, password,0,512);
//	//cout << "boot sector xor password" << endl;
//	//print_buffer(new_buffer);
//	for (int i = 0; i < 512; i += 16) {
//		for (int j = i; j < i + 16; j++) {
//			printf("%02x ", buffer[j]);
//		}
//		cout << endl;
//		//for (int j = i; j < i + 16; j++) {
//		//	printf("%02x ", new_buffer[j]);
//		//}
//		//cout << endl;
//		for (int j = i; j < i + 16; j++) {
//			printf("%02x ", new2_buffer[j]);
//		}
//		cout << endl << endl;
//	}
//}

//void print_sector_in_offset(int offset, LPCWSTR fileName) {
//	BYTE* buffer = readBlock(offset, fileName);
//	print_buffer(buffer);
//}
// 
//string get_fileName_from_path(string myFile) {
//	int index = 0;
//	for (int i = myFile.length() - 1; i >= 0; i--) {
//		if (myFile[i] == '\\') {
//			index = i + 1;
//			break;
//		}
//	}
//	return myFile.substr(index, myFile.length() - index);
//}

void print_info_bootSector(FAT32 bootSector) {
	cout << "0\tBS_JmpBoot:\t" << bootSector.BS_JmpBoot[0];
	cout << bootSector.BS_JmpBoot[1];
	cout << bootSector.BS_JmpBoot[2] << endl;
	cout << "3\tBS_OEMName:\t" << bootSector.BS_OEMName << endl;
	cout << "B\tBPB_BytsPerSec:\t" << reverseByte(bootSector.BPB_BytsPerSec, 2) << endl;
	cout << "D\tBPB_SecPerClus:\t" << reverseByte(bootSector.BPB_SecPerClus, 1) << endl;
	cout << "E\tBPB_RsvdSecCnt:\t" << reverseByte(bootSector.BPB_RsvdSecCnt, 2) << endl;
	cout << "10\tBPB_NumFATs:\t" << reverseByte(bootSector.BPB_NumFATs, 1) << endl;
	cout << "11\tBPB_ClusSavePass:\t" << reverseByte(bootSector.BPB_ClusSavePass, 2) << endl;
	cout << "13\tBPB_TotSec:\t" << reverseByte(bootSector.BPB_TotSec, 2) << endl;
	cout << "15\tBPB_Media:\t" << reverseByte(bootSector.BPB_Media, 1) << endl;
	cout << "16\tBPB_SecPerFAT:\t" << reverseByte(bootSector.BPB_SecPerFAT, 2) << endl;
	cout << "18\tBPB_SecPerTrk:\t" << reverseByte(bootSector.BPB_SecPerTrk, 2) << endl;
	cout << "1A\tBPB_NumHeads:\t" << reverseByte(bootSector.BPB_NumHeads, 2) << endl;
	cout << "1C\tBPB_HiddSec:\t" << reverseByte(bootSector.BPB_HiddSec, 4) << endl;
	cout << "20\tBPB_TotSec32:\t" << reverseByte(bootSector.BPB_TotSec32, 4) << endl;
	cout << "24\tBPB_FATSz32:\t" << reverseByte(bootSector.BPB_FATSz32, 4) << endl;
	cout << "28\tBPB_ExtFlags:\t" << reverseByte(bootSector.BPB_ExtFlags, 2) << endl;
	cout << "2A\tBPB_FSVer:\t" << reverseByte(bootSector.BPB_FSVer, 2) << endl;
	cout << "2C\tBPB_RootClus:\t" << reverseByte(bootSector.BPB_RootClus, 4) << endl;
	cout << "30\tBPB_FSInfo:\t" << reverseByte(bootSector.BPB_FSInfo, 2) << endl;
	cout << "32\tBPB_BkBootSec:\t" << reverseByte(bootSector.BPB_BkBootSec, 2) << endl;
	cout << "34\tBPB_Reserved:\t" << bootSector.BPB_Reserved << endl;
	cout << "40\tBS_DrvNum:\t" << reverseByte(bootSector.BS_DrvNum, 1) << endl;
	cout << "41\tBS_Reserved:\t" << reverseByte(bootSector.BS_Reserved, 1) << endl;
	cout << "42\tBS_BootSig:\t" << reverseByte(bootSector.BS_BootSig, 1) << endl;
	cout << "43\tBS_VolID:\t" << reverseByte(bootSector.BS_VolID, 4) << endl;
	cout << "47\tBS_VolLab:\t";
	for (int i = 0; i < 11; i++) {
		cout << bootSector.BS_VolLab[i];
	}
	cout << endl;
	cout << "52\tBS_FilSysType:\t";
	for (int i = 0; i < 8; i++) {
		cout << bootSector.BS_FilSysType[i];
	}
	cout << endl;
}

//vector<int> get_cluster_index(int S_B, int S_F, int N_F, int Clus_Begin_RDET, int S_C, LPCWSTR fileName) {
//	vector<int> cluster_index;
//	BYTE* buffer;
//	int offset = 0;
//	while (1) {
//		buffer = readBlock((S_B + S_F * N_F + (Clus_Begin_RDET - 2) * S_C) * BYTE_PER_SEC+offset, fileName);
//		if (buffer[0] == BYTE(0))
//			break;
//		for (int i = 0; i < 512; i += 32) {
//			if (buffer[i] == BYTE(0))
//				break;
//			if (buffer[i + 26] != BYTE(0) and buffer[i + 27] != BYTE(0)) {
//				BYTE clus[2];
//				clus[0] = buffer[i + 26];
//				clus[1] = buffer[i + 27];
//				cluster_index.push_back(reverseByte(clus, 2));
//			}
//		}
//		offset += BYTE_PER_SEC;
//	}
//	return cluster_index;
//}
//bool is_in_array(vector<int >a, int n) {
//	for (int i = 0; i < a.size(); i++) {
//		if (n == a[i])
//			return true;
//	}
//	return false;
//}
