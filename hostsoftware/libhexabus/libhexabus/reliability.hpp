#ifndef LIBHEXABUS_RELIABILITY_HPP
#define LIBHEXABUS_RELIABILITY_HPP 1

#include <boost/lockfree/queue.hpp>
#include <boost/statechart/event.hpp>
#include <boost/statechart/state_machine.hpp>
#include <boost/statechart/simple_state.hpp>
#include <boost/statechart/transition.hpp>

#define MAXIMUM_QUEUE_SIZE

namespace bl = boost::lockfree;
namespace sc = boost::statechart;

namespace hexabus {
	class SendingStatemachine {
		public:
			SendingStatemachine();
			~SendingStatemachine();

			void process_event(const event_base &);

		private:
			//Events for sending state machine
			struct EvReset : sc::event< EvReset > {}; 
			struct EvInit : sc::event< EvInit > {};
			struct EvSendQueueNotEmpty : sc::event< EvSendQueueNotEmpty > {};
			struct EvTimeout : sc::event< EvTimeout> {};
			struct EvAckReceived : sc::event< EvAckReceived > {};
			struct EvRecover : sc::event< EvRecover > {};

			//States for sending state machine
			struct StActive;
			struct SendingStateMachine : sc::state_machine< SendingStateMachine, StActive > {};

			struct StInit;
			struct StActive : sc::simple_state< StActive, SendingStateMachine, StInit > {
				typedef sc::custom_reaction< EvReset > reactions;
				sc::result react( const EvReset &);
			};
			struct StInit : sc::simple_state< StInit, StActive > {
				typedef sc::custom_reaction< EvInit > reactions;
				sc::result react( const EvInit &);
			};
			struct StReady : sc::simple_state< StReady, StActive > {
				typedef sc::custom_reaction< EvSendQueueNotEmpty > reactions;
				sc::result react( const EvSendQueueNotEmpty &); 
			};
			struct StWaitAck : sc::simple_state< StWaitAck, StActive > {
				typedef mpl::list<
					sc::custom_reaction< EvAckReceived >,
					sc::custom_reaction< EvTimeout >
				> reactions;
				sc::result react( const EvAckReceived &);
				sc::result react( const EvTimeout &);
			};
			struct StFailed : sc::simple_state< StFailed, StActive > {
				typedef sc::custom_reaction< EvRecover > reactions;
				sc::result react( const EvRecover &);
			};
	};

	class ReceivingStatemachine {
		public:
			ReceivingStatemachine();
			~ReceivingStatemachine();

			void process_event(const event_base &);

		private:
			//Events for sending state machine
			struct EvReset : sc::event< EvReset > {}; 
			struct EvInit : sc::event< EvInit > {};
			struct EvCorrectPacketReceived : sc::event< EvCorrectPacketReceived > {};
			struct EvFail : sc::event< EvFail > {};

			//States for sending state machine
			struct StActive;
			struct ReceivingStatemachine : sc::state_machine< ReceivingStatemachine, StActive > {};

			struct StInit;
			struct StActive : sc::simple_state< StActive, ReceivingStatemachine, StInit > {
				typedef sc::custom_reaction< EvReset > reactions;
				sc::result react( const EvReset &);
			};
			struct StInit : sc::simple_state< StInit, StActive > {
				typedef sc::custom_reaction< EvInit > reactions;
				sc::result react( const EvInit &);
			};

			struct StReady : sc::simple_state< StReady, StActive > {
				typedef mpl::list <
					sc::custom_reaction< EvCorrectPacketReceived >,
					sc::custom_reaction< EvFail >
				> reactions;
				sc::result react( const EvCorrectPacketReceived &);
				sc::result react( const EvFail &); 
			};

			struct StFailed : sc::simple_state< StFailed, StActive > {
				typedef sc::custom_reaction< EvCorrectPacketReceived > reactions;
				sc::result react( const EvCorrectPacketReceived & );
			};

	};
}

#endif