#include <netlink/genl/ctrl.h>

#include "netlink.hpp"

namespace nl {

msg::msg(int family, int hdrlen, int flags, int cmd)
	: _msg(nlmsg_alloc())
{
	if (!_msg)
		throw std::bad_alloc();

	if (!genlmsg_put(_msg, NL_AUTO_PORT, NL_AUTO_SEQ, family, hdrlen, flags, cmd, 1))
		HXBNM_THROW(nl_message, "genlmsg_put failed");
}

msg::~msg()
{
	nlmsg_free(_msg);
}

void msg::put_raw(int type, const void* data, size_t len)
{
	int err = nla_put(_msg, type, len, data);

	if (err)
		HXBNM_THROW(nl_message, nl_geterror(err));
}




socket::socket(int family)
	: nl(nl_socket_alloc())
{
	if (!nl)
		throw std::bad_alloc();

	int err = genl_connect(nl);
	if (err < 0)
		HXBNM_THROW(nl_message, nl_geterror(err));
}

socket::~socket()
{
	nl_socket_free(nl);
}

int socket::sendmsg(msg& msg)
{
	int res = nl_send_auto(nl, msg.raw());
	if (res < 0)
		HXBNM_THROW(nl_message, nl_geterror(res));

	return res;
}



int get_family(const char* family_name)
{
	nl_sock* nl = nl_socket_alloc();

	if (!nl)
		throw std::bad_alloc();

	int err = genl_connect(nl);
	if (err < 0) {
		nl_socket_free(nl);
		HXBNM_THROW(nl_message, nl_geterror(err));
	}

	int family = genl_ctrl_resolve(nl, family_name);
	nl_socket_free(nl);

	if (family < 0)
		HXBNM_THROW(nl_message, nl_geterror(err));
	return family;
}

}
