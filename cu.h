/*
    cu.h, Copyright (C) 2020 Peng-Hsien Lai

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef __CU_H__
#define __CU_H__

#include<cassert>
#include<cstdio>
#include<cstdlib>
#include<cstring>
#include<string>
#include<sys/types.h>
#include<regex.h>
#include"hsv.h"

using std::string;

extern char  cu_err[256];
extern int cu_abort;
extern FILE* cu_out;

extern string symbol_name;
extern string symbol_backup;

extern char* cutext;
extern int   culineno;
void cuerror(const char*);

int cuparse_string(string&);
int cuparse_string(char*, int);
int  culex_begin(char*, int);
int  culex();
void culex_end();
extern vector<int> cu_token_stk;

struct expr_t;

typedef float flt;

enum cu_type{
    int_type,
    flt_type,
    str_type,
    voi_type,
    bad_type,
};

class str_detail_t;
struct str_raw_t{
    str_detail_t* d;
    int    off, len;
};
class str_t;

struct symbol_value_t{
private:
    symbol_value_t();
   ~symbol_value_t();
public:
    enum{ STACKED = 0 };

    symbol_value_t*   next[0]; 
    unsigned int dimension:26;
    unsigned int   int_var: 1;
    unsigned int   flt_var: 1;
    unsigned int   str_var: 1;
    unsigned int   ref_var: 1;
    unsigned int   stacked: 1;
    unsigned int   ocupied: 1;

    union{
        int scalar_int;
        flt scalar_flt;
        int total_elms; 
    };
    str_raw_t scalar_str[0];

    struct{
        int* dims;
        union{
            int as_int[0]; 
            flt as_flt[0];
            str_raw_t
                as_str[0];
        };
    }a[0];

    template<class T>
    static symbol_value_t* malloc_int(){
        symbol_value_t* ptr = (symbol_value_t*)&((T*)malloc(sizeof(T)))->next;
        ptr->dimension = 0;
        ptr->int_var   = 1;
        ptr->flt_var   = 0;
        ptr->str_var   = 0;
        ptr->ref_var   = 0;
        ptr->stacked   = T::STACKED;
        ptr->ocupied   = 0;
        return ptr;
    }
    static symbol_value_t* malloc_int();
    static symbol_value_t* malloc_int_stacked();

    template<class T>
    static symbol_value_t* malloc_flt(){
        symbol_value_t* ptr = (symbol_value_t*)&((T*)malloc(sizeof(T)))->next;
        ptr->dimension = 0;
        ptr->int_var   = 0;
        ptr->flt_var   = 1;
        ptr->str_var   = 0;
        ptr->ref_var   = 0;
        ptr->stacked   = T::STACKED;
        ptr->ocupied   = 0;
        return ptr;
    }
    static symbol_value_t* malloc_flt();
    static symbol_value_t* malloc_flt_stacked();

    template<class T>
    static symbol_value_t* malloc_str(){
        symbol_value_t* ptr = (symbol_value_t*) &((T*)
            malloc(sizeof(T)+sizeof(str_raw_t)))->next;
        ptr->dimension = 0;
        ptr->int_var   = 0;
        ptr->flt_var   = 0;
        ptr->str_var   = 1;
        ptr->ref_var   = 0;
        ptr->stacked   = T::STACKED;
        ptr->ocupied   = 0;
        ptr->scalar_str[0].d = NULL;
        ptr->scalar_str[0].off = 0;
        ptr->scalar_str[0].len = 0;
        return ptr;
    }
    static symbol_value_t* malloc_str();
    static symbol_value_t* malloc_str_stacked();


    template<class T>
    static symbol_value_t* malloc_int(int dim, int* sz){
        int total_elms = 1;
        for(int i=0; i<dim; ++i) total_elms *= sz[i];
        symbol_value_t* ptr = (symbol_value_t*) &((T*)
            malloc(sizeof(T)+sizeof(int*)+total_elms*sizeof(int)))->next;
        ptr->dimension  = dim;
        ptr->int_var    = 1;
        ptr->flt_var    = 0;
        ptr->str_var    = 0;
        ptr->ref_var    = 0;
        ptr->stacked    = T::STACKED;
        ptr->ocupied    = 0;
        ptr->total_elms = total_elms;
        ptr->a[0].dims  = (int*)::malloc(sizeof(int)*dim);
        for(int i=0; i<dim; ++i){
            ptr->a[0].dims[i] = sz[i];
        }
        return ptr;
    }
    static symbol_value_t* malloc_int(int dim, int* sz);
    static symbol_value_t* malloc_int_stacked(int dim, int* sz);

    template<class T>
    static symbol_value_t* malloc_flt(int dim, int* sz){
        int total_elms = 1;
        for(int i=0; i<dim; ++i) total_elms *= sz[i];
        symbol_value_t* ptr = (symbol_value_t*) &((T*)
            malloc(sizeof(T)+sizeof(int*)+total_elms*sizeof(flt)))->next;
        ptr->dimension  = dim;
        ptr->int_var    = 0;
        ptr->flt_var    = 1;
        ptr->str_var    = 0;
        ptr->ref_var    = 0;
        ptr->stacked    = T::STACKED;
        ptr->ocupied    = 0;
        ptr->total_elms = total_elms;
        ptr->a[0].dims  = (int*)::malloc(sizeof(int)*dim);
        for(int i=0; i<dim; ++i){
            ptr->a[0].dims[i] = sz[i];
        }
        return ptr;
    }
    static symbol_value_t* malloc_flt(int dim, int* sz);
    static symbol_value_t* malloc_flt_stacked(int dim, int* sz);

    template<class T>
    static symbol_value_t* malloc_str(int dim, int* sz){
        int total_elms = 1;
        for(int i=0; i<dim; ++i) total_elms *= sz[i];
        symbol_value_t* ptr = (symbol_value_t*) &((T*)
            malloc(sizeof(T)+sizeof(int*)+total_elms*sizeof(str_raw_t)))->next;
        ptr->dimension  = dim;
        ptr->int_var    = 0;
        ptr->flt_var    = 0;
        ptr->str_var    = 1;
        ptr->ref_var    = 0;
        ptr->stacked    = T::STACKED;
        ptr->ocupied    = 0;
        ptr->total_elms = total_elms;
        ptr->a[0].dims  = (int*)::malloc(sizeof(int)*dim);
        for(int i=0; i<dim; ++i){
            ptr->a[0].dims[i] = sz[i];
        }
        for(int i=0; i<total_elms; ++i){
            ptr->a[0].as_str[i].d   = NULL;
            ptr->a[0].as_str[i].off = 0;
            ptr->a[0].as_str[i].len = 0;
        }
        return ptr;
    }
    static symbol_value_t* malloc_str(int dim, int* sz);
    static symbol_value_t* malloc_str_stacked(int dim, int* sz);


    void push(){
        if(!stacked){exit(1);}
        if(!ocupied){
            next[-1] = NULL;
            ocupied = 1;
        }
        if(!dimension){
            symbol_value_t* top;
            if(int_var) top = malloc_int_stacked();
            if(flt_var) top = malloc_flt_stacked();
            if(str_var) top = malloc_str_stacked();
            top->next[-1] = next[-1];
            next[-1] = top;
        }else{
            symbol_value_t* top;
            if(int_var) top = malloc_int_stacked(dimension, a[0].dims);
            if(flt_var) top = malloc_flt_stacked(dimension, a[0].dims);
            if(str_var) top = malloc_str_stacked(dimension, a[0].dims);
            top->next[-1] = next[-1];
            next[-1] = top;
        }
    }
    void pop(){
        if(!stacked){exit(1);}
        symbol_value_t* top = next[-1];
        next[-1] = top->next[-1];
        top->clear();
        free(&(top->next[-1]));
    }

    void   reinitialize_int(expr_t*);
    void   reinitialize_flt(expr_t*);
    void   reinitialize_str(expr_t*);
    int    element_offset(expr_t* expr);
    int*   element_address_int(expr_t* expr);
    flt*   element_address_flt(expr_t* expr);
    str_t* element_address_str(expr_t* expr);
    void   clear();

private:
    void __reinitialize_int(expr_t*);
    void __reinitialize_flt(expr_t*);
    void __reinitialize_str(expr_t*);
};

struct stacked_symbol_value_t{
    enum{ STACKED = 1 };
    stacked_symbol_value_t* hidden;
    stacked_symbol_value_t* next[0];
    symbol_value_t          value;
};


void  cu_push_scope(int shadow=0);
void  cu_pop_scope();
void* cu_top_scope();
void  cu_pop_scope(void*);
void  cu_delete_scopes();

void  cu_delete_functions();
expr_t* push_var(const string& symbol, int lineno, expr_t* dim=NULL, cu_type type=int_type);
int lexer_query_symbol(const string& symbol);
int dimension_consistent(expr_t*, expr_t*);

void delete_expr(expr_t* expr1, expr_t* expr2=NULL, expr_t* expr3=NULL);

struct statement_t{
    statement_t* next;
    statement_t(){
        next = NULL;
    }
    virtual int execute(){
        if(next) next->execute();
        return 0;
    }
    virtual ~statement_t(){
        if(next) delete next;
    }
};
extern statement_t* first_statement;

struct function_t{
    const string*    name;
    cu_type          return_type;
    expr_t*          parameters;
    statement_t*     statements;
    symbol_value_t** local_vars;
};
void register_function(string&, cu_type return_type=bad_type, expr_t* parameters=NULL, statement_t* statements=NULL);

struct expr_statement_t: public statement_t{
    expr_t* expr;
    expr_statement_t(expr_t* _expr){
        expr = _expr;
    }
    virtual int execute();
   ~expr_statement_t(){
        delete_expr(expr); 
    }
};

struct int_expr_statement_t: public expr_statement_t{
    int_expr_statement_t(expr_t* _expr) : expr_statement_t(_expr) {}
    virtual int execute();
};
struct flt_expr_statement_t: public expr_statement_t{
    flt_expr_statement_t(expr_t* _expr) : expr_statement_t(_expr) {}
    virtual int execute();
};
struct str_expr_statement_t: public expr_statement_t{
    str_expr_statement_t(expr_t* _expr) : expr_statement_t(_expr) {}
    virtual int execute();
};

struct if_statement_t: public statement_t{
    expr_t*      cond;
    statement_t* body;
    statement_t* altr;
    if_statement_t(expr_t* _cond, statement_t* _body, statement_t* _altr){
        cond = _cond;
        body = _body;
        altr = _altr;
    }
    virtual int execute();
   ~if_statement_t(){
        delete_expr(cond);
        if(body) delete body;
        if(altr) delete altr;
    }
};

struct block_statement_t: public statement_t{
    statement_t* inner; 
    block_statement_t(statement_t* _inner){
        inner = _inner;
    }
    virtual int execute();
   ~block_statement_t(){
        if(inner) delete inner;
    }
};

struct for_statement_t: public statement_t{
    expr_t* init;        
    expr_t* cond;
    expr_t* post;
    statement_t* body;
    for_statement_t(expr_t* _init, expr_t* _cond, expr_t* _post, statement_t* _body){
        init = _init;
        cond = _cond;
        post = _post;
        body = _body;
    }
    virtual int execute();
   ~for_statement_t(){
        delete_expr(init);
        delete_expr(cond);
        delete_expr(post);
        if(body) delete body;
    }
};

struct while_statement_t: public statement_t{
    expr_t* cond;
    statement_t* body;
    while_statement_t(expr_t* _cond, statement_t* _body){
        cond = _cond;
        body = _body;
    }
    virtual int execute();
   ~while_statement_t(){
        delete_expr(cond);
        if(body) delete body;
    }
};

struct do_statement_t: public statement_t{
    expr_t* cond;
    statement_t* body;
    do_statement_t(expr_t* _cond, statement_t* _body){
        cond = _cond;
        body = _body;
    }
    virtual int execute();
   ~do_statement_t(){
        delete_expr(cond);
        if(body) delete body;
    }
};

struct control_statement_t: public statement_t{
    int type;
    control_statement_t(int _type){
        type = _type;
    }
    virtual int execute();
};

struct return_statement_t: public statement_t{
    expr_t* expr;
    return_statement_t(expr_t* _expr){
        expr = _expr;
    }
    virtual int execute();
   ~return_statement_t(){
        delete_expr(expr);
    }
};

struct goto_statement_t: public statement_t{
    expr_t* expr;
    goto_statement_t(expr_t* _expr){
        expr = _expr;
    }
    virtual int execute();
   ~goto_statement_t(){
        delete_expr(expr);
    }
};

struct print_statement_t: public statement_t{
    expr_t* expr;
    print_statement_t(expr_t* _expr){
        expr = _expr;
    }
    virtual int execute();
   ~print_statement_t(){
        delete_expr(expr);
    }
};

struct printf_statement_t: public statement_t{
    expr_t* fmt_expr;
    expr_t* arg_list;
    printf_statement_t(expr_t* _fmt, expr_t* _arg){
        fmt_expr = _fmt;
        arg_list = _arg; 
    }
    virtual int execute();
   ~printf_statement_t(){
        delete_expr(fmt_expr);
        delete_expr(arg_list);
    }
};

struct puts_statement_t: public statement_t{
    expr_t* expr;
    puts_statement_t(expr_t* _expr){
        expr = _expr;
    }
    virtual int execute();
   ~puts_statement_t(){
        delete_expr(expr);
    }
};

class str_detail_t{
    friend class str_t;
    str_detail_t(const str_detail_t&);
    str_detail_t& operator=(const str_detail_t&);

protected:
    char* ptr;
    int   ref;
    unsigned int own : 1;
    str_detail_t(char* _p, int _o=1){
        ptr = _p;
        ref =  1;
        own = _o;
    }
   ~str_detail_t(){
       if(own) free(ptr);
    }
};

class str_t : private str_raw_t{

    str_t(char* _s, int _l){
        d = new str_detail_t(_s);
        off = 0;
        len = _l;
    }
    str_t(str_detail_t* _d, int _o, int _l){
        d = _d;
        off = _o;
        len = _l;
        d->ref++;
    }
    str_t(char*);

public:

    str_t(){
        d = NULL;
        off = 0;
        len = 0;
    }

    str_t(const str_t& src){
        d = src.d;
        off = src.off;
        len = src.len;
        if(d){
            d->ref++; 
        }
    }

    str_t(int is_static, const char* str, int n=0){
        if(str){
            d = new str_detail_t(is_static?(char*)str:(n?strndup(str,n):strdup(str)), !is_static);
            off = 0;
            len = strlen(str);
        }else{
            d = NULL;
            off = 0;
            len = 0;
        }
    }

   ~str_t(){
        clear();
    }

    void clear(){
        if(!d) return;
        if(!--(d->ref)){
            delete d;
        }
        d = NULL;
        off = 0;
        len = 0;
    }

    str_t& operator=(const str_t& rhs){
        if(this==&rhs) return *this;
        clear();
        d = rhs.d;
        off = rhs.off;
        len = rhs.len;
        if(d){
            d->ref++;
        }
        return *this;
    }

    str_t& operator=(char* str){
        clear();
        d = new str_detail_t(strdup(str));
        off = 0;
        len = strlen(str);
        return *this;
    }

    str_t operator+(int a) const{
        if(d){
            return str_t(d, off+a, len-a); 
        }else{
            return str_t();
        }
    }

    str_t operator+(const str_t& rhs) const{
        if(d){
            int len = this->size()+rhs.size();
            if(len){
                char* str = (char*) malloc(len+1);
                memcpy(str, this->c_str(), this->size());
                memcpy(str + this->size(), rhs.c_str(), rhs.size()); 
                str[len] = 0;
                return str_t(str, len); 
            }else{
                return str_t();
            }
        }else{
            return rhs;
        }
    }

    char* c_str() const{
        if(d){
            if(off<0) return (char*)"";
            if(len<0) return (char*)"";
            return d->ptr + off;
        }else{
            return (char*)"";
        }
    }
    int size() const{
        if(d){
            if(off<0) return 0;
            if(len<0) return 0;
            return len;
        }else{
            return 0;
        }
    }
    int ref() const{
        if(d) return d->ref;
        return 0;
    }
};

struct expr_t{
    union{
        expr_t*         lhs;
        int             num; 
        flt             fnm;
        int*        int_adr;
        flt*        flt_adr;
        str_t*      str_adr;
        const char*     nam;
        HsvLine*        hsv;
        str_t*          str;
        function_t*     fun;
    };
    union{
        expr_t*         rhs;
        symbol_value_t* sym;
        regex_t*        reg;
        char*           msg;
    };
    union{
        int   (expr_t::*eval_int)();
        float (expr_t::*eval_flt)();
        str_t (expr_t::*eval_str)();
        void  (expr_t::*eval_voi)();
    };
    union{
        unsigned int flags;
        struct {
            unsigned int int_expr:1;
            unsigned int flt_expr:1;
            unsigned int str_expr:1;
            unsigned int free_lhs:1;
            unsigned int free_hsv:1;
            unsigned int free_rhs:1;
            unsigned int free_reg:1;
            unsigned int free_str:1;
            unsigned int     uvwx:8; 
        } flag;
    };

    inline int lhs_eval_int(){
        return (lhs->*lhs->eval_int)();
    }
    inline int rhs_eval_int(){
        return (rhs->*rhs->eval_int)();
    }
    inline int self_eval_int(){
        return (this->*eval_int)();
    }
    inline flt lhs_eval_flt(){
        return (lhs->*lhs->eval_flt)();
    }
    inline flt rhs_eval_flt(){
        return (rhs->*rhs->eval_flt)();
    }
    inline flt self_eval_flt(){
        return (this->*eval_flt)();
    }
  inline str_t lhs_eval_str(){
        return (lhs->*lhs->eval_str)();
    }
  inline str_t rhs_eval_str(){
        return (rhs->*rhs->eval_str)();
    }
  inline str_t self_eval_str(){
        return (this->*eval_str)();
    }

    expr_t(int _num){
        eval_int = &expr_t::const_int;
        num      = _num;
        rhs      = NULL;
        flags    = 0;
        flag.int_expr = 1;
    }
    expr_t(int (expr_t::*_ptr)(), expr_t* _lhs, expr_t* _rhs){
        eval_int = _ptr;
        lhs      = _lhs;
        rhs      = _rhs;
        flags    = 0;
        flag.int_expr = 1;
        flag.free_lhs = !!lhs;
        flag.free_rhs = !!rhs;
    }
    expr_t(int (expr_t::*_ptr)(), expr_t* _lhs, regex_t* _reg){
        eval_int = _ptr;
        lhs      = _lhs;
        reg      = _reg;
        flags    = 0;
        flag.int_expr = 1;
        flag.free_lhs = !!lhs;
        flag.free_reg = !!reg;
    }
    expr_t(int (expr_t::*_ptr)(), function_t* _fun, expr_t* _rhs, void*){
        eval_int = _ptr;
        fun      = _fun;
        rhs      = _rhs;
        flags    = 0;
        flag.int_expr = 1;
        flag.free_rhs = !!rhs;
    }
    expr_t(float _num){
        eval_flt = &expr_t::const_flt;
        fnm      = _num;
        rhs      = NULL;
        flags    = 0;
        flag.flt_expr = 1;
    }
    expr_t(float (expr_t::*_ptr)(), expr_t* _lhs, expr_t* _rhs){
        eval_flt = _ptr;
        lhs      = _lhs;
        rhs      = _rhs;
        flags    = 0;
        flag.flt_expr = 1;
        flag.free_lhs = !!lhs;
        flag.free_rhs = !!rhs;
    }
    expr_t(float (expr_t::*_ptr)(), function_t* _fun, expr_t* _rhs, void*){
        eval_flt = _ptr;
        fun      = _fun;
        rhs      = _rhs;
        flags    = 0;
        flag.flt_expr = 1;
        flag.free_rhs = !!rhs;
    }
    expr_t(str_t* _str, int _free=1){
        eval_str = &expr_t::literal_str;
        str      = _str;
        rhs      = NULL;
        flags    = 0;
        flag.str_expr = 1;
        flag.free_str = _free;
    }
    expr_t(str_t (expr_t::*_ptr)(), expr_t* _lhs, expr_t* _rhs){
        eval_str = _ptr;
        lhs      = _lhs;
        rhs      = _rhs;
        flags    = 0;
        flag.str_expr = 1;
        flag.free_lhs = !!lhs;
        flag.free_rhs = !!rhs;
    }
    expr_t(str_t (expr_t::*_ptr)(), function_t* _fun, expr_t* _rhs, void*){
        eval_str = _ptr;
        fun      = _fun;
        rhs      = _rhs;
        flags    = 0;
        flag.str_expr = 1;
        flag.free_rhs = !!rhs;
    }
    expr_t(str_t (expr_t::*_ptr)(), int _num){
        eval_str = _ptr;
        num      = _num;
        rhs      = NULL;
        flags    = 0;
        flag.str_expr = 1;
    }
    expr_t(void (expr_t::*_ptr)()=NULL, expr_t* _lhs=NULL, expr_t* _rhs=NULL){
        eval_voi = _ptr;
        lhs      = _lhs;
        rhs      = _rhs;
        flags    = 0;
        flag.free_lhs = !!lhs;
        flag.free_rhs = !!rhs;
    }
    expr_t(const char* _nam, symbol_value_t* _sym);
    int is_const();
   ~expr_t();

    int zero_int();
    int const_int();
    int variable_int();
    int stacked_variable_int();
    int array_addressing_int();
    int lt_int();
    int gt_int();
    int le_int();
    int ge_int();
    int eq_int();
    int ne_int();
    int flt_lt();
    int flt_gt();
    int flt_le();
    int flt_ge();
    int flt_eq();
    int flt_ne();
    int add_int();
    int subtract_int();
    int minus_int();
    int multiply_int();
    int divide_int();
    int mod_int();
    int or_int();
    int xor_int();
    int and_int();
    int shift_left_int();
    int shift_right_int();
    int if_then_else_int();
    int assign_int();
    int assign_add_int();
    int assign_subtract_int();
    int assign_multiply_int();
    int assign_divide_int();
    int assign_mod_int();
    int assign_shift_left_int();
    int assign_shift_right_int();
    int assign_and_int();
    int assign_xor_int();
    int assign_or_int();
    int reinitialize_int();
    int comma_int();
    int logic_not_int();
    int not_int();
    int logic_and_int();
    int logic_or_int();
    int post_incr_int();
    int pre_incr_int();
    int post_decr_int();
    int pre_decr_int();

    int flt_to_int();
    int atoi_int();
    int strref_int();
    int char_at_int();
    int strlen_int();
    int strcmp_int();
    int regexec_int();
    int function_call_int();
    int get_line_int();
   
    int reinitialize_flt();
    flt const_flt(); 
    flt variable_flt();
    flt stacked_variable_flt();
    flt array_addressing_flt();
    flt assign_flt();
    flt assign_add_flt();
    flt assign_subtract_flt();
    flt assign_multiply_flt();
    flt assign_divide_flt();
    flt add_flt();
    flt subtract_flt();
    flt minus_flt();
    flt multiply_flt();
    flt divide_flt();
    flt comma_flt();
    flt int_to_flt();
    flt atof_flt();
    flt function_call_flt();

    int reinitialize_str();
  str_t variable_str();
  str_t stacked_variable_str();
  str_t array_addressing_str();
  str_t comma_str();
  str_t assign_str();
  str_t literal_str();
  str_t if_then_else_str();
  str_t get_submatch_str();
  str_t get_line_str();
  str_t shift_str();
  str_t concatenate_str();
  str_t int_to_str();
  str_t flt_to_str();
  str_t strstr_str();
  str_t strpbrk_str();
  str_t strchr_str();
  str_t strrchr_str();
  str_t function_call_str();
  
   void array_dimension_node();
   void array_init_list_node();
   void ite_node();
   void int_array_addressing_node();
   void flt_array_addressing_node();
   void str_array_addressing_node();
   void vargs_int_node();
   void vargs_flt_node();
   void vargs_str_node();
   void expr_list_extra_comma_node();
   void array_subscript_node();
   void parameter_list_node();
   void argument_list_node();
};

expr_t* const null_expr=NULL;

extern int cu_symbol_begin;
extern int cu_symbol_end;
struct culloc_t{
    int lineno;
    int begin;
    int end;
};
extern int err_symbol_begin;
extern int err_symbol_end;

#endif
