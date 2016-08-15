#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <clocale>

#include <iostream>

#include <map>
#include <set>
#include <vector>

typedef struct {
	uint32_t	magic;
	uint32_t	sizeOfCompressed;
	uint32_t	ouput;
} LZW_fileHeader;



typedef std::vector<unsigned char> code;

std::string codeToString(code input)
{
	std::string out;
	for (auto c : input)
	{
		if (isprint(c))
			out.push_back(c);
		else {
			char token[10];
			sprintf(token, "\\x%02x", c);
			out += token;
		}
	}
	out.push_back(NULL);
	return out;
}


std::string outputCode(code input)
{
	std::string out;
	for (auto c : input)
	{
			out.push_back(c);
	}
	out.push_back(NULL);
	return out;
}

class Decoder
{
private:
	unsigned int	counter;
	//std::map<std::string, uint32_t>	dict;

	std::map<uint32_t, code>	dict;

	std::set<code> tokens;
	unsigned int	sizeOfCode;
	unsigned int	position;
public:
	Decoder::Decoder()
	{
		this->counter = 257;
		this->dict.clear();
		this->tokens.clear();
		this->sizeOfCode = 9;
		this->position = 0;
	}
	void	addToken(code tok)
	{
		if (tok.size() > 1)
		{

			//if (this->dict.count(tok) == 0)
			if (this->tokens.count(tok) == 0)
			{
				uint32_t id = counter++;
				this->dict[id] = tok;
				//this->dict[tok] = counter++;

				//printf("=Adding tok: '%s' (%u) with ID: %x\n", codeToString(tok).c_str(), tok.size(), id);
			}
			else {
				//printf("Already in dict: %s\n",tok.c_str());
			}
		}
		// if dict is bigger than size of a code
		if (counter + 2 > (1 << this->sizeOfCode))
		{
			this->sizeOfCode++;
			//printf("Current size: %x (dict len: %x)\n",this->sizeOfCode,this->counter);
		}
	}
	bool isTokenInDict(code token)
	{
		return (this->tokens.count(token) > 0);
		/*try {
		return (this->dict[token] > 0);
		}
		catch (int error)
		{
		return false;
		}*/
	}

	bool getTokenForID(unsigned id, code* token)
	{
		if (id < 256)
		{
			token += (char)id;
			return true;
		}

		try
		{
			*token = this->dict[id];
			return true;
		}
		catch (int err)
		{
			return false;
		}

		/*for (auto m : this->dict)
		{
		if (m.second == id)
		{
		*token = m.first;
		return true;
		}
		}*/

		return false;
	}

	code getToken(unsigned id)
	{
		if (id < 256)
		{
			code s;
			s.push_back((char)id);
			return s;
		}
		else {
			try {

				if (this->counter > id)
				{
					code str = this->dict[id];
					//printf("For %X returning %s\n", id, codeToString(str).c_str());
					return str;
				}
				throw - 1;

			}
			catch (int err)
			{
				//printf("ID %x is not in da dict\n", id);
				throw err;
			}
		}
		return code();

		/*for (auto m : this->dict)
		{
		if (m.second == id)
		{
		return m.first;
		}
		}*/

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

		//printf("getVal %4x\n", tok);
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

	code decode(unsigned char data[], unsigned len)
	{
		code out;

		code entry;
		char ch;
		unsigned int prevcode, currcode;

		prevcode = this->getValue(data);

		try {
			code cd = this->getToken(prevcode);
			out.insert(out.end(), cd.begin(), cd.end());
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

			try {
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

			if (entry.size() == 0)
			{
				printf("Currcode is invalid: %x ", currcode);
				//return "INV";
				return code();
			}

			// append
			out.insert(out.end(), entry.begin(), entry.end());
			//printf(">>>Appending '%s' into output.\n", codeToString(entry).c_str());

			ch = entry[0];
			//add((translation of prevcode) + ch) to dictionary;

			//printf("Currcode: %x\n", currcode);

			try {
				code addCode = this->getToken(prevcode);
				addCode.push_back(ch);
				this->addToken(addCode);
			}
			catch (int a) {
				printf("Fuck %x (Curr %x)\n", prevcode, currcode);
			}
			prevcode = currcode;
			if (badVal != -1)
			{
				prevcode = badVal;
				//printf("---Push back\n");
				out.push_back(entry[0]);
			}
		}
		return out;
	}

};