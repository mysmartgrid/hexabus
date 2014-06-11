#ifndef FD_HPP_
#define FD_HPP_

#include <unistd.h>

class FD {
	private:
		int _fd;

	public:
		FD(int fd)
			: _fd(fd)
		{
		}
		
		~FD()
		{
			if (_fd >= 0)
				close(_fd);
		}

		operator int() const { return _fd; }
};

#endif
