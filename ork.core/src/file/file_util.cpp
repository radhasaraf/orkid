////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2023, Michael T. Mayers.
// Distributed under the MIT License.
// see license-mit.txt in the root of the repo, and/or https://opensource.org/license/mit/
//////////////////////////////////////////////////////////////// 


#include <ork/pch.h>

#include <ork/file/file_util.h>
#include <ork/stream/IInputStream.h>

namespace ork { namespace file {

///////////////////////////////////////////////////////////////////////////////

bool ReadLine(ork::stream::InputStreamBuffer<128>& file, char *buffer, int maxlen)
{
	bool result = false;
	char *bufmax = buffer + maxlen - 1;
		
	while(buffer < bufmax)
	{
		char c;
		if(file.Read(reinterpret_cast<unsigned char *>(&c), 1) != 1)
			break;
			
		result = true;
			
		if(c == '\r')
		{
			char peek;
			// Handle Windows/DOS line endings...
			if(file.Peek(reinterpret_cast<unsigned char *>(&peek), 1) == ork::stream::IInputStream::kEOF)
				break;
			if(peek == '\n')
			{
				file.Read(reinterpret_cast<unsigned char *>(&c), 1);
			}
			break;
		}
		if(c == '\n')
		{
			break;
		}
		*buffer++ = c;
	}
	*buffer = '\0';
	return result;
}

///////////////////////////////////////////////////////////////////////////////

void Word(ork::MutableString result, const char *buffer, int &index, int buf_size)
{
	Word(result, buffer, index, buf_size, isspace);
}

///////////////////////////////////////////////////////////////////////////////

} } // namespace ork::file
