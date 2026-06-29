#include "./hpp/client.hpp"

bool check_num(const std::string& value) { 

	if (value.empty())
        return false;

    for (size_t i = 0; i < value.size(); i++) {
        if (!std::isdigit(static_cast<unsigned char>(value[i])))
            return false;
    }
	return true;
}