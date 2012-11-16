#include "generator_flash.hpp"
#include <boost/foreach.hpp>
#include <string>
#include <sstream>
#include <map>
#include <iomanip> // for hexadecimal output TODO probably not needed later!
#include <libhba/hba_datatypes.hpp>
#include "../../../shared/hexabus_statemachine_structs.h"

#pragma GCC diagnostic warning "-Wstrict-aliasing"

using namespace hexabus;

typedef std::map<unsigned int, std::string> flash_format_map_t;
typedef std::map<unsigned int, std::string>::iterator flash_format_map_it_t;
typedef std::map<unsigned int, struct transition> flash_format_state_map_t;
typedef std::map<unsigned int, struct transition>::iterator flash_format_state_map_it_t;
typedef std::map<unsigned int, struct transition> flash_format_trans_dt_map_t;
typedef std::map<unsigned int, struct transition>::iterator flash_format_trans_dt_map_it_t;
typedef std::map<unsigned int, struct condition> flash_format_cond_map_t;
typedef std::map<unsigned int, struct condition>::iterator flash_format_cond_map_it_t;

// TODO: States am Ende so sortieren, dass der Startstate der erste ist.

unsigned int state_index = 0; // TODO make this a little more elegant

struct hba_doc_visitor : boost::static_visitor<>
{
  hba_doc_visitor(
      flash_format_state_map_t& states_bin,
      flash_format_trans_dt_map_t& trans_dt_bin,
      flash_format_cond_map_t& conditions_bin,
      const graph_t_ptr g,
      Datatypes* datatypes)
    : _states_bin(states_bin),
      _trans_dt_bin(trans_dt_bin),
      _conditions_bin(conditions_bin),
      _g(g),
      _datatypes(datatypes)
  { }

  void operator()(if_clause_doc const& clause, unsigned int state_id, unsigned int state_index) const
  {
    // MD: SILENCE!
    //std::cout << "TRANSITION INDEX: " << state_index << " ===== STATE ID: " << state_id << std::endl << std::endl;
    unsigned int cond_id;
    if(clause.name != "true")
      cond_id = find_vertex_id(_g, clause.name);
    else
      cond_id = TRUE_COND_INDEX;
    unsigned int goodstate_id = find_vertex_id(_g, clause.goodstate);
    unsigned int badstate_id = find_vertex_id(_g, clause.badstate);

    //std::cout << state_id << ": clause " << clause.name
    //  << " (id " << cond_id << ")" << std::endl;
    //std::cout << " set eid " << clause.eid
    //  << " := " << clause.value << " datatype " << (unsigned int)_datatypes->getDatatype(clause.eid) << std::endl;
    //std::cout << " goodstate: " << goodstate_id << std::endl;
    //std::cout << " badstate: " << badstate_id << std::endl;

    // construct binary representation
    struct transition t;
    t.fromState = state_id; // TODO need to cast here? // better: Make sure only valid IDs are generated
    t.cond = cond_id;
    t.eid = clause.eid;
    t.goodState = goodstate_id;
    t.badState = badstate_id;
    t.value.datatype = _datatypes->getDatatype(clause.eid);
    memset(t.value.data, 0, sizeof(t.value.data)); // most of the time only four bytes needes, so keep the rest at 0
    std::stringstream ss;
    ss << std::hex << clause.value;
    if(_datatypes->getDatatype(clause.eid) == HXB_DTYPE_FLOAT) // TODO implement ALL the datatypes
    {
      ss >> *(float*)t.value.data;
    } else { // TODO for now, act as if everything were uint32
      ss >> std::dec >> *(uint32_t*)t.value.data;
    }

    if(clause.name != "true") {
      // look for condition corresponding to our transition -- if it's a date/time-transition, put it in a separate table
      // Assumption: Condition table is complete at this point; condition ID is unique in table
      for(flash_format_cond_map_it_t it = _conditions_bin.begin(); it != _conditions_bin.end(); it++)
      {
        if(it->first == cond_id)
        {
          if(it->second.value.datatype == HXB_DTYPE_DATETIME)
          {
            _trans_dt_bin.insert(std::pair<unsigned int, struct transition>(state_index, t));
            break;
          } else {
            _states_bin.insert(std::pair<unsigned int, struct transition>(state_index, t));
            break;
          }
          std::cout << std::endl;
        }
      }
    } else {
      // if it's a "true" transition, just insert it -- there is no condition entry in that case.
      _states_bin.insert(std::pair<unsigned int, struct transition>(state_index, t));
    }

    //std::cout << std::endl;
  }

  void operator()(state_doc const& hba) const
  {
    BOOST_FOREACH(if_clause_doc const& if_clause, hba.if_clauses)
    {
      Datatypes* dt = Datatypes::getInstance("datatypes");
      hba_doc_visitor p(_states_bin, _trans_dt_bin, _conditions_bin, _g, dt);
      p(if_clause, hba.id, state_index++);
    }
  }

