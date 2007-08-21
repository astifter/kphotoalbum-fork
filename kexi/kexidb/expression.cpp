/* This file is part of the KDE project
   Copyright (C) 2003-2006 Jaroslaw Staniek <js@iidea.pl>

   Based on nexp.cpp : Parser module of Python-like language
   (C) 2001 Jaroslaw Staniek, MIMUW (www.mimuw.edu.pl)

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "expression.h"
#include "utils.h"
#include "parser/sqlparser.h"
#include "parser/parser_p.h"

#include <ctype.h>

#include <kdebug.h>
#include <klocale.h>

#include <qdatetime.h>

KEXI_DB_EXPORT QString KexiDB::exprClassName(int c)
{
	if (c==KexiDBExpr_Unary)
		return "Unary";
	else if (c==KexiDBExpr_Arithm)
		return "Arithm";
	else if (c==KexiDBExpr_Logical)
		return "Logical";
	else if (c==KexiDBExpr_Relational)
		return "Relational";
	else if (c==KexiDBExpr_SpecialBinary)
		return "SpecialBinary";
	else if (c==KexiDBExpr_Const)
		return "Const";
	else if (c==KexiDBExpr_Variable)
		return "Variable";
	else if (c==KexiDBExpr_Function)
		return "Function";
	else if (c==KexiDBExpr_Aggregation)
		return "Aggregation";
	else if (c==KexiDBExpr_TableList)
		return "TableList";
	else if (c==KexiDBExpr_QueryParameter)
		return "QueryParameter";
	
	return "Unknown";
}

using namespace KexiDB;

//=========================================

BaseExpr::BaseExpr(int token) 
 : m_cl(KexiDBExpr_Unknown)
 , m_par(0)
 , m_token(token)
{

}

BaseExpr::~BaseExpr()
{
}

Field::Type BaseExpr::type()
{
	return Field::InvalidType;
}

QString BaseExpr::debugString()
{
	return QString("BaseExpr(%1,type=%1)").arg(m_token).arg(Driver::defaultSQLTypeName(type()));
}

bool BaseExpr::validate(ParseInfo& /*parseInfo*/)
{
	return true;
}

extern const char * const tname(int offset);
#define safe_tname(token) ((token>=255 && token<=__LAST_TOKEN) ? tname(token-255) : "")

QString BaseExpr::tokenToDebugString(int token)
{
	if (token < 254) {
		if (isprint(token))
			return QString(QChar(uchar(token)));
		else
			return QString::number(token);
	}
	return QString(safe_tname(token));
}

QString BaseExpr::tokenToString()
{
	if (m_token < 255 && isprint(m_token))
		return tokenToDebugString();
	return QString::null;
}

NArgExpr* BaseExpr::toNArg() { return dynamic_cast<NArgExpr*>(this); }
UnaryExpr* BaseExpr::toUnary() { return dynamic_cast<UnaryExpr*>(this); }
BinaryExpr* BaseExpr::toBinary() { return dynamic_cast<BinaryExpr*>(this); }
ConstExpr* BaseExpr::toConst() { return dynamic_cast<ConstExpr*>(this); }
VariableExpr* BaseExpr::toVariable() { return dynamic_cast<VariableExpr*>(this); }
FunctionExpr* BaseExpr::toFunction() { return dynamic_cast<FunctionExpr*>(this); }
QueryParameterExpr* BaseExpr::toQueryParameter() { return dynamic_cast<QueryParameterExpr*>(this); }

//=========================================

NArgExpr::NArgExpr(int aClass, int token)
 : BaseExpr(token)
{
	m_cl = aClass;
	list.setAutoDelete(true);
}

NArgExpr::~NArgExpr()
{
}

QString NArgExpr::debugString()
{
	QString s = QString("NArgExpr(")
		+ "class=" + exprClassName(m_cl);
	for ( BaseExpr::ListIterator it(list); it.current(); ++it ) {
		s+=", ";
		s+=it.current()->debugString();
	}
	s+=")";
	return s;
}

QString NArgExpr::toString( QuerySchemaParameterValueListIterator* params )
{
	QString s;
	s.reserve(256);
	foreach_list( BaseExpr::ListIterator, it, list) {
		if (!s.isEmpty())
			s+=", ";
		s+=it.current()->toString(params);
	}
	return s;
}

