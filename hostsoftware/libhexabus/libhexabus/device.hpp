/**
 * This file is part of libhexabus.
 *
 * (c) Fraunhofer ITWM - Stephan Platz <platz@itwm.fhg.de>, 2014
 *
 * libhexabus is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * libhexabus is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with libhexabus. If not, see <http://www.gnu.org/licenses/>.
 *
 */
#ifndef LIBHEXABUS_DEVICE_HPP
#define LIBHEXABUS_DEVICE_HPP 1

#include <boost/asio/io_service.hpp>
#include <boost/asio/deadline_timer.hpp>

#include <libhexabus/endpoint_registry.hpp>
#include <libhexabus/packet.hpp>
#include <libhexabus/socket.hpp>

namespace hexabus {
	class EndpointFunctions {
		public:
			typedef std::tr1::shared_ptr<EndpointFunctions> Ptr;
			uint32_t eid() const { return _eid; }
			std::string name() const { return _name; }
			uint8_t datatype() const { return _datatype; }

			virtual hexabus::Packet::Ptr handle_query(uint16_t cause) const = 0;
			virtual uint8_t handle_write(const hexabus::Packet& p) const = 0;

		protected:
			EndpointFunctions(uint32_t eid, const std::string& name, uint8_t datatype)
				: _eid(eid)
				, _name(name)
				, _datatype(datatype)
			{}

		private:
			uint32_t _eid;
			std::string _name;
			uint8_t _datatype;
	};

	template<typename TValue>
	class TypedEndpointFunctions : public EndpointFunctions {
		public:
			typedef std::tr1::shared_ptr<TypedEndpointFunctions<TValue> > Ptr;
			typedef boost::function<TValue ()> endpoint_read_fn_t;
			typedef boost::function<bool (const TValue& value)> endpoint_write_fn_t;
			TypedEndpointFunctions(uint32_t eid, const std::string& name)
				: EndpointFunctions(eid, name, calculateDatatype())
			{}

			boost::signals2::connection onRead(
					const endpoint_read_fn_t& callback) {
				boost::signals2::connection result = _read.connect(callback);

				return result;
			}
			boost::signals2::connection onWrite(
					const endpoint_write_fn_t& callback) {
				boost::signals2::connection result = _write.connect(callback);

				return result;
			}

			virtual hexabus::Packet::Ptr handle_query(uint16_t cause) const {
				if ( _read.num_slots() < 1 )
						return Packet::Ptr(new ErrorPacket(HXB_ERR_UNKNOWNEID, cause));

				boost::optional<TValue> value = _read();
				if ( !value ) {
					std::stringstream oss;
					oss << "Error reading endpoint " << name() << " (" << eid() << ")";
					throw hexabus::GenericException(oss.str());
				}
				return Packet::Ptr(new InfoPacket<TValue>(eid(), *value));
			}

			virtual uint8_t handle_write(const hexabus::Packet& p) const {
				const WritePacket<TValue>* write = dynamic_cast<const WritePacket<TValue>*>(&p);
				if ( write != NULL ) {
					if ( _write.num_slots() < 1 ) {
						return HXB_ERR_WRITEREADONLY;
					}

					TValue value = write->value();
					if ( _write(value) ) {
						return HXB_ERR_SUCCESS;
					}
				}

				return HXB_ERR_INTERNAL;
			}

			static typename TypedEndpointFunctions<TValue>::Ptr fromEndpointDescriptor(const EndpointDescriptor& ep) {
				typename TypedEndpointFunctions<TValue>::Ptr result(new TypedEndpointFunctions<TValue>(ep.eid(), ep.description()));

				if (ep.type() != result->datatype())
					throw hexabus::GenericException("Datatype mismatch while creating endpoint functions.");

				return result;
			}

		private:
			boost::signals2::signal<TValue ()> _read;
			boost::signals2::signal<bool (const TValue&)> _write;

