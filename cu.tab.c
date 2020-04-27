/* A Bison parser, made by GNU Bison 2.7.91.  */

/* Bison implementation for Yacc-like parsers in C

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

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.7.91"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1


/* Substitute the variable and function names.  */
#define yyparse         cuparse
#define yylex           culex
#define yyerror         cuerror
#define yydebug         cudebug
#define yynerrs         cunerrs

#define yylval          culval
#define yychar          cuchar
#define yylloc          culloc

/* Copy the first part of user declarations.  */
#line 19 "cu.y" /* yacc.c:356  */

#include "cu.h"
int yylex();

#define YYLEX_PARAM yyssp,yylsp
#define YYTYPE_INT16 short int
#define YYLTYPE culloc_t
YYTYPE_INT16* cuyyssp=NULL;
YYLTYPE*      cuyylsp=NULL;
int           cuyylex=0;
int           from_stack=0;
vector<int>   cu_token_stk;
int yylex(YYTYPE_INT16* _ssp, YYLTYPE* _lsp){
    cuyyssp=_ssp;
    cuyylsp=_lsp;
  #if 0
    /*P When yylex returns T_function and
         the next token returned is '(',
         start to return tokens from cu_token_stk. 
        If T_function is not followed by '(',
         clear cu_token_stk. */
    if(!from_stack && cu_token_stk.size()){
        from_stack = 1;
    }else if(from_stack==1){
        if(cuyylex=='('){
            from_stack=2;
        }else{
            cu_token_stk.clear();
            from_stack=0;
        }
    }
    if(!cu_token_stk.size()) from_stack=0;
    if(from_stack==2){
        cuyylex = cu_token_stk.back();
        cu_token_stk.pop_back();
        return cuyylex;
    }
  #endif
    return cuyylex=yylex();
}

#define YYLLOC_DEFAULT(Current, Rhs, N)	                 \
    do{                                                  \
        if(N){                                           \
            (Current).lineno = YYRHSLOC (Rhs, 1).lineno; \
            (Current).begin  = YYRHSLOC (Rhs, 1).begin;  \
            (Current).end    = YYRHSLOC (Rhs, N).end  ;  \
        }else{                                           \
            (Current).lineno = culineno ;                \
            (Current).begin  = cu_symbol_begin;          \
            (Current).end    = cu_symbol_begin;          \
        }                                                \
    }while(0)

cu_type decl_type;
cu_type func_type;

expr_t* flty(expr_t* _in_expr){
    if(_in_expr->flag.int_expr) return new expr_t(&expr_t::int_to_flt, NULL, _in_expr);
    if(_in_expr->flag.flt_expr) return _in_expr;
    abort();
}
expr_t* inty(expr_t* _in_expr){
    if(_in_expr->flag.int_expr) return _in_expr;
    if(_in_expr->flag.flt_expr) return new expr_t(&expr_t::flt_to_int, NULL, _in_expr);
    abort();
}

string func_name;
expr_t call_err;

expr_t* verify_function_call(function_t* func, expr_t* args){
    /*P  convert:         to:
             node-A          node-B
              /   \           /  \
          node-B  arg2    arg1  node-A
           /  \                 /  \
        NULL  arg1           arg2  NULL
     */
    static vector<expr_t*> stk; stk.clear();
    /*P flaten the first tree: stk would contains: node-A node-B */
    for(expr_t* e=args; e; e=e->lhs) stk.push_back(e);
    /* Build the second tree */
    args=NULL;
    for(int i=0; i<stk.size(); ++i){
        stk[i]->lhs = stk[i]->rhs;
        stk[i]->rhs = args;
        stk[i]->flag.free_lhs = !!stk[i]->lhs;
        stk[i]->flag.free_rhs = !!stk[i]->rhs;
        args = stk[i]; 
    }
    /*P Check if arguments match the type of parameters*/
    expr_t* para = func->parameters;
    expr_t* arg  = args;
    int n=0;
    static const char* th[] = {"th", "st", "nd", "rd", "th", "th", "th", "th", "th", "th"};
    for(;arg && para; (para=para->rhs), (arg=arg->rhs)){
        ++n;
        symbol_value_t* p = para->lhs->sym;
        if(p->int_var && !arg->lhs->flag.int_expr){
            snprintf(cu_err, sizeof(cu_err), "Error, the %d%s argument to function `%s' should be an integer value.\n", n, th[n%10], func->name->c_str());
            delete args; return &call_err;
        }
        if(p->flt_var && !arg->lhs->flag.flt_expr){
            snprintf(cu_err, sizeof(cu_err), "Error, the %d%s argument to function `%s' should be a float value.\n", n, th[n%10], func->name->c_str());
            delete args; return &call_err;
        }
        if(p->str_var && !arg->lhs->flag.str_expr){
            snprintf(cu_err, sizeof(cu_err), "Error, the %d%s argument to function `%s' should be a string value.\n", n, th[n&10], func->name->c_str());
            delete args; return &call_err;
        }

    }
    if(arg){
        snprintf(cu_err, sizeof(cu_err), "Error, too many arguments to function %s\n", func->name->c_str());
        delete args; return &call_err;
    }
    if(para){
        snprintf(cu_err, sizeof(cu_err), "Error, too few arguments to function %s\n", func->name->c_str());
        delete args; return &call_err;
    }
    return args;
}

int verify_indexing_dimention(expr_t* arr, expr_t* subs, int lineno){
    if(!subs){
        snprintf(cu_err, sizeof(cu_err), "Error, on line %d, %s is an array.\n", lineno, arr->nam);
        return 1;
    }
    if(arr->sym->dimension!=subs->flag.uvwx){
        snprintf(cu_err, sizeof(cu_err), "Error, on line %d, %s has %d dimension(s).\n", lineno, arr->nam, arr->sym->dimension);
        return 1;
    }
    return 0;
}

int in_loop=0;
extern function_t* in_function_decl;
void* yyparse_scope;
int err_symbol_begin;
int err_symbol_end  ;

#line 218 "cu.tab.c" /* yacc.c:356  */

# ifndef YY_NULL
#  if defined __cplusplus && 201103L <= __cplusplus
#   define YY_NULL nullptr
#  else
#   define YY_NULL 0
#  endif
# endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* In a future release of Bison, this section will be replaced
   by #include "cu.tab.h".  */
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
#line 223 "cu.y" /* yacc.c:372  */

    statement_t* stat;
    expr_t*      expr;
    int          tokn;
    regex_t*     regx;
    char*        emsg;
    function_t*  func;

#line 361 "cu.tab.c" /* yacc.c:372  */
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

/* Copy the second part of user declarations.  */

#line 390 "cu.tab.c" /* yacc.c:375  */

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif

#ifndef __attribute__
/* This feature is available in gcc versions 2.5 and later.  */
# if (! defined __GNUC__ || __GNUC__ < 2 \
      || (__GNUC__ == 2 && __GNUC_MINOR__ < 5))
#  define __attribute__(Spec) /* empty */
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(E) ((void) (E))
#else
# define YYUSE(E) /* empty */
#endif


#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL \
             && defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
  YYLTYPE yyls_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE) + sizeof (YYLTYPE)) \
      + 2 * YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYSIZE_T yynewbytes;                                            \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / sizeof (*yyptr);                          \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, (Count) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYSIZE_T yyi;                         \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  3
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   2596

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  120
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  65
/* YYNRULES -- Number of rules.  */
#define YYNRULES  325
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  579