void NArgExpr::getQueryParameters(QuerySchemaParameterList& params)
{
	foreach_list( BaseExpr::ListIterator, it, list)
		it.current()->getQueryParameters(params);
}

BaseExpr* NArgExpr::arg(int nr)
{
	return list.at(nr); 
}

void NArgExpr::add(BaseExpr *expr)
{
	list.append(expr);
	expr->setParent(this);
}

void NArgExpr::prepend(BaseExpr *expr)
{
	list.prepend(expr);
	expr->setParent(this);
}

int NArgExpr::args()
{
	return list.count();
}

bool NArgExpr::validate(ParseInfo& parseInfo)
{
	if (!BaseExpr::validate(parseInfo))
		return false;

	foreach_list(BaseExpr::ListIterator, it, list) {
		if (!it.current()->validate(parseInfo))
			return false;
	}
	return true;
}

//=========================================
UnaryExpr::UnaryExpr(int token, BaseExpr *arg)
 : BaseExpr(token)
 , m_arg(arg)
{
	m_cl = KexiDBExpr_Unary;
	if (m_arg)
		m_arg->setParent(this);
}

UnaryExpr::~UnaryExpr()
{
	delete m_arg;
}

QString UnaryExpr::debugString()
{
	return "UnaryExpr('" 
		+ tokenToDebugString() + "', "
		+ (m_arg ? m_arg->debugString() : QString("<NONE>")) 
		+ QString(",type=%1)").arg(Driver::defaultSQLTypeName(type()));
}

QString UnaryExpr::toString(QuerySchemaParameterValueListIterator* params)
{
	if (m_token=='(') //parentheses (special case)
		return "(" + (m_arg ? m_arg->toString(params) : "<NULL>") + ")";
	if (m_token < 255 && isprint(m_token))
		return tokenToDebugString() + (m_arg ? m_arg->toString(params) : "<NULL>");
	if (m_token==NOT)
		return "NOT " + (m_arg ? m_arg->toString(params) : "<NULL>");
	if (m_token==SQL_IS_NULL)
		return (m_arg ? m_arg->toString(params) : "<NULL>") + " IS NULL";
	if (m_token==SQL_IS_NOT_NULL)
		return (m_arg ? m_arg->toString(params) : "<NULL>") + " IS NOT NULL";
	return QString("{INVALID_OPERATOR#%1} ").arg(m_token) + (m_arg ? m_arg->toString(params) : "<NULL>");
}

void UnaryExpr::getQueryParameters(QuerySchemaParameterList& params)
{
	if (m_arg)
		m_arg->getQueryParameters(params);
}

Field::Type UnaryExpr::type()
{
	//NULL IS NOT NULL : BOOLEAN
	//NULL IS NULL : BOOLEAN
	switch (m_token) {
	case SQL_IS_NULL:
	case SQL_IS_NOT_NULL:
		return Field::Boolean;
	}
	const Field::Type t = m_arg->type();
	if (t==Field::Null)
		return Field::Null;
	if (m_token==NOT)
		return Field::Boolean;

	return t;
}

bool UnaryExpr::validate(ParseInfo& parseInfo)
{
	if (!BaseExpr::validate(parseInfo))
		return false;

	if (!m_arg->validate(parseInfo))
		return false;

//! @todo compare types... e.g. NOT applied to Text makes no sense...

	// update type
	if (m_arg->toQueryParameter()) {
		m_arg->toQueryParameter()->setType(type());
	}

	return true;
#if 0
	BaseExpr *n = l.at(0);

	n->check();
/*typ wyniku:
		const bool dla "NOT <bool>" (negacja)
		int dla "# <str>" (dlugosc stringu)
		int dla "+/- <int>"
		*/
	if (is(NOT) && n->nodeTypeIs(TYP_BOOL)) {
		node_type=new NConstType(TYP_BOOL);
	}
	else if (is('#') && n->nodeTypeIs(TYP_STR)) {
		node_type=new NConstType(TYP_INT);
	}
	else if ((is('+') || is('-')) && n->nodeTypeIs(TYP_INT)) {
		node_type=new NConstType(TYP_INT);
	}
	else {
		ERR("Niepoprawny argument typu '%s' dla operatora '%s'",
			n->nodeTypeName(),is(NOT)?QString("not"):QChar(typ()));
	}
#endif
}
	
