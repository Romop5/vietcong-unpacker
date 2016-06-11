#include <cstdint>
#include <iostream>
#include <fstream>
#include <vector>

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
} gameFile;


class CBFparser
{
public:
	CBFparser(std::string	filePath)
	{
		std::ifstream	input;
		input.open(filePath, std::ios::binary | std::ios::in);
		if (input.is_open())
		{
			cbf_t fileHeader;
			input.read((char*)&fileHeader, sizeof(fileHeader));
			if (!input)
				throw -1;

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

				parseTableOfFile(fileTable, fileHeader.sizeOfItemArea);
				/*std::vector<gameFile> list = getListOfFiles(fileTable, fileHeader.sizeOfItemArea);
				for (auto file : list)
				{
				printf("%p[%p]\t%s\n",file.start,file.size,file.name);
				unpackFile(&file, input);
				}*/
				return 0;
			}
			fclose(input);
		}
		return 1;
	}

private:


	/*
	Decrypt raw file data
	Key: LSB byte of file size
	*/
	void	decryptFileData(unsigned char* data, unsigned int size, unsigned char key)
	{
		unsigned char subKey = 0x5A - key;
		for (unsigned i = 0; i < size; i++)
		{
			data[i] = (data[i] - subKey) ^ key;
		}
	}



	/*
	Decrypts a chunk, containing the structure that describes file
	*/
	void decryptData(unsigned char* data, unsigned size)
	{

		static unsigned char lookUpTable[16] = { 0x32, 0xF3, 0x1E, 0x06, 0x45, 0x70, 0x32, 0xAA, 0x55, 0x3F, 0xF1, 0xDE, 0xA3, 0x44, 0x21, 0xB4 };

		// AND the size of 0xF
		unsigned	entry = size;
		unsigned	byteOfData;

		for (unsigned cx = 2; cx < size + 2; cx++)
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
			fileItem*	thisIt = (fileItem*)(data + position);

			printf("Size: %u (%x)\n", thisIt->size, thisIt->size);
			decryptData((unsigned char*)thisIt, thisIt->size);

			// dump useful data
			printf("FileName: %s\n", thisIt->name);
			printf("Start:\t %p\tSize: %x\n", thisIt->startOffset, thisIt->fileSize);
			//printHex((unsigned char*)thisIt, thisIt->size + 2);
			printf("------------------------------\n");

			position += thisIt->size + 2;
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
			// append another entry
			list.push_back(file);

			position += thisIt->size + 2;
		}

		return list;
	}

	int unpackFile(gameFile* file, FILE* packedCbf)
	{
		unsigned char* data = new unsigned char[file->size];

		fseek(packedCbf, file->start, SEEK_SET);
		fread(data, file->size, 1, packedCbf);

		decryptFileData(data, file->size, file->size & 255);

		char* split = strrchr(file->name, '\\');
		// if there is a comma, create a new directory
		if (split != NULL && split[0] != '\0')
		{
			unsigned length = strlen(file->name) - strlen(split);
			char path[255] = "mkdir ";
			strncpy(path + 6, file->name, length);
			printf("%s\n", path);
			system(path);
		}
		FILE* out = fopen(file->name, "wb");
		if (!out)
			return -1;
		fwrite(data, file->size, 1, out);
		fclose(out);
		return 1;
	}
};