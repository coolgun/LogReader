#include "SimpleRegexp.h"
#include <windows.h>

namespace log_test
{
	CSimpleRegexp::CSimpleRegexp(const char* filter)
	{
		SetFilter(filter);
	}

	CSimpleRegexp::~CSimpleRegexp()
	{
		if (lookup_table)
			HeapFree(GetProcessHeap(), 0, lookup_table);
	}
	bool* lookup_table{};
	size_t lookup_size{};

	bool CSimpleRegexp::SetFilter(const char* new_filter)
	{
		filter.Reset();
		if (!new_filter) return false;
		Simplify(new_filter);
		return IsOk();
	}

	bool CSimpleRegexp::IsOk() const
	{
		return !filter.IsEmpty();
	}

	void CSimpleRegexp::Simplify(const char* new_filter)
	{
		char ch{};
		bool in_star_range = false;
		while (ch = *new_filter++)
		{
			if (ch == '*')
			{
				if (!in_star_range)
				{
					filter.PushBack(ch);
					in_star_range = true;
				}
			}
			else
			{
				in_star_range = false;
				filter.PushBack(ch);
			}
		}
	}

	void CSimpleRegexp::ResizeLookupBuffer(size_t new_lookup_size) const
	{
		if (new_lookup_size > lookup_size)
		{
			lookup_table = static_cast<bool*>(
				!lookup_size
				? HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, new_lookup_size)
				: HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, lookup_table, new_lookup_size)
				);
			lookup_size = new_lookup_size;
		}
	}

	bool CSimpleRegexp::Match(const char* test)const
	{
		if (!IsOk() && !test) return false;
		const size_t test_len = strlen(test);
		const size_t pattern_len = filter.Size();
		const auto* pattern = filter.Data();
		const size_t new_lookup_size = (test_len + 1) * (pattern_len + 1);
		ResizeLookupBuffer(new_lookup_size);
	
		ZeroMemory(lookup_table, new_lookup_size);
		lookup_table[0] = true;

		const auto lookup = [column_count = pattern_len + 1, this](size_t row, size_t column)->bool&
		{
			return lookup_table[row * column_count + column];
		};

		for (size_t j = 1; j <= pattern_len; ++j)
		{
			if (pattern[j - 1] == '*')
				lookup(0, j) = lookup(0, j - 1);
		}

		for (size_t i = 1; i <= test_len; i++)
		{
			for (size_t j = 1; j <= pattern_len; j++)
			{
				if (pattern[j - 1] == '*')
				{
					lookup(i, j) = lookup(i, j - 1) || lookup(i - 1, j);
				}
				else if (pattern[j - 1] == '?' || test[i - 1] == pattern[j - 1])
				{
					lookup(i, j) = lookup(i - 1, j - 1);
				}
				else
					lookup(i, j) = false;
			}
		}
		return lookup(test_len, pattern_len);
	}
	
}