//=========================================
BinaryExpr::BinaryExpr(int aClass, BaseExpr *left_expr, int token, BaseExpr *right_expr)
 : BaseExpr(token)
 , m_larg(left_expr)
 , m_rarg(right_expr)
{
	m_cl = aClass;
	if (m_larg)
		m_larg->setParent(this);
	if (m_rarg)
		m_rarg->setParent(this);
}

BinaryExpr::~BinaryExpr()
{
	delete m_larg;
	delete m_rarg;
}

bool BinaryExpr::validate(ParseInfo& parseInfo)
{
	if (!BaseExpr::validate(parseInfo))
		return false;

	if (!m_larg->validate(parseInfo))
		return false;
	if (!m_rarg->validate(parseInfo))
		return false;

//! @todo compare types..., BITWISE_SHIFT_RIGHT requires integers, etc...

	//update type for query parameters
	QueryParameterExpr * queryParameter = m_larg->toQueryParameter();
	if (queryParameter)
		queryParameter->setType(m_rarg->type());
	queryParameter = m_rarg->toQueryParameter();
	if (queryParameter)
		queryParameter->setType(m_larg->type());

	return true;
}

Field::Type BinaryExpr::type()
{
	const Field::Type lt = m_larg->type(), rt = m_rarg->type();
	if (lt==Field::InvalidType || rt == Field::InvalidType)
		return Field::InvalidType;
	if (lt==Field::Null || rt == Field::Null) {
		if (m_token!=OR) //note that NULL OR something   != NULL
			return Field::Null;
	}

	switch (m_token) {
	case BITWISE_SHIFT_RIGHT:
	case BITWISE_SHIFT_LEFT:
	case CONCATENATION:
		return lt;
	}

	const bool ltInt = Field::isIntegerType(lt);
	const bool rtInt = Field::isIntegerType(rt);
	if (ltInt && rtInt)
		return KexiDB::maximumForIntegerTypes(lt, rt);

	if (Field::isFPNumericType(lt) && rtInt)
		return lt;
	if (Field::isFPNumericType(rt) && ltInt)
		return rt;
	if ((lt==Field::Double || lt==Field::Float) && rtInt)
		return lt;
	if ((rt==Field::Double || rt==Field::Float) && ltInt)
		return rt;

	return Field::Boolean;
}

QString BinaryExpr::debugString()
{
	return QString("BinaryExpr(")
		+ "class=" + exprClassName(m_cl)
		+ "," + (m_larg ? m_larg->debugString() : QString("<NONE>")) 
		+ ",'" + tokenToDebugString() + "',"
		+ (m_rarg ? m_rarg->debugString() : QString("<NONE>")) 
		+ QString(",type=%1)").arg(Driver::defaultSQLTypeName(type()));
}

QString BinaryExpr::tokenToString()
{
	if (m_token < 255 && isprint(m_token))
		return tokenToDebugString();
	// other arithmetic operations: << >>
	switch (m_token) {
	case BITWISE_SHIFT_RIGHT: return ">>";
	case BITWISE_SHIFT_LEFT: return "<<";
	// other relational operations: <= >= <> (or !=) LIKE IN
	case NOT_EQUAL: return "<>";
	case NOT_EQUAL2: return "!=";
	case LESS_OR_EQUAL: return "<=";
	case GREATER_OR_EQUAL: return ">=";
	case LIKE: return "LIKE";
	case SQL_IN: return "IN";
	// other logical operations: OR (or ||) AND (or &&) XOR
	case SIMILAR_TO: return "SIMILAR TO";
	case NOT_SIMILAR_TO: return "NOT SIMILAR TO";
	case OR: return "OR";
	case AND: return "AND";
	case XOR: return "XOR";
	// other string operations: || (as CONCATENATION)
	case CONCATENATION: return "||";
	// SpecialBinary "pseudo operators":
	/* not handled here */
	default:;
	}
	return QString("{INVALID_BINARY_OPERATOR#%1} ").arg(m_token);
}

QString BinaryExpr::toString(QuerySchemaParameterValueListIterator* params)
{
#define INFIX(a) \
		(m_larg ? m_larg->toString(params) : "<NULL>") + " " + a + " " + (m_rarg ? m_rarg->toString(params) : "<NULL>")
	return INFIX(tokenToString());
}