			BOOST_STATIC_ASSERT_MSG((
				boost::is_same<TValue, bool>::value
					|| boost::is_same<TValue, uint8_t>::value
					|| boost::is_same<TValue, uint32_t>::value
					|| boost::is_same<TValue, float>::value
					|| boost::is_same<TValue, std::string>::value
					|| boost::is_same<TValue, boost::posix_time::ptime>::value
					|| boost::is_same<TValue, boost::posix_time::time_duration>::value
					|| boost::is_same<TValue, boost::array<char, 16> >::value
					|| boost::is_same<TValue, boost::array<char, 65> >::value),
				"I don't know how to handle that type");

			static uint8_t calculateDatatype()
			{
				return boost::is_same<TValue, bool>::value ? HXB_DTYPE_BOOL :
					boost::is_same<TValue, uint8_t>::value ? HXB_DTYPE_UINT8 :
					boost::is_same<TValue, uint32_t>::value ? HXB_DTYPE_UINT32 :
					boost::is_same<TValue, float>::value ? HXB_DTYPE_FLOAT :
					boost::is_same<TValue, std::string>::value ? HXB_DTYPE_128STRING :
					boost::is_same<TValue, boost::posix_time::ptime>::value ? HXB_DTYPE_DATETIME :
					boost::is_same<TValue, boost::posix_time::time_duration>::value ? HXB_DTYPE_TIMESTAMP :
					boost::is_same<TValue, boost::array<char, 16> >::value
						? HXB_DTYPE_16BYTES :
					boost::is_same<TValue, boost::array<char, 65> >::value
						? HXB_DTYPE_65BYTES :
					(throw "BUG: Unknown datatype!", HXB_DTYPE_UNDEFINED);
			}
	};

	class Device {
		public:
			typedef boost::function<std::string ()> read_name_fn_t;
			typedef boost::function<void (const std::string& name)> write_name_fn_t;
			typedef boost::function<void (const GenericException& error)> async_error_fn_t;
			Device(boost::asio::io_service& io, const std::vector<std::string>& interfaces, const std::vector<std::string>& addresses, int interval = 60);
			~Device();
			void addEndpoint(const EndpointFunctions::Ptr ep);

			boost::signals2::connection onReadName(
					const read_name_fn_t& callback);
			boost::signals2::connection onWriteName(
					const write_name_fn_t& callback);
			boost::signals2::connection onAsyncError(
					const async_error_fn_t& callback);
		protected:
			void _handle_query(hexabus::Socket* socket, const hexabus::Packet& p, const boost::asio::ip::udp::endpoint& from);
			void _handle_write(hexabus::Socket* socket, const hexabus::Packet& p, const boost::asio::ip::udp::endpoint& from);
			void _handle_epquery(const hexabus::Packet& p, const boost::asio::ip::udp::endpoint& from);
			void _handle_descquery(const hexabus::Packet& p, const boost::asio::ip::udp::endpoint& from);
			void _handle_descepquery(const hexabus::Packet& p, const boost::asio::ip::udp::endpoint& from);
			void _handle_epquery(hexabus::Socket* socket, const hexabus::Packet& p, const boost::asio::ip::udp::endpoint& from);
			void _handle_descquery(hexabus::Socket* socket, const hexabus::Packet& p, const boost::asio::ip::udp::endpoint& from);
			void _handle_descepquery(hexabus::Socket* socket, const hexabus::Packet& p, const boost::asio::ip::udp::endpoint& from);
			uint8_t _handle_smcontrolquery();
			bool _handle_smcontrolwrite(uint8_t);
			void _handle_smupload(hexabus::Socket* socket, const hexabus::Packet& p, const boost::asio::ip::udp::endpoint& from);
		private:
			void _handle_broadcasts(const boost::system::error_code& error);
			void _handle_errors(const hexabus::GenericException& error);

			boost::signals2::signal<std::string ()> _read;
			boost::signals2::signal<void (const std::string&)> _write;
			boost::signals2::signal<void (const GenericException& error)> _asyncError;

			hexabus::Listener _listener;
			std::vector<hexabus::Socket*> _sockets;
			boost::asio::deadline_timer _timer;
			int _interval;
			std::map<uint32_t, const EndpointFunctions::Ptr> _endpoints;
			uint8_t _sm_state;
	};
}

#endif // LIBHEXABUS_DEVICE_HPP
