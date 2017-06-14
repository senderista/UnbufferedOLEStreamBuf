//Copyright (c) 2010 Tobin Baker
//
//Permission is hereby granted, free of charge, to any person obtaining a copy
//of this software and associated documentation files (the "Software"), to deal
//in the Software without restriction, including without limitation the rights
//to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//copies of the Software, and to permit persons to whom the Software is
//furnished to do so, subject to the following conditions:
//
//The above copyright notice and this permission notice shall be included in
//all copies or substantial portions of the Software.
//
//THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//THE SOFTWARE.


#include <ios>
#include <streambuf>
#include <windows.h>

// Read-write unbuffered streambuf implementation which uses no
// internal buffers (conventional wisdom says this can't be done
// except for write-only streams, but I adapted Matt Austern's example
// from http://www.drdobbs.com/184401305).

class UnbufferedOLEStreamBuf : public std::streambuf {
public:
	UnbufferedOLEStreamBuf(IStream *stream, std::ios_base::openmode which = std::ios_base::in | std::ios_base::out);
	~UnbufferedOLEStreamBuf();

protected:
	virtual int overflow(int ch = traits_type::eof());
	virtual int underflow();
	virtual int uflow();
	virtual int pbackfail(int ch = traits_type::eof());
	virtual int sync();
	virtual std::streampos seekpos(std::streampos sp, std::ios_base::openmode which = std::ios_base::in | std::ios_base::out);
	virtual std::streampos seekoff(std::streamoff off, std::ios_base::seekdir way, std::ios_base::openmode which = std::ios_base::in | std::ios_base::out);
	virtual std::streamsize xsgetn(char *s, std::streamsize n);
	virtual std::streamsize xsputn(const char *s, std::streamsize n);
	virtual std::streamsize showmanyc();

private:
	IStream *stream_;
	bool readOnly_;
	bool backup();
};

UnbufferedOLEStreamBuf::UnbufferedOLEStreamBuf(IStream *stream, std::ios_base::openmode which)
: std::streambuf(), stream_(stream), readOnly_(!(which & std::ios_base::out)) {}

UnbufferedOLEStreamBuf::~UnbufferedOLEStreamBuf() { if (!readOnly_) UnbufferedOLEStreamBuf::sync(); }

bool UnbufferedOLEStreamBuf::backup() {
	LARGE_INTEGER liMove;
	liMove.QuadPart = -1LL;
	return SUCCEEDED(stream_->Seek(liMove, STREAM_SEEK_CUR, NULL));
}

int UnbufferedOLEStreamBuf::overflow(int ch) {
	if (ch != traits_type::eof()) {
		if (SUCCEEDED(stream_->Write(&ch, 1, NULL))) {
			return ch;
		}
	}
	return traits_type::eof();
}

int UnbufferedOLEStreamBuf::underflow() {
	char ch = UnbufferedOLEStreamBuf::uflow();
	if (ch != traits_type::eof()) {
		ch = backup() ? ch : traits_type::eof();
	}
	return ch;
}

int UnbufferedOLEStreamBuf::uflow() {
	char ch;
	ULONG cbRead;
	// S_FALSE indicates we've reached end of stream
	return (S_OK == stream_->Read(&ch, 1, &cbRead))
		? ch : traits_type::eof();
}

int UnbufferedOLEStreamBuf::pbackfail(int ch) {
	if (ch != traits_type::eof()) {
		ch = backup() ? ch : traits_type::eof();
	}
	return ch;
}

int UnbufferedOLEStreamBuf::sync() {
	return SUCCEEDED(stream_->Commit(STGC_DEFAULT)) ? 0 : -1;
}

std::ios::streampos UnbufferedOLEStreamBuf::seekpos(std::ios::streampos sp, std::ios_base::openmode which) {
	LARGE_INTEGER liMove;
	liMove.QuadPart = sp;
	return SUCCEEDED(stream_->Seek(liMove, STREAM_SEEK_SET, NULL)) ? sp : -1;
}

std::streampos UnbufferedOLEStreamBuf::seekoff(std::streamoff off, std::ios_base::seekdir way, std::ios_base::openmode which) {
	STREAM_SEEK sk;
	switch (way) {
		case std::ios_base::beg: sk = STREAM_SEEK_SET; break;
		case std::ios_base::cur: sk = STREAM_SEEK_CUR; break;
		case std::ios_base::end: sk = STREAM_SEEK_END; break;
		default: return -1;
	}
	LARGE_INTEGER liMove;
	liMove.QuadPart = static_cast<LONGLONG>(off);
	ULARGE_INTEGER uliNewPos;
	return SUCCEEDED(stream_->Seek(liMove, sk, &uliNewPos))
		? static_cast<std::streampos>(uliNewPos.QuadPart) : -1;
}

std::streamsize UnbufferedOLEStreamBuf::xsgetn(char *s, std::streamsize n) {
	ULONG cbRead;
	return SUCCEEDED(stream_->Read(s, static_cast<ULONG>(n), &cbRead))
		? static_cast<std::streamsize>(cbRead) : 0;
}

std::streamsize UnbufferedOLEStreamBuf::xsputn(const char *s, std::streamsize n) {
	ULONG cbWritten;
	return SUCCEEDED(stream_->Write(s, static_cast<ULONG>(n), &cbWritten))
 		? static_cast<std::streamsize>(cbWritten) : 0; 
}

std::streamsize UnbufferedOLEStreamBuf::showmanyc() {
	STATSTG stat;
	if (SUCCEEDED(stream_->Stat(&stat, STATFLAG_NONAME))) {
		std::streampos lastPos = static_cast<std::streampos>(stat.cbSize.QuadPart - 1);
		LARGE_INTEGER liMove;
		liMove.QuadPart = 0LL;
		ULARGE_INTEGER uliNewPos;
		if (SUCCEEDED(stream_->Seek(liMove, STREAM_SEEK_CUR, &uliNewPos))) {
			return std::max<std::streamsize>(lastPos - static_cast<std::streampos>(uliNewPos.QuadPart), 0);
		}
	}
	return 0;
}

std::streambuf* StdStreamBufFromOLEStream(IStream *pStream, std::ios_base::openmode which)
{
	return new (std::nothrow) UnbufferedOLEStreamBuf(pStream, which);
}
