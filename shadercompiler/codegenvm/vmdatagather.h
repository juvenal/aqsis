// Aqsis
// Copyright (C) 1997 - 2001, Paul C. Gregory
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
		\brief Compiler backend to output VM code.
		\author Paul C. Gregory (pgregory@aqsis.org)
*/

//? Is .h included already?
#ifndef VMDATAGATHER_H_INCLUDED
#define VMDATAGATHER_H_INCLUDED 1

#include	<vector>
#include	<deque>
#include	<map>

#include	"aqsis.h"

#include	"iparsenode.h"
#include	"ivardef.h"
#include	"ifuncdef.h"
#include	"icodegen.h"

namespace Aqsis {


class CqCodeGenDataGather : public IqParseNodeVisitor
{
	public:

		virtual	void Visit( IqParseNode& );
		virtual	void Visit( IqParseNodeShader& );
		virtual	void Visit( IqParseNodeFunctionCall& );
		virtual	void Visit( IqParseNodeUnresolvedCall& );
		virtual	void Visit( IqParseNodeVariable& );
		virtual	void Visit( IqParseNodeArrayVariable& );
		virtual	void Visit( IqParseNodeVariableAssign& );
		virtual	void Visit( IqParseNodeArrayVariableAssign& );
		virtual	void Visit( IqParseNodeOperator& );
		virtual	void Visit( IqParseNodeMathOp& );
		virtual	void Visit( IqParseNodeRelationalOp& );
		virtual	void Visit( IqParseNodeUnaryOp& );
		virtual	void Visit( IqParseNodeLogicalOp& );
		virtual	void Visit( IqParseNodeDiscardResult& );
		virtual	void Visit( IqParseNodeConstantFloat& );
		virtual	void Visit( IqParseNodeConstantString& );
		virtual	void Visit( IqParseNodeWhileConstruct& );
		virtual	void Visit( IqParseNodeLoopMod& );
		virtual	void Visit( IqParseNodeIlluminateConstruct& );
		virtual	void Visit( IqParseNodeIlluminanceConstruct& );
		virtual	void Visit( IqParseNodeSolarConstruct& );
		virtual	void Visit( IqParseNodeGatherConstruct& );
		virtual	void Visit( IqParseNodeConditional& );
		virtual	void Visit( IqParseNodeConditionalExpression& );
		virtual	void Visit( IqParseNodeTypeCast& );
		virtual	void Visit( IqParseNodeTriple& );
		virtual	void Visit( IqParseNodeSixteenTuple& );
		virtual	void Visit( IqParseNodeMessagePassingFunction& );

		TqInt	VariableUsage() const
		{
			return ( m_VariableUsage );
		}
		std::map<std::string, IqVarDef*>& TempVars()
		{
			return ( m_TempVars );
		}
	private:
		TqInt	m_VariableUsage;

		std::vector<std::vector<SqVarRefTranslator> > m_saTransTable;
		std::deque<std::map<std::string, std::string> >	m_StackVarMap;

		std::map<std::string, IqVarDef*>	m_TempVars;
};

//-----------------------------------------------------------------------

} // namespace Aqsis

#endif	// !VMDATAGATHER_H_INCLUDED
