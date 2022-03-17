#include "format_FAT32.h"

int main()
{
	FAT32 bootSector;
	int S_B;
	int S_F;
	int N_F;
	int Clus_Begin_RDET;
	int S_C;
	int mode_file;
	int size_volume;
	bool flag;
	string myFile= "MyFS.dat";
	LPCWSTR fileName = L"MyFS.dat";
	string password;
	string pass_sec;
	string name;
	string fileSource;
	string fileDes;
	BYTE* buffer;
	pair<int, int> starPos_offset;
	vector<string> file_used;
	buffer = readBlock(0, fileName);
	if (buffer != NULL) {
		bootSector = get_bootSector(myFile);
		get_info(bootSector, S_B, S_F, N_F, Clus_Begin_RDET, S_C);
		if (buffer[0]!=BYTE(235)) {
			while (1) {
				cout << "nhap password volume: ";
				cin >> password;
				if (checkPassword_volume(password, fileName))
					break;
			}
			lock_or_unlock_sector(password, 0, fileName, 0, BYTE_PER_SEC);
		}
		bootSector = get_bootSector(myFile);
		get_info(bootSector, S_B, S_F, N_F, Clus_Begin_RDET, S_C);
	}
	
	while (1) {
		int n;
		cout << "chon cac yeu cau" << endl;
		cout << "0: thoat" << endl;
		cout << "1: Tao, dinh dang volume MyFS.dat" << endl;
		cout << "2: Thiet lap, doi, kiem tra mat khau truy xuat MyFS" << endl;
		cout << "3: liet ke danh sach cac tap tin trong MyFS" << endl;
		cout << "4: dat, doi mat khau truy xuat cho 1 tap tin trong MyFS" << endl;
		cout << "5: chep 1 tap tin tu ben ngoai vao MyFS" << endl;
		cout << "6: chep 1 tap tin tu MyFS ra ngoai" << endl;
		cout << "7: xoa 1 tap tin trong myFS" << endl;
		cin >> n;
		if (n == 0)
			break;
		switch (n)
		{
		case 1:
			cout << "1:tu nhap kich thuoc volume" << endl;
			cout << "2:chon kich thuoc volume de xuat" << endl;
			while (1) {
				cin >> n;
				if (n == 1) {
					cout << "kich thuoc volume toi thieu là 100MB toi da la 2048 MB" << endl;
					cout << "nhap kich thuoc: ";
					while (1) {
						cin >> size_volume;
						if (size_volume >= 100 && size_volume <= 2048)
							break;
					}
					break;
				}
				else if (n == 2) {
					cout << "chon cac kich thuoc volume duoc de xuat duoi day" << endl;
					cout << "1: kich thuoc =128MB" << endl;
					cout << "2: kich thuoc =256MB" << endl;
					cout << "3: kich thuoc =512MB" << endl;
					cout << "4: kich thuoc =1024MB" << endl;
					cout << "5: kich thuoc =2048MB" << endl;
					cin >> n;
					if (n == 1) {
						size_volume = 128;
						break;
					}
					else if (n == 2) {
						size_volume = 256;
						break;
					}
					else if (n == 3) {
						size_volume = 512;
						break;
					}
					else if (n == 4) {
						size_volume = 1024;
						break;
					}
					else if (n == 5) {
						size_volume = 2048;
						break;
					}
					else {
						cout << "loi! hay thu lai sau" << endl << endl;
					}
				}
			}
			create_MyFS_with_byte_zero(myFile, size_volume);
			bootSector = create_bootSector(size_volume);
			flag = write_boolSector(bootSector, fileName);
			if (!flag) {
				cout << "loi!!! khong the tao boot sector" << endl;
				return 0;
			}
			get_info(bootSector, S_B, S_F, N_F, Clus_Begin_RDET, S_C);
			init_FAT(S_B, S_F, N_F, fileName);
			cout << "tao thanh cong volume" << endl << endl;
			break;
		case 2:
			buffer = readBlock(0, fileName);
			bootSector = get_bootSector(myFile);
			get_info(bootSector, S_B, S_F, N_F, Clus_Begin_RDET, S_C);
			if (buffer == NULL) {
				cout << "khong tim thay volume" << endl;
				cout << "hay tao volume de tiep tuc" << endl;
				break;
			}
			while (1) {
				cout << "0: Tro ve menu" << endl;
				cout << "1: Thiet lap, doi mat khau" << endl;
				cout << "2: Kiem tra mat khau" << endl;
				cout << "3: xoa mat khau" << endl;
				cin >> n;
				if (n == 0)
					break;
				switch (n)
				{
				case 1:
					cout << "nhap mat khau moi";
					cout << "mat khau toi da 32 ki tu!!" << endl;
					cout << "mat khau: ";
					cin >> password;
					flag=setPassword_volume(password, fileName, bootSector);
					if (!flag) {
						cout << "khong the cap nhat mat khau" << endl;
						cout << "hay thu lai sau" << endl << endl;;
						break;
					}
					cout << "cap nhat mat khau thanh cong !!!" << endl;
					cout << "lan truy cap volume tiep theo can nhap mat khau moi" << endl << endl;;
					break;
				case 2:
					pass_sec = getPassword_volume((S_B + S_F * N_F) * BYTE_PER_SEC, fileName);
					if (pass_sec == "") {
						cout << "volume chua thiet lap mat khau" << endl << endl;;
						break;
					}
					cout << "mat khau volume la: " << pass_sec << endl << endl;
					break;
				case 3:
					flag = setPassword_volume("", fileName, bootSector);
					if (!flag) {
						cout << "khong the xoa mat khau" << endl;
						cout << "hay thu lai sau" << endl << endl;;
						break;
					}
					cout << "xoa mat khau thanh cong !!!" << endl;
					break;
				default:
					cout << "hay thu lai !!!" << endl;
					break;
				}
			}
			break;
		case 3:
			buffer = readBlock(0, fileName);
			bootSector = get_bootSector(myFile);
			get_info(bootSector, S_B, S_F, N_F, Clus_Begin_RDET, S_C);
			if (buffer == NULL) {
				cout << "khong tim thay volume" << endl;
				cout << "hay tao volume de tiep tuc" << endl;
				break;
			}
			cout << "cac tap tin trong volume" << endl;
			listed_file((S_B + S_F * N_F+(Clus_Begin_RDET-2)*S_C) * BYTE_PER_SEC, fileName);
			cout << endl;
			break;
		case 4:
			buffer = readBlock(0, fileName);
			bootSector = get_bootSector(myFile);
			get_info(bootSector, S_B, S_F, N_F, Clus_Begin_RDET, S_C);
			if (buffer == NULL) {
				cout << "khong tim thay volume" << endl;
				cout << "hay tao volume de tiep tuc" << endl;
				break;
			}
			cout << "nhap ten file can thiet lap" << endl;
			cout << "vi du: f0.dat" << endl;
			cout << "ten file: ";
			cin >> name;
			starPos_offset = get_startPos_and_offset_file(name,
				(S_B + S_F * N_F + (Clus_Begin_RDET - 2) * S_C) * BYTE_PER_SEC,
				fileName);
			if (starPos_offset.first == 0 and starPos_offset.second == 0) {
				cout << "file khong ton tai, hay thu lai!!!" << endl;
				break;
			}
			mode_file = get_mode_file(name,
				(S_B + S_F * N_F + (Clus_Begin_RDET - 2) * S_C) * BYTE_PER_SEC,
				fileName);
			if (mode_file == 6) {
				while (1) {
					cout << "nhap password file: ";
					cin >> password;
					if (checkPassword_file(password,
						name,
						(S_B + S_F * N_F + (Clus_Begin_RDET - 2) * S_C) * BYTE_PER_SEC,
						fileName))
						break;
				}
				starPos_offset = get_startPos_and_offset_file(name,
					(S_B + S_F * N_F + (Clus_Begin_RDET - 2) * S_C) * BYTE_PER_SEC,
					fileName);
				lock_or_unlock_sector(password,
					starPos_offset.second,
					fileName,
					starPos_offset.first + 16,
					starPos_offset.first + 64);
				file_used.push_back(name);
				set_mode_file(name, 2, starPos_offset.second, fileName);
			}
			while (1) {
				cout << "0: Tro ve menu" << endl;
				cout << "1: Thiet lap, doi mat khau" << endl;
				cout << "2: Kiem tra mat khau" << endl;
				cout << "3: xoa mat khau" << endl;
				cin >> n;
				if (n == 0)
					break;
				switch (n)
				{
				case 1:
					cout << "nhap mat khau moi" << endl;
					cout << "mat khau toi da 31 ki tu!!" << endl;
					cout << "mat khau: ";
					cin >> password;
					flag = setPassword_file(password, name,
						(S_B + N_F * S_F + (Clus_Begin_RDET - 2) * S_C) * BYTE_PER_SEC,
						fileName);
					if (!flag) {
						cout << "khong the cap nhat mat khau" << endl;
						cout << "hay thu lai sau" << endl << endl;;
						break;
					}
					if(!is_in_list_file(file_used, name)){
						file_used.push_back(name);
					}
					cout << "cap nhat mat khau thanh cong !!!" << endl;
					cout << "lan truy cap file tiep theo can nhap mat khau moi" << endl << endl;;
					break;
				case 2:
					pass_sec = getPassword_file(name,
						(S_B + N_F * S_F + (Clus_Begin_RDET - 2) * S_C) * BYTE_PER_SEC,
						fileName);
					if (pass_sec == "") {
						cout << "file chua thiet lap mat khau" << endl << endl;;
						break;
					}
					cout << "mat khau file la: " << pass_sec << endl << endl;
					break;
				case 3:
					flag = setPassword_file("", name,
						(S_B + N_F * S_F + (Clus_Begin_RDET - 2) * S_C) * BYTE_PER_SEC,
						fileName);
					if (!flag) {
						cout << "khong the xoa mat khau" << endl;
						cout << "hay thu lai sau" << endl << endl;;
						break;
					}
					cout << "xoa mat khau thanh cong !!!" << endl;
					break;
				default:
					cout << "hay thu lai !!!" << endl;
					break;
				}
			}
			break;
		case 5:
			buffer = readBlock(0, fileName);
			bootSector = get_bootSector(myFile);
			get_info(bootSector, S_B, S_F, N_F, Clus_Begin_RDET, S_C);
			if (buffer == NULL) {
				cout << "khong tim thay volume" << endl;
				cout << "hay tao volume de tiep tuc" << endl;
				break;
			}
			cout << "nhap duong dan file muon chep vao" << endl;
			cout << "vi du: C:\\f0.dat" << endl;
			cin >> fileSource;
			flag = copy_file_to_volume(fileSource, fileName, bootSector);
			if (!flag) {
				cout << "loi copy file tu ben ngoai,hay thu lai !!!" << endl << endl;
				break;
			}
			cout << "copy file tu ben ngoai vao thanh cong !!!" << endl << endl;
			break;
		case 6:
			buffer = readBlock(0, fileName);
			bootSector = get_bootSector(myFile);
			get_info(bootSector, S_B, S_F, N_F, Clus_Begin_RDET, S_C);
			if (buffer == NULL) {
				cout << "khong tim thay volume" << endl;
				cout << "hay tao volume de tiep tuc" << endl;
				break;
			}
			cout << "nhap ten file can copy ra ngoai" << endl;
			cout << "vidu: MyFS.dat\\f0.dat" << endl;
			cin >> fileSource;
			starPos_offset = get_startPos_and_offset_file(fileSource,
				(S_B + S_F * N_F + (Clus_Begin_RDET - 2) * S_C) * BYTE_PER_SEC,
				fileName);
			if (starPos_offset.first == 0 and starPos_offset.second == 0) {
				cout << "file khong ton tai, hay thu lai!!!" << endl;
				break;
			}
			mode_file = get_mode_file(fileSource,
				(S_B + S_F * N_F + (Clus_Begin_RDET - 2) * S_C) * BYTE_PER_SEC,
				fileName);	
			if (mode_file == 6) {
				while (1) {
					cout << "nhap password file: ";
					cin >> password;
					if (checkPassword_file(password,
						fileSource,
						(S_B + S_F * N_F + (Clus_Begin_RDET - 2) * S_C) * BYTE_PER_SEC,
						fileName))
						break;
				}
				starPos_offset = get_startPos_and_offset_file(fileSource,
					(S_B + S_F * N_F + (Clus_Begin_RDET - 2) * S_C) * BYTE_PER_SEC,
					fileName);

				lock_or_unlock_sector(password,
					starPos_offset.second,
					fileName,
					starPos_offset.first+16,
					starPos_offset.first+64);
				file_used.push_back(fileSource);
			}
			cout << "nhap duong dan vi tri file muon copy den(bao gom ten file)" << endl;
			cout << "vidu: C:\\f1.dat (copy noi dung tu file vao f1.dat)" << endl;
			cin >> fileDes;
			if (is_exsist_file_normal(fileDes)) {
				cout << "file da ton tai,hay thu lai !!!" << endl;
				break;
			}
			flag = copy_file_from_volume(fileSource, fileDes, bootSector);
			if (!flag) {
				cout << "loi copy file ra ben ngoai,hay thu lai !!!" << endl << endl;
				break;
			}
			cout << "copy file ra ben ngoai vao thanh cong !!!" << endl << endl;
			break;
		case 7:
			buffer = readBlock(0, fileName);
			bootSector = get_bootSector(myFile);
			get_info(bootSector, S_B, S_F, N_F, Clus_Begin_RDET, S_C);
			if (buffer == NULL) {
				cout << "khong tim thay volume" << endl;
				cout << "hay tao volume de tiep tuc" << endl;
				break;
			}
			cout << "nhap ten file can xoa" << endl;
			cout << "vidu: MyFS.dat\\f0.dat" << endl;
			cin >> fileSource;

			starPos_offset = get_startPos_and_offset_file(fileSource,
				(S_B + S_F * N_F + (Clus_Begin_RDET - 2) * S_C) * BYTE_PER_SEC,
				fileName);
			if (starPos_offset.first == 0 and starPos_offset.second == 0) {
				cout << "file khong ton tai, hay thu lai!!!" << endl;
				break;
			}
			mode_file = get_mode_file(fileSource,
				(S_B + S_F * N_F + (Clus_Begin_RDET - 2) * S_C) * BYTE_PER_SEC,
				fileName);
			if (mode_file == 6) {
				while (1) {
					cout << "nhap password file: ";
					cin >> password;
					if (checkPassword_file(password,
						fileSource,
						(S_B + S_F * N_F + (Clus_Begin_RDET - 2) * S_C) * BYTE_PER_SEC,
						fileName))
						break;
				}
				starPos_offset = get_startPos_and_offset_file(fileSource,
					(S_B + S_F * N_F + (Clus_Begin_RDET - 2) * S_C) * BYTE_PER_SEC,
					fileName);

				lock_or_unlock_sector(password,
					starPos_offset.second,
					fileName,
					starPos_offset.first + 16,
					starPos_offset.first + 64);
				file_used.push_back(fileSource);
			}
			flag = remove_file(fileSource, bootSector, fileName);
			if (!flag) {
				cout << "khong the xoa file, hay thu lai !!!" << endl << endl;
				break;
			}
			cout << "xoa file thanh cong!!!" << endl << endl;
			break;
		default:
			break;
		}
	}
	//truoc khi ket thuc se khoa lai volume,cac file neu co password
	bootSector = get_bootSector(myFile);
	get_info(bootSector, S_B, S_F, N_F, Clus_Begin_RDET, S_C);
	lock_files(file_used,											//khoa cac file co password
		(S_B + N_F * S_F + (Clus_Begin_RDET - 2) * S_C)* BYTE_PER_SEC,
		fileName);
	password = getPassword_volume((S_B + S_F * N_F) * BYTE_PER_SEC, fileName);
	lock_or_unlock_sector(password, 0, fileName, 0, BYTE_PER_SEC);	//khoa volume
    return 0;
}