void BinaryExpr::getQueryParameters(QuerySchemaParameterList& params)
{
	if (m_larg)
		m_larg->getQueryParameters(params);
	if (m_rarg)
		m_rarg->getQueryParameters(params);
}

//=========================================
ConstExpr::ConstExpr( int token, const QVariant& val)
: BaseExpr( token )
, value(val)
{
	m_cl = KexiDBExpr_Const;
}

ConstExpr::~ConstExpr()
{
}

Field::Type ConstExpr::type()
{
	if (m_token==SQL_NULL)
		return Field::Null;
	else if (m_token==INTEGER_CONST) {
//TODO ok?
//TODO: add sign info?
		if (value.type() == QVariant::Int || value.type() == QVariant::UInt) {
			Q_LLONG v = value.toInt();
			if (v <= 0xff && v > -0x80)
				return Field::Byte;
			if (v <= 0xffff && v > -0x8000)
				return Field::ShortInteger;
			return Field::Integer;
		}
		return Field::BigInteger;
	}
	else if (m_token==CHARACTER_STRING_LITERAL) {
//TODO: Field::defaultTextLength() is hardcoded now!
		if (value.toString().length() > Field::defaultTextLength())
			return Field::LongText;
		else
			return Field::Text;
	}
	else if (m_token==REAL_CONST)
		return Field::Double;
	else if (m_token==DATE_CONST)
		return Field::Date;
	else if (m_token==DATETIME_CONST)
		return Field::DateTime;
	else if (m_token==TIME_CONST)
		return Field::Time;

	return Field::InvalidType;
}

QString ConstExpr::debugString()
{
	return QString("ConstExpr('") + tokenToDebugString() +"'," + toString()
		+ QString(",type=%1)").arg(Driver::defaultSQLTypeName(type()));
}

QString ConstExpr::toString(QuerySchemaParameterValueListIterator* params)
{
	Q_UNUSED(params);
	if (m_token==SQL_NULL)
		return "NULL";
	else if (m_token==CHARACTER_STRING_LITERAL)
//TODO: better escaping!
		return "'" + value.toString() + "'";
	else if (m_token==REAL_CONST)
		return QString::number(value.toPoint().x())+"."+QString::number(value.toPoint().y());
	else if (m_token==DATE_CONST)
		return "'" + value.toDate().toString(Qt::ISODate) + "'";
	else if (m_token==DATETIME_CONST)
		return "'" + value.toDateTime().date().toString(Qt::ISODate) 
			+ " " + value.toDateTime().time().toString(Qt::ISODate) + "'";
	else if (m_token==TIME_CONST)
		return "'" + value.toTime().toString(Qt::ISODate) + "'";

	return value.toString();
}

void ConstExpr::getQueryParameters(QuerySchemaParameterList& params)
{
	Q_UNUSED(params);
}

bool ConstExpr::validate(ParseInfo& parseInfo)
{
	if (!BaseExpr::validate(parseInfo))
		return false;

	return type()!=Field::InvalidType;
}

//=========================================
QueryParameterExpr::QueryParameterExpr(const QString& message)
: ConstExpr( QUERY_PARAMETER, message )
, m_type(Field::Text)
{
	m_cl = KexiDBExpr_QueryParameter;
}

QueryParameterExpr::~QueryParameterExpr()
{
}

Field::Type QueryParameterExpr::type()
{
	return m_type;
}

void QueryParameterExpr::setType(Field::Type type)
{
	m_type = type;
}

QString QueryParameterExpr::debugString()
{
	return QString("QueryParameterExpr('") + QString::fromLatin1("[%2]").arg(value.toString())
		+ QString("',type=%1)").arg(Driver::defaultSQLTypeName(type()));
}

QString QueryParameterExpr::toString(QuerySchemaParameterValueListIterator* params)
{
	return params ? params->getPreviousValueAsString(type()) : QString::fromLatin1("[%2]").arg(value.toString());
}

void QueryParameterExpr::getQueryParameters(QuerySchemaParameterList& params)
{
	QuerySchemaParameter param;
	param.message = value.toString();
	param.type = type();
	params.append( param );
}

bool QueryParameterExpr::validate(ParseInfo& parseInfo)
{
	Q_UNUSED(parseInfo);
	return type()!=Field::InvalidType;
}

