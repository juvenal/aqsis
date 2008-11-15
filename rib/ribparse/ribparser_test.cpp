// Aqsis
// Copyright (C) 1997 - 2007, Paul C. Gregory
//
// Contact: pgregory@aqsis.org
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

/** \file
 * \brief Unit tests for the RIB parser.
 * \author Chris Foster  [chris42f (at) gmail (dot) com]
 */

#include <sstream>

#include <boost/test/auto_unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>

#include <boost/any.hpp>

#include "ribparser.h"
#include "smartptr.h"
#include "primvartoken.h"

using namespace Aqsis;

//------------------------------------------------------------------------------
// Tests for getting data from the parser.

struct NullRibRequestHandler : public IqRibRequestHandler
{
	virtual void handleRequest(const std::string& requestName,
			CqRibParser& parser)
	{
		assert(0 && "shouldn't call NullRibRequestHandler::handleRequest()");
	}
};

struct NullRequestFixture
{
	std::istringstream in;
	CqRibLexer lex;
	NullRibRequestHandler nullHandler;
	CqRibParser parser;

	NullRequestFixture(const std::string& stringToParse)
		: in(stringToParse),
		lex(in),
		nullHandler(),
		parser(boost::shared_ptr<CqRibLexer>(&lex, nullDeleter),
			boost::shared_ptr<NullRibRequestHandler>(&nullHandler, nullDeleter))
	{ }
};

BOOST_AUTO_TEST_CASE(CqRibParser_get_scalar_tests)
{
	NullRequestFixture f("42 3.141592 3 \"some_string\" ");

	BOOST_CHECK_EQUAL(f.parser.getInt(), 42);
	BOOST_CHECK_CLOSE(f.parser.getFloat(), 3.141592f, 0.0001f);
	BOOST_CHECK_CLOSE(f.parser.getFloat(), 3.0f, 0.0001f);
	BOOST_CHECK_EQUAL(f.parser.getString(), "some_string");
}

BOOST_AUTO_TEST_CASE(CqRibParser_getIntArray_test)
{
	NullRequestFixture f("[1 2 3 4] [] [1 1.1]");

	// check that we can read int arrays
	const CqRibParser::TqIntArray& a1 = f.parser.getIntArray();
	BOOST_REQUIRE_EQUAL(a1.size(), 4U);
	BOOST_CHECK_EQUAL(a1[0], 1);
	BOOST_CHECK_EQUAL(a1[1], 2);
	BOOST_CHECK_EQUAL(a1[2], 3);
	BOOST_CHECK_EQUAL(a1[3], 4);

	// check that we can read empty arrays
	const CqRibParser::TqIntArray& a3 = f.parser.getIntArray();
	BOOST_CHECK_EQUAL(a3.size(), 0U);

	// check throw on reading a non-int array
	BOOST_CHECK_THROW(f.parser.getIntArray(), XqParseError);
}

BOOST_AUTO_TEST_CASE(CqRibParser_getFloatArray_test)
{
	NullRequestFixture f("[1 2.1 3.2 4] [-2.0e1 -4.1]  1 2.0  [1 2.0] [\"asdf\"]");

	const TqFloat eps = 0.0001;

	// check that mixed ints and floats can be read as a float array.
	const CqRibParser::TqFloatArray& a1 = f.parser.getFloatArray();
	BOOST_REQUIRE_EQUAL(a1.size(), 4U);
	BOOST_CHECK_CLOSE(a1[0], 1.0f, eps);
	BOOST_CHECK_CLOSE(a1[1], 2.1f, eps);
	BOOST_CHECK_CLOSE(a1[2], 3.2f, eps);
	BOOST_CHECK_CLOSE(a1[3], 4.0f, eps);

	// check that we can read a float array
	const CqRibParser::TqFloatArray& a2 = f.parser.getFloatArray();
	BOOST_REQUIRE_EQUAL(a2.size(), 2U);
	BOOST_CHECK_CLOSE(a2[0], -2.0e1f, eps);
	BOOST_CHECK_CLOSE(a2[1], -4.1f, eps);

	// check that we can read a float array with fixed length in either format.
	{
		const CqRibParser::TqFloatArray& a = f.parser.getFloatArray(2);
		BOOST_REQUIRE_EQUAL(a.size(), 2U);
		BOOST_CHECK_CLOSE(a[0], 1.0f, eps);
		BOOST_CHECK_CLOSE(a[1], 2.0f, eps);
	}
	{
		const CqRibParser::TqFloatArray& a = f.parser.getFloatArray(2);
		BOOST_REQUIRE_EQUAL(a.size(), 2U);
		BOOST_CHECK_CLOSE(a[0], 1.0f, eps);
		BOOST_CHECK_CLOSE(a[1], 2.0f, eps);
	}

	// check throw on reading a non-float array
	BOOST_CHECK_THROW(f.parser.getFloatArray(), XqParseError);
}