/* YYTRANSLATE[YYX] -- Symbol number corresponding to YYX as returned
   by yylex, with out-of-bounds checking.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   348

#define YYTRANSLATE(YYX)                                                \
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, without out-of-bounds checking.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    84,     2,    80,     2,    83,    69,     2,
     115,   116,    81,    78,   117,    79,     2,    82,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    64,   112,
      72,    52,    73,    63,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   118,    89,   119,    68,     2,    87,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   113,    67,   114,    85,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    65,    66,    70,
      71,    74,    75,    76,    77,    86,    88,    90,    91,    92,
      93,    94,    95,    96,    97,    98,    99,   100,   101,   102,
     103,   104,   105,   106,   107,   108,   109,   110,   111
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   278,   278,   279,   280,   281,   284,   300,   322,   325,
     340,   348,   351,   354,   361,   365,   386,   387,   391,   395,
     404,   407,   412,   413,   414,   415,   416,   417,   418,   419,
     423,   424,   428,   436,   439,   442,   445,   448,   451,   454,
     461,   464,   471,   475,   486,   491,   501,   504,   517,   526,
     530,   535,   540,   549,   553,   558,   596,   601,   602,   618,
     629,   631,   633,   635,   636,   637,   638,   642,   643,   643,
     645,   646,   653,   663,   665,   669,   671,   687,   688,   697,
     744,   790,   813,   814,   815,   816,   817,   818,   819,   820,
     821,   822,   823,   826,   827,   838,   863,   866,   874,   875,
     878,   879,   880,   881,   902,   909,   916,   925,   931,   934,
     940,   947,   948,   949,   950,   957,   963,   966,   969,   979,
     980,   983,   986,   989,   995,  1002,  1003,  1006,  1007,  1010,
    1018,  1029,  1030,  1033,  1038,  1039,  1042,  1050,  1062,  1063,
    1064,  1065,  1066,  1067,  1068,  1069,  1070,  1071,  1087,  1088,
    1089,  1090,  1092,  1093,  1094,  1095,  1096,  1097,  1098,  1099,
    1102,  1103,  1104,  1105,  1106,  1107,  1109,  1110,  1111,  1112,
    1113,  1114,  1116,  1117,  1118,  1119,  1120,  1121,  1123,  1124,
    1125,  1126,  1127,  1128,  1131,  1132,  1133,  1134,  1135,  1136,
    1137,  1138,  1139,  1140,  1141,  1142,  1144,  1146,  1147,  1148,
    1150,  1152,  1153,  1154,  1155,  1156,  1157,  1158,  1159,  1164,
    1168,  1175,  1176,  1183,  1184,  1185,  1204,  1217,  1233,  1237,
    1240,  1241,  1242,  1245,  1249,  1257,  1258,  1259,  1260,  1261,
    1262,  1263,  1264,  1266,  1267,  1268,  1269,  1270,  1271,  1272,
    1273,  1274,  1275,  1276,  1277,  1278,  1279,  1280,  1281,  1282,
    1283,  1284,  1285,  1290,  1294,  1302,  1306,  1319,  1320,  1321,
    1324,  1332,  1340,  1341,  1344,  1350,  1351,  1352,  1353,  1354,
    1355,  1356,  1357,  1358,  1363,  1367,  1376,  1380,  1393,  1396,
    1399,  1402,  1403,  1407,  1408,  1406,  1425,  1439,  1440,  1441,
    1442,  1443,  1444,  1445,  1448,  1449,  1457,  1459,  1459,  1487,
    1494,  1503,  1504,  1505,  1506,  1507,  1508,  1509,  1510,  1511,
    1512,  1513,  1516,  1517,  1520,  1521,  1522,  1527,  1528,  1531,
    1532,  1561,  1562,  1564,  1565,  1566
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 0
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "K_comment", "K_keyword_start", "K_sub",
  "K_void", "K_int", "K_float", "K_string", "K_if", "K_else", "K_for",
  "K_while", "K_do", "K_print", "K_printf", "K_puts", "K_break",
  "K_continue", "K_return", "K_goto", "K_strlen", "K_atoi", "K_atof",
  "K_strcmp", "K_strstr", "K_strchr", "K_strrchr", "K_strpbrk", "K_strcat",
  "K_strspn", "K_strcspn", "K_strref", "K_sizeof", "K_keyword_end",
  "K_reserved_start", "K_static", "K_char", "K_short", "K_long",
  "K_double", "K_signed", "K_unsigned", "K_switch", "K_case", "K_default",
  "K_delete", "K_new", "K_reserved_end", "K_incr", "K_decr", "'='", "Xdd",
  "Xub", "Xul", "Xiv", "Xod", "Xsl", "Xsr", "Xab", "Xxb", "Xob", "'?'",
  "':'", "K_or", "K_and", "'|'", "'^'", "'&'", "K_eq", "K_ne", "'<'",
  "'>'", "K_le", "K_ge", "K_sl", "K_sr", "'+'", "'-'", "'#'", "'*'", "'/'",
  "'%'", "'!'", "'~'", "UM", "'`'", "cast_priority", "'\\\\'", "K_s",
  "K_e", "T_regex", "T_regex_err", "T_new_id", "T_id", "T_num", "T_flt",
  "T_str", "T_arr", "T_sub", "T_flt_id", "T_str_id", "T_flt_arr",
  "T_str_arr", "T_function", "T_int_func", "T_flt_func", "T_str_func",
  "__int_param", "__flt_param", "__str_param", "';'", "'{'", "'}'", "'('",
  "')'", "','", "'['", "']'", "$accept", "assert_program",
  "pre_parse_assert", "post_parse_assert", "program", "top_statements",
  "top_statements_or_end", "statements", "int_decl", "flt_decl",
  "str_decl", "voi_decl", "any_type_decl", "statement",
  "checked_expr_list", "__cu_push_scope__", "__cu_enter_loop__",
  "__cu_leave_loop__", "return_val", "opt_init", "$@1", "opt_cond",
  "opt_post", "var_init_list", "var_init", "new_var_or_array", "T_any_id",
  "opt_init_assign", "force_array_dimension", "array_dimension",
  "opt_array_init", "init_list_selector", "init_list", "init_list_left",
  "flt_init_list", "flt_init_list_left", "str_init_list",
  "str_init_list_left", "expr_list", "expr_list_extra_comma",
  "complex_expr", "expr", "any_arr_id", "lval", "array_indexing",
  "flt_expr", "flt_lval", "str_expr", "str_lval", "printf_vargs",
  "function_decl", "$@2", "$@3", "function_name", "parameters_or_no",
  "parameters", "parameter", "$@4", "parameter_id", "other_parameters",
  "parameter_type", "statements_or_no", "matches_or_no", "matches",
  "any_expr", YY_NULL
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[NUM] -- (External) token number corresponding to the
   (internal) symbol number NUM (which must be that of a token).  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,    61,   307,   308,   309,   310,   311,   312,   313,
     314,   315,   316,    63,    58,   317,   318,   124,    94,    38,
     319,   320,    60,    62,   321,   322,   323,   324,    43,    45,
      35,    42,    47,    37,    33,   126,   325,    96,   326,    92,
     327,   328,   329,   330,   331,   332,   333,   334,   335,   336,
     337,   338,   339,   340,   341,   342,   343,   344,   345,   346,
     347,   348,    59,   123,   125,    40,    41,    44,    91,    93
};
# endif

#define YYPACT_NINF -482

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-482)))

#define YYTABLE_NINF -320

#define yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
      43,    49,  1485,  -482,     7,  -482,   -12,   -10,     0,     2,
    -482,  -482,  -482,     8,    14,    36,    47,  2245,  2245,    68,
      81,    83,    89,   110,   115,   120,   122,   123,   126,     9,
       9,  2245,  2245,  2245,  2245,  2245,  2245,  -482,  -482,  -482,
    -482,    -8,  -482,  -482,    -8,    -8,     6,    10,    12,    13,
    -482,   131,  2277,    90,  -482,  -482,  -482,  -482,  -482,   783,
    1855,   134,   -43,  2478,  1209,   -37,    29,   154,   195,  1855,
    -482,  -482,  2245,  2245,  2245,  2245,  -482,    15,  1595,  2245,
    2245,  -482,  -482,   -12,   -10,     0,   137,  2478,   100,   945,
     469,  2430,   945,   469,  2245,  2245,  2245,  2245,  2245,  2245,
    2245,  2245,  2245,    50,  -482,  -482,  -482,  -482,  -482,  -482,
    2478,   176,  -482,  -482,  -482,  2245,  -482,  -482,  -482,  -482,
    1111,  -482,  1224,  -482,  1335,  -482,  1374,  -482,  1705,    51,
      58,   146,   147,   372,  -482,  -482,   155,   159,   162,   163,
     166,   168,   169,  -482,  -482,  -482,  -482,   178,   171,  -482,
     -21,   177,  -482,  -482,  2245,  -482,  2245,  2245,  2245,  2245,
    2245,  2245,  2245,  2245,  2245,  2245,  2245,  2245,  2245,  2245,
    2245,  2245,  2245,  2245,  2245,  -482,  -482,  2245,  2245,  2245,
    2245,  2245,  2245,  2245,  2245,  2245,  2245,  2245,  2245,  2245,
    2245,  2245,  2245,  2245,  2245,  2245,  2245,  2245,  -482,  2245,
    -482,  2245,  2245,  2245,  2245,  2245,  2245,  2245,  2245,   201,
    -482,  -482,  2245,  -482,  -482,   965,   277,   410,  1213,   316,
    1588,   486,   180,  -482,   945,   469,    16,  -482,  2245,  -482,
     800,   284,   462,   608,   200,   141,   243,   414,  -482,  -482,
     423,   437,   516,   257,   287,   296,   338,   356,   536,  -482,
    -482,  -482,  -482,   208,   568,  -482,  -482,   945,   469,   210,
     213,  -482,  -482,   216,  -482,   222,  -482,   224,  -482,  2245,
     227,  1965,  -482,    -5,  2245,  2245,  -482,  2245,  -482,  -482,
    -482,   800,  2245,  2245,  -482,    80,    39,  -482,   -43,   -18,
     183,  2451,   627,  2496,  2513,   929,   729,   861,   667,   879,
     667,   879,   676,   103,   676,   103,   676,   103,   676,   103,
     343,   343,   125,   136,   125,   136,  -482,  -482,  -482,  -482,
    -482,  -482,   945,   469,  -482,  -482,  -482,  -482,  -482,  -482,
    -482,  -482,  -482,  -482,   667,   879,   667,   879,   676,   103,
     676,   103,   676,   103,   676,   103,   125,   136,   125,   136,
    -482,  -482,  -482,  -482,   200,   141,   243,  2478,   945,   469,
    2478,   945,  2478,   945,  2478,   945,  2478,   945,   343,    34,
    -482,  -482,  2478,   945,   469,  -482,  -482,  -482,  -482,  -482,
    -482,  -482,  1855,  -482,  2379,  -482,   229,  -482,  -482,  -482,
    -482,  -482,  -482,  -482,  -482,   228,   237,   238,   246,   253,
    -482,  -482,  -482,  2245,  2245,  2245,  2245,  2245,  -482,  -482,
      -8,   258,  2245,  -482,  -482,  -482,  2430,  -482,  -482,  2245,
    -482,  -482,  -482,  -482,   945,   469,  -482,  -482,   756,    22,
      23,    24,  -482,  -482,  -482,  -482,   149,  -482,  -482,  -482,
    2245,  2245,   364,  -482,   -12,   671,   753,  -482,   265,  1855,
    2245,   267,   273,   280,  -482,   543,   578,  1848,  2368,   584,
    -482,  -482,  -482,  -482,   278,  -482,  2003,  -482,  2105,  -482,
    2143,  -482,  -482,  -482,  -482,  -482,  -482,  -482,  -482,  -482,
    -482,  -482,  -482,  -482,  -482,  -482,  -482,  -482,   283,  -482,
     288,  -482,   876,  2478,   469,  1855,   800,  -482,  -482,  2245,
    -482,   285,  -482,  -482,  -482,  -482,  -482,  -482,  -482,  -482,
    2245,  -482,  2003,   286,   290,   945,   469,  2105,   295,   294,
     240,   469,  2143,   298,  -482,   945,   357,  -482,   149,  -482,
    -482,  -482,  -482,  -482,   304,  -482,   315,  1475,   314,  -482,
    1743,  -482,   326,  -482,  2105,  -482,  -482,   329,  -482,  2143,
    -482,   331,  -482,    -8,  2245,  -482,   278,   290,  -482,  -482,
     294,  -482,   328,  -482,  1965,  -482,  -482,   330,  -482,  -482,
    -482,  -482,  -482,  -482,   336,  1855,  -482,  -482,  -482
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint16 yydefact[] =
{
       6,     0,     0,     1,     7,    25,    22,    23,    24,     0,
      60,    61,    61,     0,     0,     0,     0,    66,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   216,   190,   220,
     257,   219,   255,   276,   219,   219,     0,     0,     0,     0,
      30,    60,     0,     0,     8,    27,    28,    29,    26,     0,
      10,     0,   282,   134,   195,   282,   221,   282,   258,    17,
       9,     4,     0,     0,     0,     0,    61,     0,     0,     0,
       0,    47,    48,     0,     0,     0,     0,    63,   195,    64,
      65,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   191,   192,   184,   233,   185,   234,
       0,   200,   186,   187,   264,     0,   217,   256,   277,    54,
       0,   210,     0,   254,     0,   275,     0,    31,     0,     0,
       0,     0,   127,     0,     7,     2,    82,    83,    86,    84,
      85,    87,    88,    89,    90,    91,    92,     0,    77,    79,
      94,     0,    14,    33,   281,    11,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   193,   194,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    34,   281,
      12,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      35,    13,     0,    16,    15,     0,     0,     0,     0,     0,
       0,     0,     0,    57,    59,    58,     0,    62,     0,    62,
       0,     0,     0,     0,   282,   282,   282,     0,    55,    56,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   212,
     213,   214,   215,     0,     0,    53,   323,   324,   325,     0,
     320,   321,   209,     0,   253,     0,   274,     0,    43,     0,
       0,    18,    20,   127,     0,     0,   148,     0,   262,     3,
      32,     0,     0,     0,    80,    98,     0,   128,   282,   282,
     282,     0,     0,   189,   188,   149,   150,   151,   152,   160,
     153,   161,   154,   166,   155,   167,   156,   172,   157,   173,
     158,   159,   178,   239,   179,   240,   180,   241,   181,   242,
     182,   135,   136,   137,   138,   139,   140,   141,   142,   143,
     144,   145,   146,   147,   162,   164,   163,   165,   168,   170,
     169,   171,   174,   176,   175,   177,   243,   235,   244,   236,
     245,   237,   246,   238,   282,   282,   282,   223,   222,   224,
     229,   225,   230,   226,   231,   227,   232,   228,   266,   265,
     201,   196,   260,   261,   259,   205,   204,   206,   249,   250,
     267,   268,     0,    62,    67,    39,     0,    41,    82,    83,
      86,    84,    85,    87,    88,     0,     0,     0,     0,     0,
     197,   198,   251,     0,     0,     0,     0,     0,   199,   211,
     219,     0,     0,   208,   252,   273,    21,    42,    19,   133,
     203,   202,   247,   248,   130,   129,    78,    93,     0,     0,
       0,     0,   103,    81,    99,   286,   295,   279,   280,   278,
       0,     0,    36,    45,    68,     0,     0,    70,     0,     0,
       0,     0,     0,     0,    46,     0,     0,     0,     0,     0,
     218,    52,   322,   132,    97,   104,     0,   105,     0,   106,
       0,    22,    23,    24,   301,   302,   305,   303,   304,   306,
     307,   308,   309,   310,   311,   314,   315,   316,     0,   294,
     313,   300,   299,   183,   263,     0,     0,    71,    72,    73,
      62,     0,    50,    51,    49,   207,   269,   271,   272,   270,
       0,    95,     0,     0,   113,   109,   110,     0,     0,   120,
     120,   118,     0,     0,   123,   124,   126,   284,     0,   296,
     297,    37,    69,    74,     0,    38,     0,     0,     0,   100,
     112,   108,     0,   101,     0,   117,   116,     0,   102,     0,
     122,     0,   312,   219,    75,    62,    97,   113,   114,   111,
     120,   119,   126,   125,   318,   298,    76,     0,    40,    96,
     107,   115,   121,   317,     0,     0,   285,    62,    44
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -482,  -482,  -482,   -27,  -482,    -1,  -482,  -270,  -417,  -415,
    -414,  -482,    26,   -66,  -208,   405,    20,  -211,  -482,  -482,
    -482,  -482,  -482,  -275,  -482,  -482,  -218,  -482,  -482,   -98,
    -482,  -482,  -462,   -97,  -452,  -481,  -456,  -103,   -48,    44,
     610,   894,  -482,   401,   -42,    -2,  -482,   207,  -482,   -57,
    -482,  -482,  -482,  -482,  -482,   -63,  -482,  -482,   -30,  -482,
    -482,  -482,   -24,  -482,  -273
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     1,     2,    71,    53,    54,   214,   270,    55,    56,
      57,    58,   230,    60,   222,    76,    77,   385,    86,   448,
     496,   534,   567,   147,   148,   149,   150,   284,   285,   511,
     433,   434,   513,   541,   518,   545,   523,   550,    61,   272,
     132,    63,   253,    88,   116,    92,    66,    93,    68,   437,
      69,   436,   551,   151,   488,   489,   490,   553,   491,   529,
     492,   574,   259,   260,   261
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      65,   418,   117,   118,   131,   155,   426,   119,   200,   427,
     211,   121,   231,   123,   125,    89,   227,   383,   387,   485,
     386,   486,   487,   465,   467,   469,   135,   223,    59,   107,
     109,   282,    78,   188,   189,   190,   191,   192,   193,   546,
     435,   194,   195,    -5,   196,   197,  -283,  -283,  -283,     3,
     538,   249,   188,   189,   190,   191,   192,   193,    65,   152,
     194,   195,   271,   196,   197,   542,   547,    65,   213,  -127,
     216,   219,   221,   224,   154,   198,   232,   235,   559,   571,
     199,   201,   202,   203,   204,   205,    59,   429,   430,   431,
      -7,   134,   561,   563,  -130,    59,   226,   283,   263,   199,
     265,    70,   267,    72,    37,    73,   287,   279,    41,  -131,
     115,   485,   419,   486,   487,    74,   208,    75,   257,   209,
     257,   120,   257,    79,   257,   122,   232,   124,   126,    80,
     228,   384,   432,  -283,  -283,   466,   468,   470,  -283,   462,
    -283,  -283,  -283,  -283,  -283,  -283,  -283,  -283,    81,   250,
     175,   176,   289,   251,   252,  -283,   471,   472,   473,    82,
     299,   301,   303,   305,   307,   309,    72,   274,   313,   315,
     317,   319,   443,    73,   275,   322,   447,   396,   397,   398,
     223,   194,   195,    94,   196,   197,   335,   337,   339,   341,
     343,   345,   347,   349,   351,   353,    95,   355,    96,   358,
     361,   363,   365,   367,    97,   271,   172,   173,   174,    67,
     373,   188,   189,   190,   191,   192,   193,   196,   197,   194,
     195,   532,   196,   197,    90,    98,   224,   497,   498,   287,
      99,   206,   438,   439,   207,   100,   208,   101,   102,   209,
     111,   103,   501,   474,   475,   127,   153,   212,   476,   238,
     477,   478,   479,   480,   481,   482,   483,   484,   199,   133,
     206,   209,   276,   207,   277,   208,   210,    67,   209,   232,
    -287,   199,   421,   423,  -288,   424,    67,  -289,  -290,   217,
     257,  -292,   225,  -291,  -293,   233,   236,   237,   281,   535,
     280,   533,   286,   371,   573,  -129,   382,   395,   438,   439,
     199,   240,   241,   242,   243,   244,   245,   246,   247,   248,
     188,   189,   190,   191,   192,   193,   442,   199,   194,   195,
     206,   196,   197,   207,   409,   208,   411,   258,   209,   258,
     412,   258,   413,   258,   206,   233,   223,   207,   414,   208,
     415,   417,   209,   450,   568,   449,   566,   188,   189,   190,
     191,   192,   193,   451,   452,   194,   195,   544,   196,   197,
     199,   290,   453,   292,   206,   454,   578,   207,   460,   208,
     461,   287,   209,   206,   403,   495,   207,   499,   208,   502,
     232,   209,   224,   500,   323,   503,   188,   189,   190,   191,
     192,   193,   504,   376,   194,   195,   510,   196,   197,   527,
     539,   536,   223,    64,   404,   528,   356,   540,   359,   543,
     257,   544,   548,   405,   369,   206,   554,   424,   207,   374,
     208,   170,   171,   209,   172,   173,   174,   555,   557,   531,
     104,   105,   379,   206,   206,   225,   207,   207,   208,   208,
     560,   209,   209,   562,   564,   549,   575,   232,   224,   206,
     576,   223,   207,    64,   208,   406,   128,   209,   569,   572,
     570,    64,   530,   463,   515,   552,   520,     0,   525,     0,
      64,     0,     0,   407,   549,     0,    64,     0,   233,    64,
      64,     0,     0,     0,   425,     0,     0,   206,   278,   258,
     207,   206,   208,   232,   207,   209,   208,   224,   271,   209,
     206,     0,     0,   207,     0,   208,   223,     0,   209,   577,
     515,   565,     0,     0,   206,   520,     0,   207,     0,   208,
     525,    64,   209,    64,     0,    64,   377,    64,     0,    64,
     399,     0,   188,   189,   190,   191,   192,   193,   515,   400,
     194,   195,   520,   196,   197,     0,   206,   525,     0,   207,
       0,   208,   224,   401,   209,    64,   188,   189,   190,   191,
     192,   193,   232,     0,   194,   195,     0,   196,   197,     0,
       0,     0,     0,   232,   198,     0,     0,     0,    64,    64,
      64,    64,    64,    64,    64,    64,    64,    64,    64,   233,
       0,   225,     0,   206,     0,     0,   207,     0,   208,     0,
      64,   209,   381,     0,     0,     0,     0,     0,     0,     0,
     455,   456,    62,   206,   459,     0,   207,     0,   208,   258,
     206,   209,     0,   207,     0,   208,   425,     0,   209,    64,
       0,   156,   402,   157,   158,   159,   160,   161,   162,   163,
     164,   165,   166,   167,   168,   169,   170,   171,   494,   172,
     173,   174,   408,     0,     0,   206,   233,   225,   207,   505,
     208,   206,     0,   209,   207,     0,   208,     0,     0,   209,
      62,     0,    64,   516,     0,   521,     0,   526,    64,    62,
       0,     0,     0,    64,     0,   206,     0,   410,   207,   234,
     208,   441,     0,   209,   506,     0,     0,     0,     0,     0,
     509,     0,   233,     0,   206,     0,   225,   207,     0,   208,
       0,     0,   209,     0,     0,     0,     0,     0,     0,   516,
     210,     0,     0,     0,   521,     0,     0,     0,     0,   526,
     256,     0,   256,     0,   256,     0,   256,     0,   273,   164,
     165,   166,   167,   168,   169,   170,   171,   516,   172,   173,
     174,   521,   168,   169,   170,   171,   526,   172,   173,   174,
       0,   225,     0,     0,   288,   388,   389,     0,     0,     0,
     390,   233,   391,   392,   393,   394,   143,   144,   145,   146,
       0,     0,   233,    64,     0,    64,    73,   321,   324,   325,
     326,   327,   328,   329,   330,   331,   332,   333,   161,   162,
     163,   164,   165,   166,   167,   168,   169,   170,   171,   354,
     172,   173,   174,    64,     0,     0,     0,     0,     0,   156,
      64,   157,   158,   159,   160,   161,   162,   163,   164,   165,
     166,   167,   168,   169,   170,   171,     0,   172,   173,   174,
       0,     0,     0,     0,     0,     0,     0,   388,   389,     0,
      64,    64,   390,     0,   391,   392,   393,   394,   143,   144,
     145,   146,     0,     0,     0,     0,     0,    64,    74,    64,
       0,    64,     0,     0,     0,   464,     0,   136,   137,     0,
       0,   273,   138,     0,   139,   140,   141,   142,   143,   144,
     145,   146,   256,     0,   388,   389,    64,     0,     0,   390,
      64,   391,   392,   393,   394,   143,   144,   145,   146,     0,
       0,    87,    91,    64,     0,     0,     0,     0,    64,     0,
       0,     0,     0,    64,     0,   106,   108,   110,   112,   113,
     114,   162,   163,   164,   165,   166,   167,   168,   169,   170,
     171,    64,   172,   173,   174,    64,     0,     0,     0,     0,
      64,   190,   191,   192,   193,    64,     0,   194,   195,     0,
     196,   197,     0,     0,     0,    64,   215,   218,   220,     0,
     474,   475,     0,     0,   110,   476,    64,   477,   478,   479,
     480,   481,   482,   483,   484,     0,     0,     0,   110,   110,
     110,   110,   110,   110,   110,   110,   110,   160,   161,   162,
     163,   164,   165,   166,   167,   168,   169,   170,   171,   254,
     172,   173,   174,     0,     0,   188,   189,   190,   191,   192,
     193,     0,   256,   194,   195,     0,   196,   197,   156,   273,
     157,   158,   159,   160,   161,   162,   163,   164,   165,   166,
     167,   168,   169,   170,   171,     0,   172,   173,   174,     0,
     291,   293,   294,   295,   296,   297,   298,   300,   302,   304,
     306,   308,   310,   311,   312,   314,   316,   318,   320,     0,
       0,     0,     0,     0,     0,     0,   514,     0,   519,     0,
     524,   375,   334,   336,   338,   340,   342,   344,   346,   348,
     350,   352,     0,     0,     0,   357,   360,   362,   364,   366,
     368,   110,   370,     0,     0,     0,   372,     0,     0,     0,
       0,     0,   255,     0,     0,     0,     0,     0,    83,    84,
      85,     0,   514,     0,     0,     0,     0,   519,     0,     0,
       0,     0,   524,    19,    20,    21,    22,    23,    24,    25,
      26,     0,     0,     0,    27,    28,     0,     0,     0,     0,
     514,     0,     0,     0,   519,     0,     0,     0,     0,   524,
       0,    29,    30,   416,     0,     0,     0,     0,   420,   422,
       0,     0,     0,     0,   273,     0,     0,   428,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    31,
      32,     0,    33,     0,     0,    34,    35,     0,    36,     0,
       0,     0,     0,     0,     0,     0,    37,    38,    39,    40,
      41,     0,    42,    43,    44,    45,     0,    47,    48,    49,
       0,     0,     0,     0,     0,   262,    52,  -319,     0,     0,
       0,    83,    84,    85,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    19,    20,    21,    22,
      23,    24,    25,    26,     0,     0,     0,    27,    28,   175,
     176,   177,   178,   179,   180,   181,   182,   183,   184,   185,
     186,   187,     0,     0,    29,    30,   156,     0,   157,   158,
     159,   160,   161,   162,   163,   164,   165,   166,   167,   168,
     169,   170,   171,     0,   172,   173,   174,   110,   110,   457,
     458,   110,    31,    32,     0,    33,     0,     0,    34,    35,
       0,    36,     0,     0,     0,     0,     0,     0,     0,    37,
      38,    39,    40,    41,     0,    42,    43,    44,    45,   378,
      47,    48,    49,     0,   493,   110,   264,     0,     0,    52,
    -319,     0,    83,    84,    85,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    19,    20,    21,
      22,    23,    24,    25,    26,     0,     0,     0,    27,    28,
       0,     0,     0,     0,     0,   266,     0,     0,     0,     0,
       0,    83,    84,    85,     0,    29,    30,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    19,    20,    21,    22,
      23,    24,    25,    26,   537,     0,     0,    27,    28,     0,
       0,     0,     0,    31,    32,     0,    33,     0,     0,    34,
      35,     0,    36,     0,    29,    30,     0,     0,     0,     0,
      37,    38,    39,    40,    41,     0,    42,    43,    44,    45,
       0,    47,    48,    49,     0,     0,     0,     0,     0,     0,
      52,  -319,    31,    32,     0,    33,     0,     0,    34,    35,
       0,    36,     0,     0,     0,     0,     0,     0,     0,    37,
      38,    39,    40,    41,     0,    42,    43,    44,    45,     0,
      47,    48,    49,     0,     0,     0,     4,     0,     0,    52,
    -319,     5,     6,     7,     8,     9,     0,    10,    11,    12,
       0,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,     0,     0,     0,    27,    28,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    29,    30,     0,   156,     0,
     157,   158,   159,   160,   161,   162,   163,   164,   165,   166,
     167,   168,   169,   170,   171,     0,   172,   173,   174,     0,
       0,     0,     0,    31,    32,     0,    33,     0,     0,    34,
      35,     0,    36,     0,     0,     0,     0,     0,     0,     0,
      37,    38,    39,    40,    41,     0,    42,    43,    44,    45,
      46,    47,    48,    49,   556,     0,   229,    50,    51,     0,
      52,     5,     6,     7,     8,     9,     0,    10,    11,    12,
       0,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,     0,     0,     0,    27,    28,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    29,    30,     0,     0,     0,
       0,   156,     0,   157,   158,   159,   160,   161,   162,   163,
     164,   165,   166,   167,   168,   169,   170,   171,     0,   172,
     173,   174,     0,    31,    32,     0,    33,     0,     0,    34,
      35,     0,    36,     0,     0,     0,     0,     0,     0,     0,
      37,    38,    39,    40,    41,     0,    42,    43,    44,    45,
      46,    47,    48,    49,   380,     0,   268,    50,    51,     0,
      52,     5,     6,     7,     8,     9,     0,    10,    11,    12,
       0,    13,    14,    15,    16,    17,   269,    19,    20,    21,
      22,    23,    24,    25,    26,     0,     0,     0,    27,    28,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      83,    84,    85,     0,     0,    29,    30,     0,     0,     0,
       0,     0,     0,     0,     0,    19,    20,    21,    22,    23,
      24,    25,    26,     0,     0,     0,    27,    28,     0,     0,
       0,     0,     0,    31,    32,     0,    33,     0,     0,    34,
      35,     0,    36,    29,    30,     0,     0,     0,     0,     0,
      37,    38,    39,    40,    41,     0,    42,    43,    44,    45,
      46,    47,    48,    49,     0,     0,     0,    50,    51,     0,
      52,    31,    32,     0,    33,     0,     0,    34,    35,     0,
      36,     0,     0,     0,     0,     0,     0,     0,    37,    38,
      39,    40,    41,     0,    42,    43,    44,    45,     0,    47,
      48,    49,     0,     0,     0,     0,   512,     0,    52,     0,
     558,     5,     6,     7,     8,     9,     0,    10,    11,    12,
       0,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,     0,     0,     0,    27,    28,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    29,    30,     0,     0,     0,
       0,   156,     0,   157,   158,   159,   160,   161,   162,   163,
     164,   165,   166,   167,   168,   169,   170,   171,     0,   172,
     173,   174,     0,    31,    32,     0,    33,     0,     0,    34,
      35,     0,    36,     0,     0,     0,     0,     0,     0,     0,
      37,    38,    39,    40,    41,     0,    42,    43,    44,    45,
      46,    47,    48,    49,   507,     0,     0,    50,    51,     0,
      52,     5,     6,     7,     8,     9,     0,    10,    11,    12,
       0,    13,    14,    15,    16,    17,   269,    19,    20,    21,
      22,    23,    24,    25,    26,     0,     0,     0,    27,    28,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      83,    84,    85,     0,     0,    29,    30,     0,     0,     0,
       0,     0,     0,     0,     0,    19,    20,    21,    22,    23,
      24,    25,    26,     0,     0,     0,    27,    28,     0,     0,
       0,     0,     0,    31,    32,     0,    33,     0,     0,    34,
      35,     0,    36,    29,    30,     0,     0,     0,     0,     0,
      37,    38,    39,    40,    41,     0,    42,    43,    44,    45,
      46,    47,    48,    49,     0,     0,     0,    50,    51,     0,
      52,    31,    32,     0,    33,     0,     0,    34,    35,     0,
      36,     0,     0,     0,     0,     0,     0,     0,    37,    38,
      39,    40,    41,     0,    42,    43,    44,    45,     0,    47,
      48,    49,    83,    84,    85,     0,   512,     0,    52,     0,
       0,     0,     0,     0,     0,     0,     0,    19,    20,    21,
      22,    23,    24,    25,    26,     0,     0,     0,    27,    28,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      83,    84,    85,     0,     0,    29,    30,     0,     0,     0,
       0,     0,     0,     0,     0,    19,    20,    21,    22,    23,
      24,    25,    26,     0,     0,     0,    27,    28,     0,     0,
       0,     0,     0,    31,    32,     0,    33,     0,     0,    34,
      35,     0,    36,    29,    30,     0,     0,     0,     0,     0,
      37,    38,    39,    40,    41,     0,    42,    43,    44,    45,
       0,    47,    48,    49,     0,     0,     0,     0,   517,     0,
      52,    31,    32,     0,    33,     0,     0,    34,    35,     0,
      36,     0,     0,     0,     0,     0,     0,     0,    37,    38,
      39,    40,    41,     0,    42,    43,    44,    45,     0,    47,
      48,    49,    83,    84,    85,     0,   522,     0,    52,     0,
       0,     0,     0,     0,     0,     0,     0,    19,    20,    21,
      22,    23,    24,    25,    26,     0,     0,     0,    27,    28,
       0,     0,     0,     0,   129,   130,    85,     0,     0,     0,
       0,     0,     0,     0,     0,    29,    30,     0,     0,    19,
      20,    21,    22,    23,    24,    25,    26,     0,     0,     0,
      27,    28,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    31,    32,     0,    33,    29,    30,    34,
      35,     0,    36,     0,     0,     0,     0,     0,     0,     0,
      37,    38,    39,    40,    41,     0,    42,    43,    44,    45,
       0,    47,    48,    49,     0,    31,    32,     0,    33,     0,
      52,    34,    35,     0,    36,     0,     0,     0,     0,     0,
       0,     0,    37,    38,    39,    40,    41,     0,    42,    43,
      44,    45,     0,    47,    48,    49,   444,   445,   446,     0,
       0,     0,    52,     0,     0,     0,     0,     0,     0,     0,
       0,    19,    20,    21,    22,    23,    24,    25,    26,     0,
       0,     0,    27,    28,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    29,
      30,   156,     0,   157,   158,   159,   160,   161,   162,   163,
     164,   165,   166,   167,   168,   169,   170,   171,     0,   172,
     173,   174,     0,     0,     0,     0,     0,    31,    32,     0,
      33,     0,     0,    34,    35,     0,    36,     0,     0,     0,
       0,     0,     0,     0,    37,    38,    39,    40,    41,     0,
      42,    43,    44,    45,   508,    47,    48,    49,     0,     0,
       0,     0,     0,   156,    52,   157,   158,   159,   160,   161,
     162,   163,   164,   165,   166,   167,   168,   169,   170,   171,
       0,   172,   173,   174,   156,   440,   157,   158,   159,   160,
     161,   162,   163,   164,   165,   166,   167,   168,   169,   170,
     171,     0,   172,   173,   174,     0,     0,     0,     0,     0,
       0,   156,   239,   157,   158,   159,   160,   161,   162,   163,
     164,   165,   166,   167,   168,   169,   170,   171,     0,   172,
     173,   174,   158,   159,   160,   161,   162,   163,   164,   165,
     166,   167,   168,   169,   170,   171,     0,   172,   173,   174,
     159,   160,   161,   162,   163,   164,   165,   166,   167,   168,
     169,   170,   171,     0,   172,   173,   174
};

static const yytype_int16 yycheck[] =
{
       2,   271,    44,    45,    52,    62,   281,     1,    65,   282,
      67,     1,    78,     1,     1,    17,     1,     1,   229,   436,
     228,   436,   436,     1,     1,     1,    53,    75,     2,    31,
      32,    52,    12,    70,    71,    72,    73,    74,    75,   520,
       1,    78,    79,     0,    81,    82,     7,     8,     9,     0,
     512,     1,    70,    71,    72,    73,    74,    75,    60,    60,
      78,    79,   128,    81,    82,   517,   522,    69,    69,   112,
      72,    73,    74,    75,   117,   112,    78,    79,   540,   560,
     117,    52,    53,    54,    55,    56,    60,     7,     8,     9,
       0,     1,   544,   549,   112,    69,    76,   118,   122,   117,
     124,    94,   126,   115,    95,   115,   154,   134,    99,   114,
     118,   528,   117,   528,   528,   115,    82,   115,   120,    85,
     122,   115,   124,   115,   126,   115,   128,   115,   115,   115,
     115,   115,    52,    94,    95,   113,   113,   113,    99,   412,
     101,   102,   103,   104,   105,   106,   107,   108,   112,    99,
      50,    51,   154,   103,   104,   116,     7,     8,     9,   112,
     162,   163,   164,   165,   166,   167,   115,   116,   170,   171,
     172,   173,   383,   115,   116,   177,   384,   234,   235,   236,
     228,    78,    79,   115,    81,    82,   188,   189,   190,   191,
     192,   193,   194,   195,   196,   197,   115,   199,   115,   201,
     202,   203,   204,   205,   115,   271,    81,    82,    83,     2,
     212,    70,    71,    72,    73,    74,    75,    81,    82,    78,
      79,   496,    81,    82,    17,   115,   228,   445,   446,   277,
     115,    77,   289,   290,    80,   115,    82,   115,   115,    85,
      33,   115,   450,    94,    95,   114,   112,    52,    99,   112,
     101,   102,   103,   104,   105,   106,   107,   108,   117,    52,
      77,    85,   116,    80,   117,    82,   112,    60,    85,   271,
     115,   117,   274,   275,   115,   277,    69,   115,   115,    72,
     282,   115,    75,   115,   115,    78,    79,    80,   117,   500,
     112,   499,   115,    92,   564,   112,   116,    13,   355,   356,
     117,    94,    95,    96,    97,    98,    99,   100,   101,   102,
      70,    71,    72,    73,    74,    75,   382,   117,    78,    79,
      77,    81,    82,    80,   116,    82,   116,   120,    85,   122,
     117,   124,   116,   126,    77,   128,   384,    80,   116,    82,
     116,   114,    85,   115,   555,   116,   554,    70,    71,    72,
      73,    74,    75,   116,   116,    78,    79,   117,    81,    82,
     117,   154,   116,   156,    77,   112,   577,    80,   410,    82,
     112,   419,    85,    77,   117,    11,    80,   112,    82,   112,
     382,    85,   384,   449,   177,   112,    70,    71,    72,    73,
      74,    75,   112,   116,    78,    79,   118,    81,    82,   116,
     114,   116,   450,     2,   117,   117,   199,   117,   201,   114,
     412,   117,   114,   117,   207,    77,   112,   419,    80,   212,
      82,    78,    79,    85,    81,    82,    83,   112,   114,   495,
      29,    30,   116,    77,    77,   228,    80,    80,    82,    82,
     114,    85,    85,   114,   113,   117,   116,   449,   450,    77,
     114,   499,    80,    52,    82,   117,    51,    85,   556,   562,
     557,    60,   492,   419,   466,   528,   468,    -1,   470,    -1,
      69,    -1,    -1,   117,   117,    -1,    75,    -1,   271,    78,
      79,    -1,    -1,    -1,   277,    -1,    -1,    77,   116,   282,
      80,    77,    82,   495,    80,    85,    82,   499,   564,    85,
      77,    -1,    -1,    80,    -1,    82,   554,    -1,    85,   575,
     512,   553,    -1,    -1,    77,   517,    -1,    80,    -1,    82,
     522,   120,    85,   122,    -1,   124,   116,   126,    -1,   128,
     116,    -1,    70,    71,    72,    73,    74,    75,   540,   116,
      78,    79,   544,    81,    82,    -1,    77,   549,    -1,    80,
      -1,    82,   554,   116,    85,   154,    70,    71,    72,    73,
      74,    75,   564,    -1,    78,    79,    -1,    81,    82,    -1,
      -1,    -1,    -1,   575,   112,    -1,    -1,    -1,   177,   178,
     179,   180,   181,   182,   183,   184,   185,   186,   187,   382,
      -1,   384,    -1,    77,    -1,    -1,    80,    -1,    82,    -1,
     199,    85,   116,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     403,   404,     2,    77,   407,    -1,    80,    -1,    82,   412,
      77,    85,    -1,    80,    -1,    82,   419,    -1,    85,   228,
      -1,    63,   116,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,   441,    81,
      82,    83,   116,    -1,    -1,    77,   449,   450,    80,   116,
      82,    77,    -1,    85,    80,    -1,    82,    -1,    -1,    85,
      60,    -1,   271,   466,    -1,   468,    -1,   470,   277,    69,
      -1,    -1,    -1,   282,    -1,    77,    -1,   119,    80,    79,
      82,    64,    -1,    85,   116,    -1,    -1,    -1,    -1,    -1,
     116,    -1,   495,    -1,    77,    -1,   499,    80,    -1,    82,
      -1,    -1,    85,    -1,    -1,    -1,    -1,    -1,    -1,   512,
     112,    -1,    -1,    -1,   517,    -1,    -1,    -1,    -1,   522,
     120,    -1,   122,    -1,   124,    -1,   126,    -1,   128,    72,
      73,    74,    75,    76,    77,    78,    79,   540,    81,    82,
      83,   544,    76,    77,    78,    79,   549,    81,    82,    83,
      -1,   554,    -1,    -1,   154,    94,    95,    -1,    -1,    -1,
      99,   564,   101,   102,   103,   104,   105,   106,   107,   108,
      -1,    -1,   575,   382,    -1,   384,   115,   177,   178,   179,
     180,   181,   182,   183,   184,   185,   186,   187,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,   199,
      81,    82,    83,   412,    -1,    -1,    -1,    -1,    -1,    63,
     419,    65,    66,    67,    68,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    -1,    81,    82,    83,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    94,    95,    -1,
     449,   450,    99,    -1,   101,   102,   103,   104,   105,   106,
     107,   108,    -1,    -1,    -1,    -1,    -1,   466,   115,   468,
      -1,   470,    -1,    -1,    -1,   119,    -1,    94,    95,    -1,
      -1,   271,    99,    -1,   101,   102,   103,   104,   105,   106,
     107,   108,   282,    -1,    94,    95,   495,    -1,    -1,    99,
     499,   101,   102,   103,   104,   105,   106,   107,   108,    -1,
      -1,    17,    18,   512,    -1,    -1,    -1,    -1,   517,    -1,
      -1,    -1,    -1,   522,    -1,    31,    32,    33,    34,    35,
      36,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,   540,    81,    82,    83,   544,    -1,    -1,    -1,    -1,
     549,    72,    73,    74,    75,   554,    -1,    78,    79,    -1,
      81,    82,    -1,    -1,    -1,   564,    72,    73,    74,    -1,
      94,    95,    -1,    -1,    80,    99,   575,   101,   102,   103,
     104,   105,   106,   107,   108,    -1,    -1,    -1,    94,    95,
      96,    97,    98,    99,   100,   101,   102,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,   115,
      81,    82,    83,    -1,    -1,    70,    71,    72,    73,    74,
      75,    -1,   412,    78,    79,    -1,    81,    82,    63,   419,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    -1,    81,    82,    83,    -1,
     156,   157,   158,   159,   160,   161,   162,   163,   164,   165,
     166,   167,   168,   169,   170,   171,   172,   173,   174,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   466,    -1,   468,    -1,
     470,   116,   188,   189,   190,   191,   192,   193,   194,   195,
     196,   197,    -1,    -1,    -1,   201,   202,   203,   204,   205,
     206,   207,   208,    -1,    -1,    -1,   212,    -1,    -1,    -1,
      -1,    -1,     1,    -1,    -1,    -1,    -1,    -1,     7,     8,
       9,    -1,   512,    -1,    -1,    -1,    -1,   517,    -1,    -1,
      -1,    -1,   522,    22,    23,    24,    25,    26,    27,    28,
      29,    -1,    -1,    -1,    33,    34,    -1,    -1,    -1,    -1,
     540,    -1,    -1,    -1,   544,    -1,    -1,    -1,    -1,   549,
      -1,    50,    51,   269,    -1,    -1,    -1,    -1,   274,   275,
      -1,    -1,    -1,    -1,   564,    -1,    -1,   283,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    78,
      79,    -1,    81,    -1,    -1,    84,    85,    -1,    87,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    95,    96,    97,    98,
      99,    -1,   101,   102,   103,   104,    -1,   106,   107,   108,
      -1,    -1,    -1,    -1,    -1,     1,   115,   116,    -1,    -1,
      -1,     7,     8,     9,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    22,    23,    24,    25,
      26,    27,    28,    29,    -1,    -1,    -1,    33,    34,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    -1,    -1,    50,    51,    63,    -1,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    -1,    81,    82,    83,   403,   404,   405,
     406,   407,    78,    79,    -1,    81,    -1,    -1,    84,    85,
      -1,    87,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    95,
      96,    97,    98,    99,    -1,   101,   102,   103,   104,   116,
     106,   107,   108,    -1,   440,   441,     1,    -1,    -1,   115,
     116,    -1,     7,     8,     9,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    22,    23,    24,
      25,    26,    27,    28,    29,    -1,    -1,    -1,    33,    34,
      -1,    -1,    -1,    -1,    -1,     1,    -1,    -1,    -1,    -1,
      -1,     7,     8,     9,    -1,    50,    51,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    22,    23,    24,    25,
      26,    27,    28,    29,   510,    -1,    -1,    33,    34,    -1,
      -1,    -1,    -1,    78,    79,    -1,    81,    -1,    -1,    84,
      85,    -1,    87,    -1,    50,    51,    -1,    -1,    -1,    -1,
      95,    96,    97,    98,    99,    -1,   101,   102,   103,   104,
      -1,   106,   107,   108,    -1,    -1,    -1,    -1,    -1,    -1,
     115,   116,    78,    79,    -1,    81,    -1,    -1,    84,    85,
      -1,    87,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    95,
      96,    97,    98,    99,    -1,   101,   102,   103,   104,    -1,
     106,   107,   108,    -1,    -1,    -1,     1,    -1,    -1,   115,
     116,     6,     7,     8,     9,    10,    -1,    12,    13,    14,
      -1,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    -1,    -1,    -1,    33,    34,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    50,    51,    -1,    63,    -1,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    -1,    81,    82,    83,    -1,
      -1,    -1,    -1,    78,    79,    -1,    81,    -1,    -1,    84,
      85,    -1,    87,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      95,    96,    97,    98,    99,    -1,   101,   102,   103,   104,
     105,   106,   107,   108,   119,    -1,     1,   112,   113,    -1,
     115,     6,     7,     8,     9,    10,    -1,    12,    13,    14,
      -1,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    -1,    -1,    -1,    33,    34,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    50,    51,    -1,    -1,    -1,
      -1,    63,    -1,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    -1,    81,
      82,    83,    -1,    78,    79,    -1,    81,    -1,    -1,    84,
      85,    -1,    87,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      95,    96,    97,    98,    99,    -1,   101,   102,   103,   104,
     105,   106,   107,   108,   116,    -1,     1,   112,   113,    -1,
     115,     6,     7,     8,     9,    10,    -1,    12,    13,    14,
      -1,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    -1,    -1,    -1,    33,    34,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
       7,     8,     9,    -1,    -1,    50,    51,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    22,    23,    24,    25,    26,
      27,    28,    29,    -1,    -1,    -1,    33,    34,    -1,    -1,
      -1,    -1,    -1,    78,    79,    -1,    81,    -1,    -1,    84,
      85,    -1,    87,    50,    51,    -1,    -1,    -1,    -1,    -1,
      95,    96,    97,    98,    99,    -1,   101,   102,   103,   104,
     105,   106,   107,   108,    -1,    -1,    -1,   112,   113,    -1,
     115,    78,    79,    -1,    81,    -1,    -1,    84,    85,    -1,
      87,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    95,    96,
      97,    98,    99,    -1,   101,   102,   103,   104,    -1,   106,
     107,   108,    -1,    -1,    -1,    -1,   113,    -1,   115,    -1,
     117,     6,     7,     8,     9,    10,    -1,    12,    13,    14,
      -1,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    -1,    -1,    -1,    33,    34,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    50,    51,    -1,    -1,    -1,
      -1,    63,    -1,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    -1,    81,
      82,    83,    -1,    78,    79,    -1,    81,    -1,    -1,    84,
      85,    -1,    87,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      95,    96,    97,    98,    99,    -1,   101,   102,   103,   104,
     105,   106,   107,   108,   116,    -1,    -1,   112,   113,    -1,
     115,     6,     7,     8,     9,    10,    -1,    12,    13,    14,
      -1,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    -1,    -1,    -1,    33,    34,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
       7,     8,     9,    -1,    -1,    50,    51,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    22,    23,    24,    25,    26,
      27,    28,    29,    -1,    -1,    -1,    33,    34,    -1,    -1,
      -1,    -1,    -1,    78,    79,    -1,    81,    -1,    -1,    84,
      85,    -1,    87,    50,    51,    -1,    -1,    -1,    -1,    -1,
      95,    96,    97,    98,    99,    -1,   101,   102,   103,   104,
     105,   106,   107,   108,    -1,    -1,    -1,   112,   113,    -1,
     115,    78,    79,    -1,    81,    -1,    -1,    84,    85,    -1,
      87,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    95,    96,
      97,    98,    99,    -1,   101,   102,   103,   104,    -1,   106,
     107,   108,     7,     8,     9,    -1,   113,    -1,   115,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    22,    23,    24,
      25,    26,    27,    28,    29,    -1,    -1,    -1,    33,    34,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
       7,     8,     9,    -1,    -1,    50,    51,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    22,    23,    24,    25,    26,
      27,    28,    29,    -1,    -1,    -1,    33,    34,    -1,    -1,
      -1,    -1,    -1,    78,    79,    -1,    81,    -1,    -1,    84,
      85,    -1,    87,    50,    51,    -1,    -1,    -1,    -1,    -1,
      95,    96,    97,    98,    99,    -1,   101,   102,   103,   104,
      -1,   106,   107,   108,    -1,    -1,    -1,    -1,   113,    -1,
     115,    78,    79,    -1,    81,    -1,    -1,    84,    85,    -1,
      87,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    95,    96,
      97,    98,    99,    -1,   101,   102,   103,   104,    -1,   106,
     107,   108,     7,     8,     9,    -1,   113,    -1,   115,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    22,    23,    24,
      25,    26,    27,    28,    29,    -1,    -1,    -1,    33,    34,
      -1,    -1,    -1,    -1,     7,     8,     9,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    50,    51,    -1,    -1,    22,
      23,    24,    25,    26,    27,    28,    29,    -1,    -1,    -1,
      33,    34,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    78,    79,    -1,    81,    50,    51,    84,
      85,    -1,    87,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      95,    96,    97,    98,    99,    -1,   101,   102,   103,   104,
      -1,   106,   107,   108,    -1,    78,    79,    -1,    81,    -1,
     115,    84,    85,    -1,    87,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    95,    96,    97,    98,    99,    -1,   101,   102,
     103,   104,    -1,   106,   107,   108,     7,     8,     9,    -1,
      -1,    -1,   115,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    22,    23,    24,    25,    26,    27,    28,    29,    -1,
      -1,    -1,    33,    34,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    50,
      51,    63,    -1,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    -1,    81,
      82,    83,    -1,    -1,    -1,    -1,    -1,    78,    79,    -1,
      81,    -1,    -1,    84,    85,    -1,    87,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    95,    96,    97,    98,    99,    -1,
     101,   102,   103,   104,   116,   106,   107,   108,    -1,    -1,
      -1,    -1,    -1,    63,   115,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      -1,    81,    82,    83,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    -1,    81,    82,    83,    -1,    -1,    -1,    -1,    -1,
      -1,    63,   112,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    -1,    81,
      82,    83,    66,    67,    68,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    -1,    81,    82,    83,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    -1,    81,    82,    83
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,   121,   122,     0,     1,     6,     7,     8,     9,    10,
      12,    13,    14,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    33,    34,    50,
      51,    78,    79,    81,    84,    85,    87,    95,    96,    97,
      98,    99,   101,   102,   103,   104,   105,   106,   107,   108,
     112,   113,   115,   124,   125,   128,   129,   130,   131,   132,
     133,   158,   160,   161,   163,   165,   166,   167,   168,   170,
      94,   123,   115,   115,   115,   115,   135,   136,   136,   115,
     115,   112,   112,     7,     8,     9,   138,   161,   163,   165,
     167,   161,   165,   167,   115,   115,   115,   115,   115,   115,
     115,   115,   115,   115,   163,   163,   161,   165,   161,   165,
     161,   167,   161,   161,   161,   118,   164,   164,   164,     1,
     115,     1,   115,     1,   115,     1,   115,   114,   135,     7,
       8,   158,   160,   167,     1,   123,    94,    95,    99,   101,
     102,   103,   104,   105,   106,   107,   108,   143,   144,   145,
     146,   173,   125,   112,   117,   169,    63,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    81,    82,    83,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    70,    71,
      72,    73,    74,    75,    78,    79,    81,    82,   112,   117,
     169,    52,    53,    54,    55,    56,    77,    80,    82,    85,
     112,   169,    52,   125,   126,   161,   165,   167,   161,   165,
     161,   165,   134,   158,   165,   167,   136,     1,   115,     1,
     132,   133,   165,   167,   160,   165,   167,   167,   112,   112,
     167,   167,   167,   167,   167,   167,   167,   167,   167,     1,
      99,   103,   104,   162,   161,     1,   160,   165,   167,   182,
     183,   184,     1,   182,     1,   182,     1,   182,     1,    21,
     127,   133,   159,   160,   116,   116,   116,   117,   116,   123,
     112,   117,    52,   118,   147,   148,   115,   158,   160,   165,
     167,   161,   167,   161,   161,   161,   161,   161,   161,   165,
     161,   165,   161,   165,   161,   165,   161,   165,   161,   165,
     161,   161,   161,   165,   161,   165,   161,   165,   161,   165,
     161,   160,   165,   167,   160,   160,   160,   160,   160,   160,
     160,   160,   160,   160,   161,   165,   161,   165,   161,   165,
     161,   165,   161,   165,   161,   165,   161,   165,   161,   165,
     161,   165,   161,   165,   160,   165,   167,   161,   165,   167,
     161,   165,   161,   165,   161,   165,   161,   165,   161,   167,
     161,    92,   161,   165,   167,   116,   116,   116,   116,   116,
     116,   116,   116,     1,   115,   137,   134,   137,    94,    95,
      99,   101,   102,   103,   104,    13,   169,   169,   169,   116,
     116,   116,   116,   117,   117,   117,   117,   117,   116,   116,
     119,   116,   117,   116,   116,   116,   161,   114,   127,   117,
     161,   165,   161,   165,   165,   167,   143,   184,   161,     7,
       8,     9,    52,   150,   151,     1,   171,   169,   169,   169,
      64,    64,   133,   137,     7,     8,     9,   134,   139,   116,
     115,   116,   116,   116,   112,   167,   167,   161,   161,   167,
     164,   112,   184,   159,   119,     1,   113,     1,   113,     1,
     113,     7,     8,     9,    94,    95,    99,   101,   102,   103,
     104,   105,   106,   107,   108,   128,   129,   130,   174,   175,
     176,   178,   180,   161,   167,    11,   140,   146,   146,   112,
     133,   134,   112,   112,   112,   116,   116,   116,   116,   116,
     118,   149,   113,   152,   160,   165,   167,   113,   154,   160,
     165,   167,   113,   156,   160,   165,   167,   116,   117,   179,
     178,   133,   143,   134,   141,   137,   116,   161,   152,   114,
     117,   153,   154,   114,   117,   155,   155,   156,   114,   117,
     157,   172,   175,   177,   112,   112,   119,   114,   117,   152,
     114,   154,   114,   156,   113,   164,   134,   142,   137,   149,
     153,   155,   157,   127,   181,   116,   114,   133,   137
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,   120,   121,   121,   121,   121,   122,   123,   124,   124,
     125,   125,   125,   125,   125,   125,   126,   126,   127,   127,
     127,   127,   128,   129,   130,   131,   132,   132,   132,   132,
     133,   133,   133,   133,   133,   133,   133,   133,   133,   133,
     133,   133,   133,   133,   133,   133,   133,   133,   133,   133,
     133,   133,   133,   133,   133,   133,   133,   134,   134,   134,
     135,   136,   137,   138,   138,   138,   138,   139,   140,   139,
     139,   139,   139,   141,   141,   142,   142,   143,   143,   144,
     145,   145,   146,   146,   146,   146,   146,   146,   146,   146,
     146,   146,   146,   147,   147,   148,   149,   149,   150,   150,
     151,   151,   151,   151,   151,   151,   151,   152,   152,   152,
     152,   153,   153,   153,   153,   154,   154,   154,   154,   155,
     155,   156,   156,   156,   156,   157,   157,   158,   158,   158,
     158,   159,   159,   159,   160,   160,   160,   160,   160,   160,
     160,   160,   160,   160,   160,   160,   160,   160,   161,   161,
     161,   161,   161,   161,   161,   161,   161,   161,   161,   161,
     161,   161,   161,   161,   161,   161,   161,   161,   161,   161,
     161,   161,   161,   161,   161,   161,   161,   161,   161,   161,
     161,   161,   161,   161,   161,   161,   161,   161,   161,   161,
     161,   161,   161,   161,   161,   161,   161,   161,   161,   161,
     161,   161,   161,   161,   161,   161,   161,   161,   161,   161,
     161,   161,   161,   162,   162,   162,   163,   163,   164,   164,
     165,   165,   165,   165,   165,   165,   165,   165,   165,   165,
     165,   165,   165,   165,   165,   165,   165,   165,   165,   165,
     165,   165,   165,   165,   165,   165,   165,   165,   165,   165,
     165,   165,   165,   165,   165,   166,   166,   167,   167,   167,
     167,   167,   167,   167,   167,   167,   167,   167,   167,   167,
     167,   167,   167,   167,   167,   167,   168,   168,   169,   169,
     169,   169,   169,   171,   172,   170,   170,   173,   173,   173,
     173,   173,   173,   173,   174,   174,   175,   177,   176,   176,
     176,   178,   178,   178,   178,   178,   178,   178,   178,   178,
     178,   178,   179,   179,   180,   180,   180,   181,   181,   182,
     182,   183,   183,   184,   184,   184
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     3,     4,     3,     0,     0,     0,     1,     2,
       1,     2,     2,     2,     2,     2,     1,     0,     1,     2,
       1,     2,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     2,     3,     2,     2,     2,     5,     7,     7,     4,
       9,     4,     4,     3,    12,     5,     5,     2,     2,     6,
       6,     6,     5,     3,     2,     3,     3,     1,     1,     1,
       0,     0,     0,     1,     1,     1,     0,     0,     0,     3,
       1,     2,     2,     0,     1,     0,     1,     1,     3,     1,
       2,     3,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     2,     0,     4,     4,     0,     0,     1,
       4,     4,     4,     1,     2,     2,     2,     4,     2,     1,
       1,     2,     1,     0,     2,     4,     2,     2,     1,     2,
       0,     4,     2,     1,     1,     2,     0,     1,     3,     3,
       3,     1,     3,     2,     1,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     5,     2,     2,     2,     2,     3,     3,
       1,     2,     2,     2,     2,     1,     3,     4,     4,     4,
       2,     3,     4,     4,     4,     4,     4,     6,     4,     3,
       2,     4,     3,     1,     1,     1,     1,     2,     4,     0,
       1,     1,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     2,     2,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     4,     4,     4,
       4,     4,     4,     3,     2,     1,     2,     1,     1,     3,
       3,     3,     3,     5,     2,     3,     3,     4,     4,     6,
       6,     6,     6,     4,     3,     2,     1,     2,     3,     3,
       3,     1,     0,     0,     0,    10,     4,     1,     1,     1,
       1,     1,     1,     1,     1,     0,     2,     0,     4,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     2,     0,     1,     1,     1,     1,     0,     0,
       1,     1,     3,     1,     1,     1
};


#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)
#define YYEMPTY         (-2)
#define YYEOF           0

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                  \
do                                                              \
  if (yychar == YYEMPTY)                                        \
    {                                                           \
      yychar = (Token);                                         \
      yylval = (Value);                                         \
      YYPOPSTACK (yylen);                                       \
      yystate = *yyssp;                                         \
      goto yybackup;                                            \
    }                                                           \
  else                                                          \
    {                                                           \
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;                                                  \
    }                                                           \
while (0)

/* Error token number */
#define YYTERROR        1
#define YYERRCODE       256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)                                \
    do                                                                  \
      if (N)                                                            \
        {                                                               \
          (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;        \
          (Current).first_column = YYRHSLOC (Rhs, 1).first_column;      \
          (Current).last_line    = YYRHSLOC (Rhs, N).last_line;         \
          (Current).last_column  = YYRHSLOC (Rhs, N).last_column;       \
        }                                                               \
      else                                                              \
        {                                                               \
          (Current).first_line   = (Current).last_line   =              \
            YYRHSLOC (Rhs, 0).last_line;                                \
          (Current).first_column = (Current).last_column =              \
            YYRHSLOC (Rhs, 0).last_column;                              \
        }                                                               \
    while (0)
#endif

#define YYRHSLOC(Rhs, K) ((Rhs)[K])


/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL

/* Print *YYLOCP on YYO.  Private, do not rely on its existence. */

__attribute__((__unused__))
static unsigned
yy_location_print_ (FILE *yyo, YYLTYPE const * const yylocp)
{
  unsigned res = 0;
  int end_col = 0 != yylocp->last_column ? yylocp->last_column - 1 : 0;
  if (0 <= yylocp->first_line)
    {
      res += YYFPRINTF (yyo, "%d", yylocp->first_line);
      if (0 <= yylocp->first_column)
        res += YYFPRINTF (yyo, ".%d", yylocp->first_column);
    }
  if (0 <= yylocp->last_line)
    {
      if (yylocp->first_line < yylocp->last_line)
        {
          res += YYFPRINTF (yyo, "-%d", yylocp->last_line);
          if (0 <= end_col)
            res += YYFPRINTF (yyo, ".%d", end_col);
        }
      else if (0 <= end_col && yylocp->first_column < end_col)
        res += YYFPRINTF (yyo, "-%d", end_col);
    }
  return res;
 }

#  define YY_LOCATION_PRINT(File, Loc)          \
  yy_location_print_ (File, &(Loc))

# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


# define YY_SYMBOL_PRINT(Title, Type, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Type, Value, Location); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp)
{
  FILE *yyo = yyoutput;
  YYUSE (yyo);
  YYUSE (yylocationp);
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# endif
  YYUSE (yytype);
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp)
{
  YYFPRINTF (yyoutput, "%s %s (",
             yytype < YYNTOKENS ? "token" : "nterm", yytname[yytype]);

  YY_LOCATION_PRINT (yyoutput, *yylocationp);
  YYFPRINTF (yyoutput, ": ");
  yy_symbol_value_print (yyoutput, yytype, yyvaluep, yylocationp);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yytype_int16 *yyssp, YYSTYPE *yyvsp, YYLTYPE *yylsp, int yyrule)
{
  unsigned long int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       yystos[yyssp[yyi + 1 - yynrhs]],
                       &(yyvsp[(yyi + 1) - (yynrhs)])
                       , &(yylsp[(yyi + 1) - (yynrhs)])                       );
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, yylsp, Rule); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif


#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
static YYSIZE_T
yystrlen (const char *yystr)
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
yystpcpy (char *yydest, const char *yysrc)
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
        switch (*++yyp)
          {
          case '\'':
          case ',':
            goto do_not_strip_quotes;

          case '\\':
            if (*++yyp != '\\')
              goto do_not_strip_quotes;
            /* Fall through.  */
          default:
            if (yyres)
              yyres[yyn] = *yyp;
            yyn++;
            break;

          case '"':
            if (yyres)
              yyres[yyn] = '\0';
            return yyn;
          }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return 1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return 2 if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYSIZE_T *yymsg_alloc, char **yymsg,
                yytype_int16 *yyssp, int yytoken)
{
  YYSIZE_T yysize0 = yytnamerr (YY_NULL, yytname[yytoken]);
  YYSIZE_T yysize = yysize0;
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULL;
  /* Arguments of yyformat. */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Number of reported tokens (one for the "unexpected", one per
     "expected"). */
  int yycount = 0;

  /* There are many possibilities here to consider:
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (yytoken != YYEMPTY)
    {
      int yyn = yypact[*yyssp];
      yyarg[yycount++] = yytname[yytoken];
      if (!yypact_value_is_default (yyn))
        {
          /* Start YYX at -YYN if negative to avoid negative indexes in
             YYCHECK.  In other words, skip the first -YYN actions for
             this state because they are default actions.  */
          int yyxbegin = yyn < 0 ? -yyn : 0;
          /* Stay within bounds of both yycheck and yytname.  */
          int yychecklim = YYLAST - yyn + 1;
          int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
          int yyx;

          for (yyx = yyxbegin; yyx < yyxend; ++yyx)
            if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR
                && !yytable_value_is_error (yytable[yyx + yyn]))
              {
                if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                  {
                    yycount = 1;
                    yysize = yysize0;
                    break;
                  }
                yyarg[yycount++] = yytname[yyx];
                {
                  YYSIZE_T yysize1 = yysize + yytnamerr (YY_NULL, yytname[yyx]);
                  if (! (yysize <= yysize1
                         && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
                    return 2;
                  yysize = yysize1;
                }
              }
        }
    }

  switch (yycount)
    {
# define YYCASE_(N, S)                      \
      case N:                               \
        yyformat = S;                       \
      break
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
# undef YYCASE_
    }

  {
    YYSIZE_T yysize1 = yysize + yystrlen (yyformat);
    if (! (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
      return 2;
    yysize = yysize1;
  }

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return 1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *yyp = *yymsg;
    int yyi = 0;
    while ((*yyp = *yyformat) != '\0')
      if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
        {
          yyp += yytnamerr (yyp, yyarg[yyi++]);
          yyformat += 2;
        }
      else
        {
          yyp++;
          yyformat++;
        }
  }
  return 0;
}
#endif /* YYERROR_VERBOSE */