//=========================================
VariableExpr::VariableExpr(const QString& _name)
: BaseExpr( 0/*undefined*/ )
, name(_name)
, field(0)
, tablePositionForField(-1)
, tableForQueryAsterisk(0)
{
	m_cl = KexiDBExpr_Variable;
}

VariableExpr::~VariableExpr()
{
}

QString VariableExpr::debugString()
{
	return QString("VariableExpr(") + name
		+ QString(",type=%1)").arg(field ? Driver::defaultSQLTypeName(type()) : QString("FIELD NOT DEFINED YET"));
}

QString VariableExpr::toString(QuerySchemaParameterValueListIterator* params)
{
	Q_UNUSED(params);
	return name;
}

void VariableExpr::getQueryParameters(QuerySchemaParameterList& params)
{
	Q_UNUSED(params);
}

//! We're assuming it's called after VariableExpr::validate()
Field::Type VariableExpr::type()
{
	if (field)
		return field->type();
	
	//BTW, asterisks are not stored in VariableExpr outside of parser, so ok.
	return Field::InvalidType;
}

#define IMPL_ERROR(errmsg) parseInfo.errMsg = "Implementation error"; parseInfo.errDescr = errmsg

bool VariableExpr::validate(ParseInfo& parseInfo)
{
	if (!BaseExpr::validate(parseInfo))
		return false;
	field = 0;
	tablePositionForField = -1;
	tableForQueryAsterisk = 0;

/* taken from parser's addColumn(): */
	KexiDBDbg << "checking variable name: " << name << endl;
	int dotPos = name.find('.');
	QString tableName, fieldName;
//TODO: shall we also support db name?
	if (dotPos>0) {
		tableName = name.left(dotPos);
		fieldName = name.mid(dotPos+1);
	}
	if (tableName.isEmpty()) {//fieldname only
		fieldName = name;
		if (fieldName=="*") {
//			querySchema->addAsterisk( new QueryAsterisk(querySchema) );
			return true;
		}

		//find first table that has this field
		Field *firstField = 0;
		foreach_list(TableSchema::ListIterator, it, *parseInfo.querySchema->tables()) {
			Field *f = it.current()->field(fieldName);
			if (f) {
				if (!firstField) {
					firstField = f;
				}
				else if (f->table()!=firstField->table()) {
					//ambiguous field name
					parseInfo.errMsg = i18n("Ambiguous field name");
					parseInfo.errDescr = i18n("Both table \"%1\" and \"%2\" have defined \"%3\" field. "
						"Use \"<tableName>.%4\" notation to specify table name.")
						.arg(firstField->table()->name()).arg(f->table()->name())
						.arg(fieldName).arg(fieldName);
					return false;
				}
			}
		}
		if (!firstField) {
			parseInfo.errMsg = i18n("Field not found");
			parseInfo.errDescr = i18n("Table containing \"%1\" field not found").arg(fieldName);
			return false;
		}
		//ok
		field = firstField; //store
//		querySchema->addField(firstField);
		return true;
	}

	//table.fieldname or tableAlias.fieldname
	tableName = tableName.lower();
	TableSchema *ts = parseInfo.querySchema->table( tableName );
	if (ts) {//table.fieldname
		//check if "table" is covered by an alias
		const QValueList<int> tPositions = parseInfo.querySchema->tablePositions(tableName);
		QValueList<int>::ConstIterator it = tPositions.constBegin();
		QCString tableAlias;
		bool covered = true;
		for (; it!=tPositions.constEnd() && covered; ++it) {
			tableAlias = parseInfo.querySchema->tableAlias(*it);
			if (tableAlias.isEmpty() || tableAlias.lower()==tableName.latin1())
				covered = false; //uncovered
			KexiDBDbg << " --" << "covered by " << tableAlias << " alias" << endl;
		}
		if (covered) {
			parseInfo.errMsg = i18n("Could not access the table directly using its name");
			parseInfo.errDescr = i18n("Table \"%1\" is covered by aliases. Instead of \"%2\", "
				"you can write \"%3\"").arg(tableName)
				.arg(tableName+"."+fieldName).arg(tableAlias+"."+fieldName.latin1());
			return false;
		}
	}
	
	int tablePosition = -1;
	if (!ts) {//try to find tableAlias
		tablePosition = parseInfo.querySchema->tablePositionForAlias( tableName.latin1() );
		if (tablePosition>=0) {
			ts = parseInfo.querySchema->tables()->at(tablePosition);
			if (ts) {
//				KexiDBDbg << " --it's a tableAlias.name" << endl;
			}
		}
	}

	if (!ts) {
		parseInfo.errMsg = i18n("Table not found");
		parseInfo.errDescr = i18n("Unknown table \"%1\"").arg(tableName);
		return false;
	}

	QValueList<int> *positionsList = parseInfo.repeatedTablesAndAliases[ tableName ];
	if (!positionsList) { //for sanity
		IMPL_ERROR(tableName + "." + fieldName + ", !positionsList ");
		return false;
	}

	//it's a table.*
	if (fieldName=="*") {
		if (positionsList->count()>1) {
			parseInfo.errMsg = i18n("Ambiguous \"%1.*\" expression").arg(tableName);
			parseInfo.errDescr = i18n("More than one \"%1\" table or alias defined").arg(tableName);
			return false;
		}
		tableForQueryAsterisk = ts;
//			querySchema->addAsterisk( new QueryAsterisk(querySchema, ts) );
		return true;
	}

//	KexiDBDbg << " --it's a table.name" << endl;
	Field *realField = ts->field(fieldName);
	if (!realField) {
		parseInfo.errMsg = i18n("Field not found");
		parseInfo.errDescr = i18n("Table \"%1\" has no \"%2\" field")
			.arg(tableName).arg(fieldName);
		return false;
	}

	// check if table or alias is used twice and both have the same column
	// (so the column is ambiguous)
	int numberOfTheSameFields = 0;
	for (QValueList<int>::iterator it = positionsList->begin();
		it!=positionsList->end();++it)
	{
		TableSchema *otherTS = parseInfo.querySchema->tables()->at(*it);
		if (otherTS->field(fieldName))
			numberOfTheSameFields++;
		if (numberOfTheSameFields>1) {
			parseInfo.errMsg = i18n("Ambiguous \"%1.%2\" expression")
				.arg(tableName).arg(fieldName);
			parseInfo.errDescr = i18n("More than one \"%1\" table or alias defined containing \"%2\" field")
				.arg(tableName).arg(fieldName);
			return false;
		}
	}
	field = realField; //store
	tablePositionForField = tablePosition;
//				querySchema->addField(realField, tablePosition);

	return true;
}

