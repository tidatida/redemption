/*
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

   Product name: redemption, a FLOSS RDP proxy
   Copyright (C) Wallix 2010
   Author(s): Christophe Grosjean, Javier Caverni
   Based on xrdp Copyright (C) Jay Sorg 2004-2010

   Unit test for bitmap class, compression performance

*/

#define BOOST_AUTO_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE TestDfaRegex
#include <boost/test/auto_unit_test.hpp>

#include "log.hpp"
#define LOGNULL

#include "regex.hpp"

#include <vector>
#include <sstream>

using namespace rndfa;

inline void st_to_string(StateBase * st, std::ostream& os,
                         const std::vector<StateBase*>& states, unsigned depth = 0)
{
    size_t n = std::find(states.begin(), states.end(), st) - states.begin();
    os << std::string(depth, '\t') << n;
    if (st && st->id != -30u) {
        os << "\t" << st->utfc << "\t" << *st << "\n";
        st->id = -30u;
        st_to_string(st->out1, os, states, depth+1);
        st_to_string(st->out2, os, states, depth+1);
    }
    else {
        os << "\n";
    }
}

inline std::string st_to_string(StateBase * st)
{
    std::vector<StateBase*> states(1);
    append_state(st, states);
    std::ostringstream os;
    st_to_string(st, os, states);
    return os.str();
}

inline size_t multi_char(const char * c)
{
    return rndfa::utf_consumer(c).bumpc();
}