/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep, YYLTYPE *yylocationp)
{
  YYUSE (yyvaluep);
  YYUSE (yylocationp);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {
          case 95: /* T_id  */
#line 262 "cu.y" /* yacc.c:1274  */
      {delete ((*yyvaluep).expr);}
#line 2083 "cu.tab.c" /* yacc.c:1274  */
        break;

    case 96: /* T_num  */
#line 262 "cu.y" /* yacc.c:1274  */
      {delete ((*yyvaluep).expr);}
#line 2089 "cu.tab.c" /* yacc.c:1274  */
        break;

    case 97: /* T_flt  */
#line 262 "cu.y" /* yacc.c:1274  */
      {delete ((*yyvaluep).expr);}
#line 2095 "cu.tab.c" /* yacc.c:1274  */
        break;

    case 98: /* T_str  */
#line 262 "cu.y" /* yacc.c:1274  */
      {delete ((*yyvaluep).expr);}
#line 2101 "cu.tab.c" /* yacc.c:1274  */
        break;

    case 99: /* T_arr  */
#line 262 "cu.y" /* yacc.c:1274  */
      {delete ((*yyvaluep).expr);}
#line 2107 "cu.tab.c" /* yacc.c:1274  */
        break;

    case 100: /* T_sub  */
#line 262 "cu.y" /* yacc.c:1274  */
      {delete ((*yyvaluep).expr);}
#line 2113 "cu.tab.c" /* yacc.c:1274  */
        break;

    case 101: /* T_flt_id  */
#line 262 "cu.y" /* yacc.c:1274  */
      {delete ((*yyvaluep).expr);}
#line 2119 "cu.tab.c" /* yacc.c:1274  */
        break;

    case 102: /* T_str_id  */
#line 262 "cu.y" /* yacc.c:1274  */
      {delete ((*yyvaluep).expr);}
#line 2125 "cu.tab.c" /* yacc.c:1274  */
        break;

    case 103: /* T_flt_arr  */
#line 262 "cu.y" /* yacc.c:1274  */
      {delete ((*yyvaluep).expr);}
#line 2131 "cu.tab.c" /* yacc.c:1274  */
        break;

    case 104: /* T_str_arr  */
#line 262 "cu.y" /* yacc.c:1274  */
      {delete ((*yyvaluep).expr);}
#line 2137 "cu.tab.c" /* yacc.c:1274  */
        break;

    case 125: /* top_statements  */
#line 261 "cu.y" /* yacc.c:1274  */
      {delete ((*yyvaluep).stat);}
#line 2143 "cu.tab.c" /* yacc.c:1274  */
        break;

    case 126: /* top_statements_or_end  */
#line 261 "cu.y" /* yacc.c:1274  */
      {delete ((*yyvaluep).stat);}
#line 2149 "cu.tab.c" /* yacc.c:1274  */
        break;

    case 127: /* statements  */
#line 261 "cu.y" /* yacc.c:1274  */
      {delete ((*yyvaluep).stat);}
#line 2155 "cu.tab.c" /* yacc.c:1274  */
        break;

    case 133: /* statement  */
#line 261 "cu.y" /* yacc.c:1274  */
      {delete ((*yyvaluep).stat);}
#line 2161 "cu.tab.c" /* yacc.c:1274  */
        break;

    case 134: /* checked_expr_list  */
#line 262 "cu.y" /* yacc.c:1274  */
      {delete ((*yyvaluep).expr);}
#line 2167 "cu.tab.c" /* yacc.c:1274  */
        break;

    case 138: /* return_val  */
#line 262 "cu.y" /* yacc.c:1274  */
      {delete ((*yyvaluep).expr);}
#line 2173 "cu.tab.c" /* yacc.c:1274  */
        break;

    case 139: /* opt_init  */
#line 262 "cu.y" /* yacc.c:1274  */
      {delete ((*yyvaluep).expr);}
#line 2179 "cu.tab.c" /* yacc.c:1274  */
        break;

    case 141: /* opt_cond  */
#line 262 "cu.y" /* yacc.c:1274  */
      {delete ((*yyvaluep).expr);}
#line 2185 "cu.tab.c" /* yacc.c:1274  */
        break;

    case 142: /* opt_post  */
#line 262 "cu.y" /* yacc.c:1274  */
      {delete ((*yyvaluep).expr);}
#line 2191 "cu.tab.c" /* yacc.c:1274  */
        break;

    case 143: /* var_init_list  */
#line 262 "cu.y" /* yacc.c:1274  */
      {delete ((*yyvaluep).expr);}
#line 2197 "cu.tab.c" /* yacc.c:1274  */
        break;

    case 144: /* var_init  */
#line 262 "cu.y" /* yacc.c:1274  */
      {delete ((*yyvaluep).expr);}
#line 2203 "cu.tab.c" /* yacc.c:1274  */
        break;

    case 145: /* new_var_or_array  */
#line 262 "cu.y" /* yacc.c:1274  */
      {delete ((*yyvaluep).expr);}
#line 2209 "cu.tab.c" /* yacc.c:1274  */
        break;

    case 147: /* opt_init_assign  */
#line 262 "cu.y" /* yacc.c:1274  */
      {delete ((*yyvaluep).expr);}
#line 2215 "cu.tab.c" /* yacc.c:1274  */
        break;

    case 148: /* force_array_dimension  */
#line 262 "cu.y" /* yacc.c:1274  */
      {delete ((*yyvaluep).expr);}
#line 2221 "cu.tab.c" /* yacc.c:1274  */
        break;

    case 149: /* array_dimension  */
#line 262 "cu.y" /* yacc.c:1274  */
      {delete ((*yyvaluep).expr);}
#line 2227 "cu.tab.c" /* yacc.c:1274  */
        break;

    case 150: /* opt_array_init  */
#line 262 "cu.y" /* yacc.c:1274  */
      {delete ((*yyvaluep).expr);}
#line 2233 "cu.tab.c" /* yacc.c:1274  */
        break;

    case 151: /* init_list_selector  */
#line 262 "cu.y" /* yacc.c:1274  */
      {delete ((*yyvaluep).expr);}
#line 2239 "cu.tab.c" /* yacc.c:1274  */
        break;

    case 152: /* init_list  */
#line 262 "cu.y" /* yacc.c:1274  */
      {delete ((*yyvaluep).expr);}
#line 2245 "cu.tab.c" /* yacc.c:1274  */
        break;

    case 153: /* init_list_left  */
#line 262 "cu.y" /* yacc.c:1274  */
      {delete ((*yyvaluep).expr);}
#line 2251 "cu.tab.c" /* yacc.c:1274  */
        break;

    case 154: /* flt_init_list  */
#line 262 "cu.y" /* yacc.c:1274  */
      {delete ((*yyvaluep).expr);}
#line 2257 "cu.tab.c" /* yacc.c:1274  */
        break;

    case 155: /* flt_init_list_left  */
#line 262 "cu.y" /* yacc.c:1274  */
      {delete ((*yyvaluep).expr);}
#line 2263 "cu.tab.c" /* yacc.c:1274  */
        break;

    case 156: /* str_init_list  */
#line 262 "cu.y" /* yacc.c:1274  */
      {delete ((*yyvaluep).expr);}
#line 2269 "cu.tab.c" /* yacc.c:1274  */
        break;

    case 157: /* str_init_list_left  */
#line 262 "cu.y" /* yacc.c:1274  */
      {delete ((*yyvaluep).expr);}
#line 2275 "cu.tab.c" /* yacc.c:1274  */
        break;

    case 158: /* expr_list  */
#line 262 "cu.y" /* yacc.c:1274  */
      {delete ((*yyvaluep).expr);}
#line 2281 "cu.tab.c" /* yacc.c:1274  */
        break;

    case 159: /* expr_list_extra_comma  */
#line 262 "cu.y" /* yacc.c:1274  */
      {delete ((*yyvaluep).expr);}
#line 2287 "cu.tab.c" /* yacc.c:1274  */
        break;

    case 160: /* complex_expr  */
#line 262 "cu.y" /* yacc.c:1274  */
      {delete ((*yyvaluep).expr);}
#line 2293 "cu.tab.c" /* yacc.c:1274  */
        break;

    case 161: /* expr  */
#line 262 "cu.y" /* yacc.c:1274  */
      {delete ((*yyvaluep).expr);}
#line 2299 "cu.tab.c" /* yacc.c:1274  */
        break;

    case 162: /* any_arr_id  */
#line 262 "cu.y" /* yacc.c:1274  */
      {delete ((*yyvaluep).expr);}
#line 2305 "cu.tab.c" /* yacc.c:1274  */
        break;

    case 163: /* lval  */
#line 262 "cu.y" /* yacc.c:1274  */
      {delete ((*yyvaluep).expr);}
#line 2311 "cu.tab.c" /* yacc.c:1274  */
        break;

    case 164: /* array_indexing  */
#line 262 "cu.y" /* yacc.c:1274  */
      {delete ((*yyvaluep).expr);}
#line 2317 "cu.tab.c" /* yacc.c:1274  */
        break;

    case 165: /* flt_expr  */
#line 262 "cu.y" /* yacc.c:1274  */
      {delete ((*yyvaluep).expr);}
#line 2323 "cu.tab.c" /* yacc.c:1274  */
        break;

    case 166: /* flt_lval  */
#line 262 "cu.y" /* yacc.c:1274  */
      {delete ((*yyvaluep).expr);}
#line 2329 "cu.tab.c" /* yacc.c:1274  */
        break;

    case 167: /* str_expr  */
#line 262 "cu.y" /* yacc.c:1274  */
      {delete ((*yyvaluep).expr);}
#line 2335 "cu.tab.c" /* yacc.c:1274  */
        break;

    case 168: /* str_lval  */
#line 262 "cu.y" /* yacc.c:1274  */
      {delete ((*yyvaluep).expr);}
#line 2341 "cu.tab.c" /* yacc.c:1274  */
        break;

    case 169: /* printf_vargs  */
#line 262 "cu.y" /* yacc.c:1274  */
      {delete ((*yyvaluep).expr);}
#line 2347 "cu.tab.c" /* yacc.c:1274  */
        break;

    case 173: /* function_name  */
#line 262 "cu.y" /* yacc.c:1274  */
      {delete ((*yyvaluep).expr);}
#line 2353 "cu.tab.c" /* yacc.c:1274  */
        break;

    case 174: /* parameters_or_no  */
#line 262 "cu.y" /* yacc.c:1274  */
      {delete ((*yyvaluep).expr);}
#line 2359 "cu.tab.c" /* yacc.c:1274  */
        break;

    case 175: /* parameters  */
#line 262 "cu.y" /* yacc.c:1274  */
      {delete ((*yyvaluep).expr);}
#line 2365 "cu.tab.c" /* yacc.c:1274  */
        break;

    case 176: /* parameter  */
#line 262 "cu.y" /* yacc.c:1274  */
      {delete ((*yyvaluep).expr);}
#line 2371 "cu.tab.c" /* yacc.c:1274  */
        break;

    case 178: /* parameter_id  */
#line 262 "cu.y" /* yacc.c:1274  */
      {delete ((*yyvaluep).expr);}
#line 2377 "cu.tab.c" /* yacc.c:1274  */
        break;

    case 179: /* other_parameters  */
#line 262 "cu.y" /* yacc.c:1274  */
      {delete ((*yyvaluep).expr);}
#line 2383 "cu.tab.c" /* yacc.c:1274  */
        break;

    case 181: /* statements_or_no  */
#line 261 "cu.y" /* yacc.c:1274  */
      {delete ((*yyvaluep).stat);}
#line 2389 "cu.tab.c" /* yacc.c:1274  */
        break;

    case 182: /* matches_or_no  */
#line 262 "cu.y" /* yacc.c:1274  */
      {delete ((*yyvaluep).expr);}
#line 2395 "cu.tab.c" /* yacc.c:1274  */
        break;

    case 183: /* matches  */
#line 262 "cu.y" /* yacc.c:1274  */
      {delete ((*yyvaluep).expr);}
#line 2401 "cu.tab.c" /* yacc.c:1274  */
        break;

    case 184: /* any_expr  */
#line 262 "cu.y" /* yacc.c:1274  */
      {delete ((*yyvaluep).expr);}
#line 2407 "cu.tab.c" /* yacc.c:1274  */
        break;


      default:
        break;
    }
}




