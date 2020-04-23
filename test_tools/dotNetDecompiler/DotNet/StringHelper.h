#ifndef __DOTNET_STRINGHELPER_H__
#define __DOTNET_STRINGHELPER_H__

#include <sstream>
#include <algorithm>
#include <cctype>
#include <iostream>
#include <string>
#include <vector>

namespace PEAnalyzer
{
	class StringHelper
	{
	public:
		static std::string toLower(std::string source)
		{
			std::transform(source.begin(), source.end(), source.begin(), std::tolower);
			return source;
		}

		static std::string toUpper(std::string source)
		{
			std::transform(source.begin(), source.end(), source.begin(), std::toupper);
			return source;
		}

		static std::string trimStart(std::string source, const std::string &trimChars = " \t\n\r\v\f")
		{
			return source.erase(0, source.find_first_not_of(trimChars));
		}

		static std::string trimEnd(std::string source, const std::string &trimChars = " \t\n\r\v\f")
		{
			return source.erase(source.find_last_not_of(trimChars) + 1);
		}

		static std::string trim(std::string source, const std::string &trimChars = " \t\n\r\v\f")
		{
			return trimStart(trimEnd(source, trimChars), trimChars);
		}

		static std::string replace(std::string source, const std::string &find, const std::string &replace)
		{
			size_t pos = 0;
			while ((pos = source.find(find, pos)) != std::string::npos)
			{
				source.replace(pos, find.length(), replace);
				pos += replace.length();
			}
			return source;
		}

		static bool startsWith(const std::string &source, const std::string &value)
		{
			if (source.length() < value.length())
				return false;
			else
				return source.compare(0, value.length(), value) == 0;
		}

		static bool endsWith(const std::string &source, const std::string &value)
		{
			if (source.length() < value.length())
				return false;
			else
				return source.compare(source.length() - value.length(), value.length(), value) == 0;
		}

		static bool isEmptyOrWhiteSpace(const std::string &source)
		{
			if (source.length() == 0)
				return true;
			else
			{
				for (int index = 0; index < source.length(); index++)
				{
					if (!std::isspace(source[index]))
						return false;
				}

				return true;
			}
		}

		static std::vector<std::string> split(const std::string &source, char delimiter)
		{
			std::vector<std::string> output;
			std::istringstream ss(source);
			std::string nextItem;

			while (std::getline(ss, nextItem, delimiter))
			{
				output.push_back(nextItem);
			}

			return output;
		}

		template<typename T>
		static std::string toString(const T &subject)
		{
			std::ostringstream ss;
			ss << subject;
			return ss.str();
		}

		template<typename T>
		static T fromString(const std::string &subject)
		{
			std::istringstream ss(subject);
			T target;
			ss >> target;
			return target;
		}
	};
}


#endif // __DOTNET_STRINGHELPER_H__