//=========================================
static QValueList<QCString> FunctionExpr_builtIns;
static const char* FunctionExpr_builtIns_[] = 
{"SUM", "MIN", "MAX", "AVG", "COUNT", "STD", "STDDEV", "VARIANCE", 0 };

QValueList<QCString> FunctionExpr::builtInAggregates()
{
	if (FunctionExpr_builtIns.isEmpty()) {
		for (const char **p = FunctionExpr_builtIns_; *p; p++)
			FunctionExpr_builtIns << *p;
	}
	return FunctionExpr_builtIns;
}

FunctionExpr::FunctionExpr( const QString& _name, NArgExpr* args_ )
 : BaseExpr( 0/*undefined*/ )
 , name(_name)
 , args(args_)
{
	if (isBuiltInAggregate(name.latin1()))
		m_cl = KexiDBExpr_Aggregation;
	else
		m_cl = KexiDBExpr_Function;
	if (args)
		args->setParent( this );
}

FunctionExpr::~FunctionExpr()
{
	delete args;
}

QString FunctionExpr::debugString()
{
	QString res;
	res.append( QString("FunctionExpr(") + name );
	if (args)
		res.append(QString(",") + args->debugString());
	res.append(QString(",type=%1)").arg(Driver::defaultSQLTypeName(type())));
	return res;
}

QString FunctionExpr::toString(QuerySchemaParameterValueListIterator* params)
{
	return name + "(" + (args ? args->toString(params) : QString::null) + ")";
}

void FunctionExpr::getQueryParameters(QuerySchemaParameterList& params)
{
	args->getQueryParameters(params);
}

Field::Type FunctionExpr::type()
{
	//TODO
	return Field::InvalidType;
}

bool FunctionExpr::validate(ParseInfo& parseInfo)
{
	if (!BaseExpr::validate(parseInfo))
		return false;

	return args ? args->validate(parseInfo) : true;
}

bool FunctionExpr::isBuiltInAggregate(const QCString& fname)
{
	return builtInAggregates().find(fname.upper())!=FunctionExpr_builtIns.end();
}