/* The lookahead symbol.  */
int yychar;


#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval YY_INITIAL_VALUE (yyval_default);

/* Location data for the lookahead symbol.  */
YYLTYPE yylloc
# if defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL
  = { 1, 1, 1, 1 }
# endif
;


/* Number of syntax errors so far.  */
int yynerrs;


/*----------.
| yyparse.  |
`----------*/

int
yyparse (void)
{
    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       'yyss': related to states.
       'yyvs': related to semantic values.
       'yyls': related to locations.

       Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    /* The location stack.  */
    YYLTYPE yylsa[YYINITDEPTH];
    YYLTYPE *yyls;
    YYLTYPE *yylsp;

    /* The locations where the error started and ended.  */
    YYLTYPE yyerror_range[3];

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken = 0;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;
  YYLTYPE yyloc;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N), yylsp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yyssp = yyss = yyssa;
  yyvsp = yyvs = yyvsa;
  yylsp = yyls = yylsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */
  yylsp[0] = yylloc;
  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        YYSTYPE *yyvs1 = yyvs;
        yytype_int16 *yyss1 = yyss;
        YYLTYPE *yyls1 = yyls;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * sizeof (*yyssp),
                    &yyvs1, yysize * sizeof (*yyvsp),
                    &yyls1, yysize * sizeof (*yylsp),
                    &yystacksize);

        yyls = yyls1;
        yyss = yyss1;
        yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yytype_int16 *yyss1 = yyss;
        union yyalloc *yyptr =
          (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
        if (! yyptr)
          goto yyexhaustedlab;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
        YYSTACK_RELOCATE (yyls_alloc, yyls);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;
      yylsp = yyls + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
                  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = yylex ();
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END
  *++yylsp = yylloc;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];

  /* Default location.  */
  YYLLOC_DEFAULT (yyloc, (yylsp - yylen), yylen);
  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 5:
#line 281 "cu.y" /* yacc.c:1678  */
    {first_statement=NULL;}
#line 2706 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 6:
#line 284 "cu.y" /* yacc.c:1678  */
    {
   *cu_err = 0;
    assert(!in_loop);
    assert(!in_function_decl);
    yyparse_scope = cu_top_scope();
    cu_push_scope(1);
    err_symbol_begin = 0;
    err_symbol_end   = 0;
 }
#line 2726 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 7:
#line 300 "cu.y" /* yacc.c:1678  */
    {
    assert(!in_loop);
    assert(!in_function_decl);
    void cu_merge_scope();
    void cu_install_new_func(int);
    if(YYRECOVERING()){
        if(!*cu_err){
            sprintf(cu_err, "Error, on line %d, syntax error.\n", culineno);
        }
        cu_pop_scope();
        cu_install_new_func(0);
    }else{
        cu_merge_scope();
        cu_install_new_func(1);
    }
    assert(yyparse_scope==cu_top_scope());
    if(YYRECOVERING()){
        YYABORT;
    }
 }
#line 2751 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 8:
#line 322 "cu.y" /* yacc.c:1678  */
    {first_statement=(yyvsp[0].stat);}
#line 2757 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 9:
#line 325 "cu.y" /* yacc.c:1678  */
    {
              if(!*cu_err){
                  snprintf(cu_err, sizeof(cu_err), "Error, on line %d, `%s' is not defined.\n", (yylsp[0]).lineno, symbol_name.c_str());
                  err_symbol_begin = (yylsp[0]).begin;
                  err_symbol_end   = (yylsp[0]).end;
              }
              YYERROR;
          }
#line 2770 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 10:
#line 340 "cu.y" /* yacc.c:1678  */
    {
                  (yyval.stat) = (yyvsp[0].stat);
              }
#line 2778 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 11:
#line 348 "cu.y" /* yacc.c:1678  */
    {
                  (yyval.stat) = new printf_statement_t((yyvsp[-1].expr), (yyvsp[0].expr));
              }
#line 2786 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 12:
#line 351 "cu.y" /* yacc.c:1678  */
    {
                  (yyval.stat) = new printf_statement_t((yyvsp[-1].expr), (yyvsp[0].expr));
              }
#line 2794 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 13:
#line 354 "cu.y" /* yacc.c:1678  */
    {
                  static str_t emtpy_str(1, "");
                  expr_t* n = new expr_t(&expr_t::vargs_str_node, (yyvsp[-1].expr), (yyvsp[0].expr));
                  (yyval.stat) = new printf_statement_t(new expr_t(&emtpy_str, 0), n);
              }
#line 2804 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 14:
#line 361 "cu.y" /* yacc.c:1678  */
    {
                  (yyval.stat) = (yyvsp[-1].stat);
                  (yyvsp[-1].stat)->next = (yyvsp[0].stat);
              }
#line 2813 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 15:
#line 365 "cu.y" /* yacc.c:1678  */
    {
                  if(!(yyvsp[0].stat)) (yyvsp[0].stat) = new statement_t();
                  (yyval.stat) = (yyvsp[0].stat);
              }
#line 2830 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 17:
#line 387 "cu.y" /* yacc.c:1678  */
    {(yyval.stat) = NULL;}
#line 2836 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 18:
#line 391 "cu.y" /* yacc.c:1678  */
    {
              (yyval.stat) = (yyvsp[0].stat); 
          }
#line 2844 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 19:
#line 395 "cu.y" /* yacc.c:1678  */
    {
              (yyval.stat) = (yyvsp[-1].stat);
              (yyvsp[-1].stat)->next = (yyvsp[0].stat);
          }
#line 2853 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 20:
#line 404 "cu.y" /* yacc.c:1678  */
    {
              (yyval.stat) = new expr_statement_t((yyvsp[0].expr));
          }
#line 2861 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 21:
#line 407 "cu.y" /* yacc.c:1678  */
    {
              (yyval.stat) = new goto_statement_t((yyvsp[0].expr));
          }
#line 2869 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 22:
#line 412 "cu.y" /* yacc.c:1678  */
    {decl_type=int_type;}
#line 2875 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 23:
#line 413 "cu.y" /* yacc.c:1678  */
    {decl_type=flt_type;}
#line 2881 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 24:
#line 414 "cu.y" /* yacc.c:1678  */
    {decl_type=str_type;}
#line 2887 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 25:
#line 415 "cu.y" /* yacc.c:1678  */
    {decl_type=voi_type;}
#line 2893 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 30:
#line 423 "cu.y" /* yacc.c:1678  */
    {(yyval.stat)=new statement_t;}
#line 2899 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 31:
#line 424 "cu.y" /* yacc.c:1678  */
    {(yyval.stat)=new statement_t;}
#line 2905 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 32:
#line 428 "cu.y" /* yacc.c:1678  */
    {
              (yyval.stat) = new int_expr_statement_t((yyvsp[-1].expr));
          }
#line 2918 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 33:
#line 436 "cu.y" /* yacc.c:1678  */
    {
              (yyval.stat) = new expr_statement_t((yyvsp[-1].expr));
          }
#line 2926 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 34:
#line 439 "cu.y" /* yacc.c:1678  */
    {
              (yyval.stat) = new flt_expr_statement_t((yyvsp[-1].expr));
          }
#line 2934 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 35:
#line 442 "cu.y" /* yacc.c:1678  */
    {
              (yyval.stat) = new str_expr_statement_t((yyvsp[-1].expr));
          }
#line 2942 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 36:
#line 445 "cu.y" /* yacc.c:1678  */
    {
              (yyval.stat) = new if_statement_t((yyvsp[-2].expr), (yyvsp[0].stat), NULL);
          }
#line 2950 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 37:
#line 448 "cu.y" /* yacc.c:1678  */
    {
              (yyval.stat) = new if_statement_t((yyvsp[-4].expr), (yyvsp[-2].stat), (yyvsp[0].stat));
          }
#line 2958 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 38:
#line 451 "cu.y" /* yacc.c:1678  */
    {
              (yyval.stat) = new while_statement_t((yyvsp[-3].expr), (yyvsp[-1].stat));
          }
#line 2966 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 39:
#line 454 "cu.y" /* yacc.c:1678  */
    {
              (yyval.stat) = NULL;
              YYERROR;
          }
#line 2978 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 40:
#line 461 "cu.y" /* yacc.c:1678  */
    {
              (yyval.stat) = new do_statement_t((yyvsp[-3].expr), (yyvsp[-6].stat));
          }
#line 2986 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 41:
#line 464 "cu.y" /* yacc.c:1678  */
    {
              (yyval.stat) = NULL;
              YYERROR;
          }
#line 2998 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 42:
#line 471 "cu.y" /* yacc.c:1678  */
    {
              (yyval.stat) = new block_statement_t((yyvsp[-1].stat));
              cu_pop_scope();
          }
#line 3007 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 43:
#line 475 "cu.y" /* yacc.c:1678  */
    {
              (yyval.stat) = NULL;
              cu_pop_scope();
              YYERROR;
          }
#line 3023 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 44:
#line 487 "cu.y" /* yacc.c:1678  */
    {
              (yyval.stat) = new for_statement_t((yyvsp[-7].expr), (yyvsp[-5].expr), (yyvsp[-3].expr), (yyvsp[-1].stat)); 
              cu_pop_scope();
          }
#line 3032 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 45:
#line 491 "cu.y" /* yacc.c:1678  */
    {
              // $$ = new statement_t();
              // cu_pop_scope();
              (yyval.stat) = NULL;
              cu_pop_scope();
              YYERROR;
          }
#line 3044 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 46:
#line 501 "cu.y" /* yacc.c:1678  */
    {
              (yyval.stat) = new puts_statement_t((yyvsp[-2].expr));
          }
#line 3052 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 47:
#line 504 "cu.y" /* yacc.c:1678  */
    {
              (yyval.stat) = new control_statement_t(K_break);
              if(!in_loop){
                  sprintf(cu_err, "Error, on line %d, break statement not within a loop.\n", (yylsp[-1]).lineno);
                  err_symbol_begin = (yylsp[-1]).begin;
                  err_symbol_end   = (yylsp[-1]).end;
                  YYERROR;
              }
          }
#line 3070 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 48:
#line 517 "cu.y" /* yacc.c:1678  */
    {
              (yyval.stat) = new control_statement_t(K_continue);
              if(!in_loop){
                  sprintf(cu_err, "Error, on line %d, continue statement not within a loop.\n", (yylsp[-1]).lineno);
                  err_symbol_begin = (yylsp[-1]).begin;
                  err_symbol_end   = (yylsp[-1]).end;
                  YYERROR;
              }
          }
#line 3084 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 49:
#line 526 "cu.y" /* yacc.c:1678  */
    {
              //$$ = new printf_statement_t(new expr_t('s', $3, null_expr), $4);
              (yyval.stat) = new printf_statement_t((yyvsp[-3].expr), (yyvsp[-2].expr));
          }
#line 3093 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 50:
#line 530 "cu.y" /* yacc.c:1678  */
    {
              /* this rule allow the first argument to printf
                  not being a string */
              (yyval.stat) = new printf_statement_t((yyvsp[-3].expr), (yyvsp[-2].expr));
          }
#line 3103 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 51:
#line 535 "cu.y" /* yacc.c:1678  */
    {
              /* this rule allow the first argument to printf
                  not being a string */
              (yyval.stat) = new printf_statement_t((yyvsp[-3].expr), (yyvsp[-2].expr));
          }
#line 3113 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 52:
#line 540 "cu.y" /* yacc.c:1678  */
    {
              expr_t* args = verify_function_call((yyvsp[-4].func), (yyvsp[-2].expr));
              if(args == &call_err) YYERROR;
              (yyval.stat) = new expr_statement_t(
                    new expr_t(&expr_t::function_call_int, (yyvsp[-4].func), args, NULL));
          }
#line 3127 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 53:
#line 549 "cu.y" /* yacc.c:1678  */
    {
              (yyval.stat) = NULL;
              YYERROR;
          }
#line 3136 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 54:
#line 553 "cu.y" /* yacc.c:1678  */
    {
              snprintf(cu_err, sizeof(cu_err), "Error, on line %d, %s is a function.\n", (yylsp[-1]).lineno, (yyvsp[-1].func)->name->c_str());
              (yyval.stat) = NULL;
              YYERROR;
          }
#line 3146 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 55:
#line 558 "cu.y" /* yacc.c:1678  */
    {
              if(!in_function_decl){
                  sprintf(cu_err, "Error, on line %d, return statement not within a function.\n", (yylsp[-2]).lineno);
                  delete (yyvsp[-1].expr);
                  err_symbol_begin = (yylsp[-2]).begin;
                  err_symbol_end   = (yylsp[0]).end;
                  YYERROR;
              }
              if(func_type==voi_type && (yyvsp[-1].expr)){
                  snprintf(cu_err, sizeof(cu_err), "Error, on line %d, function `%s' returns no value.\n", (yylsp[-2]).lineno, func_name.c_str());
                  delete (yyvsp[-1].expr);
                  err_symbol_begin = (yylsp[-1]).begin;
                  err_symbol_end   = (yylsp[-1]).end;
                  YYERROR;
              }
              if(func_type!=voi_type && !(yyvsp[-1].expr)){
                  sprintf(cu_err, "Error, on line %d, missing a return value.\n", (yylsp[-2]).lineno);
                  err_symbol_begin = (yylsp[-2]).begin;
                  err_symbol_end   = (yylsp[0]).end;
                  YYERROR;
              }
              int type_mismatch = 0;
              switch(func_type){
                case int_type : if(!(yyvsp[-1].expr)->flag.int_expr) type_mismatch=1; break;
                case flt_type : if(!(yyvsp[-1].expr)->flag.flt_expr) type_mismatch=2; break;
                case str_type : if(!(yyvsp[-1].expr)->flag.str_expr) type_mismatch=3; break;
                case voi_type : if(!!!((yyvsp[-1].expr)==null_expr)) type_mismatch=4; break;
              }
              if(type_mismatch){
                  const char* type_name[] = {"", "int", "float", "string", "void"};
                  snprintf(cu_err, sizeof(cu_err), "Error, on line %d, function `%s' returns a %s value.\n", (yylsp[-2]).lineno, func_name.c_str(), type_name[type_mismatch]); 
                  delete (yyvsp[-1].expr);
                  err_symbol_begin = (yylsp[-1]).begin;
                  err_symbol_end   = (yylsp[-1]).end;
                  YYERROR;
              }
              (yyval.stat) = new return_statement_t((yyvsp[-1].expr));
          }