BOOST_AUTO_TEST_CASE(CqRibParser_getStringArray_test)
{
	NullRequestFixture f("[\"asdf\" \"1234\" \"!@#$\"] 123");

	// Check that we can read string arrays
	const CqRibParser::TqStringArray& a = f.parser.getStringArray();
	BOOST_REQUIRE_EQUAL(a.size(), 3U);
	BOOST_CHECK_EQUAL(a[0], "asdf");
	BOOST_CHECK_EQUAL(a[1], "1234");
	BOOST_CHECK_EQUAL(a[2], "!@#$");

	// check throw on reading a non-string array
	BOOST_CHECK_THROW(f.parser.getStringArray(), XqParseError);
}

class StringToBasisDummy : public IqStringToBasis
{
	public:
		virtual CqRibParser::TqBasis* getBasis(const std::string& name) const
		{
			static CqRibParser::TqBasis testBasis = {
				{1, 2, 3, 4},
				{5, 6, 7, 8},
				{9, 10, 11, 12},
				{13, 14, 15, 16}
			};
			if(name == "test")
				return &testBasis;
			else
				return 0;
		}
};

BOOST_AUTO_TEST_CASE(CqRibParser_getBasis_test)
{
	NullRequestFixture f("[ 0 1 2 3   4 5 6 7   8 9 10 11   12 13 14 15] "
			" \"test\" \"unknown\"");
	const CqRibParser::TqBasis* basis = f.parser.getBasis(StringToBasisDummy());
	for(int i = 0; i < 16; ++i)
		BOOST_CHECK_CLOSE((*basis)[i/4][i%4], TqFloat(i), 0.00001);

	basis = f.parser.getBasis(StringToBasisDummy());
	for(int i = 0; i < 16; ++i)
		BOOST_CHECK_CLOSE((*basis)[i/4][i%4], TqFloat(i+1), 0.00001);

	basis = f.parser.getBasis(StringToBasisDummy());
	BOOST_CHECK_EQUAL(basis, static_cast<CqRibParser::TqBasis*>(0));
}

BOOST_AUTO_TEST_CASE(CqRibParser_getIntParam_test)
{
	NullRequestFixture f("1 [2 3]");

	const CqRibParser::TqIntArray& a1 = f.parser.getIntParam();
	BOOST_REQUIRE_EQUAL(a1.size(), 1U);
	BOOST_CHECK_EQUAL(a1[0], 1);

	const CqRibParser::TqIntArray& a2 = f.parser.getIntParam();
	BOOST_REQUIRE_EQUAL(a2.size(), 2U);
	BOOST_CHECK_EQUAL(a2[0], 2);
	BOOST_CHECK_EQUAL(a2[1], 3);
}

BOOST_AUTO_TEST_CASE(CqRibParser_getFloatParam_test)
{
	NullRequestFixture f("1 2.0 [3.0 4]");

	const CqRibParser::TqFloatArray& a1 = f.parser.getFloatParam();
	BOOST_REQUIRE_EQUAL(a1.size(), 1U);
	BOOST_CHECK_CLOSE(a1[0], 1.0f, 0.00001);

	const CqRibParser::TqFloatArray& a2 = f.parser.getFloatParam();
	BOOST_REQUIRE_EQUAL(a2.size(), 1U);
	BOOST_CHECK_CLOSE(a2[0], 2.0f, 0.00001);

	const CqRibParser::TqFloatArray& a3 = f.parser.getFloatParam();
	BOOST_REQUIRE_EQUAL(a3.size(), 2U);
	BOOST_CHECK_CLOSE(a3[0], 3.0f, 0.00001);
	BOOST_CHECK_CLOSE(a3[1], 4.0f, 0.00001);
}

BOOST_AUTO_TEST_CASE(CqRibParser_getStringParam_test)
{
	NullRequestFixture f("\"aa\" [\"bb\" \"cc\"]");

	const CqRibParser::TqStringArray& a1 = f.parser.getStringParam();
	BOOST_REQUIRE_EQUAL(a1.size(), 1U);
	BOOST_CHECK_EQUAL(a1[0], "aa");

	const CqRibParser::TqStringArray& a2 = f.parser.getStringParam();
	BOOST_REQUIRE_EQUAL(a2.size(), 2U);
	BOOST_CHECK_EQUAL(a2[0], "bb");
	BOOST_CHECK_EQUAL(a2[1], "cc");
}


