///////////////////////////////////////////////////////////////////////////////
// Orkid
// Copyright 1996-2010, Michael T. Mayers
///////////////////////////////////////////////////////////////////////////////
#ifndef _ORK_STREAM_IINPUTSTREAM_H_
#define _ORK_STREAM_IINPUTSTREAM_H_

#include<cstddef>

namespace ork { namespace stream {

class IInputStream
{
public:
    typedef size_t size_type;
    static const size_t kEOF = (0xffffffff);

    virtual size_t Read(unsigned char *buffer, size_type bufmax) = 0;
    virtual ~IInputStream();
};

} }


#endif
