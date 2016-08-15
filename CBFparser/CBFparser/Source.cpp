#include <stdio.h>
#include <cstdint>
#include <vector>

#include "LZWdecoder.h"

#include <Windows.h>



void printHex(unsigned char data[], unsigned size)
{
	printf("Printing '%u' bytes of raw data: \n", size);
	for (unsigned i = 0; i < size; i++)
		printf("%02X%c", data[i], ((i + 1) % 16) ? ' ' : '\n');
	putchar('\n');
}

void printChars(unsigned char data[], unsigned size)
{
	printf("Printing '%u' bytes of raw data: \n", size);
	for (unsigned i = 0; i < size; i++)
		printf("%2c%c", data[i], ((i + 1) % 16) ? ' ' : '\n');
	putchar('\n');
}

/*---------------------------------------------------------------------------*/

#pragma pack(1)
typedef struct {
	uint16_t		size;
	uint32_t		startOffset;
	unsigned char	padd01[16];
	uint32_t		fileSize;
	unsigned char	padding2[8];
	uint32_t		representation;
	unsigned char	padding3[4];
	char			name[1];
} fileItem;
#pragma pack()

typedef struct {
	uint32_t		signature;
	uint32_t		signatureB;
	uint32_t		fileSize;
	uint32_t		unk01;
	uint32_t		countOfFiles;	// count of files, packed in CBF
	uint32_t		fileItemOffset;	// file offset to the start of fileItem area
	uint32_t		unk02;
	uint32_t		sizeOfItemArea;	// the size of fileItem array
} cbf_t;


typedef struct {
	char name[255];
	uint32_t	start;
	uint32_t	size;
	bool		isCompressed;
} gameFile;


/*
	Decrypt raw file data
	Key: LSB byte of file size
*/
void	decryptFileData(unsigned char* data,unsigned int size, unsigned char key)
{
	unsigned char subKey = 0x5A - key;
	for(unsigned i = 0; i < size; i++)
	{
		data[i] = (data[i]-subKey) ^ key;
	}
}



/*
	Decrypts a chunk, containing the structure that describes file
*/
void decryptData(unsigned char* data, unsigned size)
{

	static unsigned char lookUpTable[16] = { 0x32, 0xF3, 0x1E, 0x06, 0x45, 0x70, 0x32, 0xAA, 0x55, 0x3F, 0xF1, 0xDE, 0xA3, 0x44, 0x21, 0xB4};

	// AND the size of 0xF
	unsigned	entry = size;
	unsigned	byteOfData;

	for (unsigned cx = 2; cx < size+2; cx++)
	{
		byteOfData = data[cx];
		data[cx] = data[cx] ^ lookUpTable[entry & 15];
		entry = byteOfData;
	}
}

/*
	Loop through all files in CBF
*/
void parseTableOfFile(unsigned char* data, unsigned size)
{
	unsigned long position = 0;
	while (position + 2 < size)
	{
		fileItem*	thisIt = (fileItem*) (data + position);
		
		printf("Size: %u (%x)\n",thisIt->size,thisIt->size);
		decryptData((unsigned char*) thisIt, thisIt->size);

		// dump useful data
		printf("FileName: %s\n",thisIt->name);
		printf("Start:\t %p\tSize: %x\n",thisIt->startOffset,thisIt->fileSize);
		printHex((unsigned char*)thisIt,thisIt->size+2);
		printf("------------------------------\n");

		position += thisIt->size+2;
	}
}


/*
	Create a vector, containing all file entries in .cbf
*/
std::vector<gameFile> getListOfFiles(unsigned char* data, unsigned size)
{
	unsigned long position = 0;
	std::vector <gameFile> list;
	while (position + 2 < size)
	{
		fileItem*	thisIt = (fileItem*)(data + position);

		decryptData((unsigned char*)thisIt, thisIt->size);

		gameFile	file;
		strcpy(file.name, thisIt->name);
		file.start = thisIt->startOffset;
		file.size = thisIt->fileSize;
		file.isCompressed = (bool) (thisIt->representation > 0);
		// append another entry
		list.push_back(file);

		position += thisIt->size + 2;
	}

	return list;
}

