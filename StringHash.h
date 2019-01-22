/*
* Compile-time hashed strings based in an article from "Game Engine Gems 3 - Stefan Reinalter"
* 
* With the code below it is possible to hash string, both at compile- and runtime. This technique can
* come in handy for places where performance is a key concern. Costly string comparisons can be replaced
* by integer-integer comparisons and the hashing needed for this is done during compilation.
*/

// The MIT License(MIT)
// 
// Copyright(c) 2015 Stefan Reinalter
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#ifndef _STRING_HASH_H_
#define _STRING_HASH_H_

#include <cstdint>
#include <cstring>

/*
* Several function overloads th ergonomically create a FNV1-a hash of a
* const char* at runtime. The details and prime/offset values can be
* simply looked up at https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function
*
* We use a 32-bit variant here, therefore 16777619u as FNV prime and 2166136261u as the offset value.
*/
uint32_t GenerateFNV1aHash(const char* str, size_t length, uint32_t hash)
{
	for (size_t byte = 0u; byte < length; ++byte)
	{
		uint32_t value = static_cast<uint32_t>(*str++);
		hash ^= value;
		hash *= 16777619u;
	}

	return hash;
}

uint32_t GenerateFNV1aHash(const char* str, size_t length)
{
	return GenerateFNV1aHash(str, length, 2166136261u);
}

uint32_t GenerateFNV1aHash(const char* str)
{
	return GenerateFNV1aHash(str, strlen(str));
}

/*
* The following two 'Hash' structs are the basis for the template-metaprogram that computes the FNV1-a
* hash recursivly during compile-time. Beginning from the LAST element, each recursion step performs
* a prime-multiply and XOR with the previous value. 
*
* The second partial specialization serves at the end-condition for the recursion. It simply emits
* the FNV1-a prime offset.
*
* It is also interesting that bc the GenerateHash func is marked as constexpr, it is possible
* to use it in context where only constants are allowed (switches, non-type template params, ...)
*
*/

template<size_t I>
struct Hash
{
	template <size_t N>
	static constexpr uint32_t GenerateHash(const char(&str)[N])
	{
		return (Hash<I - 1u>::GenerateHash(str) ^ static_cast<uint32_t>(str[I - 1])) * 16777619u;
	}
};

template<>
struct Hash<0u>
{
	template <size_t N>
	static constexpr uint32_t GenerateHash(const char(&str)[N])
	{
		return 2166136261u;
	}
};

/*
* Because it is also desirable to have the same hashing capabilities for runtime strings (const char* / std::string's c_str())
* those two helper templates obtain the needed knowledge of the provided types to decide whether it is a compile-time or run-time
* construct. The distinct type of a string literal ("abc") is const char (&) [N] and just decays to a pointer when passed to a const char*
*/

template<typename T>
struct HashHelper {};

template<>
struct HashHelper<const char*>
{
	static uint32_t Generate(const char* str)
	{
		return GenerateFNV1aHash(str);
	}
};

template<size_t N>
struct HashHelper<char [N]>
{
	static constexpr uint32_t Generate(const char(&str)[N])
	{
		return Hash<N - 1u>::GenerateHash(str);
	}
};

template<typename T>
constexpr uint32_t GenerateHash(const T& str)
{
	return HashHelper<T>::Generate(str);
}

/*
* The 'StringHash' class is just a small helper to pass hashed strings to functions without having
* to worry whether they are run-time or compile-time strings. The non-explicit CTOR allows for that.
*/

class StringHash
{
public:
	template<typename T>
	StringHash(const T& str)
		: m_hash(GenerateHash(str))
	{}

	uint32_t Get(void) { return m_hash; }

private:
	uint32_t m_hash;
};

#endif // _STRING_HASH_H_