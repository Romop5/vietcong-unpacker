#include <map>
#include <iostream>
#include <cstdint>


typedef struct {
	uint32_t	magicNumber;
	uint32_t	sizeOfCompressed;
	uint32_t	sizeOfDecompressed;
} LZW_fileHeader;

class Decoder
{
private:
	unsigned int	counter;
	std::map<std::string, uint32_t>	dict;
	unsigned int	sizeOfCode;
	unsigned int	position;
public:
	Decoder::Decoder()
	{
		this->counter = 257;
		this->dict.clear();
		this->sizeOfCode = 9;
		this->position = 0;
	}
	void	addToken(std::string tok)
	{
		if (tok.size() > 1)
		{
			if (this->dict.count(tok) == 0)
			{
				this->dict[tok] = counter++;
				printf("Adding tok: '%s' (%u) with ID: %x\n", tok.c_str(), tok.size(), this->dict[tok]);
			}
			else {
				//printf("Already in dict: %s\n",tok.c_str());
			}
		}
		// if dict is bigger than size of a code
		if (counter + 2 > (1 << this->sizeOfCode))
		{
			this->sizeOfCode++;
			printf("Current size: %x (dict len: %x)\n", this->sizeOfCode, this->counter);
		}
	}
	bool isTokenInDict(std::string token)
	{
		try {
			return (this->dict[token] > 0);
		}
		catch (int error)
		{
			return false;
		}
	}

	bool getTokenForID(unsigned id, std::string* token)
	{
		if (id < 256)
		{
			token += (char)id;
			return true;
		}
		for (auto m : this->dict)
		{
			if (m.second == id)
			{
				*token = m.first;
				return true;
			}
		}

		return false;
	}

	std::string getToken(unsigned id)
	{
		if (id < 256)
		{
			std::string s;
			s += (char)id;
			return s;
		}
		for (auto m : this->dict)
		{
			if (m.second == id)
			{
				return m.first;
			}
		}

		throw - 1;
	}

	/*unsigned short	getValue(unsigned char data[], unsigned position)
	{
	unsigned bits = position * 9;

	unsigned at = bits / 8;
	return ((*(unsigned short*) (data+at) >> (bits % 8))) & (~(~0 << 9));
	}*/

	uint32_t	getValue(unsigned char data[])
	{
		//unsigned bits = position * 9;

		unsigned bytePos = this->position / 16;

		uint16_t* dt = (uint16_t*)data;

		uint16_t a = dt[bytePos];
		uint16_t b = dt[bytePos + 1];

		uint16_t aShift = this->position % 16;
		uint16_t bShift = 16 - aShift;

		uint32_t tok = ((a >> aShift) | b << bShift) & ((1 << this->sizeOfCode) - 1);
		//printf("getVal(%x) = %X - %u - %u (%d)\n", tok, *(uint32_t*)(data + bytePos), aShift, bShift, this->sizeOfCode);
		position += this->sizeOfCode;

		//printf("getVal %4x\n",tok);
		return tok;






		/*	unsigned char a = data[bytePos];
		unsigned char b = data[bytePos+1];

		unsigned short aShift = this->position % 8;
		unsigned short bShift = 8- aShift;

		unsigned short tok = ((a >> aShift) | b << bShift) & ((1 << this->sizeOfCode)-1);
		printf("getVal(%x) = %X - %u - %u (%d)\n",tok,*(uint32_t*) (data+bytePos),aShift,bShift,this->sizeOfCode);
		position += this->sizeOfCode;

		//printf("getVal %4x\n",tok);
		return tok;
		*/
		//unsigned at = bits / 8;
		//return ((*(unsigned short*)(data + at) >> (bits % 8))) & (~(~0 << 9));
	}

	/*void printHex(std::string input)
	{
	for (auto chr : input)
	{
	if (isspace(chr))
	{
	putchar(chr);
	} else if (isprint(chr))
	putchar(chr);
	else {
	printf("0x%2X",chr);
	}
	}
	}*/

	std::string toHex(std::string input)
	{
		return input;
		std::string output = "";
		for (unsigned i = 0; i < input.size(); i++)
		{
			if (isprint(input[i]))
				output += input[i];
			else
			{
				char hexcode[10];
				sprintf(hexcode, "0x%X", input[i]);
				output += hexcode;
			}
		}
		return output;
	}

	std::string decode(unsigned char data[], unsigned len)
	{
		std::string out = "";

		std::string entry;
		char ch;
		unsigned int prevcode, currcode;

		prevcode = this->getValue(data);

		try{
			out += this->getToken(prevcode);
		}
		catch (int error)
		{
			printf("Error with gettoken at start\n");
			return out;
		}
		unsigned count = 0;


		while (this->position < len * 8)
		{
			unsigned int badVal = -1;
			currcode = this->getValue(data);

			if (currcode == 0x100)
				break;

			try{
				entry = getToken(currcode);
			}
			catch (int error)
			{
				//printf("Error with gettoken at second time\n");
				//return out;
				badVal = currcode;
				currcode = prevcode;
				entry = getToken(currcode);
			}
			out += entry;

			ch = entry[0];
			//add((translation of prevcode) + ch) to dictionary;

			//printf("Currcode: %x\n", currcode);

			try{
				this->addToken(this->getToken(prevcode) + ch);
			}
			catch (int a) {
				printf("Fuck %x (Curr %x)\n", prevcode, currcode);
			}
			prevcode = currcode;
			if (badVal != -1)
				prevcode = badVal;
		}
		return out;
	}
};