#line 3189 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 56:
#line 596 "cu.y" /* yacc.c:1678  */
    {
              (yyval.stat) = new goto_statement_t((yyvsp[-1].expr));
          }
#line 3197 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 58:
#line 602 "cu.y" /* yacc.c:1678  */
    {
                     if((yyvsp[0].expr)->eval_str==&expr_t::get_line_str){
                         (yyvsp[0].expr)->eval_int=&expr_t::get_line_int;
                         (yyvsp[0].expr)->flag.str_expr = 0;
                         (yyvsp[0].expr)->flag.int_expr = 0;
                         (yyval.expr)=(yyvsp[0].expr);
                         break;
                     }
                     sprintf(cu_err, "Error, on line %d, expecting a int value, got a string value.\n", (yylsp[0]).lineno);
                     err_symbol_begin = (yylsp[0]).begin;
                     err_symbol_end   = (yylsp[0]).end;
                     delete (yyvsp[0].expr);
                     (yyval.expr)=NULL;
                     YYERROR;
                 }
#line 3218 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 59:
#line 618 "cu.y" /* yacc.c:1678  */
    {
                     sprintf(cu_err, "Error, on line %d, expecting a int value, got a float value.\n", (yylsp[0]).lineno);
                     err_symbol_begin = (yylsp[0]).begin;
                     err_symbol_end   = (yylsp[0]).end;
                     delete (yyvsp[0].expr);
                     (yyval.expr)=NULL;
                     YYERROR;
                 }
#line 3231 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 60:
#line 629 "cu.y" /* yacc.c:1678  */
    {cu_push_scope();}
#line 3237 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 61:
#line 631 "cu.y" /* yacc.c:1678  */
    {in_loop++;}
#line 3243 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 62:
#line 633 "cu.y" /* yacc.c:1678  */
    {in_loop--;}
#line 3249 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 66:
#line 638 "cu.y" /* yacc.c:1678  */
    {(yyval.expr)=NULL;}
#line 3255 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 67:
#line 642 "cu.y" /* yacc.c:1678  */
    {(yyval.expr) = NULL;}
#line 3261 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 68:
#line 643 "cu.y" /* yacc.c:1678  */
    {decl_type=int_type;}
#line 3267 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 69:
#line 643 "cu.y" /* yacc.c:1678  */
    {(yyval.expr) =  (yyvsp[0].expr) ;}
#line 3273 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 70:
#line 645 "cu.y" /* yacc.c:1678  */
    {(yyval.expr) =  (yyvsp[0].expr) ;}
#line 3279 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 71:
#line 646 "cu.y" /* yacc.c:1678  */
    {
             sprintf(cu_err, "Sorry, on line %d, currently, only int variables could be declared here.\n", (yylsp[-1]).lineno);
             err_symbol_begin = (yylsp[-1]).begin;
             err_symbol_end   = (yylsp[-1]).end;
             (yyval.expr) = NULL;
             YYERROR;
         }
#line 3291 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 72:
#line 653 "cu.y" /* yacc.c:1678  */
    {
             sprintf(cu_err, "Sorry, on line %d, currently, only int variables could be declared here.\n", (yylsp[-1]).lineno);
             err_symbol_begin = (yylsp[-1]).begin;
             err_symbol_end   = (yylsp[-1]).end;
             (yyval.expr) = NULL;
             YYERROR;
         }
#line 3303 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 73:
#line 663 "cu.y" /* yacc.c:1678  */
    {(yyval.expr) = NULL;}
#line 3309 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 74:
#line 665 "cu.y" /* yacc.c:1678  */
    {(yyval.expr) =  (yyvsp[0].expr) ;}
#line 3315 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 75:
#line 669 "cu.y" /* yacc.c:1678  */
    {(yyval.expr) = NULL;}
#line 3321 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 76:
#line 671 "cu.y" /* yacc.c:1678  */
    {(yyval.expr) =  (yyvsp[0].expr) ;}
#line 3327 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 78:
#line 688 "cu.y" /* yacc.c:1678  */
    {
                  (yyval.expr) = new expr_t(&expr_t::comma_int, (yyvsp[-2].expr), (yyvsp[0].expr));
              }
#line 3338 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 79:
#line 697 "cu.y" /* yacc.c:1678  */
    {(yyval.expr) = (yyvsp[0].expr);}
#line 3344 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 80:
#line 744 "cu.y" /* yacc.c:1678  */
    {
             if(decl_type==voi_type){
                sprintf(cu_err, "Error, on line %d, declaring variables as void.\n", (yylsp[-1]).lineno);
                delete_expr((yyvsp[0].expr));
                YYERROR;
             }
             (yyval.expr) = push_var(symbol_backup, (yylsp[-1]).lineno, NULL, decl_type);
             if(!(yyval.expr)){
                 delete_expr((yyvsp[0].expr));
                 YYERROR;
             }
             int type_error = 0;
             switch(decl_type){
               case int_type: if((yyvsp[0].expr) && !(yyvsp[0].expr)->flag.int_expr) type_error=1; break;
               case flt_type: if((yyvsp[0].expr) && !(yyvsp[0].expr)->flag.flt_expr) type_error=1; break;
               case str_type: if((yyvsp[0].expr) && !(yyvsp[0].expr)->flag.str_expr) type_error=1; break;
             }
             if(!type_error){
                 switch(decl_type){
                   case int_type: (yyval.expr) = new expr_t(&expr_t::reinitialize_int, (yyval.expr), (yyvsp[0].expr)); break;
                   case flt_type: (yyval.expr) = new expr_t(&expr_t::reinitialize_flt, (yyval.expr), (yyvsp[0].expr)); break;
                   case str_type: (yyval.expr) = new expr_t(&expr_t::reinitialize_str, (yyval.expr), (yyvsp[0].expr)); break;
                 }
             }else{
                 switch(decl_type){
                   case int_type: snprintf(cu_err, sizeof(cu_err), "Error, on line %d, the initial value to `%s' is not a int value.\n", (yylsp[-1]).lineno, symbol_backup.c_str()); break;
                   case flt_type: snprintf(cu_err, sizeof(cu_err), "Error, on line %d, the initial value to `%s' is not a float value.\n", (yylsp[-1]).lineno, symbol_backup.c_str()); break;
                   case str_type: snprintf(cu_err, sizeof(cu_err), "Error, on line %d, the initial value to `%s' is not a string value.\n", (yylsp[-1]).lineno, symbol_backup.c_str()); break;
                 }
                 delete_expr((yyval.expr), (yyvsp[0].expr));
                 err_symbol_begin = (yylsp[0]).begin;
                 err_symbol_end   = (yylsp[0]).end;
                 YYERROR;
             }
         }
#line 3384 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 81:
#line 790 "cu.y" /* yacc.c:1678  */
    {
             if(decl_type==voi_type){
                sprintf(cu_err, "Error, on line %d, declaring variables as void array.\n", (yylsp[-2]).lineno);
                delete_expr((yyvsp[-1].expr), (yyvsp[0].expr));
                YYERROR;
             }
             (yyval.expr) = push_var(symbol_backup, (yylsp[-2]).lineno, (yyvsp[-1].expr), decl_type);
             if(!(yyval.expr)){
                 delete_expr((yyvsp[-1].expr), (yyvsp[0].expr));
                 YYERROR;
             }
             if((yyvsp[0].expr) && !dimension_consistent((yyvsp[-1].expr),(yyvsp[0].expr))){
                 delete_expr((yyval.expr), (yyvsp[-1].expr), (yyvsp[0].expr));
                 YYERROR;
             }
             delete_expr((yyvsp[-1].expr));
             switch(decl_type){
               case int_type: (yyval.expr) = new expr_t(&expr_t::reinitialize_int, (yyval.expr), (yyvsp[0].expr)); break;
               case flt_type: (yyval.expr) = new expr_t(&expr_t::reinitialize_flt, (yyval.expr), (yyvsp[0].expr)); break;
               case str_type: (yyval.expr) = new expr_t(&expr_t::reinitialize_str, (yyval.expr), (yyvsp[0].expr)); break;
             }
         }
#line 3411 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 82:
#line 813 "cu.y" /* yacc.c:1678  */
    {symbol_backup.swap(symbol_name);}
#line 3417 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 83:
#line 814 "cu.y" /* yacc.c:1678  */
    {symbol_backup.swap(symbol_name); delete (yyvsp[0].expr);}
#line 3423 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 84:
#line 815 "cu.y" /* yacc.c:1678  */
    {symbol_backup.swap(symbol_name); delete (yyvsp[0].expr);}
#line 3429 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 85:
#line 816 "cu.y" /* yacc.c:1678  */
    {symbol_backup.swap(symbol_name); delete (yyvsp[0].expr);}
#line 3435 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 86:
#line 817 "cu.y" /* yacc.c:1678  */
    {symbol_backup.swap(symbol_name); delete (yyvsp[0].expr);}
#line 3441 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 87:
#line 818 "cu.y" /* yacc.c:1678  */
    {symbol_backup.swap(symbol_name); delete (yyvsp[0].expr);}
#line 3447 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 88:
#line 819 "cu.y" /* yacc.c:1678  */
    {symbol_backup.swap(symbol_name); delete (yyvsp[0].expr);}
#line 3453 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 89:
#line 820 "cu.y" /* yacc.c:1678  */
    {symbol_backup.swap(symbol_name);}
#line 3459 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 90:
#line 821 "cu.y" /* yacc.c:1678  */
    {symbol_backup.swap(symbol_name);}
#line 3465 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 91:
#line 822 "cu.y" /* yacc.c:1678  */
    {symbol_backup.swap(symbol_name);}
#line 3471 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 92:
#line 823 "cu.y" /* yacc.c:1678  */
    {symbol_backup.swap(symbol_name);}
#line 3477 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 93:
#line 826 "cu.y" /* yacc.c:1678  */
    {(yyval.expr)=(yyvsp[0].expr); (yyloc)=(yylsp[0]);}
#line 3483 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 94:
#line 827 "cu.y" /* yacc.c:1678  */
    {(yyval.expr)=NULL;}
#line 3489 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 95:
#line 838 "cu.y" /* yacc.c:1678  */
    {
                    if(!(yyvsp[-2].expr)->is_const()){
                        sprintf(cu_err, "Error, on line %d, array size must be constant.\n", (yylsp[-2]).lineno);
                        err_symbol_begin = (yylsp[-2]).begin;
                        err_symbol_end   = (yylsp[-2]).end;
                        delete_expr((yyvsp[-2].expr),(yyvsp[0].expr));
                        (yyval.expr)=NULL;
                        YYERROR;
                    }
                    if((yyvsp[-2].expr)->self_eval_int()<1){
                        sprintf(cu_err, "Error, on line %d, array size is not positive.\n", (yylsp[-2]).lineno);
                        err_symbol_begin = (yylsp[-2]).begin;
                        err_symbol_end   = (yylsp[-2]).end;
                        delete_expr((yyvsp[-2].expr),(yyvsp[0].expr));
                        (yyval.expr)=NULL;
                        YYERROR;
                    }

                    (yyval.expr) = new expr_t(&expr_t::array_dimension_node, (yyvsp[-2].expr), (yyvsp[0].expr));
                }
#line 3517 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 96:
#line 863 "cu.y" /* yacc.c:1678  */
    {
                   (yyval.expr) = new expr_t(&expr_t::array_dimension_node, (yyvsp[-2].expr), (yyvsp[0].expr));
                }
#line 3525 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 97:
#line 866 "cu.y" /* yacc.c:1678  */
    {(yyval.expr) = NULL;}
#line 3531 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 98:
#line 874 "cu.y" /* yacc.c:1678  */
    {(yyval.expr) = NULL;}
#line 3537 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 99:
#line 875 "cu.y" /* yacc.c:1678  */
    {(yyval.expr)=(yyvsp[0].expr);}
#line 3543 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 100:
#line 878 "cu.y" /* yacc.c:1678  */
    {(yyval.expr)=(yyvsp[-1].expr);}
#line 3549 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 101:
#line 879 "cu.y" /* yacc.c:1678  */
    {(yyval.expr)=(yyvsp[-1].expr);}
#line 3555 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 102:
#line 880 "cu.y" /* yacc.c:1678  */
    {(yyval.expr)=(yyvsp[-1].expr);}
#line 3561 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 103:
#line 881 "cu.y" /* yacc.c:1678  */
    {
                      (yyval.expr) = NULL;
                      if(decl_type==int_type){
                          YYBACKUP(K_int,yylval);
                      }else if(decl_type==flt_type){
                          YYBACKUP(K_float,yylval);
                      }else if(decl_type==str_type){
                          YYBACKUP(K_string,yylval);
                      }
                   }
#line 3587 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 104:
#line 902 "cu.y" /* yacc.c:1678  */
    {
                       if(!*cu_err){
                           sprintf(cu_err, "Error, on line %d, initial values to an array should be enclosed with {}.\n", (yylsp[-1]).lineno);
                       }
                       (yyval.expr) = NULL;
                       YYERROR;
                   }
#line 3599 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 105:
#line 909 "cu.y" /* yacc.c:1678  */
    {
                       if(!*cu_err){
                           sprintf(cu_err, "Error, on line %d, initial values to an array should be enclosed with {}.\n", (yylsp[-1]).lineno);
                       }
                       (yyval.expr) = NULL;
                       YYERROR;
                   }
#line 3611 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 106:
#line 916 "cu.y" /* yacc.c:1678  */
    {
                       if(!*cu_err){
                           sprintf(cu_err, "Error, on line %d, initial values to an array should be enclosed with {}.\n", (yylsp[-1]).lineno);
                       }
                       (yyval.expr) = NULL;
                       YYERROR;
                   }
#line 3623 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 107:
#line 925 "cu.y" /* yacc.c:1678  */
    {
              (yyval.expr) = new expr_t(&expr_t::array_init_list_node, (yyvsp[-2].expr), (yyvsp[0].expr));
          }
#line 3631 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 108:
#line 931 "cu.y" /* yacc.c:1678  */
    {
              (yyval.expr) = new expr_t(&expr_t::array_init_list_node, (yyvsp[-1].expr), (yyvsp[0].expr));
          }
#line 3639 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 109:
#line 934 "cu.y" /* yacc.c:1678  */
    {
              delete (yyvsp[0].expr);
              sprintf(cu_err, "Error, on line %d, expecting a int value, got a float value.\n", (yylsp[0]).lineno);
              (yyval.expr) = NULL;
              YYERROR;
          }
#line 3650 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 110:
#line 940 "cu.y" /* yacc.c:1678  */
    {
              delete (yyvsp[0].expr);
              sprintf(cu_err, "Error, on line %d, expecting a int value, got a string value.\n", (yylsp[0]).lineno);
              (yyval.expr) = NULL;
              YYERROR;
          }
#line 3661 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 111:
#line 947 "cu.y" /* yacc.c:1678  */
    {(yyval.expr)=(yyvsp[0].expr);}
#line 3667 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 112:
#line 948 "cu.y" /* yacc.c:1678  */
    {(yyval.expr)=NULL;}
#line 3673 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 113:
#line 949 "cu.y" /* yacc.c:1678  */
    {(yyval.expr)=NULL;}
#line 3679 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 114:
#line 950 "cu.y" /* yacc.c:1678  */
    {
                   sprintf(cu_err, "Error, on line %d, missing a value between ','.\n", (yylsp[-1]).lineno);
                   (yyval.expr) = NULL;
                   YYERROR;
               }
#line 3689 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 115:
#line 957 "cu.y" /* yacc.c:1678  */
    {
                  (yyval.expr) = new expr_t(&expr_t::array_init_list_node, (yyvsp[-2].expr), (yyvsp[0].expr));
              }
#line 3697 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 116:
#line 963 "cu.y" /* yacc.c:1678  */
    {
                  (yyval.expr) = new expr_t(&expr_t::array_init_list_node, (yyvsp[-1].expr), (yyvsp[0].expr));
              }
#line 3705 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 117:
#line 966 "cu.y" /* yacc.c:1678  */
    {
                  (yyval.expr) = new expr_t(&expr_t::array_init_list_node, flty((yyvsp[-1].expr)), (yyvsp[0].expr));
              }
#line 3713 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 118:
#line 969 "cu.y" /* yacc.c:1678  */
    {
                  delete (yyvsp[0].expr);
                  sprintf(cu_err, "Error, on line %d, expecting a float value, got a string value.\n", (yylsp[0]).lineno);
                  (yyval.expr) = NULL;
                  err_symbol_begin = (yylsp[0]).begin;
                  err_symbol_end   = (yylsp[0]).end;
                  printf("%d~%d\n", err_symbol_begin, err_symbol_end);
                  YYERROR;
              }
#line 3727 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 119:
#line 979 "cu.y" /* yacc.c:1678  */
    {(yyval.expr)=(yyvsp[0].expr);}
#line 3733 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 120:
#line 980 "cu.y" /* yacc.c:1678  */
    {(yyval.expr)=NULL;}
#line 3739 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 121:
#line 983 "cu.y" /* yacc.c:1678  */
    {
                  (yyval.expr) = new expr_t(&expr_t::array_init_list_node, (yyvsp[-2].expr), (yyvsp[0].expr));
              }
#line 3747 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 122:
#line 986 "cu.y" /* yacc.c:1678  */
    {
                  (yyval.expr) = new expr_t(&expr_t::array_init_list_node, (yyvsp[-1].expr), (yyvsp[0].expr));
              }
#line 3755 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 123:
#line 989 "cu.y" /* yacc.c:1678  */
    {
                  delete (yyvsp[0].expr);
                  sprintf(cu_err, "Error, on line %d, expecting a string value, got a int value.\n", (yylsp[0]).lineno);
                  (yyval.expr) = NULL;
                  YYERROR;
              }
#line 3766 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 124:
#line 995 "cu.y" /* yacc.c:1678  */
    {
                  delete (yyvsp[0].expr);
                  sprintf(cu_err, "Error, on line %d, expecting a string value, got a float value.\n", (yylsp[0]).lineno);
                  (yyval.expr) = NULL;
                  YYERROR;
              }
#line 3777 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 125:
#line 1002 "cu.y" /* yacc.c:1678  */
    {(yyval.expr)=(yyvsp[0].expr);}
#line 3783 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 126:
#line 1003 "cu.y" /* yacc.c:1678  */
    {(yyval.expr)=NULL;}
#line 3789 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 128:
#line 1007 "cu.y" /* yacc.c:1678  */
    {
              (yyval.expr) = new expr_t(&expr_t::comma_int, (yyvsp[-2].expr), (yyvsp[0].expr));
          }
#line 3797 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 129:
#line 1010 "cu.y" /* yacc.c:1678  */
    {
              sprintf(cu_err, "Sorry, on line %d, currently, ',' could not be placed between int and string expressions.\n", (yylsp[-1]).lineno);
              err_symbol_begin = (yylsp[-1]).begin;
              err_symbol_end   = (yylsp[0]).end;
              delete_expr((yyvsp[-2].expr), (yyvsp[0].expr)); 
              (yyval.expr)=NULL;
              YYERROR;
          }
#line 3810 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 130:
#line 1018 "cu.y" /* yacc.c:1678  */
    {
              sprintf(cu_err, "Sorry, on line %d, currently, ',' could not be placed between int and float expressions.\n", (yylsp[-1]).lineno);
              err_symbol_begin = (yylsp[-1]).begin;
              err_symbol_end   = (yylsp[0]).end;
              delete_expr((yyvsp[-2].expr), (yyvsp[0].expr)); 
              (yyval.expr)=NULL;
              YYERROR;
          }
#line 3823 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 132:
#line 1030 "cu.y" /* yacc.c:1678  */
    {
                        (yyval.expr) = new expr_t(&expr_t::expr_list_extra_comma_node, (yyvsp[-2].expr), (yyvsp[0].expr));
                      }