// Dummy param list which just accumulates all parameters into a list of
// (token, value) pairs.
struct DummyParamListHandler : IqRibParamListHandler
{
	std::vector<std::pair<std::string, boost::any> > tokValPairs;
	virtual void readParameter(const std::string& name, CqRibParser& parser)
	{
		CqPrimvarToken token(name.c_str());
		switch(token.storageType())
		{
			case type_integer:
				tokValPairs.push_back(std::pair<std::string, boost::any>(name,
							parser.getIntParam()));
				break;
			case type_float:
				tokValPairs.push_back(std::pair<std::string, boost::any>(name,
							parser.getFloatParam()));
				break;
			case type_string:
				tokValPairs.push_back(std::pair<std::string, boost::any>(name,
							parser.getStringParam()));
				break;
			default:
				assert(0 && "type not recognized.");
				break;
		}
	}
};

BOOST_AUTO_TEST_CASE(CqRibParser_getParamList_test)
{
	NullRequestFixture f("\"uniform vector P\" [1 2 3] \"constant integer a\" 42\n"
			              "\"constant string texname\" \"somefile.map\"");

	// Grab the parameter list from the parser.
	DummyParamListHandler pList;
	f.parser.getParamList(pList);
	BOOST_REQUIRE_EQUAL(pList.tokValPairs.size(), 3U);

	// Check first parameter
	BOOST_CHECK_EQUAL(pList.tokValPairs[0].first, "uniform vector P");
	const CqRibParser::TqFloatArray& P = boost::any_cast<const CqRibParser::TqFloatArray&>(pList.tokValPairs[0].second);
	BOOST_REQUIRE_EQUAL(P.size(), 3U);
	BOOST_CHECK_CLOSE(P[0], 1.0f, 0.00001);
	BOOST_CHECK_CLOSE(P[1], 2.0f, 0.00001);
	BOOST_CHECK_CLOSE(P[2], 3.0f, 0.00001);
	// Check second parameter
	BOOST_CHECK_EQUAL(pList.tokValPairs[1].first, "constant integer a");
	const CqRibParser::TqIntArray& a = boost::any_cast<const CqRibParser::TqIntArray&>(pList.tokValPairs[1].second);
	BOOST_REQUIRE_EQUAL(a.size(), 1U);
	BOOST_CHECK_EQUAL(a[0], 42);
	// Check third parameter
	BOOST_CHECK_EQUAL(pList.tokValPairs[2].first, "constant string texname");
	const CqRibParser::TqStringArray& texname = boost::any_cast<const CqRibParser::TqStringArray&>(pList.tokValPairs[2].second);
	BOOST_CHECK_EQUAL(texname[0], "somefile.map");
}

//------------------------------------------------------------------------------
// Request handler invocation tests.

// Test the rib parser with 
struct TestRequestHandler : public IqRibRequestHandler
{
	std::string name;
	std::string s;
	CqRibParser::TqIntArray a1;
	CqRibParser::TqFloatArray a2;

	virtual void handleRequest(const std::string& requestName, CqRibParser& parser)
	{
		name = requestName;
		s = parser.getString();
		a1 = parser.getIntArray();
		a2 = parser.getFloatArray();
	}
};

BOOST_AUTO_TEST_CASE(CqRibParser_simple_req_test)
{
	std::istringstream in("SomeRequest \"blah\" [1 2] [1.1 1.2]\n");
	CqRibLexer lex(in);
	TestRequestHandler handler;
	CqRibParser parser( boost::shared_ptr<CqRibLexer>(&lex, nullDeleter),
			boost::shared_ptr<TestRequestHandler>(&handler, nullDeleter));

	BOOST_CHECK_EQUAL(parser.parseNextRequest(), true);
	BOOST_CHECK_EQUAL(handler.name, "SomeRequest");
	BOOST_CHECK_EQUAL(handler.s, "blah");
	BOOST_REQUIRE_EQUAL(handler.a1.size(), 2U);
	BOOST_CHECK_EQUAL(handler.a1[0], 1);
	BOOST_CHECK_EQUAL(handler.a1[1], 2);
	BOOST_REQUIRE_EQUAL(handler.a2.size(), 2U);
	BOOST_CHECK_CLOSE(handler.a2[0], 1.1f, 0.0001f);
	BOOST_CHECK_CLOSE(handler.a2[1], 1.2f, 0.0001f);
}

