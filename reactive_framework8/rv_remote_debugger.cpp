#include "stdafx.h"
#include "rv_remote_debugger.hpp"


using namespace reactive_framework8;
using namespace std;
using namespace utility;
namespace pt = boost::property_tree; 

// types of messages
enum E_EVENT_TYPE : size_t
{
	EVENT_TYPE_VALUE_CHANGE,
	EVENT_TYPE_NEW_INPUT_EDGE,
	EVENT_TYPE_NEW_OUTPUT_EDGE,
};

enum E_VERTEX_MODE : int { GENERIC, INPUT_EDGE, OUTPUT_EDGE };

string EMPTY_STRING;

rv_remote_debugger::rv_remote_debugger()
	: _client { "localhost", desc { "8000" } }
{
}

rv_remote_debugger::~rv_remote_debugger()
{
}

void rv_remote_debugger::notify_value_change(string rv_name_, string value_)
{
	stringstream json_stream;
	pt::ptree ptree;

	ptree.put("event", "value.change");
	ptree.put("vertex", move(rv_name_));
	ptree.put("covertex", EMPTY_STRING);
	ptree.put("value", move(value_));

	pt::json_parser::write_json(json_stream, ptree);

	_client.write(json_stream.str().c_str(), json_stream.str().length());	
}

void rv_remote_debugger::notify_rv_assigned_to(std::string rv_name_)
{
	if (_known_objects.find(rv_name_) == _known_objects.end())
	{
		_known_objects.insert(rv_name_);
		stringstream json_stream;
		pt::ptree ptree;

		ptree.put("event", "value.add");
		ptree.put("vertex", move(rv_name_));
		ptree.put("vertextype", "value");

		pt::json_parser::write_json(json_stream, ptree);

		_client.write(json_stream.str().c_str(), json_stream.str().length());
	}
}

void rv_remote_debugger::notify_new_operator(std::string op_name_)
{
	if(_known_objects.find(op_name_) == _known_objects.end())
	{
		_known_objects.insert(op_name_);

		stringstream json_stream;
		pt::ptree ptree;

		ptree.put("event", "operator.add");
		ptree.put("vertex", move(op_name_));
		ptree.put("vertextype", "operator");

		pt::json_parser::write_json(json_stream, ptree);

		_client.write(json_stream.str().c_str(), json_stream.str().length());
	}
}

void rv_remote_debugger::add_edge_from(void* node_ptr_, std::type_index node_type_, void* operator_ptr_, std::type_index operator_type_)
{
	stringstream json_stream;
	pt::ptree ptree;

	ptree.put("event", "edge.add");
	ptree.put("vertex", name_of(node_ptr_, node_type_));
	ptree.put("covertex", name_of(operator_ptr_, operator_type_));
	ptree.put("vertextype", "value");
	ptree.put("covertextype", "operator");

	pt::json_parser::write_json(json_stream, ptree);

	_client.write(json_stream.str().c_str(), json_stream.str().length());
}

void rv_remote_debugger::add_edge_to(void* operator_ptr_, std::type_index operator_type_, void* node_ptr_, std::type_index node_type_)
{
	stringstream json_stream;
	pt::ptree ptree;

	ptree.put("event", "edge.add");
	ptree.put("vertex", name_of(operator_ptr_, operator_type_));
	ptree.put("covertex", name_of(node_ptr_, node_type_));
	ptree.put("vertextype", "operator");
	ptree.put("covertextype", "value");

	pt::json_parser::write_json(json_stream, ptree);

	_client.write(json_stream.str().c_str(), json_stream.str().length());
}