#line 3831 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 133:
#line 1033 "cu.y" /* yacc.c:1678  */
    {
                        (yyval.expr) = new expr_t(&expr_t::expr_list_extra_comma_node, (yyvsp[-1].expr), null_expr);
                      }
#line 3839 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 135:
#line 1039 "cu.y" /* yacc.c:1678  */
    { 
                 (yyval.expr) = new expr_t(&expr_t::assign_int, (yyvsp[-2].expr), (yyvsp[0].expr));
             }
#line 3847 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 136:
#line 1042 "cu.y" /* yacc.c:1678  */
    {
                 sprintf(cu_err, "Error, on line %d, assigning a float value to a int variable.\n", (yylsp[-1]).lineno);
                 err_symbol_begin = (yylsp[-2]).begin;
                 err_symbol_end   = (yylsp[0]).end;
                 delete_expr((yyvsp[-2].expr), (yyvsp[0].expr));
                 (yyval.expr) = NULL;
                 YYERROR;
             }
#line 3860 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 137:
#line 1050 "cu.y" /* yacc.c:1678  */
    {
                 sprintf(cu_err, "Error, on line %d, assigning a string value to a int variable.\n", (yylsp[-1]).lineno);
                 err_symbol_begin = (yylsp[-2]).begin;
                 err_symbol_end   = (yylsp[0]).end;
                 delete_expr((yyvsp[-2].expr), (yyvsp[0].expr));
                 (yyval.expr) = NULL;
                 YYERROR;
             }