  void operator()(condition_doc const& condition) const
  {
    // MD: SILENCE!
    //std::cout << "condition " << condition.id << ":" << std::endl;
    if(condition.cond.which() == 0) // eidvalue
    {
      cond_eidvalue_doc cond = boost::get<cond_eidvalue_doc>(condition.cond);
      //std::cout << "IPv6 addr: " << cond.ipv6_address << std::endl;
      //std::cout << "EID: " << cond.eid << std::endl;
      //std::cout << "Operator: " << cond.op << std::endl;
      //std::cout << "Value: " << cond.value << std::endl;
      //std::cout << "Datatype: " << (unsigned int)_datatypes->getDatatype(cond.eid) << std::endl;

      //construct binary representation
      struct condition c;
      // convert IP adress string to binary
      for(unsigned int i = 0; i < sizeof(c.sourceIP); i++)
      {
        std::stringstream ss;
        unsigned int ipbyte;
        ss << std::hex << cond.ipv6_address.substr(2 * i, 2); // read two bytes from the string, since two hex digits correspond to one byte in binary
        ss >> ipbyte;
        c.sourceIP[i] = ipbyte;
      }
      c.sourceEID = cond.eid;
      c.op = cond.op;
      c.value.datatype = _datatypes->getDatatype(cond.eid);
      std::stringstream ss;
      ss << std::hex << cond.value;
      if(_datatypes->getDatatype(cond.eid) == HXB_DTYPE_FLOAT) // TODO implement ALL the datatypes
      {
        ss >> *(float*)c.value.data;
      } else { // TODO for now just treat everything as uint32
        ss >> std::dec >> *(uint32_t*)c.value.data;
      }
      _conditions_bin.insert(std::pair<unsigned int, struct condition>(condition.id, c));
    } else if(condition.cond.which() == 1) { // timeout
      cond_timeout_doc cond = boost::get<cond_timeout_doc>(condition.cond);
      //std::cout << "Timeout: " << cond.value << std::endl;

      // construct binary representation
      struct condition c;
      memset(&c.sourceIP, 0, 15);
      c.sourceIP[15] = 1; // set IP address to ::1 for localost
      c.sourceEID = 0; // set to 0 because state machine won't care about the EID in a timestamp condition
      c.op = HXB_SM_TIMESTAMP_OP; // operator for timestamp comparison
      c.value.datatype = HXB_DTYPE_TIMESTAMP;
      *(uint32_t*)&c.value.data = cond.value;
      _conditions_bin.insert(std::pair<unsigned int, struct condition>(condition.id, c));
    } else {
      cond_datetime_doc cond = boost::get<cond_datetime_doc>(condition.cond);
      // MD: SILENCE!
      //std::cout << "Date/Time - ";
      //switch(cond.field)
      //{
      //  case HXB_SM_HOUR:
      //    std::cout << "hour";
      //    break;
      //  case HXB_SM_MINUTE:
      //    std::cout << "minute";
      //    break;
      //  case HXB_SM_SECOND:
      //    std::cout << "second";
      //    break;
      //  case HXB_SM_DAY:
      //    std::cout << "day";
      //    break;
      //  case HXB_SM_MONTH:
      //    std::cout << "month";
      //    break;
      //  case HXB_SM_YEAR:
      //    std::cout << "year";
      //    break;
      //  case HXB_SM_WEEKDAY:
      //    std::cout << "weekday";
      //    break;
      //  default:
      //    std::cout << "Date/time field definition error!" << std::endl;
      //}
      //if(cond.op == HXB_SM_DATETIME_OP_GEQ)
      //  std::cout << " >= ";
      //else
      //  std::cout << " < ";
      //std::cout << cond.value << std::endl;

      struct condition c;
      memset(&c.sourceIP, 0, 15);
      c.sourceIP[15] = 1; // set IP address to ::1 (localhost)
      c.sourceEID = 0;
      c.op = cond.field | cond.op;
      c.value.datatype = HXB_DTYPE_DATETIME;
      *(uint32_t*)&c.value.data = cond.value;

      _conditions_bin.insert(std::pair<unsigned int, struct condition>(condition.id, c));
    }
    // std::cout << std::endl;
  }

  flash_format_state_map_t& _states_bin;
  flash_format_trans_dt_map_t& _trans_dt_bin;
  flash_format_cond_map_t& _conditions_bin;
  graph_t_ptr _g;
  Datatypes* _datatypes;
};

