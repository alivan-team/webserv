// ConfigParser.hpp
#ifndef CONFIG_PARSER_HPP
#define CONFIG_PARSER_HPP

#include "ServerConfig.hpp"
#include <string>
#include <iostream>
#include <fstream>


class ConfigParser {

	private:
		using Setter = void (ServerConfig::*)(const std::vector<std::string>&);
		using LocationSetter = void (LocationConfig::*)(const std::vector<std::string>&);

    	std::map<std::string, Setter> setters;
    	std::map<std::string, LocationSetter> locationSetters;

		template <typename ObjectType, typename SetterType>
		void findKey(ObjectType& object, std::map<std::string, SetterType>& settersMap, 
			const std::vector<std::string>& configTokens, size_t& i)
		{
			std::string key = configTokens[i];

			// std::cout << "key -> HI " << key << std::endl;

			typename std::map<std::string, SetterType>::iterator it = settersMap.find(key);

			if (it == settersMap.end()) {
				throw std::runtime_error("Unknown directive: " + key);
				// break ;
			}

			i++;

			std::vector<std::string> values;

			while (i < configTokens.size() && configTokens[i] != ";")
			{
				values.push_back(configTokens[i]);
				i++;
			}

			if (i >= configTokens.size())
				throw std::runtime_error("Missing ; after " + key);

			if (values.empty())
				throw std::runtime_error("Missing value after " + key);

			SetterType fun = it->second;
			(object.*fun)(values);

			i++;
		}

	    std::vector<ServerConfig> servers;
	
	public:
		ConfigParser();
		void parse(const std::string& filename);
		const std::vector<ServerConfig>& getServers() const;

};

#endif
