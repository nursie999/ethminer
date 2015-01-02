/*
    This file is part of cpp-ethereum.

    cpp-ethereum is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    cpp-ethereum is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with cpp-ethereum.  If not, see <http://www.gnu.org/licenses/>.
*/
/** @file CodeHighlighter.cpp
 * @author Arkadiy Paronyan arkadiy@ethdev.com
 * @date 2015
 * Ethereum IDE client.
 */

#include <algorithm>
#include <QRegularExpression>
#include <QTextDocument>
#include <QTextBlock>
#include <QTextLayout>
#include "CodeHighlighter.h"
#include "libsolidity/ASTVisitor.h"
#include "libsolidity/AST.h"
#include "libsolidity/Scanner.h"

namespace dev
{
namespace mix
{

CodeHighlighterSettings::CodeHighlighterSettings()
{
	bg = QColor(0x00, 0x2b, 0x36);
	fg = QColor (0xee, 0xe8, 0xd5);
	formats[Keyword].setForeground(QColor(0x93, 0xa1, 0xa1));
	//formats[Type].setForeground(Qt::darkCyan);
	formats[Comment].setForeground(QColor(0x85, 0x99, 0x00));
	formats[StringLiteral].setForeground(QColor(0xdc, 0x32, 0x2f));
	formats[NumLiteral].setForeground(fg);
	formats[Import].setForeground(QColor(0x6c, 0x71, 0xc4));
}

namespace
{
	using namespace dev::solidity;
	class HighlightVisitor : public solidity::ASTConstVisitor
	{
	public:
		HighlightVisitor(CodeHighlighter::Formats* _formats) { m_formats = _formats; }
	private:
		CodeHighlighter::Formats* m_formats;

		virtual bool visit(ImportDirective const& _node)
		{
			m_formats->push_back(CodeHighlighter::FormatRange(CodeHighlighterSettings::Import, _node.getLocation()));
			return true;
		}
	};
}


CodeHighlighter::FormatRange::FormatRange(CodeHighlighterSettings::Token _t, solidity::Location const& _location):
	token(_t), start(_location.start), length(_location.end - _location.start)
{}

void CodeHighlighter::processSource(solidity::Scanner* _scanner)
{
	solidity::Token::Value token = _scanner->getCurrentToken();
	while (token != Token::EOS)
	{
		if ((token >= Token::BREAK && token < Token::TYPES_END) ||
				token == Token::IN || token == Token::DELETE || token == Token::NULL_LITERAL || token == Token::TRUE_LITERAL || token == Token::FALSE_LITERAL)
			m_formats.push_back(FormatRange(CodeHighlighterSettings::Keyword, _scanner->getCurrentLocation()));
		else if (token == Token::STRING_LITERAL)
			m_formats.push_back(FormatRange(CodeHighlighterSettings::StringLiteral, _scanner->getCurrentLocation()));
		else if (token == Token::COMMENT_LITERAL)
			m_formats.push_back(FormatRange(CodeHighlighterSettings::Comment, _scanner->getCurrentLocation()));
		else if (token == Token::NUMBER)
			m_formats.push_back(FormatRange(CodeHighlighterSettings::NumLiteral, _scanner->getCurrentLocation()));

		token = _scanner->next();
	}
	std::sort(m_formats.begin(), m_formats.end());
}

void CodeHighlighter::processAST(solidity::ASTNode const& _ast)
{
	HighlightVisitor visitor(&m_formats);
	_ast.accept(visitor);

	std::sort(m_formats.begin(), m_formats.end());
}

void CodeHighlighter::updateFormatting(QTextDocument* _document, CodeHighlighterSettings const& _settings)
{
	QTextBlock block = _document->firstBlock();
	QList<QTextLayout::FormatRange> ranges;

	Formats::const_iterator format = m_formats.begin();
	while (true)
	{
		while ((format == m_formats.end() || (block.position() + block.length() <= format->start)) && block.isValid())
		{
			auto layout = block.layout();
			layout->clearAdditionalFormats();
			layout->setAdditionalFormats(ranges);
			_document->markContentsDirty(block.position(), block.length());
			block = block.next();
			ranges.clear();
		}
		if (!block.isValid())
			break;

		int intersectionStart = std::max(format->start, block.position());
		int intersectionLength = std::min(format->start + format->length, block.position() + block.length()) - intersectionStart;
		if (intersectionLength > 0)
		{
			QTextLayout::FormatRange range;
			range.format = _settings.formats[format->token];
			range.start = format->start - block.position();
			range.length = format->length;
			ranges.append(range);
		}
		++format;
	}
}




}
}
