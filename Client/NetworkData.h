#pragma once
#include <unordered_map>
#include <vector>

using std::vector;

class NetworkData
{
public:
	// Inserts a key-value pair of {const char*, T}, converting T to bytes
	template <typename T> void Insert(const char* _Key, T _Val)
	{
		// Already contains key, exit
		if (Contains(_Key))
			return;

		// Doesn't contain key, add it
		std::vector<unsigned char> data = ToBytes(_Val);
		m_keys.push_back(_Key);
		m_vals.push_back(data);
	}

	// Inserts a key-value pair {const char*, known vector of bytes}
	void InsertBytes(const char* _Key, std::vector<unsigned char> _Val)
	{
		// Already contains key, exit
		if (Contains(_Key))
			return;

		// Doesn't contain key, add it
		m_keys.push_back(_Key);
		m_vals.push_back(_Val);
	}

	// Sets element with _Key to _Val, if it exists, otherwise inserts it
	template <typename T> void SetElement(const char* _Key, T _Val)
	{
		// If contains key, overwrite the value...
		int index = Index(_Key);
		if (index != -1)
		{
			m_vals[index] = ToBytes(_Val);
			return;
		}

		// ...otherwise insert it
		Insert(_Key, _Val);
	}

	// Sets element _Key to vector of bytes _Val, if it exists, otherwise inserts it
	void SetElementBytes(const char* _Key, std::vector<unsigned char> _Val)
	{
		// Already contains key, set value to _Val...
		int index = Index(_Key);
		if (index != -1)
		{
			m_vals[index] = _Val;
			return;
		}

		// ...otherwise insert key-value pair
		InsertBytes(_Key, _Val);
	}

	// Erases element with _Key
	void Erase(const char* _Key)
	{
		// Only erase _Key if it exists in the network data
		int index = Index(_Key);
		if (index != -1)
		{
			m_keys.erase(m_keys.begin() + index);
			m_vals.erase(m_vals.begin() + index);
		}
	}

	// Clears both vectors
	void Clear()
	{
		m_keys.clear();
		m_vals.clear();
	}

	// Gets element with _Key, throws if none exists
	template <typename T> T GetElement(const char* _Key)
	{
		// Doesn't contain key, return new object of type T
		int index = Index(_Key);
		if (index == -1)
		{
			return T();
		}

		// Does contain key, return bytes -> T
		return FromBytes<T>(m_vals[index]);
	}

	// Checks if m_keys contains _Key
	bool Contains(const char* _Key)
	{
		for (auto member : m_keys)
		{
			size_t size = strlen(member);

			// Strings not same length, skip to next one
			if (size != strlen(_Key))
				continue;

			// Count the number of same chars, if this count
			// is same as the length of the string they are equal
			int sameChars = 0;

			for (int i = 0; i < (int)size; i++) // loop through chars of each member
			{
				if (member[i] == _Key[i]) // char is same, update count
					sameChars++;

				else break; // not same, break immediately
			}

			// Char count same, so they are the same string
			if (sameChars == (int)size)
			{
				return true;
			}
		}

		return false;
	}

	// Return index of _Key if it exists in m_keys
	int Index(const char* _Key)
	{
		for (int i = 0; i < m_keys.size(); i++)
		{
			size_t size = strlen(m_keys[i]);

			// Strings not same length, skip to next one
			if (size != strlen(_Key))
				continue;

			// Count the number of same chars, if this count
			// is same as the length of the string they are equal
			int sameChars = 0;

			for (int j = 0; j < (int)size; j++) // loop through chars of each member
			{
				if (m_keys[i][j] == _Key[j]) // char is same, update count
					sameChars++;

				else break; // not same, break immediately
			}

			// Char count same, so they are the same string
			if (sameChars == (int)size)
			{
				return i;
			}
		}

		return -1;
	}

	// Returns amount of elements in vectors
	int Size()
	{
		return (int)m_keys.size();
	}

	// Converts T object to vector of its' bytes (unsigned char)
	template<typename T> std::vector<unsigned char> ToBytes(T object)
	{
		// Resize vector to byte size of T object
		std::vector<unsigned char> bytes;
		bytes.resize(sizeof(object));

		// Set begin and end of first byte
		auto begin = reinterpret_cast<unsigned char*>(std::addressof(object));
		auto end = begin + sizeof(unsigned char);

		// Loop through all bytes and copy them to vector
		for (int i = 0; i < sizeof(object); i++)
		{
			std::copy(begin, end, &bytes[i]);

			// Adjust begin / end to next byte
			begin = end;
			end += sizeof(unsigned char);
		}

		return bytes;
	}

	// Converts vector of bytes (unsigned char) to T object
	template<typename T> T FromBytes(std::vector<unsigned char>& bytes)
	{
		// Initialise object of type T
		T out = T();

		// Guarantee that this vector is the same size as the object
		bytes.resize(sizeof(out));

		// The address to copy the individual bytes to
		auto copyAddr = reinterpret_cast<unsigned char*>(std::addressof(out));

		// Loop through and copy each byte to T object
		for (auto b : bytes)
		{
			// Get begin / end of this byte
			auto begin = std::addressof(b);
			auto end = begin + sizeof(unsigned char);

			// Copy it to the copy address of T object
			std::copy(begin, end, copyAddr);

			// Move copy address forward by 1 byte
			copyAddr += sizeof(unsigned char);
		}

		return out;
	}

	// Returns reference to m_keys
	vector<const char*>& Keys()
	{
		return m_keys;
	}

	// Returns reference to m_vals
	vector<vector<unsigned char>>& Values()
	{
		return m_vals;
	}

protected:
	vector<const char*> m_keys;
	vector<vector<unsigned char>> m_vals;
};