bool createDir(std::string path)
{
	size_t start = 0;
	size_t pos = 0;
	while((pos = path.find('\\', start)) != std::string::npos)
	{
		printf("Start: %d Stop: %d\n",start,pos);
		
		std::string dir = path.substr(0, pos);

		printf("Dir: %s\n", dir.c_str());

		bool result = CreateDirectoryA(dir.c_str(), NULL);
		if (!result)
		{
			int err = GetLastError();
			printf("Errno: %x\n",err);
			if (err != ERROR_ALREADY_EXISTS)
				return false;
		}
		start = pos + 1;

	} 
	return true;
}

int unpackFile(gameFile* file, FILE* packedCbf)
{
	// Create directory
	char* split = strrchr(file->name, '\\');
	// if there is a comma, create a new directory
	if (split != NULL && split[0] != '\0')
	{
		/*unsigned length = strlen(file->name) - strlen(split);
		char path[255] = "mkdir ";
		strncpy(path + 6, file->name, length);
		printf("%s\n", path);
		*/

		//if (system(path) != 0)
		printf("createDir enter...\n");
		int result = createDir(file->name);
		if (!result)
		{
			printf("Failed to create directory for file\n");
			return -1;
		}
		printf("createDir exit...\n");
	}

	// set read stream position to file position
	fseek(packedCbf, file->start, SEEK_SET);

	FILE* out = fopen(file->name, "wb"); 
	if (!out)
	{
		printf("Failed to open output file\n");
		return -1;
	}

	// the process of data extraction depends on the type of storage data
	switch (file->isCompressed)
	{
		// non-compressed file
		case 0:
		{
				  unsigned char* data = new unsigned char[file->size];

				  fread(data, file->size, 1, packedCbf);

				  printf("Decrypting ...\n");
				  decryptFileData(data, file->size, file->size & 255);
				  fwrite(data, file->size, 1, out);

				  printf("Decrypting OK\n");
				  delete data;
				  break;
		}
			// compressed file
		case 1:
		{
				  LZW_fileHeader	inputHeader;
				  fread(&inputHeader, sizeof(inputHeader), 1, packedCbf);
				  unsigned char* data = new unsigned char[inputHeader.sizeOfCompressed];

				  fread(data, inputHeader.sizeOfCompressed, 1, packedCbf);
				  
				  printf("Decrypting ...\n");

				  Decoder	dec;
				  code output = dec.decode(data, inputHeader.sizeOfCompressed);

				  fwrite(outputCode(output).c_str(), output.size(), 1, out);


				  printf("Decrypting OK\n");
				  if (output.size() != inputHeader.ouput)
				  {
					  printf("WARNING ! The length of decompressed file differs from CBF value.\n");
					  printf("Stored: %u Ouput: %u\n",output.size(),inputHeader.ouput);
				  }
				  
				  delete data;
				  break;
		}
		default:
		{
				   printf("[Err] Unk file type \n");
				   return -1;
		}
	}

	// close file
	fclose(out);
	return 1;
}

int main(int argc, const char* argv[])
{
	if (argc <= 1)
	{
		printf("USAGE: name.cb\n");
		return 1;
	}

	FILE* input = fopen(argv[1], "rb");
	if (input)
	{
		cbf_t	fileHeader;
		if (fread(&fileHeader, sizeof(cbf_t), 1, input))
		{
			// Dump file info
			printf("Size of CBF: \t\t\t%08u\n", fileHeader.fileSize);
			printf("Count of files: \t\t%08u\n", fileHeader.countOfFiles);
			printf("Starting offset: \t\t%08p\n", fileHeader.fileItemOffset);

			fseek(input, fileHeader.fileItemOffset, SEEK_SET);

			unsigned char	*fileTable = new unsigned char[fileHeader.sizeOfItemArea];

			fread(fileTable, fileHeader.sizeOfItemArea, 1, input);

			CreateDirectoryA("unpack", NULL);
			SetCurrentDirectoryA("./unpack");
			//parseTableOfFile(fileTable, fileHeader.sizeOfItemArea);
			std::vector<gameFile> list = getListOfFiles(fileTable, fileHeader.sizeOfItemArea);
			for (auto file : list)
			{
				printf("%p[%p]\t%s\n",file.start,file.size,file.name);
				unpackFile(&file, input);
			}
			delete[] fileTable;
			return 0;
		}
		fclose(input);
	}
	else {
		char dir[200];
		GetCurrentDirectoryA(200, dir);
		printf("%s\n",dir);
		printf("Failed to open '%s'\n", argv[1]);
		system("pause");
	}
	
	return	2;
}