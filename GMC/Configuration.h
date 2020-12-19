#pragma once
#include <string>
#include <vector>


class Configuration
{
public:

	Configuration() {}
	explicit Configuration(std::string filename);
	virtual ~Configuration() = default;

	std::string GetActiveRom() const;
	void SetActiveRom(std::string filename);


private:


	using MRUListType = std::vector<std::string>;

	std::string	m_Filename;
	MRUListType	m_MRURoms;
	std::string	m_ActiveROM;
};