#line 3873 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 138:
#line 1062 "cu.y" /* yacc.c:1678  */
    { (yyval.expr) = new expr_t(&expr_t::assign_add_int        , (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 3879 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 139:
#line 1063 "cu.y" /* yacc.c:1678  */
    { (yyval.expr) = new expr_t(&expr_t::assign_subtract_int   , (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 3885 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 140:
#line 1064 "cu.y" /* yacc.c:1678  */
    { (yyval.expr) = new expr_t(&expr_t::assign_multiply_int   , (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 3891 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 141:
#line 1065 "cu.y" /* yacc.c:1678  */
    { (yyval.expr) = new expr_t(&expr_t::assign_divide_int     , (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 3897 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 142:
#line 1066 "cu.y" /* yacc.c:1678  */
    { (yyval.expr) = new expr_t(&expr_t::assign_mod_int        , (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 3903 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 143:
#line 1067 "cu.y" /* yacc.c:1678  */
    { (yyval.expr) = new expr_t(&expr_t::assign_shift_left_int , (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 3909 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 144:
#line 1068 "cu.y" /* yacc.c:1678  */
    { (yyval.expr) = new expr_t(&expr_t::assign_shift_right_int, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 3915 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 145:
#line 1069 "cu.y" /* yacc.c:1678  */
    { (yyval.expr) = new expr_t(&expr_t::assign_and_int        , (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 3921 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 146:
#line 1070 "cu.y" /* yacc.c:1678  */
    { (yyval.expr) = new expr_t(&expr_t::assign_xor_int        , (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 3927 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 147:
#line 1071 "cu.y" /* yacc.c:1678  */
    { (yyval.expr) = new expr_t(&expr_t::assign_or_int         , (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 3933 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 148:
#line 1087 "cu.y" /* yacc.c:1678  */
    { (yyval.expr) = (yyvsp[-1].expr); }
#line 3939 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 149:
#line 1088 "cu.y" /* yacc.c:1678  */
    { (yyval.expr) = new expr_t(&expr_t::or_int , (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 3945 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 150:
#line 1089 "cu.y" /* yacc.c:1678  */
    { (yyval.expr) = new expr_t(&expr_t::xor_int, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 3951 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 151:
#line 1090 "cu.y" /* yacc.c:1678  */
    { (yyval.expr) = new expr_t(&expr_t::and_int, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 3957 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 152:
#line 1092 "cu.y" /* yacc.c:1678  */
    { (yyval.expr) = new expr_t(&expr_t::eq_int , (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 3963 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 153:
#line 1093 "cu.y" /* yacc.c:1678  */
    { (yyval.expr) = new expr_t(&expr_t::ne_int , (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 3969 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 154:
#line 1094 "cu.y" /* yacc.c:1678  */
    { (yyval.expr) = new expr_t(&expr_t::lt_int , (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 3975 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 155:
#line 1095 "cu.y" /* yacc.c:1678  */
    { (yyval.expr) = new expr_t(&expr_t::gt_int , (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 3981 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 156:
#line 1096 "cu.y" /* yacc.c:1678  */
    { (yyval.expr) = new expr_t(&expr_t::le_int , (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 3987 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 157:
#line 1097 "cu.y" /* yacc.c:1678  */
    { (yyval.expr) = new expr_t(&expr_t::ge_int , (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 3993 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 158:
#line 1098 "cu.y" /* yacc.c:1678  */
    { (yyval.expr) = new expr_t(&expr_t::shift_left_int , (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 3999 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 159:
#line 1099 "cu.y" /* yacc.c:1678  */
    { (yyval.expr) = new expr_t(&expr_t::shift_right_int, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 4005 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 160:
#line 1102 "cu.y" /* yacc.c:1678  */
    { (yyval.expr) = new expr_t(&expr_t::flt_eq, flty((yyvsp[-2].expr)), (yyvsp[0].expr));}
#line 4011 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 161:
#line 1103 "cu.y" /* yacc.c:1678  */
    { (yyval.expr) = new expr_t(&expr_t::flt_ne, flty((yyvsp[-2].expr)), (yyvsp[0].expr));}
#line 4017 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 162:
#line 1104 "cu.y" /* yacc.c:1678  */
    { (yyval.expr) = new expr_t(&expr_t::flt_eq, (yyvsp[-2].expr), flty((yyvsp[0].expr)));}
#line 4023 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 163:
#line 1105 "cu.y" /* yacc.c:1678  */
    { (yyval.expr) = new expr_t(&expr_t::flt_ne, (yyvsp[-2].expr), flty((yyvsp[0].expr)));}
#line 4029 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 164:
#line 1106 "cu.y" /* yacc.c:1678  */
    { (yyval.expr) = new expr_t(&expr_t::flt_eq, ((yyvsp[-2].expr)), (yyvsp[0].expr));}
#line 4035 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 165:
#line 1107 "cu.y" /* yacc.c:1678  */
    { (yyval.expr) = new expr_t(&expr_t::flt_ne, ((yyvsp[-2].expr)), (yyvsp[0].expr));}
#line 4041 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 166:
#line 1109 "cu.y" /* yacc.c:1678  */
    { (yyval.expr) = new expr_t(&expr_t::flt_lt , flty((yyvsp[-2].expr)), (yyvsp[0].expr));}
#line 4047 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 167:
#line 1110 "cu.y" /* yacc.c:1678  */
    { (yyval.expr) = new expr_t(&expr_t::flt_gt , flty((yyvsp[-2].expr)), (yyvsp[0].expr));}
#line 4053 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 168:
#line 1111 "cu.y" /* yacc.c:1678  */
    { (yyval.expr) = new expr_t(&expr_t::flt_lt , (yyvsp[-2].expr), flty((yyvsp[0].expr)));}
#line 4059 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 169:
#line 1112 "cu.y" /* yacc.c:1678  */
    { (yyval.expr) = new expr_t(&expr_t::flt_gt , (yyvsp[-2].expr), flty((yyvsp[0].expr)));}
#line 4065 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 170:
#line 1113 "cu.y" /* yacc.c:1678  */
    { (yyval.expr) = new expr_t(&expr_t::flt_lt , (yyvsp[-2].expr), ((yyvsp[0].expr)));}
#line 4071 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 171:
#line 1114 "cu.y" /* yacc.c:1678  */
    { (yyval.expr) = new expr_t(&expr_t::flt_gt , (yyvsp[-2].expr), ((yyvsp[0].expr)));}
#line 4077 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 172:
#line 1116 "cu.y" /* yacc.c:1678  */
    { (yyval.expr) = new expr_t(&expr_t::flt_le, flty((yyvsp[-2].expr)), (yyvsp[0].expr));}
#line 4083 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 173:
#line 1117 "cu.y" /* yacc.c:1678  */
    { (yyval.expr) = new expr_t(&expr_t::flt_ge, flty((yyvsp[-2].expr)), (yyvsp[0].expr));}
#line 4089 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 174:
#line 1118 "cu.y" /* yacc.c:1678  */
    { (yyval.expr) = new expr_t(&expr_t::flt_le, (yyvsp[-2].expr), flty((yyvsp[0].expr)));}
#line 4095 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 175:
#line 1119 "cu.y" /* yacc.c:1678  */
    { (yyval.expr) = new expr_t(&expr_t::flt_ge, (yyvsp[-2].expr), flty((yyvsp[0].expr)));}
#line 4101 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 176:
#line 1120 "cu.y" /* yacc.c:1678  */
    { (yyval.expr) = new expr_t(&expr_t::flt_le, ((yyvsp[-2].expr)), (yyvsp[0].expr));}
#line 4107 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 177:
#line 1121 "cu.y" /* yacc.c:1678  */
    { (yyval.expr) = new expr_t(&expr_t::flt_ge, ((yyvsp[-2].expr)), (yyvsp[0].expr));}
#line 4113 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 178:
#line 1123 "cu.y" /* yacc.c:1678  */
    { (yyval.expr) = new expr_t(&expr_t::add_int     , (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 4119 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 179:
#line 1124 "cu.y" /* yacc.c:1678  */
    { (yyval.expr) = new expr_t(&expr_t::subtract_int, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 4125 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 180:
#line 1125 "cu.y" /* yacc.c:1678  */
    { (yyval.expr) = new expr_t(&expr_t::multiply_int, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 4131 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 181:
#line 1126 "cu.y" /* yacc.c:1678  */
    { (yyval.expr) = new expr_t(&expr_t::divide_int  , (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 4137 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 182:
#line 1127 "cu.y" /* yacc.c:1678  */
    { (yyval.expr) = new expr_t(&expr_t::mod_int     , (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 4143 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 183:
#line 1128 "cu.y" /* yacc.c:1678  */
    {
        (yyval.expr) = new expr_t(&expr_t::if_then_else_int, (yyvsp[-4].expr), new expr_t(&expr_t::ite_node, (yyvsp[-2].expr), (yyvsp[0].expr)));
     }
#line 4151 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 184:
#line 1131 "cu.y" /* yacc.c:1678  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 4157 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 185:
#line 1132 "cu.y" /* yacc.c:1678  */
    { (yyval.expr) = new expr_t(&expr_t::minus_int      ,NULL, (yyvsp[0].expr)); }
#line 4163 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 186:
#line 1133 "cu.y" /* yacc.c:1678  */
    { (yyval.expr) = new expr_t(&expr_t::logic_not_int  ,NULL, (yyvsp[0].expr)); }
#line 4169 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 187:
#line 1134 "cu.y" /* yacc.c:1678  */
    { (yyval.expr) = new expr_t(&expr_t::not_int        ,NULL, (yyvsp[0].expr)); }
#line 4175 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 188:
#line 1135 "cu.y" /* yacc.c:1678  */
    { (yyval.expr) = new expr_t(&expr_t::logic_and_int, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 4181 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 189:
#line 1136 "cu.y" /* yacc.c:1678  */
    { (yyval.expr) = new expr_t(&expr_t::logic_or_int , (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 4187 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 191:
#line 1138 "cu.y" /* yacc.c:1678  */
    {(yyval.expr) = new expr_t(&expr_t::pre_incr_int, NULL, (yyvsp[0].expr));}
#line 4193 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 192:
#line 1139 "cu.y" /* yacc.c:1678  */
    {(yyval.expr) = new expr_t(&expr_t::pre_decr_int, NULL, (yyvsp[0].expr));}
#line 4199 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 193:
#line 1140 "cu.y" /* yacc.c:1678  */
    {(yyval.expr) = new expr_t(&expr_t::post_incr_int, (yyvsp[-1].expr), null_expr);}
#line 4205 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 194:
#line 1141 "cu.y" /* yacc.c:1678  */
    {(yyval.expr) = new expr_t(&expr_t::post_decr_int, (yyvsp[-1].expr), null_expr);}
#line 4211 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 196:
#line 1144 "cu.y" /* yacc.c:1678  */
    { (yyval.expr) = new expr_t(&expr_t::regexec_int, (yyvsp[-2].expr), (yyvsp[0].regx)); }
#line 4217 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 197:
#line 1146 "cu.y" /* yacc.c:1678  */
    { (yyval.expr) = new expr_t(&expr_t::strlen_int, NULL, (yyvsp[-1].expr)); }
#line 4223 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 198:
#line 1147 "cu.y" /* yacc.c:1678  */
    { (yyval.expr) = new expr_t(&expr_t::atoi_int  , NULL, (yyvsp[-1].expr)); }
#line 4229 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 199:
#line 1148 "cu.y" /* yacc.c:1678  */
    { (yyval.expr) = new expr_t(&expr_t::strref_int, NULL, (yyvsp[-1].expr)); }
#line 4235 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 200:
#line 1150 "cu.y" /* yacc.c:1678  */
    { (yyval.expr) = new expr_t(&expr_t::char_at_int,  (yyvsp[0].expr), new expr_t(0)); }
#line 4241 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 201:
#line 1152 "cu.y" /* yacc.c:1678  */
    { (yyval.expr) = new expr_t(&expr_t::char_at_int,  (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 4247 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 202:
#line 1153 "cu.y" /* yacc.c:1678  */
    { (yyval.expr) = inty((yyvsp[0].expr)); }
#line 4253 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 203:
#line 1154 "cu.y" /* yacc.c:1678  */
    { (yyval.expr) = inty((yyvsp[0].expr)); }
#line 4259 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 204:
#line 1155 "cu.y" /* yacc.c:1678  */
    { (yyval.expr) = inty((yyvsp[-1].expr)); }
#line 4265 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 205:
#line 1156 "cu.y" /* yacc.c:1678  */
    { (yyval.expr) = inty((yyvsp[-1].expr)); }
#line 4271 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 206:
#line 1157 "cu.y" /* yacc.c:1678  */
    { (yyval.expr) = new expr_t(&expr_t::atoi_int  , NULL, (yyvsp[-1].expr)); }
#line 4277 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 207:
#line 1158 "cu.y" /* yacc.c:1678  */
    { (yyval.expr) = new expr_t(&expr_t::strcmp_int,  (yyvsp[-3].expr), (yyvsp[-1].expr)); }
#line 4283 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 208:
#line 1159 "cu.y" /* yacc.c:1678  */
    {
         expr_t* args = verify_function_call((yyvsp[-3].func), (yyvsp[-1].expr));
         if(args==&call_err) YYERROR;
         (yyval.expr) = new expr_t(&expr_t::function_call_int, (yyvsp[-3].func), args, NULL);
     }
#line 4293 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 209:
#line 1164 "cu.y" /* yacc.c:1678  */
    {
         (yyval.expr) = NULL;
         YYERROR;
     }
#line 4302 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 210:
#line 1168 "cu.y" /* yacc.c:1678  */
    {
         snprintf(cu_err, sizeof(cu_err), "Error, on line %d, %s is a function\n", (yylsp[-1]).lineno, (yyvsp[-1].func)->name->c_str());
         (yyval.expr) = NULL;
         err_symbol_begin = (yylsp[-1]).begin;
         err_symbol_end   = (yylsp[-1]).end;
         YYERROR;
     }
#line 4314 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 211:
#line 1175 "cu.y" /* yacc.c:1678  */
    { (yyval.expr) = new expr_t((yyvsp[-1].expr)->sym->a[0].dims[0]); delete_expr((yyvsp[-1].expr)); }
#line 4320 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 212:
#line 1176 "cu.y" /* yacc.c:1678  */
    { 
         sprintf(cu_err, "Error, on line %d, sizeof only applies on array.\n", (yylsp[-2]).lineno);
         (yyval.expr) = NULL;
         YYERROR;
     }
#line 4330 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 216:
#line 1204 "cu.y" /* yacc.c:1678  */
    {
        (yyval.expr) = (yyvsp[0].expr);
        (yyval.expr)->int_adr = &((yyval.expr)->sym->scalar_int);
     }
#line 4348 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 217:
#line 1217 "cu.y" /* yacc.c:1678  */
    {
         if(verify_indexing_dimention((yyvsp[-1].expr),(yyvsp[0].expr),(yylsp[-1]).lineno)){
             delete_expr((yyvsp[-1].expr), (yyvsp[0].expr));
             err_symbol_begin = (yylsp[-1]).begin;
             err_symbol_end   = (yylsp[-1]).end;
             YYERROR;
         }
         (yyvsp[-1].expr)->lhs = (yyvsp[0].expr);
         (yyvsp[-1].expr)->flag.free_lhs = 1;
         (yyval.expr) = new expr_t(&expr_t::array_addressing_int, NULL, (yyvsp[-1].expr));
     }
#line 4367 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 218:
#line 1233 "cu.y" /* yacc.c:1678  */
    {
                   (yyval.expr) = new expr_t(&expr_t::array_subscript_node, (yyvsp[-2].expr), (yyvsp[0].expr));
                   (yyval.expr)->flag.uvwx = ((yyvsp[0].expr)?(yyvsp[0].expr)->flag.uvwx+1:1);
               }
#line 4376 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 219:
#line 1237 "cu.y" /* yacc.c:1678  */
    {(yyval.expr) = NULL;}
#line 4382 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 222:
#line 1242 "cu.y" /* yacc.c:1678  */
    {
             (yyval.expr) = new expr_t(&expr_t::assign_flt, (yyvsp[-2].expr), (yyvsp[0].expr));
         }
#line 4390 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 223:
#line 1245 "cu.y" /* yacc.c:1678  */
    {
             (yyvsp[0].expr) = new expr_t(&expr_t::int_to_flt, null_expr, (yyvsp[0].expr)); 
             (yyval.expr) = new expr_t(&expr_t::assign_flt, (yyvsp[-2].expr), (yyvsp[0].expr));
         }
#line 4399 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 224:
#line 1249 "cu.y" /* yacc.c:1678  */
    {
             sprintf(cu_err, "Error, on line %d, assigning a string value to a float variable.\n", (yylsp[-1]).lineno);
             err_symbol_begin = (yylsp[-2]).begin;
             err_symbol_end   = (yylsp[0]).end;
             delete_expr((yyvsp[-2].expr), (yyvsp[0].expr));
             (yyval.expr) = NULL;
             YYERROR;
         }
#line 4412 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 225:
#line 1257 "cu.y" /* yacc.c:1678  */
    { (yyval.expr) = new expr_t(&expr_t::assign_add_flt        , (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 4418 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 226:
#line 1258 "cu.y" /* yacc.c:1678  */
    { (yyval.expr) = new expr_t(&expr_t::assign_subtract_flt   , (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 4424 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 227:
#line 1259 "cu.y" /* yacc.c:1678  */
    { (yyval.expr) = new expr_t(&expr_t::assign_multiply_flt   , (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 4430 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 228:
#line 1260 "cu.y" /* yacc.c:1678  */
    { (yyval.expr) = new expr_t(&expr_t::assign_divide_flt     , (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 4436 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 229:
#line 1261 "cu.y" /* yacc.c:1678  */
    { (yyval.expr) = new expr_t(&expr_t::assign_add_flt        , (yyvsp[-2].expr), flty((yyvsp[0].expr))); }
#line 4442 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 230:
#line 1262 "cu.y" /* yacc.c:1678  */
    { (yyval.expr) = new expr_t(&expr_t::assign_subtract_flt   , (yyvsp[-2].expr), flty((yyvsp[0].expr))); }
#line 4448 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 231:
#line 1263 "cu.y" /* yacc.c:1678  */
    { (yyval.expr) = new expr_t(&expr_t::assign_multiply_flt   , (yyvsp[-2].expr), flty((yyvsp[0].expr))); }
#line 4454 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 232:
#line 1264 "cu.y" /* yacc.c:1678  */
    { (yyval.expr) = new expr_t(&expr_t::assign_divide_flt     , (yyvsp[-2].expr), flty((yyvsp[0].expr))); }
#line 4460 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 233:
#line 1266 "cu.y" /* yacc.c:1678  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 4466 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 234:
#line 1267 "cu.y" /* yacc.c:1678  */
    { (yyval.expr) = new expr_t(&expr_t::minus_flt   ,NULL,(yyvsp[0].expr)); }
#line 4472 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 235:
#line 1268 "cu.y" /* yacc.c:1678  */
    { (yyval.expr) = new expr_t(&expr_t::add_flt     , (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 4478 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 236:
#line 1269 "cu.y" /* yacc.c:1678  */
    { (yyval.expr) = new expr_t(&expr_t::subtract_flt, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 4484 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 237:
#line 1270 "cu.y" /* yacc.c:1678  */
    { (yyval.expr) = new expr_t(&expr_t::multiply_flt, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 4490 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 238:
#line 1271 "cu.y" /* yacc.c:1678  */
    { (yyval.expr) = new expr_t(&expr_t::divide_flt  , (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 4496 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 239:
#line 1272 "cu.y" /* yacc.c:1678  */
    { (yyval.expr) = new expr_t(&expr_t::add_flt     , flty((yyvsp[-2].expr)), (yyvsp[0].expr)); }
#line 4502 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 240:
#line 1273 "cu.y" /* yacc.c:1678  */
    { (yyval.expr) = new expr_t(&expr_t::subtract_flt, flty((yyvsp[-2].expr)), (yyvsp[0].expr)); }
#line 4508 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 241:
#line 1274 "cu.y" /* yacc.c:1678  */
    { (yyval.expr) = new expr_t(&expr_t::multiply_flt, flty((yyvsp[-2].expr)), (yyvsp[0].expr)); }
#line 4514 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 242:
#line 1275 "cu.y" /* yacc.c:1678  */
    { (yyval.expr) = new expr_t(&expr_t::divide_flt  , flty((yyvsp[-2].expr)), (yyvsp[0].expr)); }
#line 4520 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 243:
#line 1276 "cu.y" /* yacc.c:1678  */
    { (yyval.expr) = new expr_t(&expr_t::add_flt     , (yyvsp[-2].expr), flty((yyvsp[0].expr))); }
#line 4526 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 244:
#line 1277 "cu.y" /* yacc.c:1678  */
    { (yyval.expr) = new expr_t(&expr_t::subtract_flt, (yyvsp[-2].expr), flty((yyvsp[0].expr))); }
#line 4532 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 245:
#line 1278 "cu.y" /* yacc.c:1678  */
    { (yyval.expr) = new expr_t(&expr_t::multiply_flt, (yyvsp[-2].expr), flty((yyvsp[0].expr))); }
#line 4538 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 246:
#line 1279 "cu.y" /* yacc.c:1678  */
    { (yyval.expr) = new expr_t(&expr_t::divide_flt  , (yyvsp[-2].expr), flty((yyvsp[0].expr))); }
#line 4544 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 247:
#line 1280 "cu.y" /* yacc.c:1678  */
    { (yyval.expr) = flty((yyvsp[0].expr)); }
#line 4550 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 248:
#line 1281 "cu.y" /* yacc.c:1678  */
    { (yyval.expr) = flty((yyvsp[0].expr)); }
#line 4556 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 249:
#line 1282 "cu.y" /* yacc.c:1678  */
    { (yyval.expr) = flty((yyvsp[-1].expr)); }
#line 4562 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 250:
#line 1283 "cu.y" /* yacc.c:1678  */
    { (yyval.expr) = flty((yyvsp[-1].expr)); }
#line 4568 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 251:
#line 1284 "cu.y" /* yacc.c:1678  */
    { (yyval.expr) = new expr_t(&expr_t::atof_flt, NULL, (yyvsp[-1].expr)); }
#line 4574 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 252:
#line 1285 "cu.y" /* yacc.c:1678  */
    {
             expr_t* args = verify_function_call((yyvsp[-3].func), (yyvsp[-1].expr));
             if(args==&call_err) YYERROR;
             (yyval.expr) = new expr_t(&expr_t::function_call_flt, (yyvsp[-3].func), args, NULL);
         }
#line 4584 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 253:
#line 1290 "cu.y" /* yacc.c:1678  */
    {
             (yyval.expr) = NULL;
             YYERROR;
         }
#line 4593 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 254:
#line 1294 "cu.y" /* yacc.c:1678  */
    {
             snprintf(cu_err, sizeof(cu_err), "Error, on line %d, %s is a function\n", (yylsp[-1]).lineno, (yyvsp[-1].func)->name->c_str());
             (yyval.expr) = NULL;
             err_symbol_begin = (yylsp[-1]).begin;
             err_symbol_end   = (yylsp[-1]).end;
             YYERROR;
         }
#line 4605 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 255:
#line 1302 "cu.y" /* yacc.c:1678  */
    {
             (yyval.expr) = (yyvsp[0].expr);
             (yyval.expr)->flt_adr = &((yyval.expr)->sym->scalar_flt);
         }
#line 4614 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 256:
#line 1306 "cu.y" /* yacc.c:1678  */
    {
             if(verify_indexing_dimention((yyvsp[-1].expr),(yyvsp[0].expr),(yylsp[-1]).lineno)){
                 delete_expr((yyvsp[-1].expr), (yyvsp[0].expr));
                 err_symbol_begin = (yylsp[-1]).begin;
                 err_symbol_end   = (yylsp[-1]).end;
                 YYERROR;
             }
             (yyvsp[-1].expr)->lhs = (yyvsp[0].expr);
             (yyvsp[-1].expr)->flag.free_lhs = 1;
             (yyval.expr) = new expr_t(&expr_t::array_addressing_flt, NULL, (yyvsp[-1].expr));
         }
#line 4630 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 259:
#line 1321 "cu.y" /* yacc.c:1678  */
    {
             (yyval.expr) = new expr_t(&expr_t::assign_str, (yyvsp[-2].expr), (yyvsp[0].expr));
         }
#line 4638 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 260:
#line 1324 "cu.y" /* yacc.c:1678  */
    {
             sprintf(cu_err, "Error, on line %d, assigning a int value to a string variable.\n", (yylsp[-1]).lineno);
             err_symbol_begin = (yylsp[-2]).begin;
             err_symbol_end   = (yylsp[0]).end;
             delete_expr((yyvsp[-2].expr), (yyvsp[0].expr));
             (yyval.expr) = NULL;
             YYERROR;
         }
#line 4651 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 261:
#line 1332 "cu.y" /* yacc.c:1678  */
    {
             sprintf(cu_err, "Error, on line %d, assigning a float value to a string variable.\n", (yylsp[-1]).lineno);
             err_symbol_begin = (yylsp[-2]).begin;
             err_symbol_end   = (yylsp[0]).end;
             delete_expr((yyvsp[-2].expr), (yyvsp[0].expr));
             (yyval.expr) = NULL;
             YYERROR;
         }
#line 4664 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 262:
#line 1340 "cu.y" /* yacc.c:1678  */
    {(yyval.expr) = (yyvsp[-1].expr);}
#line 4670 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 263:
#line 1341 "cu.y" /* yacc.c:1678  */
    {
             (yyval.expr) = new expr_t(&expr_t::if_then_else_str, (yyvsp[-4].expr), new expr_t(&expr_t::ite_node, (yyvsp[-2].expr), (yyvsp[0].expr)));
         }
#line 4678 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 264:
#line 1344 "cu.y" /* yacc.c:1678  */
    {
             (yyval.expr) = new expr_t(&expr_t::get_line_str, NULL, (yyvsp[0].expr));
         }
#line 4686 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 265:
#line 1350 "cu.y" /* yacc.c:1678  */
    {(yyval.expr) = new expr_t(&expr_t::concatenate_str, (yyvsp[-2].expr), (yyvsp[0].expr));}
#line 4692 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 266:
#line 1351 "cu.y" /* yacc.c:1678  */
    {(yyval.expr) = new expr_t(&expr_t::shift_str, (yyvsp[-2].expr), (yyvsp[0].expr));}
#line 4698 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 267:
#line 1352 "cu.y" /* yacc.c:1678  */
    {(yyval.expr) = new expr_t(&expr_t::int_to_str, null_expr, (yyvsp[-1].expr));}
#line 4704 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 268:
#line 1353 "cu.y" /* yacc.c:1678  */
    {(yyval.expr) = new expr_t(&expr_t::flt_to_str, null_expr, (yyvsp[-1].expr));}
#line 4710 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 269:
#line 1354 "cu.y" /* yacc.c:1678  */
    {(yyval.expr) = new expr_t(&expr_t::strstr_str , (yyvsp[-3].expr), (yyvsp[-1].expr));}
#line 4716 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 270:
#line 1355 "cu.y" /* yacc.c:1678  */
    {(yyval.expr) = new expr_t(&expr_t::strpbrk_str, (yyvsp[-3].expr), (yyvsp[-1].expr));}
#line 4722 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 271:
#line 1356 "cu.y" /* yacc.c:1678  */
    {(yyval.expr) = new expr_t(&expr_t::strchr_str , (yyvsp[-3].expr), (yyvsp[-1].expr));}
#line 4728 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 272:
#line 1357 "cu.y" /* yacc.c:1678  */
    {(yyval.expr) = new expr_t(&expr_t::strrchr_str, (yyvsp[-3].expr), (yyvsp[-1].expr));}
#line 4734 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 273:
#line 1358 "cu.y" /* yacc.c:1678  */
    {
             expr_t* args = verify_function_call((yyvsp[-3].func), (yyvsp[-1].expr));
             if(args==&call_err) YYERROR;
             (yyval.expr) = new expr_t(&expr_t::function_call_str, (yyvsp[-3].func), args, NULL);
         }
#line 4744 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 274:
#line 1363 "cu.y" /* yacc.c:1678  */
    {
             (yyval.expr) = NULL;
             YYERROR;
         }
#line 4753 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 275:
#line 1367 "cu.y" /* yacc.c:1678  */
    {
             snprintf(cu_err, sizeof(cu_err), "Error, on line %d, %s is a function\n", (yylsp[-1]).lineno, (yyvsp[-1].func)->name->c_str());
             (yyval.expr) = NULL;
             err_symbol_begin = (yylsp[-1]).begin;
             err_symbol_end   = (yylsp[-1]).end;
             YYERROR;
         }
#line 4765 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 276:
#line 1376 "cu.y" /* yacc.c:1678  */
    {
             (yyval.expr) = (yyvsp[0].expr);
             (yyval.expr)->str_adr = (str_t*)&((yyval.expr)->sym->scalar_str[0]);
         }
#line 4774 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 277:
#line 1380 "cu.y" /* yacc.c:1678  */
    {
             if(verify_indexing_dimention((yyvsp[-1].expr),(yyvsp[0].expr),(yylsp[-1]).lineno)){
                 delete_expr((yyvsp[-1].expr), (yyvsp[0].expr));
                 err_symbol_begin = (yylsp[-1]).begin;
                 err_symbol_end   = (yylsp[-1]).end;
                 YYERROR;
             }
             (yyvsp[-1].expr)->lhs = (yyvsp[0].expr);
             (yyvsp[-1].expr)->flag.free_lhs = 1;
             (yyval.expr) = new expr_t(&expr_t::array_addressing_str, NULL, (yyvsp[-1].expr));
         }
#line 4790 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 278:
#line 1393 "cu.y" /* yacc.c:1678  */
    {
                 (yyval.expr) = new expr_t(&expr_t::vargs_str_node, (yyvsp[-1].expr), (yyvsp[0].expr));
             }
#line 4798 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 279:
#line 1396 "cu.y" /* yacc.c:1678  */
    {
                 (yyval.expr) = new expr_t(&expr_t::vargs_int_node, (yyvsp[-1].expr), (yyvsp[0].expr));
             }
#line 4806 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 280:
#line 1399 "cu.y" /* yacc.c:1678  */
    {
                 (yyval.expr) = new expr_t(&expr_t::vargs_flt_node, (yyvsp[-1].expr), (yyvsp[0].expr));
             }
#line 4814 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 281:
#line 1402 "cu.y" /* yacc.c:1678  */
    {(yyval.expr) = NULL;}
#line 4820 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 282:
#line 1403 "cu.y" /* yacc.c:1678  */
    {(yyval.expr) = NULL;}
#line 4826 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 283:
#line 1407 "cu.y" /* yacc.c:1678  */
    { register_function(func_name); }
#line 4832 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 284:
#line 1408 "cu.y" /* yacc.c:1678  */
    {
                    if(!(yyvsp[-4].expr)) register_function(func_name, func_type, (yyvsp[-1].expr));
                  }
#line 4842 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 285:
#line 1413 "cu.y" /* yacc.c:1678  */
    {
                  cu_pop_scope();
                  if((yyvsp[-8].expr)){
                      snprintf(cu_err, sizeof(cu_err), "Error, on line %d, name `%s' already used.\n", (yylsp[-8]).lineno, (yyvsp[-8].expr)->nam);
                      delete_expr((yyvsp[-8].expr), (yyvsp[-5].expr));
                      delete (yyvsp[-1].stat);
                      YYERROR;
                  }
                  register_function(func_name, func_type, (yyvsp[-5].expr), (yyvsp[-1].stat)); 
                  func_type = bad_type;
              }
#line 4858 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 286:
#line 1425 "cu.y" /* yacc.c:1678  */
    {
                  if((yyvsp[-2].expr)){ delete_expr((yyvsp[-2].expr));}
                     in_function_decl->parameters = NULL;
                  if(in_function_decl->statements) delete in_function_decl->statements;
                  in_function_decl = NULL;
                  cu_pop_scope();
                  YYERROR;
              }
#line 4875 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 287:
#line 1439 "cu.y" /* yacc.c:1678  */
    {func_name.swap(symbol_name); (yyval.expr)=NULL; func_type=decl_type;}
#line 4881 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 295:
#line 1449 "cu.y" /* yacc.c:1678  */
    {(yyval.expr)=NULL;}
#line 4887 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 296:
#line 1457 "cu.y" /* yacc.c:1678  */
    {(yyval.expr)=new expr_t(&expr_t::parameter_list_node,(yyvsp[-1].expr),(yyvsp[0].expr));}
#line 4893 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 297:
#line 1459 "cu.y" /* yacc.c:1678  */
    {symbol_backup.swap(symbol_name);}
#line 4899 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 298:
#line 1459 "cu.y" /* yacc.c:1678  */
    {
               if(decl_type==bad_type){
                   sprintf(cu_err, "Error, on line %d, parameter type unspecified.\n", (yylsp[-2]).lineno);
                   err_symbol_begin = (yylsp[-2]).begin;
                   err_symbol_end   = (yylsp[-2]).end;
                   delete_expr((yyvsp[-2].expr), (yyvsp[0].expr));
                   (yyval.expr)=NULL;
                   YYERROR;
               }
               if((yyvsp[-2].expr)) delete (yyvsp[-2].expr);
               if((yyvsp[0].expr)){
                   delete (yyvsp[0].expr);
                   sprintf(cu_err, "Sorry, on line %d, array as a parameter is not supported yet.\n", (yylsp[-3]).lineno);
                   YYERROR;
               }
               (yyval.expr) = push_var(symbol_backup, (yylsp[-2]).lineno, NULL, decl_type);
               if(!(yyval.expr)){
                  snprintf(cu_err, sizeof(cu_err), "Error, on line %d, parameter name `%s' already used.\n", (yylsp[-2]).lineno, symbol_name.c_str());
                  YYERROR;
               }
               switch(decl_type){
                 case int_type: (yyval.expr)->int_adr = &((yyval.expr)->sym->scalar_int); break;
                 case flt_type: (yyval.expr)->flt_adr = &((yyval.expr)->sym->scalar_flt); break;
                 case str_type: (yyval.expr)->str_adr = (str_t*)&((yyval.expr)->sym->scalar_str[0]); break;
               }
           }
#line 4932 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 299:
#line 1487 "cu.y" /* yacc.c:1678  */
    {
               sprintf(cu_err, "Error, on line %d, parameter name unspecified.\n", (yylsp[0]).lineno);
               err_symbol_begin = (yylsp[0]).begin;
               err_symbol_end   = (yylsp[0]).end;
               (yyval.expr)=NULL;
               YYERROR;
           }
#line 4944 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 300:
#line 1494 "cu.y" /* yacc.c:1678  */
    {
               sprintf(cu_err, "Error, on line %d, parameter type unspecified.\n", (yylsp[0]).lineno);
               err_symbol_begin = (yylsp[0]).begin;
               err_symbol_end   = (yylsp[0]).end;
               delete_expr((yyvsp[0].expr));
               (yyval.expr)=NULL;
               YYERROR;
           }
#line 4957 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 301:
#line 1503 "cu.y" /* yacc.c:1678  */
    {(yyval.expr)=NULL;}
#line 4963 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 308:
#line 1510 "cu.y" /* yacc.c:1678  */
    {(yyval.expr)=NULL;}
#line 4969 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 309:
#line 1511 "cu.y" /* yacc.c:1678  */
    {(yyval.expr)=NULL;}
#line 4975 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 310:
#line 1512 "cu.y" /* yacc.c:1678  */
    {(yyval.expr)=NULL;}
#line 4981 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 311:
#line 1513 "cu.y" /* yacc.c:1678  */
    {(yyval.expr)=NULL;}
#line 4987 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 312:
#line 1516 "cu.y" /* yacc.c:1678  */
    {(yyval.expr)=(yyvsp[0].expr);}
#line 4993 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 313:
#line 1517 "cu.y" /* yacc.c:1678  */
    {(yyval.expr)=NULL;}
#line 4999 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 318:
#line 1528 "cu.y" /* yacc.c:1678  */
    {(yyval.stat)=new statement_t();}
#line 5005 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 319:
#line 1531 "cu.y" /* yacc.c:1678  */
    {(yyval.expr)=NULL;}
#line 5011 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 320:
#line 1532 "cu.y" /* yacc.c:1678  */
    {(yyval.expr)=(yyvsp[0].expr);}
#line 5017 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 321:
#line 1561 "cu.y" /* yacc.c:1678  */
    {(yyval.expr)=new expr_t(&expr_t::argument_list_node, NULL, (yyvsp[0].expr));}
#line 5023 "cu.tab.c" /* yacc.c:1678  */
    break;

  case 322:
#line 1562 "cu.y" /* yacc.c:1678  */
    {(yyval.expr)=new expr_t(&expr_t::argument_list_node,   (yyvsp[-2].expr), (yyvsp[0].expr));}
#line 5029 "cu.tab.c" /* yacc.c:1678  */
    break;


#line 5033 "cu.tab.c" /* yacc.c:1678  */
      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;
  *++yylsp = yyloc;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYEMPTY : YYTRANSLATE (yychar);

  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
# define YYSYNTAX_ERROR yysyntax_error (&yymsg_alloc, &yymsg, \
                                        yyssp, yytoken)
      {
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = YYSYNTAX_ERROR;
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == 1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = (char *) YYSTACK_ALLOC (yymsg_alloc);
            if (!yymsg)
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = 2;
              }
            else
              {
                yysyntax_error_status = YYSYNTAX_ERROR;
                yymsgp = yymsg;
              }
          }
        yyerror (yymsgp);
        if (yysyntax_error_status == 2)
          goto yyexhaustedlab;
      }
# undef YYSYNTAX_ERROR
#endif
    }

  yyerror_range[1] = yylloc;

  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == YYEOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval, &yylloc);
          yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  yyerror_range[1] = yylsp[1-yylen];
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYTERROR;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;

      yyerror_range[1] = *yylsp;
      yydestruct ("Error: popping",
                  yystos[yystate], yyvsp, yylsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  yyerror_range[2] = yylloc;
  /* Using YYLLOC is tempting, but would change the location of
     the lookahead.  YYLOC is available though.  */
  YYLLOC_DEFAULT (yyloc, yyerror_range, 2);
  *++yylsp = yyloc;

  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#if !defined yyoverflow || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval, &yylloc);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  yystos[*yyssp], yyvsp, yylsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  return yyresult;
}
#line 1569 "cu.y" /* yacc.c:1923  */