BOOST_AUTO_TEST_CASE(TestRegexState)
{
    struct Reg {
        Reg(const char * s)
        : st(str2reg(s))
        {}

        ~Reg()
        { free_st(st); }

        std::string to_string() const
        { return st_to_string(st); }

        StateBase * st;
    };

    {
        StateBase st_d(NORMAL, 'd');
        StateBase st_c(NORMAL, 'c', &st_d);
        StateBase st_b(NORMAL, 'b', &st_c);
        StateBase st_a(NORMAL, 'a', &st_b);
        Reg rgx("abcd");
        BOOST_CHECK_EQUAL(st_to_string(&st_a), rgx.to_string());
    }
    {
        StateBorder st_first(1);
        StateBorder st_last(0);
        StateBase st_a(NORMAL, 'a', &st_last);
        st_first.out1 = &st_a;
        Reg rgx("^a$");
        BOOST_CHECK_EQUAL(st_to_string(&st_first), rgx.to_string());
    }
    {
        StateBase split(SPLIT);
        StateBase st_a(NORMAL, 'a', &split);
        split.out1 = &state_finish;
        split.out2 = &st_a;
        {
            Reg rgx("a*");
            BOOST_CHECK_EQUAL(st_to_string(&split), rgx.to_string());
        }
        {
            Reg rgx("a+");
            BOOST_CHECK_EQUAL(st_to_string(&st_a), rgx.to_string());
        }
        StateBase st_b(NORMAL, 'b');
        split.out1 = &st_b;
        {
            Reg rgx("a*b");
            BOOST_CHECK_EQUAL(st_to_string(&split), rgx.to_string());
        }
        {
            Reg rgx("a+b");
            BOOST_CHECK_EQUAL(st_to_string(&st_a), rgx.to_string());
        }
    }
    {
        StateMultiTest st_multi;
        st_multi.push_checker(new CheckerInterval('a', 'c'));
        st_multi.push_checker(new CheckerString("f"));
        Reg rgx("[a-cf]");
        BOOST_CHECK_EQUAL(st_to_string(&st_multi), rgx.to_string());
    }
    {
        StateMultiTest st_multi;
        st_multi.push_checker(new CheckerInterval('a', 'c'));
        st_multi.push_checker(new CheckerInterval('y', 'z'));
        st_multi.push_checker(new CheckerInterval('f', 'h'));
        st_multi.push_checker(new CheckerString("tfv"));
        {
            Reg rgx("[ta-cfy-zf-hv]");
            BOOST_CHECK_EQUAL(st_to_string(&st_multi), rgx.to_string());
        }
        st_multi.result_true_check = false;
        {
            Reg rgx("[^ta-cfy-zf-hv]");
            BOOST_CHECK_EQUAL(st_to_string(&st_multi), rgx.to_string());
        }
    }
    {
        StateBase st_b(NORMAL, 'b');
        StateBase st_a(NORMAL, 'a', &st_b);
        StateBase split(SPLIT, 0, &st_b, &st_a);
        Reg rgx("a?b");
        BOOST_CHECK_EQUAL(st_to_string(&split), rgx.to_string());
    }
    {
        StateBorder last(0);
        StateBase close2(CAPTURE_CLOSE, 0, &last);
        StateBase any(ANY_CHARACTER);
        StateBase split_any(SPLIT, 0, &close2, &any);
        any.out1 = &split_any;

        StateBase open2(CAPTURE_OPEN, 0, &split_any);
        StateBase st_d(NORMAL, 'd', &open2);

        StateBase close1(CAPTURE_CLOSE, 0, &st_d);
        StateBase st_c(NORMAL, 'c', &close1);
        StateBase split_c(SPLIT, 0, &close1, &st_c);

        StateBase st_b(NORMAL, 'b', &split_c);
        StateBase split_b(SPLIT, 0, &split_c, &st_b);

        StateBase st_a(NORMAL, 'a', &split_b);
        StateBase split_a(SPLIT, 0, &split_b, &st_a);

        StateBase open1(CAPTURE_OPEN, 0, &split_a);

        Reg rgx("(a?b?c?)d(.*)$");
        BOOST_CHECK_EQUAL(st_to_string(&open1), rgx.to_string());
    }
    {
        StateBase st_a2(NORMAL, 'a');
        StateBase split2(SPLIT, 0, 0, &st_a2);
        StateBase st_a1(NORMAL, 'a', &split2);
        StateBase split1(SPLIT, 0, &split2, &st_a1);
        Reg rgx("a{0,2}");
        BOOST_CHECK_EQUAL(st_to_string(&split1), rgx.to_string());
    }
    {
        StateBase st_b(NORMAL, 'b');
        StateBase split3(SPLIT, 0, &st_b);
        StateBase st_a1(NORMAL, 'a', &split3);
        StateBase st_a2(NORMAL, 'a', &split3);
        StateBase split2(SPLIT, 0, &st_a1, &split3);
        StateBase split1(SPLIT, 0, &st_a2, &split2);
        StateBase st_a3(NORMAL, 'a', &split1);
        StateBase st_a4(NORMAL, 'a', &st_a3);
        Reg rgx("a{2,4}b");
        BOOST_CHECK_EQUAL(st_to_string(&st_a4), rgx.to_string());
    }
    {
        StateBase st_b(NORMAL, 'b');
        StateBase st_a2(NORMAL, 'a');
        StateBase split1(SPLIT, 0, &st_b, &st_a2);
        st_a2.out1 = &split1;
        {
            Reg rgx("a{0,}b");
            BOOST_CHECK_EQUAL(st_to_string(&split1), rgx.to_string());
        }
        StateBase st_a3(NORMAL, 'a', &split1);
        StateBase st_a4(NORMAL, 'a', &st_a3);
        {
            Reg rgx("a{2,}b");
            BOOST_CHECK_EQUAL(st_to_string(&st_a4), rgx.to_string());
        }
    }
    {
        StateBase st1(NORMAL, '}');
        StateBase st2(NORMAL, '-', &st1);
        StateBase st3(NORMAL, '2', &st2);
        StateBase st4(NORMAL, '{', &st3);
        StateBase st5(NORMAL, 'a', &st4);
        Reg rgx("a{2-}");
        BOOST_CHECK_EQUAL(st_to_string(&st5), rgx.to_string());
    }
    {
        StateBase st_b(NORMAL, 'b');
        StateBase split2(SPLIT, 0, &state_finish, &st_b);
        st_b.out1 = &split2;
        StateBase st_a2(NORMAL, 'a');
        StateBase split1(SPLIT, 0, &split2, &st_a2);
        st_a2.out1 = &split1;
        {
            Reg rgx("a{0,}b*");
            BOOST_CHECK_EQUAL(st_to_string(&split1), rgx.to_string());
        }
        StateBase st_a3(NORMAL, 'a', &split1);
        StateBase st_a4(NORMAL, 'a', &st_a3);
        split1.out1 = &st_b;
        {
            Reg rgx("a{2,}b+");
            BOOST_CHECK_EQUAL(st_to_string(&st_a4), rgx.to_string());
        }
    }
    {
        StateBase close1(CAPTURE_CLOSE);
        StateDigit digit;
        StateBase split(SPLIT, 0, &close1, &digit);
        digit.out1 = &split;
        StateBase open1(CAPTURE_OPEN, 0, &split);
        Reg rgx("(\\d*)");
        BOOST_CHECK_EQUAL(st_to_string(&open1), rgx.to_string());
    }
    {
        StateBase st_c4(NORMAL, multi_char("Þ"));
        StateBase st_c3(NORMAL, 'a', &st_c4);
        StateBase st_c2(NORMAL, multi_char("Ë"), &st_c3);
        StateBase st_c1(NORMAL, multi_char("¥"), &st_c2);
        Reg rgx("¥ËaÞ");
        BOOST_CHECK_EQUAL(st_to_string(&st_c1), rgx.to_string());
    }
}

