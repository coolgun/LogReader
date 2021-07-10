#pragma once
#include "Utilities.h"

/************************************************************************************************************************************************************ 
/* Method of Implementation
/* We will solve the problem by applying the following approach:
/* Step 1: If the current character of the pattern is * and if it matches with the present character of the string we can just move one step forward in string.
/* Step 2: If the * does not matches then we can just ignore it and move forward in the pattern by one step.
/* Step 3: If the current character of the pattern is '?' then we move forward in both pattern and string.
/* Step 4: If the current character of the pattern is not a wildcard character then it should positively match with the current character of the string.
/* Step 5: If we have reached the end of both string and pattern then we return true.
/* Step 6: If still string is left to be traversed and we have already reached the end of pattern then return false.
/* Step 7: If we have reached the end of string but the pattern is still left to be traversed then we will return true /* only if the rest of the characters in the pattern are only *.
/* 
/* Pseudocode
/* Step 1: Create a boolean 2-D matrix mat[m+1][n+1] where m is the length of the string and n is the length of the pattern.
/* Step 2: Initialize mat[0][0]=1(because empty string and empty pattern always match.
/* Step 3: Initialize mat[i][0]=0, because pattern will be empty.
/* Step 4: Update the value of mat[0][i]=mat[0][i-1] if the current pat[i-1] = *.
/* Step 5: Now start the comparison. If the current character of pattern is * then the value of mat[i][j]=mat[i-1][j]||mat[i][j-1].
/* Step 6: If the current character of pattern is '?', then both the current characters of pattern and string should match, therefore mat[i][j]= mat[i-1][j-1].
/* Step 7: If the current character of pattern is not wildcard character and if both the characters of pattern and string matches then mat[i][j]=mat[i-1][j-1]. Else if they don't match mat[i][j]=0.
/************************************************************************************************************************************************************/
namespace log_test
{
	// Implementing string - to - pattern matching
	class CSimpleRegexp
	{
		SimpleString filter;
		mutable bool* lookup_table{};
		mutable size_t lookup_size{};
	public:
		CSimpleRegexp(const char* filter);
		~CSimpleRegexp();
		
		// Flag that the correct wildcard is set
		bool IsOk() const;
		
		// Sets a new wildcard
		bool SetFilter(const char* filter);
		
		// Matches a string to a pattern
		bool Match(const char* test)const;
	private:
		// Collapsing consecutive asterisks
		void Simplify(const char* filter);
		void ResizeLookupBuffer(size_t new_lookup_size) const;
	};
}

