#include "ROM.h"
#include <fstream>
#include <windows.h>
using namespace std;

bool ROM::Load(std::string filename)
{

//	std::ifstream input(filename, std::ifstream::binary);
	ifstream input(filename,ios::binary);
	if (!input.is_open())
    {
		return false;
    }

    int begin = input.tellg();
	input.seekg(0,ios::end);
	int end = input.tellg();
	int fileLength=end-begin;
	input.seekg(0,ios::beg);

//	input.seekg(0, std::ifstream::ios_base::end);
//    const auto fileLength(input.tellg().seekpos());
//    input.seekg(0, std::ifstream::ios_base::beg);

    std::vector<unsigned char> fileData(fileLength);
    input.read(reinterpret_cast<char*>(&fileData[0]), fileData.size());

    if (fileLength != fileData.size())
    {
		return false;
    }

	m_Filename = move(filename);
    m_Data = move(fileData);
	m_BankOffset = 0;
	m_Bank = 0;

//	char msg[256];
//	sprintf(msg,"%s loaded %d",&m_Filename[0],m_Data.size());
//	MessageBoxA(nullptr, msg, "Rom loaded", MB_OK);
	return true;
}


void ROM::Unload()
{
	m_Data.clear();
	m_BankOffset = 0;
	m_Bank = 0;
}


void ROM::Reset()
{
	SetBank(0);
}




void ROM::SetBank(unsigned char blockId)
{
	if (!m_Data.empty())
	{
		m_Bank = blockId;
		m_BankOffset = blockId * BankSize;
	}
}


unsigned char ROM::GetBank() const
{
	return m_Bank;
}




ROM::value_type ROM::Read(size_type readOffset) const
{
	if (m_Data.empty())
	{
		return 0;
	}

	return m_Data[(m_BankOffset + readOffset) % m_Data.size()];
}
