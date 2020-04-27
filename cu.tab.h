/* A Bison parser, made by GNU Bison 2.7.91.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2013 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

#ifndef YY_CU_CU_TAB_H_INCLUDED
# define YY_CU_CU_TAB_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int cudebug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    K_comment = 258,
    K_keyword_start = 259,
    K_sub = 260,
    K_void = 261,
    K_int = 262,
    K_float = 263,
    K_string = 264,
    K_if = 265,
    K_else = 266,
    K_for = 267,
    K_while = 268,
    K_do = 269,
    K_print = 270,
    K_printf = 271,
    K_puts = 272,
    K_break = 273,
    K_continue = 274,
    K_return = 275,
    K_goto = 276,
    K_strlen = 277,
    K_atoi = 278,
    K_atof = 279,
    K_strcmp = 280,
    K_strstr = 281,
    K_strchr = 282,
    K_strrchr = 283,
    K_strpbrk = 284,
    K_strcat = 285,
    K_strspn = 286,
    K_strcspn = 287,
    K_strref = 288,
    K_sizeof = 289,
    K_keyword_end = 290,
    K_reserved_start = 291,
    K_static = 292,
    K_char = 293,
    K_short = 294,
    K_long = 295,
    K_double = 296,
    K_signed = 297,
    K_unsigned = 298,
    K_switch = 299,
    K_case = 300,
    K_default = 301,
    K_delete = 302,
    K_new = 303,
    K_reserved_end = 304,
    K_incr = 305,
    K_decr = 306,
    Xdd = 307,
    Xub = 308,
    Xul = 309,
    Xiv = 310,
    Xod = 311,
    Xsl = 312,
    Xsr = 313,
    Xab = 314,
    Xxb = 315,
    Xob = 316,
    K_or = 317,
    K_and = 318,
    K_eq = 319,
    K_ne = 320,
    K_le = 321,
    K_ge = 322,
    K_sl = 323,
    K_sr = 324,
    UM = 325,
    cast_priority = 326,
    K_s = 327,
    K_e = 328,
    T_regex = 329,
    T_regex_err = 330,
    T_new_id = 331,
    T_id = 332,
    T_num = 333,
    T_flt = 334,
    T_str = 335,
    T_arr = 336,
    T_sub = 337,
    T_flt_id = 338,
    T_str_id = 339,
    T_flt_arr = 340,
    T_str_arr = 341,
    T_function = 342,
    T_int_func = 343,
    T_flt_func = 344,
    T_str_func = 345,
    __int_param = 346,
    __flt_param = 347,
    __str_param = 348
  };
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE YYSTYPE;
union YYSTYPE
{
#line 223 "cu.y" /* yacc.c:1932  */

    statement_t* stat;
    expr_t*      expr;
    int          tokn;
    regex_t*     regx;
    char*        emsg;
    function_t*  func;

#line 157 "cu.tab.h" /* yacc.c:1932  */
};
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif

/* Location type.  */
#if ! defined YYLTYPE && ! defined YYLTYPE_IS_DECLARED
typedef struct YYLTYPE YYLTYPE;
struct YYLTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
};
# define YYLTYPE_IS_DECLARED 1
# define YYLTYPE_IS_TRIVIAL 1
#endif


extern YYSTYPE culval;
extern YYLTYPE culloc;
int cuparse (void);

#endif /* !YY_CU_CU_TAB_H_INCLUDED  */