BOOST_AUTO_TEST_CASE(TestRegexCheck)
{
    {
        StateBase st(NORMAL, multi_char("Þ"));
        BOOST_CHECK(st.check(multi_char("Þ")));
        BOOST_CHECK( ! st.check('a'));
    }
    {
        StateCharacters st("aÎbps");
        BOOST_CHECK(st.check('a'));
        BOOST_CHECK(st.check(multi_char("Î")));
        BOOST_CHECK(st.check('b'));
        BOOST_CHECK(st.check('p'));
        BOOST_CHECK(st.check('s'));
        BOOST_CHECK( ! st.check('t'));
    }
    {
        StateRange st('e','g');
        BOOST_CHECK(st.check('e'));
        BOOST_CHECK(st.check('f'));
        BOOST_CHECK(st.check('g'));
        BOOST_CHECK( ! st.check('d'));
        BOOST_CHECK( ! st.check('h'));
    }
    {
        StateSpace st;
        BOOST_CHECK(st.check(' '));
        BOOST_CHECK(st.check('\t'));
        BOOST_CHECK(st.check('\n'));
        BOOST_CHECK( ! st.check('a'));
    }
    {
        StateNoSpace st;
        BOOST_CHECK( ! st.check(' '));
        BOOST_CHECK( ! st.check('\t'));
        BOOST_CHECK( ! st.check('\n'));
        BOOST_CHECK(st.check('a'));
    }
    {
        StateDigit st;
        BOOST_CHECK(st.check('0'));
        BOOST_CHECK(st.check('1'));
        BOOST_CHECK(st.check('2'));
        BOOST_CHECK(st.check('3'));
        BOOST_CHECK(st.check('4'));
        BOOST_CHECK(st.check('5'));
        BOOST_CHECK(st.check('6'));
        BOOST_CHECK(st.check('7'));
        BOOST_CHECK(st.check('8'));
        BOOST_CHECK(st.check('9'));
        BOOST_CHECK( ! st.check('a'));
    }
    {
        StateWord st;
        BOOST_CHECK(st.check('0'));
        BOOST_CHECK(st.check('9'));
        BOOST_CHECK(st.check('a'));
        BOOST_CHECK(st.check('z'));
        BOOST_CHECK(st.check('A'));
        BOOST_CHECK(st.check('Z'));
        BOOST_CHECK(st.check('_'));
        BOOST_CHECK( ! st.check('-'));
        BOOST_CHECK( ! st.check(':'));
    }
    {
        StateMultiTest st;
        st.push_checker(new CheckerDigit);
        st.push_checker(new CheckerString("abc"));
        BOOST_CHECK(st.check('0'));
        BOOST_CHECK(st.check('1'));
        BOOST_CHECK(st.check('9'));
        BOOST_CHECK(st.check('a'));
        BOOST_CHECK(st.check('b'));
        BOOST_CHECK(st.check('c'));
        BOOST_CHECK( ! st.check('d'));
    }
}

