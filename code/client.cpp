#include "./hpp/client.hpp"

/**

 * @brief Checks whether the given string contains only decimal digits.
 *
 * This function validates that the input string is not empty and
 * consists exclusively of characters '0'–'9'.
 *
 * @param value String to validate.
 * @return true if the string contains only digits, false otherwise.
 */
bool check_num(const std::string& value) { 

	if (value.empty())
        return false;

    for (size_t i = 0; i < value.size(); i++) {
        if (!std::isdigit(static_cast<unsigned char>(value[i])))
            return false;
    }
	return true;
}

/**
 * @brief Validates a URI path.
 *
 * A valid URI path must:
 * - not be empty;
 * - start with the '/' character;
 * - not contain whitespace or control characters.
 *
 * @param uripath URI path to validate.
 * @return true if the URI path is valid, false otherwise.
 */
bool checkUriPath(const std::string& uripath)
{
    if (uripath.empty() || uripath[0] != '/')
        return false;

    for (std::string::size_type i = 0; i < uripath.size(); ++i) {
        unsigned char ch = static_cast<unsigned char>(uripath[i]);
        if (std::isspace(ch) || std::iscntrl(ch))
            return false;
    }
    return true;
}

/**
 * @brief Validates a filesystem path.
 *
 * A valid filesystem path must:
 * - not be empty;
 * - not contain whitespace;
 * - not contain control characters.
 *
 * The function performs only a syntax check and does not verify
 * whether the path exists in the filesystem.
 *
 * @param fspath Filesystem path to validate.
 * @return true if the path is syntactically valid, false otherwise.
 */
bool checkFSPath(const std::string &fspath)
{
    if (fspath.empty())
        return false;

    for (std::string::size_type i = 0; i < fspath.size(); ++i) {
        unsigned char ch = static_cast<unsigned char>(fspath[i]);
        if (std::isspace(ch) || std::iscntrl(ch))
            return false;
    }
    return true;
}

/**
 * @brief Checks whether a string contains control characters.
 *
 * Control characters are non-printable characters such as newline,
 * carriage return, tab, null character, etc.
 *
 * @param s String to inspect.
 * @return true if at least one control character is found, false otherwise.
 */
bool hasControlChar(const std::string& s)
{
    for (char ch : s) {
        if (std::iscntrl(static_cast<unsigned char>(ch)))
            return true;
    }
    return false;
}