void generator_flash::operator()(std::vector<uint8_t>& v) const
{
  flash_format_cond_map_t conditions_bin;
  flash_format_state_map_t states_bin;
  flash_format_trans_dt_map_t trans_dt_bin;

  unsigned char conditions_buffer[512];
  unsigned char* conditions_pos = &conditions_buffer[1];
  memset(conditions_buffer, 0, sizeof(conditions_buffer));
  unsigned char transitions_buffer[512];
  unsigned char* transitions_pos = &transitions_buffer[1];
  memset(transitions_buffer, 0, sizeof(transitions_buffer));
  unsigned char trans_dt_buffer[512];
  unsigned char* trans_dt_pos = &trans_dt_buffer[1];
  memset(trans_dt_buffer, 0, sizeof(trans_dt_buffer));

  //MD: SILENCE!

//  std::cout << "start state: " << _ast.start_state << std::endl;
  BOOST_FOREACH(hba_doc_block const& block, _ast.blocks)
  {
    Datatypes* dt = Datatypes::getInstance(_dtypes);
    boost::apply_visitor(hba_doc_visitor(states_bin, trans_dt_bin, conditions_bin, _g, dt), block);
  }

 // std::cout << "Binary condition table:" << std::endl;
  for(flash_format_cond_map_it_t it = conditions_bin.begin(); it != conditions_bin.end(); it++)
  {
  //  std::cout << it->first << ": ";
    struct condition cond = it->second;
   // unsigned char* c = (unsigned char*)&cond;
   // for(unsigned int i = 0; i < sizeof(condition); i++)
   // {
   //   std::cout << std::hex << std::setfill('0') << std::setw(2) << (unsigned short int)c[i] << " ";
   // }
   // std::cout << std::endl;

    // insert into buffer
    if(conditions_pos < conditions_buffer + sizeof(conditions_buffer) - sizeof(condition))
    {
      conditions_buffer[0]++;
      memcpy(conditions_pos, &cond, sizeof(cond));
      conditions_pos += sizeof(cond);
    }
  }
  //std::cout << std::endl;

  //std::cout << "Binary transition table:" << std::endl;
  for(flash_format_state_map_it_t it = states_bin.begin(); it != states_bin.end(); it++)
  {
   // std::cout << it->first << ": ";
    struct transition t = it->second;
   // char* c = (char*)&t;
   // for(unsigned int i = 0; i < sizeof(transition); i++)
   // {
   //   std::cout << std::hex << std::setfill('0') << std::setw(2) << (unsigned short int)c[i] << " ";
   // }
   // std::cout << std::endl;

   // insert into buffer
    if(transitions_pos < transitions_buffer + sizeof(transitions_buffer) - sizeof(transition))
    {
      transitions_buffer[0]++;
      memcpy(transitions_pos, &t, sizeof(t));
      transitions_pos += sizeof(t);
    }
  }
  //std::cout << std::endl;

  //std::cout << "Binary date/time transition table:" << std::endl;
  for(flash_format_trans_dt_map_it_t it = trans_dt_bin.begin(); it != trans_dt_bin.end(); it++)
  {
  //  std::cout << it->first << ": ";
    struct transition t = it->second;
   // char* c = (char*)&t;
   // for(unsigned int i = 0; i < sizeof(transition); i++)
   // {
   //   std::cout << std::hex << std::setfill('0') << std::setw(2) << (unsigned short int)c[i] << " ";
   // }
   // std::cout << std::endl;

    if(trans_dt_pos < trans_dt_buffer + sizeof(trans_dt_buffer) - sizeof(transition))
    {
      trans_dt_buffer[0]++;
      memcpy(trans_dt_pos, &t, sizeof(t));
      trans_dt_pos += sizeof(t);
    }
  }
  //std::cout << std::endl;

  //for debug purposes
  //std::cout << "Buffers to send to EEPROM:" << std::endl;
  //std::cout << "Conditions:" << std::endl;
  for(unsigned int i = 0; i < sizeof(conditions_buffer); i++)
  {
  //  std::cout << std::hex << std::setfill('0') << std::setw(2) << (unsigned short int)conditions_buffer[i] << " ";
  //  if(i % 24 == 23)
  //    std::cout << std::endl;

    v.push_back(conditions_buffer[i]);
  }
  //std::cout << std::endl << std::endl;

  //std::cout << "Date/Time-Transitions:" << std::endl;
  for(unsigned int i = 0; i < sizeof(trans_dt_buffer); i++)
  {
   // std::cout << std::hex << std::setfill('0') << std::setw(2) << (unsigned short int)trans_dt_buffer[i] << " ";
   //   if(i % 24 == 23)
   //   std::cout << std::endl;

    v.push_back(trans_dt_buffer[i]);
  }
  //std::cout << std::endl << std::endl;

  //std::cout << "Transitions:" << std::endl;
  for(unsigned int i = 0; i < sizeof(transitions_buffer); i++)
  {
   // std::cout << std::hex << std::setfill('0') << std::setw(2) << (unsigned short int)transitions_buffer[i] << " ";
   // if(i % 24 == 23)
   //   std::cout << std::endl;

    v.push_back(transitions_buffer[i]);
  }
  //std::cout << std::endl << std::endl;

 // std::cout << "done." << std::endl;
}