BOOST_AUTO_TEST_CASE(TestRegexSt)
{
    struct Reg {
        Reg(const char * s)
        : st(str2reg(s))
        , reset(0)
        {
            append_state_whitout_finish(this->st, this->sts);
        }

        ~Reg()
        {
            std::for_each(this->sts.begin(), this->sts.end(), StateDeleter());
        }

        bool search(const char * s)
        {
            this->preload();
            return st_search(this->st, s);
        }

        bool exact_search(const char * s)
        {
            this->preload();
            return st_exact_search(this->st, s);
        }

        void preload()
        {
            if (this->reset) {
                st_reset_id(this->sts);
            }
            this->reset = true;
        }

        StateBase * st;
        bool reset;
        std::vector<StateBase*> sts;
    };

    {
        Reg reg("a");

        BOOST_CHECK(reg.st);

        BOOST_CHECK_EQUAL(reg.search(""), false);
        BOOST_CHECK_EQUAL(reg.search("a"), true);
        BOOST_CHECK_EQUAL(reg.search("aaaa"), true);
        BOOST_CHECK_EQUAL(reg.search("baaaa"), true);
        BOOST_CHECK_EQUAL(reg.search("b"), false);
        BOOST_CHECK_EQUAL(reg.search("ab"), true);
        BOOST_CHECK_EQUAL(reg.search("abc"), true);
        BOOST_CHECK_EQUAL(reg.search("dabc"), true);

        BOOST_CHECK_EQUAL(reg.exact_search(""), false);
        BOOST_CHECK_EQUAL(reg.exact_search("a"), true);
        BOOST_CHECK_EQUAL(reg.exact_search("aaaa"), false);
        BOOST_CHECK_EQUAL(reg.exact_search("baaaa"), false);
        BOOST_CHECK_EQUAL(reg.exact_search("b"), false);
        BOOST_CHECK_EQUAL(reg.exact_search("ab"), false);
        BOOST_CHECK_EQUAL(reg.exact_search("abc"), false);
        BOOST_CHECK_EQUAL(reg.exact_search("dabc"), false);
    }

    {
        Reg reg("^a");

        BOOST_CHECK(reg.st);

        BOOST_CHECK_EQUAL(reg.search(""), false);
        BOOST_CHECK_EQUAL(reg.search("a"), true);
        BOOST_CHECK_EQUAL(reg.search("aaaa"), true);
        BOOST_CHECK_EQUAL(reg.search("baaaa"), false);
        BOOST_CHECK_EQUAL(reg.search("b"), false);
        BOOST_CHECK_EQUAL(reg.search("ab"), true);
        BOOST_CHECK_EQUAL(reg.search("abc"), true);
        BOOST_CHECK_EQUAL(reg.search("dabc"), false);

        BOOST_CHECK_EQUAL(reg.exact_search(""), false);
        BOOST_CHECK_EQUAL(reg.exact_search("a"), true);
        BOOST_CHECK_EQUAL(reg.exact_search("aaaa"), false);
        BOOST_CHECK_EQUAL(reg.exact_search("baaaa"), false);
        BOOST_CHECK_EQUAL(reg.exact_search("b"), false);
        BOOST_CHECK_EQUAL(reg.exact_search("ab"), false);
        BOOST_CHECK_EQUAL(reg.exact_search("abc"), false);
        BOOST_CHECK_EQUAL(reg.exact_search("dabc"), false);
    }

    {
        Reg reg("a$");

        BOOST_CHECK(reg.st);

        BOOST_CHECK_EQUAL(reg.search(""), false);
        BOOST_CHECK_EQUAL(reg.search("a"), true);
        BOOST_CHECK_EQUAL(reg.search("aaaa"), true);
        BOOST_CHECK_EQUAL(reg.search("baaaa"), true);
        BOOST_CHECK_EQUAL(reg.search("b"), false);
        BOOST_CHECK_EQUAL(reg.search("ab"), false);
        BOOST_CHECK_EQUAL(reg.search("abc"), false);
        BOOST_CHECK_EQUAL(reg.search("dabc"), false);

        BOOST_CHECK_EQUAL(reg.exact_search(""), false);
        BOOST_CHECK_EQUAL(reg.exact_search("a"), true);
        BOOST_CHECK_EQUAL(reg.exact_search("aaaa"), false);
        BOOST_CHECK_EQUAL(reg.exact_search("baaaa"), false);
        BOOST_CHECK_EQUAL(reg.exact_search("b"), false);
        BOOST_CHECK_EQUAL(reg.exact_search("ab"), false);
        BOOST_CHECK_EQUAL(reg.exact_search("abc"), false);
        BOOST_CHECK_EQUAL(reg.exact_search("dabc"), false);
    }

    {
        Reg reg("^a$");

        BOOST_CHECK(reg.st);

        BOOST_CHECK_EQUAL(reg.search(""), false);
        BOOST_CHECK_EQUAL(reg.search("a"), true);
        BOOST_CHECK_EQUAL(reg.search("aaaa"), false);
        BOOST_CHECK_EQUAL(reg.search("baaaa"), false);
        BOOST_CHECK_EQUAL(reg.search("b"), false);
        BOOST_CHECK_EQUAL(reg.search("ab"), false);
        BOOST_CHECK_EQUAL(reg.search("abc"), false);
        BOOST_CHECK_EQUAL(reg.search("dabc"), false);

        BOOST_CHECK_EQUAL(reg.exact_search(""), false);
        BOOST_CHECK_EQUAL(reg.exact_search("a"), true);
        BOOST_CHECK_EQUAL(reg.exact_search("aaaa"), false);
        BOOST_CHECK_EQUAL(reg.exact_search("baaaa"), false);
        BOOST_CHECK_EQUAL(reg.exact_search("b"), false);
        BOOST_CHECK_EQUAL(reg.exact_search("ab"), false);
        BOOST_CHECK_EQUAL(reg.exact_search("abc"), false);
        BOOST_CHECK_EQUAL(reg.exact_search("dabc"), false);
    }

    {
        Reg reg("a+");

        BOOST_CHECK(reg.st);

        BOOST_CHECK_EQUAL(reg.search(""), false);
        BOOST_CHECK_EQUAL(reg.search("a"), true);
        BOOST_CHECK_EQUAL(reg.search("aaaa"), true);
        BOOST_CHECK_EQUAL(reg.search("baaaa"), true);
        BOOST_CHECK_EQUAL(reg.search("b"), false);
        BOOST_CHECK_EQUAL(reg.search("ab"), true);
        BOOST_CHECK_EQUAL(reg.search("abc"), true);
        BOOST_CHECK_EQUAL(reg.search("dabc"), true);

        BOOST_CHECK_EQUAL(reg.exact_search(""), false);
        BOOST_CHECK_EQUAL(reg.exact_search("a"), true);
        BOOST_CHECK_EQUAL(reg.exact_search("aaaa"), true);
        BOOST_CHECK_EQUAL(reg.exact_search("baaaa"), false);
        BOOST_CHECK_EQUAL(reg.exact_search("b"), false);
        BOOST_CHECK_EQUAL(reg.exact_search("ab"), false);
        BOOST_CHECK_EQUAL(reg.exact_search("abc"), false);
        BOOST_CHECK_EQUAL(reg.exact_search("dabc"), false);
    }

    {
        Reg reg(".*a.*$");

        BOOST_CHECK(reg.st);

        BOOST_CHECK_EQUAL(reg.search(""), false);
        BOOST_CHECK_EQUAL(reg.search("a"), true);
        BOOST_CHECK_EQUAL(reg.search("aaaa"), true);
        BOOST_CHECK_EQUAL(reg.search("baaaa"), true);
        BOOST_CHECK_EQUAL(reg.search("b"), false);
        BOOST_CHECK_EQUAL(reg.search("ab"), true);
        BOOST_CHECK_EQUAL(reg.search("abc"), true);
        BOOST_CHECK_EQUAL(reg.search("dabc"), true);

        BOOST_CHECK_EQUAL(reg.exact_search(""), false);
        BOOST_CHECK_EQUAL(reg.exact_search("a"), true);
        BOOST_CHECK_EQUAL(reg.exact_search("aaaa"), true);
        BOOST_CHECK_EQUAL(reg.exact_search("baaaa"), true);
        BOOST_CHECK_EQUAL(reg.exact_search("b"), false);
        BOOST_CHECK_EQUAL(reg.exact_search("ab"), true);
        BOOST_CHECK_EQUAL(reg.exact_search("abc"), true);
        BOOST_CHECK_EQUAL(reg.exact_search("dabc"), true);
    }

    {
        Reg reg("aa+$");

        BOOST_CHECK(reg.st);

        BOOST_CHECK_EQUAL(reg.search(""), false);
        BOOST_CHECK_EQUAL(reg.search("a"), false);
        BOOST_CHECK_EQUAL(reg.search("aaaa"), true);
        BOOST_CHECK_EQUAL(reg.search("baaaa"), true);
        BOOST_CHECK_EQUAL(reg.search("b"), false);
        BOOST_CHECK_EQUAL(reg.search("ab"), false);
        BOOST_CHECK_EQUAL(reg.search("abc"), false);
        BOOST_CHECK_EQUAL(reg.search("dabc"), false);

        BOOST_CHECK_EQUAL(reg.exact_search(""), false);
        BOOST_CHECK_EQUAL(reg.exact_search("a"), false);
        BOOST_CHECK_EQUAL(reg.exact_search("aaaa"), true);
        BOOST_CHECK_EQUAL(reg.exact_search("baaaa"), false);
        BOOST_CHECK_EQUAL(reg.exact_search("b"), false);
        BOOST_CHECK_EQUAL(reg.exact_search("ab"), false);
        BOOST_CHECK_EQUAL(reg.exact_search("abc"), false);
        BOOST_CHECK_EQUAL(reg.exact_search("dabc"), false);
    }

    {
        Reg reg("aa*b?a");

        BOOST_CHECK(reg.st);

        BOOST_CHECK_EQUAL(reg.search(""), false);
        BOOST_CHECK_EQUAL(reg.search("a"), false);
        BOOST_CHECK_EQUAL(reg.search("aaaa"), true);
        BOOST_CHECK_EQUAL(reg.search("baaaa"), true);
        BOOST_CHECK_EQUAL(reg.search("baaba"), true);
        BOOST_CHECK_EQUAL(reg.search("abaaba"), true);
        BOOST_CHECK_EQUAL(reg.search("b"), false);
        BOOST_CHECK_EQUAL(reg.search("ab"), false);
        BOOST_CHECK_EQUAL(reg.search("abc"), false);
        BOOST_CHECK_EQUAL(reg.search("dabc"), false);

        BOOST_CHECK_EQUAL(reg.exact_search(""), false);
        BOOST_CHECK_EQUAL(reg.exact_search("a"), false);
        BOOST_CHECK_EQUAL(reg.exact_search("aaaa"), true);
        BOOST_CHECK_EQUAL(reg.exact_search("baaaa"), false);
        BOOST_CHECK_EQUAL(reg.exact_search("abaaba"), false);
        BOOST_CHECK_EQUAL(reg.exact_search("baaba"), false);
        BOOST_CHECK_EQUAL(reg.exact_search("aaba"), true);
        BOOST_CHECK_EQUAL(reg.exact_search("b"), false);
        BOOST_CHECK_EQUAL(reg.exact_search("ab"), false);
        BOOST_CHECK_EQUAL(reg.exact_search("abc"), false);
        BOOST_CHECK_EQUAL(reg.exact_search("dabc"), false);
    }

    {
        Reg reg("[a-ce]");

        BOOST_CHECK(reg.st);

        BOOST_CHECK_EQUAL(reg.search("a"), true);
        BOOST_CHECK_EQUAL(reg.search("b"), true);
        BOOST_CHECK_EQUAL(reg.search("c"), true);
        BOOST_CHECK_EQUAL(reg.search("d"), false);
        BOOST_CHECK_EQUAL(reg.search("e"), true);
        BOOST_CHECK_EQUAL(reg.search(""), false);
        BOOST_CHECK_EQUAL(reg.search("lka"), true);
        BOOST_CHECK_EQUAL(reg.search("lkd"), false);
    }

    {
        Reg reg("[^a-cd]");

        BOOST_CHECK(reg.st);

        BOOST_CHECK_EQUAL(reg.search("a"), !true);
        BOOST_CHECK_EQUAL(reg.search("b"), !true);
        BOOST_CHECK_EQUAL(reg.search("c"), !true);
        BOOST_CHECK_EQUAL(reg.search("d"), !true);
        BOOST_CHECK_EQUAL(reg.search(""), false);
        BOOST_CHECK_EQUAL(reg.search("lka"), !false);
    }

    {
        Reg reg("(a?b?c?)d(.*)$");

        BOOST_CHECK(reg.st);

        BOOST_CHECK_EQUAL(reg.search("adefg"), true);
    }

    {
        Reg reg("b?a{,1}c");

        BOOST_CHECK(reg.st);

        BOOST_CHECK_EQUAL(reg.search("a{,1}c"), true);
        BOOST_CHECK_EQUAL(reg.exact_search("a{,1}c"), true);
    }

    {
        Reg reg("b?a{0,3}c");

        BOOST_CHECK(reg.st);

        BOOST_CHECK_EQUAL(reg.search("ac"), true);
        BOOST_CHECK_EQUAL(reg.exact_search("ac"), true);
    }

    {
        Reg reg("b?a{2,4}c");

        BOOST_CHECK(reg.st);

        BOOST_CHECK_EQUAL(reg.search("aaac"), true);
        BOOST_CHECK_EQUAL(reg.exact_search("aaac"), true);
    }

    {
        Reg reg("b?a{2,}c");

        BOOST_CHECK(reg.st);

        BOOST_CHECK_EQUAL(reg.search("aaaaaaaac"), true);
        BOOST_CHECK_EQUAL(reg.exact_search("aaaaaaac"), true);
    }

    {
        Reg reg("b?a{0,}c");

        BOOST_CHECK(reg.st);

        BOOST_CHECK_EQUAL(reg.search("c"), true);
        BOOST_CHECK_EQUAL(reg.exact_search("c"), true);
    }
}



inline void regex_test(Regex & p_regex,
                       const char * p_str,
                       const int p_exact_result_search,
                       const int p_result_search,
                       const int p_exact_result_match,
                       const Regex::range_matches & p_exact_match_result,
                       const bool p_result_match,
                       const Regex::range_matches & p_match_result)
{
    BOOST_CHECK_EQUAL(p_regex.exact_search(p_str), p_exact_result_search);
    BOOST_CHECK_EQUAL(p_regex.search(p_str), p_result_search);
    BOOST_CHECK_EQUAL(p_regex.exact_search_with_matches(p_str), p_exact_result_match);
    Regex::range_matches exact_matches = p_regex.match_result();
    BOOST_CHECK(exact_matches == p_exact_match_result);
    BOOST_CHECK_EQUAL(p_regex.search_with_matches(p_str), p_result_match);
    Regex::range_matches matches = p_regex.match_result();
    BOOST_CHECK(matches == p_match_result);
}

BOOST_AUTO_TEST_CASE(TestRegex)
{
    Regex::range_matches matches;

    const char * str_regex = "a";
    Regex regex(str_regex);
    if (regex.message_error()) {
        std::ostringstream os;
        os << str_regex << (regex.message_error())
        << " at offset " << regex.position_error();
        BOOST_CHECK_MESSAGE(false, os.str());
    }

    regex_test(regex, "aaaa", 0, 1, 0, matches, 1, matches);
    regex_test(regex, "a", 1, 1, 1, matches, 1, matches);
    regex_test(regex, "", 0, 0, 0, matches, 0, matches);
    regex_test(regex, "baaaa", 0, 1, 0, matches, 1, matches);
    regex_test(regex, "ba", 0, 1, 0, matches, 1, matches);
    regex_test(regex, "b", 0, 0, 0, matches, 0, matches);
    regex_test(regex, "ab", 0, 1, 0, matches, 1, matches);
    regex_test(regex, "abc", 0, 1, 0, matches, 1, matches);
    regex_test(regex, "dabc", 0, 1, 0, matches, 1, matches);

    str_regex = "^a";
    regex.reset(str_regex);
    if (regex.message_error()) {
        std::ostringstream os;
        os << str_regex << (regex.message_error())
        << " at offset " << regex.position_error();
        BOOST_CHECK_MESSAGE(false, os.str());
    }

    regex_test(regex, "aaaa", 0, 1, 0, matches, 1, matches);
    regex_test(regex, "a", 1, 1, 1, matches, 1, matches);
    regex_test(regex, "", 0, 0, 0, matches, 0, matches);
    regex_test(regex, "baaaa", 0, 0, 0, matches, 0, matches);
    regex_test(regex, "ba", 0, 0, 0, matches, 0, matches);
    regex_test(regex, "b", 0, 0, 0, matches, 0, matches);
    regex_test(regex, "ab", 0, 1, 0, matches, 1, matches);
    regex_test(regex, "abc", 0, 1, 0, matches, 1, matches);
    regex_test(regex, "dabc", 0, 0, 0, matches, 0, matches);

    str_regex = "a$";
    regex.reset(str_regex);
    if (regex.message_error()) {
        std::ostringstream os;
        os << str_regex << (regex.message_error())
        << " at offset " << regex.position_error();
        BOOST_CHECK_MESSAGE(false, os.str());
    }

    regex_test(regex, "aaaa", 0, 1, 0, matches, 1, matches);
    regex_test(regex, "a", 1, 1, 1, matches, 1, matches);
    regex_test(regex, "", 0, 0, 0, matches, 0, matches);
    regex_test(regex, "baaaa", 0, 1, 0, matches, 1, matches);
    regex_test(regex, "aaaab", 0, 0, 0, matches, 0, matches);
    regex_test(regex, "ba", 0, 1, 0, matches, 1, matches);
    regex_test(regex, "b", 0, 0, 0, matches, 0, matches);
    regex_test(regex, "ab", 0, 0, 0, matches, 0, matches);
    regex_test(regex, "abc", 0, 0, 0, matches, 0, matches);
    regex_test(regex, "dabc", 0, 0, 0, matches, 0, matches);

    str_regex = "^a$";
    regex.reset(str_regex);
    if (regex.message_error()) {
        std::ostringstream os;
        os << str_regex << (regex.message_error())
        << " at offset " << regex.position_error();
        BOOST_CHECK_MESSAGE(false, os.str());
    }

    regex_test(regex, "aaaa", 0, 0, 0, matches, 0, matches);
    regex_test(regex, "a", 1, 1, 1, matches, 1, matches);
    regex_test(regex, "", 0, 0, 0, matches, 0, matches);
    regex_test(regex, "baaaa", 0, 0, 0, matches, 0, matches);
    regex_test(regex, "ba", 0, 0, 0, matches, 0, matches);
    regex_test(regex, "b", 0, 0, 0, matches, 0, matches);
    regex_test(regex, "ab", 0, 0, 0, matches, 0, matches);
    regex_test(regex, "abc", 0, 0, 0, matches, 0, matches);
    regex_test(regex, "dabc", 0, 0, 0, matches, 0, matches);

    str_regex = "a+";
    regex.reset(str_regex);
    if (regex.message_error()) {
        std::ostringstream os;
        os << str_regex << (regex.message_error())
        << " at offset " << regex.position_error();
        BOOST_CHECK_MESSAGE(false, os.str());
    }

    regex_test(regex, "aaaa", 1, 1, 1, matches, 1, matches);
    regex_test(regex, "a", 1, 1, 1, matches, 1, matches);
    regex_test(regex, "", 0, 0, 0, matches, 0, matches);
    regex_test(regex, "baaaa", 0, 1, 0, matches, 1, matches);
    regex_test(regex, "ba", 0, 1, 0, matches, 1, matches);
    regex_test(regex, "b", 0, 0, 0, matches, 0, matches);
    regex_test(regex, "ab", 0, 1, 0, matches, 1, matches);
    regex_test(regex, "abc", 0, 1, 0, matches, 1, matches);
    regex_test(regex, "dabc", 0, 1, 0, matches, 1, matches);

    str_regex = ".*a.*$";
    regex.reset(str_regex);
    if (regex.message_error()) {
        std::ostringstream os;
        os << str_regex << (regex.message_error())
        << " at offset " << regex.position_error();
        BOOST_CHECK_MESSAGE(false, os.str());
    }

    regex_test(regex, "aaaa", 1, 1, 1, matches, 1, matches);
    regex_test(regex, "a", 1, 1, 1, matches, 1, matches);
    regex_test(regex, "", 0, 0, 0, matches, 0, matches);
    regex_test(regex, "baaaa", 1, 1, 1, matches, 1, matches);
    regex_test(regex, "ba", 1, 1, 1, matches, 1, matches);
    regex_test(regex, "b", 0, 0, 0, matches, 0, matches);
    regex_test(regex, "ab", 1, 1, 1, matches, 1, matches);
    regex_test(regex, "abc", 1, 1, 1, matches, 1, matches);
    regex_test(regex, "dabc", 1, 1, 1, matches, 1, matches);

    str_regex = "aa+$";
    regex.reset(str_regex);
    if (regex.message_error()) {
        std::ostringstream os;
        os << str_regex << (regex.message_error())
        << " at offset " << regex.position_error();
        BOOST_CHECK_MESSAGE(false, os.str());
    }

    regex_test(regex, "aaaa", 1, 1, 1, matches, 1, matches);
    regex_test(regex, "a", 0, 0, 0, matches, 0, matches);
    regex_test(regex, "", 0, 0, 0, matches, 0, matches);
    regex_test(regex, "baaaa", 0, 1, 0, matches, 1, matches);
    regex_test(regex, "baa", 0, 1, 0, matches, 1, matches);
    regex_test(regex, "ba", 0, 0, 0, matches, 0, matches);
    regex_test(regex, "b", 0, 0, 0, matches, 0, matches);
    regex_test(regex, "ab", 0, 0, 0, matches, 0, matches);
    regex_test(regex, "dabc", 0, 0, 0, matches, 0, matches);

    str_regex = "aa*b?a";
    regex.reset(str_regex);
    if (regex.message_error()) {
        std::ostringstream os;
        os << str_regex << (regex.message_error())
        << " at offset " << regex.position_error();
        BOOST_CHECK_MESSAGE(false, os.str());
    }

    regex_test(regex, "aa", 1, 1, 1, matches, 1, matches);
    regex_test(regex, "aaaa", 0, 1, 0, matches, 1, matches);
    regex_test(regex, "ab", 0, 0, 0, matches, 0, matches);
    regex_test(regex, "", 0, 0, 0, matches, 0, matches);
    regex_test(regex, "baabaa", 0, 1, 0, matches, 1, matches);
    regex_test(regex, "baab", 0, 1, 0, matches, 1, matches);
    regex_test(regex, "bba", 0, 0, 0, matches, 0, matches);
    regex_test(regex, "b", 0, 0, 0, matches, 0, matches);
    regex_test(regex, "aba", 1, 1, 1, matches, 1, matches);
    regex_test(regex, "dabc", 0, 0, 0, matches, 0, matches);

    str_regex = "[a-cd]";
    regex.reset(str_regex);
    if (regex.message_error()) {
        std::ostringstream os;
        os << str_regex << (regex.message_error())
        << " at offset " << regex.position_error();
        BOOST_CHECK_MESSAGE(false, os.str());
    }

    regex_test(regex, "a", 1, 1, 1, matches, 1, matches);
    regex_test(regex, "b", 1, 1, 1, matches, 1, matches);
    regex_test(regex, "c", 1, 1, 1, matches, 1, matches);
    regex_test(regex, "d", 1, 1, 1, matches, 1, matches);
    regex_test(regex, "", 0, 0, 0, matches, 0, matches);
    regex_test(regex, "lka", 0, 1, 0, matches, 1, matches);

    str_regex = "[^a-cd]";
    regex.reset(str_regex);
    if (regex.message_error()) {
        std::ostringstream os;
        os << str_regex << (regex.message_error())
        << " at offset " << regex.position_error();
        BOOST_CHECK_MESSAGE(false, os.str());
    }

    regex_test(regex, "a", 0, 0, 0, matches, 0, matches);
    regex_test(regex, "b", 0, 0, 0, matches, 0, matches);
    regex_test(regex, "c", 0, 0, 0, matches, 0, matches);
    regex_test(regex, "d", 0, 0, 0, matches, 0, matches);
    regex_test(regex, "lka", 0, 1, 0, matches, 1, matches);

    str_regex = "(a?b?c?)d(.*)$";
    regex.reset(str_regex);
    if (regex.message_error()) {
        std::ostringstream os;
        os << str_regex << (regex.message_error())
        << " at offset " << regex.position_error();
        BOOST_CHECK_MESSAGE(false, os.str());
    }

    const char * str = "abcdefg";
    typedef rndfa::StateMachine2::range_t range_t;
    matches.push_back(range_t(str, str+3));
    matches.push_back(range_t(str+4, str+6));
    regex_test(regex, str, 1, 1, 1, matches, 1, matches);

    regex.reset("a{0}");
    if (!regex.message_error()) {
        BOOST_CHECK_MESSAGE(false, "fail");
    }

    regex.reset("a{2,1}");
    if (!regex.message_error()) {
        BOOST_CHECK_MESSAGE(false, "fail");
    }

   matches.clear();

    str_regex = "b?a{,1}c";
    regex.reset(str_regex);
    if (regex.message_error()) {
        std::ostringstream os;
        os << str_regex << (regex.message_error())
        << " at offset " << regex.position_error();
        BOOST_CHECK_MESSAGE(false, os.str());
    }
    regex_test(regex, "a{,1}c", 1, 1, 1, matches, 1, matches);

    str_regex = "b?a{0,1}c";
    regex.reset(str_regex);
    if (regex.message_error()) {
        std::ostringstream os;
        os << str_regex << (regex.message_error())
        << " at offset " << regex.position_error();
        BOOST_CHECK_MESSAGE(false, os.str());
    }
    regex_test(regex, "ac", 1, 1, 1, matches, 1, matches);

    str_regex = "b?a{0,3}c";
    regex.reset(str_regex);
    if (regex.message_error()) {
        std::ostringstream os;
        os << str_regex << (regex.message_error())
        << " at offset " << regex.position_error();
        BOOST_CHECK_MESSAGE(false, os.str());
    }
    regex_test(regex, "ac", 1, 1, 1, matches, 1, matches);

    str_regex = "b?a{2,4}c";
    regex.reset(str_regex);
    if (regex.message_error()) {
        std::ostringstream os;
        os << str_regex << (regex.message_error())
        << " at offset " << regex.position_error();
        BOOST_CHECK_MESSAGE(false, os.str());
    }
    regex_test(regex, "aaac", 1, 1, 1, matches, 1, matches);

    str_regex = "b?a{2,}c";
    regex.reset(str_regex);
    if (regex.message_error()) {
        std::ostringstream os;
        os << str_regex << (regex.message_error())
        << " at offset " << regex.position_error();
        BOOST_CHECK_MESSAGE(false, os.str());
    }
    regex_test(regex, "aaaaaac", 1, 1, 1, matches, 1, matches);

    str_regex = "b?a{0,}c";
    regex.reset(str_regex);
    if (regex.message_error()) {
        std::ostringstream os;
        os << str_regex << (regex.message_error())
        << " at offset " << regex.position_error();
        BOOST_CHECK_MESSAGE(false, os.str());
    }
    regex_test(regex, "c", 1, 1, 1, matches, 1, matches);
}
