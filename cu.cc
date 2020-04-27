/*
    cu.cc, Copyright (C) 2020 Peng-Hsien Lai

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

#include<vector>
#include<string>
#include<set>
#include<map>
#include<cstring>
#include "cu.h"
#include "cu.tab.h"
#include<cassert>
#define p_assert assert

#define yyparse cuparse
#define yylex   culex
#define yyerror cuerror
#define yylval  culval
#define yychar  cuchar
#define yydebug cudebug
#define yynerrs cunerrs
#define yytext cutext
#define yylineno culineno

string symbol_name;
string symbol_backup;
char   cu_err[256];
int  cu_abort = 0;
FILE*  cu_out = NULL;

symbol_value_t* symbol_value_t::malloc_int()        { return malloc_int<symbol_value_t>(); }
symbol_value_t* symbol_value_t::malloc_flt()        { return malloc_flt<symbol_value_t>(); }
symbol_value_t* symbol_value_t::malloc_str()        { return malloc_str<symbol_value_t>(); }
symbol_value_t* symbol_value_t::malloc_int_stacked(){ return malloc_int<stacked_symbol_value_t>(); }
symbol_value_t* symbol_value_t::malloc_flt_stacked(){ return malloc_flt<stacked_symbol_value_t>(); }
symbol_value_t* symbol_value_t::malloc_str_stacked(){ return malloc_str<stacked_symbol_value_t>(); }

symbol_value_t*  symbol_value_t::malloc_int(int dim, int* sz){
                          return malloc_int<symbol_value_t>(dim, sz);
                      }
symbol_value_t*  symbol_value_t::malloc_flt(int dim, int* sz){
                          return malloc_flt<symbol_value_t>(dim, sz);
                      }
symbol_value_t*  symbol_value_t::malloc_str(int dim, int* sz){
                          return malloc_str<symbol_value_t>(dim, sz);
                      }
symbol_value_t*  symbol_value_t::malloc_int_stacked(int dim, int* sz){
                          return malloc_int<stacked_symbol_value_t>(dim, sz);
                      }
symbol_value_t*  symbol_value_t::malloc_flt_stacked(int dim, int* sz){
                          return malloc_flt<stacked_symbol_value_t>(dim, sz);
                      }
symbol_value_t*  symbol_value_t::malloc_str_stacked(int dim, int* sz){
                          return malloc_str<stacked_symbol_value_t>(dim, sz);
                      }

int symbol_value_t::element_offset(expr_t* expr){
    int offset = 0;
    for(int i=0; i<dimension; ++i){
        int  sz = a[0].dims[i];
        int idx = expr->lhs_eval_int();
        if(((unsigned int)idx)>=(unsigned int)sz){
            fprintf(cu_out, "Error, index (%d) out of range [0,%d).\n", idx, sz);
            if(idx<0) idx = 0;
            else   idx = sz-1;
        }
        offset *= sz; 
        offset += idx;
        expr = expr->rhs;
    }
    return offset;
}
int* symbol_value_t::element_address_int(expr_t* expr){
    if(!stacked){
        return a[0].as_int + element_offset(expr);
    }else{
        return next[-1]->a[0].as_int + element_offset(expr);
    }
}
flt* symbol_value_t::element_address_flt(expr_t* expr){
    if(!stacked){
        return a[0].as_flt + element_offset(expr);
    }else{
        return next[-1]->a[0].as_flt + element_offset(expr);
    }
}
str_t* symbol_value_t::element_address_str(expr_t* expr){
    if(!stacked){
        return (str_t*)(a[0].as_str + element_offset(expr));
    }else{
        return (str_t*)(next[-1]->a[0].as_str + element_offset(expr));
    }
}

void symbol_value_t::clear(){
    if(dimension){
        free(a[0].dims);
        if(str_var){
            for(int i=0; i<total_elms; ++i){
                ((str_t*)(&(a[0].as_str[i])))->clear();
            }
        }
    }else{
        if(str_var){
            ((str_t*)(&scalar_str[0]))->clear();
        }
    }
}

void fill_array(int*& ptr, expr_t* list){
    if(list){
        if(list->eval_voi==&expr_t::array_init_list_node){
            fill_array(ptr, list->lhs);
            fill_array(ptr, list->rhs);
        }else{
            *ptr++ = list->self_eval_int();
        }
    }
}
void fill_array(flt*& ptr, expr_t* list){
    if(list){
        if(list->eval_voi==&expr_t::array_init_list_node){
            fill_array(ptr, list->lhs);
            fill_array(ptr, list->rhs);
        }else{
            *ptr++ = list->self_eval_flt();
        }
    }
}
void fill_array(str_t*& ptr, expr_t* list){
    if(list){
        if(list->eval_voi==&expr_t::array_init_list_node){
            fill_array(ptr, list->lhs);
            fill_array(ptr, list->rhs);
        }else{
            *ptr++ = list->self_eval_str();
        }
    }
}

void symbol_value_t::__reinitialize_int(expr_t* init_list){
    if(dimension){
        for(int i=0; i<total_elms; ++i){
            a[0].as_int[i] = 0; 
        }
        int* ptr = &(a[0].as_int[0]);
        fill_array(ptr, init_list);
    }else{
        scalar_int = (init_list?init_list->self_eval_int():0);
    }
}
void symbol_value_t::reinitialize_int(expr_t* init_list){
    if(!stacked){
        __reinitialize_int(init_list);
    }else{
        next[-1]->__reinitialize_int(init_list);
    }
}

void symbol_value_t::__reinitialize_flt(expr_t* init_list){
    if(dimension){
        for(int i=0; i<total_elms; ++i){
            a[0].as_flt[i] = 0; 
        }
        flt* ptr = &(a[0].as_flt[0]);
        fill_array(ptr, init_list);
    }else{
        scalar_flt = (init_list?init_list->self_eval_flt():0.0f);
    }
}
void symbol_value_t::reinitialize_flt(expr_t* init_list){
    if(!stacked){
        __reinitialize_flt(init_list);
    }else{
        next[-1]->__reinitialize_flt(init_list);
    }
}

void symbol_value_t::__reinitialize_str(expr_t* init_list){
    if(dimension){
        for(int i=0; i<total_elms; ++i){
            ((str_t*)&(a[0].as_str[i]))->clear();
        }
        str_t* ptr = (str_t*)&(a[0].as_str[0]);
        fill_array(ptr, init_list);
    }else{
        ((str_t*)&(scalar_str[0]))->clear();
        if(init_list){
            *((str_t*)&(scalar_str[0])) = init_list->self_eval_str();
        }
    }
}
void symbol_value_t::reinitialize_str(expr_t* init_list){
    if(!stacked){
        __reinitialize_str(init_list);
    }else{
        next[-1]->__reinitialize_str(init_list);
    }
}

void push_vars(symbol_value_t** ptr){
    for(;*ptr;++ptr){
        (*ptr)->push(); 
    }
}
void pop_vars(symbol_value_t** ptr){
    for(;*ptr;++ptr){
        (*ptr)->pop(); 
    }
}

int expr_statement_t::execute(){
    if(expr->flag.int_expr){
        int result = expr->self_eval_int();
    }else if(expr->flag.flt_expr){
        flt result = expr->self_eval_flt();
    }else if(expr->flag.str_expr){
        expr->self_eval_str();
    }
    if(cu_abort) return 0;
    if(next) return next->execute();
    return 0;
}

int int_expr_statement_t::execute(){
    int result = expr->self_eval_int();
    if(cu_abort) return 0;
    if(next) return next->execute();
    return 0;
}
int flt_expr_statement_t::execute(){
    flt result = expr->self_eval_flt();
    if(cu_abort) return 0;
    if(next) return next->execute();
    return 0;
}
int str_expr_statement_t::execute(){
    expr->self_eval_str();
    if(cu_abort) return 0;
    if(next) return next->execute();
    return 0;
}

int print_statement_t::execute(){
    if(expr->flag.str_expr){
        fprintf(cu_out, "%s\n", expr->self_eval_str().c_str());
    }else{
        int result = expr->self_eval_int();
        fprintf(cu_out, "%d\n", result);
    }
    if(cu_abort) return 0;
    if(next) return next->execute();
    return 0;
}

int printf_statement_t::execute(){
    expr_t* arg = arg_list;

    if(!fmt_expr){
        const char* sep = "";
        for(;arg;){
            if(arg->eval_voi==&expr_t::expr_list_extra_comma_node){
                fprintf(cu_out, "%s%d", sep, arg->lhs_eval_int());
                arg=arg->rhs;
            }else{
                fprintf(cu_out, "%s%d", sep, arg->self_eval_int());
                break;
            }
            sep = ", ";
        }
        fprintf(cu_out, "\n");
        p_assert(!next);
        return 0;
    }else if(fmt_expr->flag.str_expr){
        str_t fmt = fmt_expr->self_eval_str();
        char* fmt_str = fmt.c_str();
        char c;
        for(;c=*fmt_str; ++fmt_str){
            if(!arg){
                fprintf(cu_out, "%s" , fmt_str);
                break;
            }

            if(c!='%'){
                fputc(c, cu_out); 
                continue;
            }
            char* fmt = fmt_str;
            c=*++fmt_str;
            if(c=='%'){
                fputc('%', cu_out);
                continue;
            }

            int can_have_asterisk = 1;
            int has_asterisk = 0;

            do{
                if(can_have_asterisk && c=='*') has_asterisk = 1;
                if(c>='0') can_have_asterisk = 0;
            }while((c=*++fmt_str) && c!='%');

           *fmt_str = 0;
            if(!has_asterisk){
                if(arg->eval_voi == 
                       &expr_t::vargs_int_node)
                           fprintf(cu_out, fmt, arg->lhs_eval_int());
                if(arg->eval_voi ==
                       &expr_t::vargs_flt_node)
                           fprintf(cu_out, fmt, arg->lhs_eval_flt());
                if(arg->eval_voi ==
                       &expr_t::vargs_str_node)
                           fprintf(cu_out, fmt, arg->lhs_eval_str().c_str());
                arg = arg->rhs;
            }else{
                if(!arg->rhs){
                   *fmt_str = c;
                    fprintf(cu_out, "%s" , fmt);
                    break;
                }
                int num = (arg->lhs->flag.int_expr?arg->lhs_eval_int():0);
                arg = arg->rhs;
                if(arg->eval_voi == 
                       &expr_t::vargs_int_node)
                           fprintf(cu_out, fmt, num, arg->lhs_eval_int());
                if(arg->eval_voi ==
                       &expr_t::vargs_flt_node)
                           fprintf(cu_out, fmt, num, arg->lhs_eval_flt());
                if(arg->eval_voi ==
                       &expr_t::vargs_str_node)
                           fprintf(cu_out, fmt, num, arg->lhs_eval_str().c_str());
                arg = arg->rhs;
            }
           *fmt_str-- = c;
        }

    }else{
        if(fmt_expr->flag.int_expr)
          fprintf(cu_out, "%d", fmt_expr->self_eval_int());
        else
          fprintf(cu_out, "%g", fmt_expr->self_eval_flt());
    }

    for(;arg;arg=arg->rhs){
        if(arg->eval_voi==&expr_t::vargs_int_node)
            fprintf(cu_out, " %d", arg->lhs_eval_int());
        if(arg->eval_voi==&expr_t::vargs_flt_node)
            fprintf(cu_out, " %g", arg->lhs_eval_flt());
        if(arg->eval_voi==&expr_t::vargs_str_node)
            fprintf(cu_out, " %s", arg->lhs_eval_str().c_str());
    }

    if(cu_abort) return 0;
    if(next) return next->execute();
    return 0;
}

int puts_statement_t::execute(){
    fprintf(cu_out, "%s\n", expr->self_eval_str().c_str());
    if(cu_abort) return 0;
    if(next) return next->execute();
    return 0;
}

int if_statement_t::execute(){
    if(cond->self_eval_int()){
        int status = body->execute();
        if(status) return status;
    }else if(altr){
        int status = altr->execute();
        if(status) return status;
    }
    if(cu_abort) return 0;
    if(next) return next->execute();
    return 0;
}

int block_statement_t::execute(){
    int status = inner->execute();
    if(cu_abort) return 0;
    if(status) return status;
    if(next) return next->execute();
    return 0;
}

int for_statement_t::execute(){
    if(init) init->self_eval_int();
    for(
      init?init->self_eval_int():1
    ; cond?cond->self_eval_int():1
    ; post?post->self_eval_int():1
    ){
        int status = body->execute();
        if(status==K_break) break;
        if(status==K_return) return K_return;
        if(cu_abort) return 0;
    }
    if(next) return next->execute();
    return 0;
}

int while_statement_t::execute(){
    while(cond->self_eval_int()){
        int status = body->execute();
        if(status==K_break) break;
        if(status==K_return) return K_return;
        if(cu_abort) return 0;
    }
    if(next) return next->execute();
    return 0;
}

int do_statement_t::execute(){
    do{
        int status = body->execute();
        if(status==K_break) break;
        if(status==K_return) return K_return;
        if(cu_abort) return 0;
    }while(cond->self_eval_int());
    if(next) return next->execute();
    return 0;
}

int control_statement_t::execute(){
    return type;
}

int   returned_int;
flt   returned_flt;
str_t returned_str;
int return_statement_t::execute(){
    if(expr){
        if(expr->flag.int_expr){
            returned_int = expr->self_eval_int();
        }else if(expr->flag.flt_expr){
            returned_flt = expr->self_eval_flt();
        }else if(expr->flag.str_expr){
            returned_str = expr->self_eval_str();
        }
    }
    return K_return;
}

void (*app_goto)(int) = NULL;
int goto_statement_t::execute(){
    if(app_goto){
        (*app_goto)(expr->self_eval_int());
    }
    return K_return;
}

void delete_expr(expr_t* expr1, expr_t* expr2, expr_t* expr3){
    delete expr1;
    delete expr2;
    delete expr3;
}

expr_t::expr_t(const char* _nam, symbol_value_t* _sym){
    nam   = _nam;
    sym   = _sym;
    flags = 0;
    if(_sym->int_var){
        if(!_sym->dimension) if(!_sym->stacked) eval_int = &expr_t::variable_int;
                             else               eval_int = &expr_t::stacked_variable_int;
        else                 eval_voi = &expr_t::int_array_addressing_node;
        flag.int_expr = 1;
    }
    if(_sym->flt_var){
        if(!_sym->dimension) if(!_sym->stacked) eval_flt = &expr_t::variable_flt; 
                             else               eval_flt = &expr_t::stacked_variable_flt;
        else                 eval_voi = &expr_t::flt_array_addressing_node;
        flag.flt_expr = 1;
    }
    if(_sym->str_var){
        if(!_sym->dimension) if(!_sym->stacked) eval_str = &expr_t::variable_str; 
                             else               eval_str = &expr_t::stacked_variable_str;
        else                 eval_voi = &expr_t::str_array_addressing_node;
        flag.str_expr = 1;
    }
}

expr_t::~expr_t(){
    if(flag.free_lhs) delete lhs;
    if(flag.free_rhs) delete rhs;
    if(flag.free_reg){
        regfree(reg);
        free(reg);
    }
    if(flag.free_str) delete str;
}

int expr_t::zero_int(){
    return 0;
}
int expr_t::const_int(){
    return num;
}
int expr_t::variable_int(){
    return *int_adr;
}
int expr_t::stacked_variable_int(){
    return *(int_adr=&(sym->next[-1]->scalar_int));
}
int expr_t::array_addressing_int(){
    int_adr = rhs->sym->element_address_int(rhs->lhs);
    return *int_adr;
}
int expr_t::lt_int(){
    return lhs_eval_int() < rhs_eval_int();
}
int expr_t::gt_int(){
    return lhs_eval_int() > rhs_eval_int();
}
int expr_t::le_int(){
    return lhs_eval_int() <= rhs_eval_int();
}
int expr_t::ge_int(){
    return lhs_eval_int() >= rhs_eval_int();
}
int expr_t::eq_int(){
    return lhs_eval_int() == rhs_eval_int();
}
int expr_t::ne_int(){
    return lhs_eval_int() != rhs_eval_int();
}
int expr_t::flt_lt(){
    return lhs_eval_flt() < rhs_eval_flt();
}
int expr_t::flt_gt(){
    return lhs_eval_flt() > rhs_eval_flt();
}
int expr_t::flt_le(){
    return lhs_eval_flt() <= rhs_eval_flt();
}
int expr_t::flt_ge(){
    return lhs_eval_flt() >= rhs_eval_flt();
}
int expr_t::flt_eq(){
    return lhs_eval_flt() == rhs_eval_flt();
}
int expr_t::flt_ne(){
    return lhs_eval_flt() != rhs_eval_flt();
}
int expr_t::add_int(){
    return lhs_eval_int() + rhs_eval_int();
}
int expr_t::subtract_int(){
    return lhs_eval_int() - rhs_eval_int();
}
int expr_t::minus_int(){
    return -rhs_eval_int();
}
int expr_t::multiply_int(){
    return lhs_eval_int() * rhs_eval_int();
}
int expr_t::divide_int(){
    return lhs_eval_int() / rhs_eval_int();
}
int expr_t::mod_int(){
    return lhs_eval_int() % rhs_eval_int();
}
int expr_t::or_int(){
    return lhs_eval_int() | rhs_eval_int();
}
int expr_t::xor_int(){
    return lhs_eval_int() ^ rhs_eval_int();
}
int expr_t::and_int(){
    return lhs_eval_int() & rhs_eval_int();
}
int expr_t::shift_left_int(){
    return lhs_eval_int() << rhs_eval_int();
}
int expr_t::shift_right_int(){
    return lhs_eval_int() >> rhs_eval_int();
}
int expr_t::if_then_else_int(){
    return lhs_eval_int()
         ? rhs->lhs_eval_int()
         : rhs->rhs_eval_int();
}
int expr_t::assign_int(){
    lhs_eval_int();
    return *lhs->int_adr = rhs_eval_int();
}
int expr_t::assign_add_int(){
    lhs_eval_int();
    return *lhs->int_adr += rhs_eval_int();
}
int expr_t::assign_subtract_int(){
    lhs_eval_int();
    return *lhs->int_adr -= rhs_eval_int();
}
int expr_t::assign_multiply_int(){
    lhs_eval_int();
    return *lhs->int_adr *= rhs_eval_int();
}
int expr_t::assign_divide_int(){
    lhs_eval_int();
    return *lhs->int_adr /= rhs_eval_int();
}
int expr_t::assign_mod_int(){
    lhs_eval_int();
    return *lhs->int_adr %= rhs_eval_int();
}
int expr_t::assign_shift_left_int(){
    lhs_eval_int();
    return *lhs->int_adr <<= rhs_eval_int();
}
int expr_t::assign_shift_right_int(){
    lhs_eval_int();
    return *lhs->int_adr >>= rhs_eval_int();
}
int expr_t::assign_and_int(){
    lhs_eval_int();
    return *lhs->int_adr &= rhs_eval_int();
}
int expr_t::assign_xor_int(){
    lhs_eval_int();
    return *lhs->int_adr ^= rhs_eval_int();
}
int expr_t::assign_or_int(){
    lhs_eval_int();
    return *lhs->int_adr |= rhs_eval_int();
}
int expr_t::reinitialize_int(){
    lhs->sym->reinitialize_int(rhs);
    return 0; 
}
int expr_t::comma_int(){
    lhs_eval_int(); return rhs_eval_int();
}
int expr_t::logic_not_int(){
    return !rhs_eval_int();
}
int expr_t::not_int(){
    return ~rhs_eval_int();
}
int expr_t::logic_and_int(){
    return lhs_eval_int() && rhs_eval_int();
}
int expr_t::logic_or_int(){
    return lhs_eval_int() || rhs_eval_int();
}
int expr_t::post_incr_int(){
    lhs_eval_int();
    return (*lhs->int_adr)++;
}
int expr_t::pre_incr_int(){
    rhs_eval_int();
    return ++(*rhs->int_adr);
}
int expr_t::post_decr_int(){
    lhs_eval_int();
    return (*lhs->int_adr)--;
}
int expr_t::pre_decr_int(){
    rhs_eval_int();
    return --(*rhs->int_adr);
}
int expr_t::flt_to_int(){
    return (int) rhs_eval_flt();
}
int expr_t::atoi_int(){
    return atoi(rhs_eval_str().c_str());
}
int expr_t::strref_int(){
    int ref = 0; {
        ref = rhs_eval_str().ref();
    }
    return ref;
}
int expr_t::char_at_int(){
    str_t str = lhs_eval_str();
    int   idx = rhs_eval_int();
    if(idx<0) return 0;
    if(idx>str.size()) return 0;
    return *(str.c_str() + idx);
}
int expr_t::strlen_int(){
    return rhs_eval_str().size();
}
int expr_t::strcmp_int(){
    return strcmp(lhs_eval_str().c_str(), rhs_eval_str().c_str());
}
str_t      reg_test_string; 
regmatch_t reg_matches[10];
int expr_t::regexec_int(){
    reg_test_string = lhs_eval_str();
    int result = regexec(reg, reg_test_string.c_str(), 10, reg_matches, REG_EXTENDED);
    if(result!=REG_NOMATCH){
        return 1;
    }else{
        for(int i=0; i<10; ++i) reg_matches[i].rm_so = -1;
        return 0;
    }
}
int expr_t::function_call_int(){
    expr_t* para = fun->parameters;
    expr_t* args = rhs;
    for(;para; para=para->rhs, args=args->rhs){
        if(args->lhs->flag.int_expr){
            para->lhs->sym->scalar_int = args->lhs_eval_int();
        }
        if(args->lhs->flag.flt_expr){
            para->lhs->sym->scalar_flt = args->lhs_eval_flt();
        }
        if(args->lhs->flag.str_expr){
            *((str_t*)&(para->lhs->sym->scalar_str[0])) = args->lhs_eval_str();
        }
    }
    push_vars(fun->local_vars); 
    para = fun->parameters;
    for(;para;para=para->rhs){
        if(para->lhs->sym->int_var){
            para->lhs->sym->next[-1]->scalar_int = para->lhs->sym->scalar_int;
        }
        if(para->lhs->sym->flt_var){
            para->lhs->sym->next[-1]->scalar_flt = para->lhs->sym->scalar_flt;
        }
        if(para->lhs->sym->str_var){
            *((str_t*)&(para->lhs->sym->next[-1]->scalar_str[0])) = *((str_t*)&(para->lhs->sym->scalar_str[0]));
            ((str_t*)&(para->lhs->sym->scalar_str[0]))->clear();
        }
    }
    fun->statements->execute();
    pop_vars(fun->local_vars); 
    return returned_int;
}
int expr_t::get_line_int(){
    HsvLine* ptr = hsv_app_get_line(rhs_eval_int()-1);
    if(ptr){
        delete ptr;
        return 1;
    }
    return 0;
}

int expr_t::reinitialize_flt(){
    lhs->sym->reinitialize_flt(rhs);
    return 0; 
}
flt expr_t::const_flt(){
    return fnm;
}
flt expr_t::variable_flt(){
    return *flt_adr;
}
flt expr_t::stacked_variable_flt(){
    return *(flt_adr=&(sym->next[-1]->scalar_flt));
}
flt expr_t::array_addressing_flt(){
    flt_adr = rhs->sym->element_address_flt(rhs->lhs);
    return *flt_adr;
}
flt expr_t::assign_flt(){
    lhs_eval_flt();
    return *lhs->flt_adr = rhs_eval_flt();
}
flt expr_t::assign_add_flt(){
    lhs_eval_flt();
    return *lhs->flt_adr += rhs_eval_flt();
}
flt expr_t::assign_subtract_flt(){
    lhs_eval_flt();
    return *lhs->flt_adr -= rhs_eval_flt();
}
flt expr_t::assign_multiply_flt(){
    lhs_eval_flt();
    return *lhs->flt_adr *= rhs_eval_flt();
}
flt expr_t::assign_divide_flt(){
    lhs_eval_flt();
    return *lhs->flt_adr /= rhs_eval_flt();
}
flt expr_t::add_flt(){
    return lhs_eval_flt() + rhs_eval_flt();
}
flt expr_t::subtract_flt(){
    return lhs_eval_flt() - rhs_eval_flt();
}
flt expr_t::minus_flt(){
    return -rhs_eval_flt();
}
flt expr_t::multiply_flt(){
    return lhs_eval_flt() * rhs_eval_flt();
}
flt expr_t::divide_flt(){
    return lhs_eval_flt() / rhs_eval_flt();
}
flt expr_t::comma_flt(){
    lhs_eval_flt(); return rhs_eval_flt();
}
flt expr_t::int_to_flt(){
    return rhs_eval_int();
}
flt expr_t::atof_flt(){
    return atof(rhs_eval_str().c_str());
}
flt expr_t::function_call_flt(){
    function_call_int();
    return returned_flt;
}

int expr_t::reinitialize_str(){
    lhs->sym->reinitialize_str(rhs);
    return 0; 
}
str_t expr_t::variable_str(){
    return *str_adr;
}
str_t expr_t::stacked_variable_str(){
    return *(str_adr=(str_t*)&(sym->next[-1]->scalar_str[0]));
}
str_t expr_t::array_addressing_str(){
    str_adr = rhs->sym->element_address_str(rhs->lhs);
    return *str_adr;
}
str_t expr_t::comma_str(){
    lhs_eval_str(); return rhs_eval_str();
}
str_t expr_t::assign_str(){
    lhs_eval_str();
    return *lhs->str_adr = rhs_eval_str();
}
str_t expr_t::literal_str(){
    return *((str_t*) lhs);
}
str_t expr_t::if_then_else_str(){
    if(lhs_eval_int()){
        return rhs->lhs_eval_str();
    }else{
        return rhs->rhs_eval_str();
    }
}
str_t expr_t::get_submatch_str(){
    int begin = reg_matches[num].rm_so;
    int end   = reg_matches[num].rm_eo;
    if(begin>=0 && end > begin){
        return str_t(0, reg_test_string.c_str()+begin, end-begin);
    }else{
        return str_t();
    }
}
str_t expr_t::get_line_str(){
    HsvLine* ptr = hsv_app_get_line(rhs_eval_int()-1);
    if(ptr){
        HsvLine tmp = *ptr;
        delete  ptr;
        return str_t(0, tmp.c_str());
    }else{
        return str_t();
    }
}
str_t expr_t::shift_str(){
    return lhs_eval_str() + rhs_eval_int();
}
str_t expr_t::concatenate_str(){
    return lhs_eval_str() + rhs_eval_str();
}
str_t expr_t::int_to_str(){
    int i = rhs_eval_int();
    char buffer[16];
    sprintf(buffer, "%d", i);
    return str_t(0, buffer);
}
str_t expr_t::flt_to_str(){
    flt i = rhs_eval_flt();
    char buffer[16];
    sprintf(buffer, "%g", i);
    return str_t(0, buffer);
}
str_t expr_t::strstr_str(){
    str_t haystack = lhs_eval_str();
    char* haystack_c = haystack.c_str();
    char* result = strstr(haystack_c, rhs_eval_str().c_str());
    if(!result){ return str_t();}
    int offset = result - haystack_c;
    return haystack + offset;
}
str_t expr_t::strpbrk_str(){
    str_t haystack = lhs_eval_str();
    char* haystack_c = haystack.c_str();
    char* result = strpbrk(haystack_c, rhs_eval_str().c_str());
    if(!result){ return str_t();}
    int offset = result - haystack_c;
    return haystack + offset;
}
str_t expr_t::strchr_str(){
    str_t haystack = lhs_eval_str();
    char* haystack_c = haystack.c_str();
    char* result = strchr(haystack_c, rhs_eval_int());
    if(!result){ return str_t();}
    int offset = result - haystack_c;
    return haystack + offset;
}
str_t expr_t::strrchr_str(){
    str_t haystack = lhs_eval_str();
    char* haystack_c = haystack.c_str();
    char* result = strrchr(haystack_c, rhs_eval_int());
    if(!result){ return str_t();}
    int offset = result - haystack_c;
    return haystack + offset;
}
str_t expr_t::function_call_str(){
    function_call_int();
    str_t tmp = returned_str;
    returned_str.clear();
    return tmp;
}

void expr_t::array_dimension_node(){
    exit(1);
}
void expr_t::array_init_list_node(){
    exit(1);
}
void expr_t::ite_node(){
    exit(1);
}
void expr_t::int_array_addressing_node(){
    exit(1);
}
void expr_t::flt_array_addressing_node(){
    exit(1);
}
void expr_t::str_array_addressing_node(){
    exit(1);
}
void expr_t::vargs_int_node(){
    exit(1);
}
void expr_t::vargs_flt_node(){
    exit(1);
}
void expr_t::vargs_str_node(){
    exit(1);
}
void expr_t::expr_list_extra_comma_node(){
    exit(1);
}
void expr_t::array_subscript_node(){
    exit(1);
}
void expr_t::parameter_list_node(){
    exit(1);
}
void expr_t::argument_list_node(){
    exit(1);
}

int expr_t::is_const(){
    if(eval_voi!=&expr_t::array_dimension_node){
        if(!flag.int_expr) return 0;
        if(eval_int==&expr_t::zero_int)  return 1;
        if(eval_int==&expr_t::const_int) return 1;
        if(eval_int==&expr_t::variable_int) return 0;
        if(eval_int==&expr_t::stacked_variable_int) return 0;
        if(eval_int==&expr_t::array_addressing_int) return 0;
        if(eval_int==&expr_t::strlen_int) return 0;
        if(eval_int==&expr_t::atoi_int) return 0;
        if(eval_int==&expr_t::comma_int) return 0;
    }
    if(lhs && !lhs->is_const()) return 0;
    if(rhs && !rhs->is_const()) return 0;
    return 1;
}

statement_t* first_statement;

using namespace std;
typedef map<string,symbol_value_t*> scope_t;
vector<scope_t*> scope_stack;
vector<scope_t*> all_scopes;

void cu_push_scope(int shadow){
    scope_t* inner_scope = new scope_t(); 
    if(shadow){
        *inner_scope = *scope_stack.back();
        for(scope_t::iterator
            itr = inner_scope->begin()
         ;  itr!= inner_scope->end()
         ;++itr){
            itr->second = NULL;
        }
    }
    scope_stack.push_back(inner_scope);
}

void cu_pop_scope(){
    all_scopes.push_back(scope_stack.back());
    scope_stack.pop_back();
}

void* cu_top_scope(){
    return scope_stack.back();
}
void cu_pop_scope(void* scope){
    while(scope!=(void*)scope_stack.back()){
        all_scopes.push_back(scope_stack.back());
        scope_stack.pop_back();
    }
}
void cu_merge_scope(){
    scope_t* inner =  scope_stack.back(); scope_stack.pop_back();
    scope_t& outer = *scope_stack.back();
    for(scope_t::iterator
        itr  = inner->begin()
     ;  itr != inner->end()
     ;++itr){
        if(itr->second){
            outer[itr->first] = itr->second; 
        }
    }
    delete inner;
}

void push_function_scope(){
    all_scopes.push_back(NULL);
    scope_stack.push_back(new scope_t);
}
void delete_scope_t(scope_t* scope){
    if(!scope) return;
    for(scope_t::iterator
      itr  = scope->begin()
    ; itr != scope->end()
    ; itr ++ ){
        if(!itr->second) continue;
        itr->second->clear();
        if(!itr->second->stacked){
            free(itr->second);
        }else{
            free(&(itr->second->next[-1]));
        }
    }
    delete scope;
}

function_t* in_function_decl = NULL;

void cu_delete_scopes(){
    for(int i=0; i<all_scopes.size(); ++i){
        delete_scope_t(all_scopes[i]);
    }
    all_scopes.clear();
}

typedef map<string, function_t> function_tbl_t;
function_tbl_t function_tbl;
vector<function_tbl_t::iterator> new_funcs;

void register_function(string& name, cu_type return_type, expr_t* parameters, statement_t* statements){
    function_t& func = function_tbl[name];
    func.return_type = return_type;
    func.parameters  = parameters;
    func.statements  = statements;
    func.local_vars  = NULL;
    if(return_type==bad_type){
        in_function_decl = &func;
        push_function_scope();
        new_funcs.push_back(function_tbl.find(name));
        return;
    }
    if(!statements){
        return;
    }
    int k=all_scopes.size();
    int count = 0;
    while(all_scopes[k-1]!=NULL){
        count += all_scopes[k-1]->size();
        --k;
    }
    symbol_value_t** localvars = (symbol_value_t**) malloc(sizeof(symbol_value_t*)*(count+1));
    localvars[count] = NULL;
    while(1){
        scope_t* s = all_scopes.back(); all_scopes.pop_back(); 
        if(!s) break;
        for(scope_t::iterator
            itr  = s->begin()
         ;  itr != s->end()
         ;++itr){
            localvars[--count] = itr->second;
        }
        delete s;
    }
    func.local_vars  = localvars;
    in_function_decl = NULL;
}

void cu_delete_functions(){
    for(function_tbl_t::iterator
        itr = function_tbl.begin()
     ;  itr!= function_tbl.end()
     ;++itr){
        if(itr->second.parameters) delete itr->second.parameters;
        if(itr->second.statements) delete itr->second.statements;
        if(itr->second.local_vars){
            for(symbol_value_t *s, **ptr=itr->second.local_vars; s=*ptr; ++ptr){
                s->clear();
                if(!s->stacked){
                    free(s);
                }else{
                    free(&(s->next[-1])); 
                }
            }
            free(itr->second.local_vars);
        }
    }
    function_tbl.clear();
}

void cu_install_new_func(int install){
    if(!install){
        for(int i=0; i<new_funcs.size(); ++i){
            function_tbl_t::iterator itr = new_funcs[i];
            if(itr->second.parameters) delete itr->second.parameters;
            if(itr->second.statements) delete itr->second.statements;
            if(itr->second.local_vars){
                for(symbol_value_t *s, **ptr=itr->second.local_vars; s=*ptr; ++ptr){
                    s->clear();
                    if(!s->stacked){
                        free(s);
                    }else{
                        free(&(s->next[-1])); 
                    }
                }
                free(itr->second.local_vars);
            }
            function_tbl.erase(itr);
        }
    }else{
    }
    new_funcs.clear();
}

int lexer_query_symbol(const string& symbol){
    scope_t::iterator itr;
    for(int i=scope_stack.size()-1; i>=0; --i){
        itr = scope_stack[i]->find(symbol);
        if(itr!=scope_stack[i]->end()){
            if(itr->second==NULL) continue;

            if(itr->second->int_var){
                if(itr->second->dimension){
                    yylval.expr = new expr_t(itr->first.c_str(), itr->second);
                    return T_arr;
                }else{
                    yylval.expr = new expr_t(itr->first.c_str(), itr->second);
                    return T_id; 
                }
            }
            if(itr->second->flt_var){
                if(itr->second->dimension){
                    yylval.expr = new expr_t(itr->first.c_str(), itr->second);
                    return T_flt_arr;
                }else{
                    yylval.expr = new expr_t(itr->first.c_str(), itr->second);
                    return T_flt_id; 
                }
            }
            if(itr->second->str_var){
                if(itr->second->dimension){
                    yylval.expr = new expr_t(itr->first.c_str(), itr->second);
                    return T_str_arr;
                }else{
                    yylval.expr = new expr_t(itr->first.c_str(), itr->second);
                    return T_str_id; 
                }
            }
        }
    }
    map<string,function_t>::iterator func_itr = function_tbl.find(symbol);
    if(func_itr!=function_tbl.end() && func_itr->second.return_type!=bad_type){
        yylval.func       = &func_itr->second;
        yylval.func->name = &func_itr->first;
        switch(yylval.func->return_type){
          case int_type: return T_int_func;
          case flt_type: return T_flt_func;
          case str_type: return T_str_func;
          case voi_type: return T_function;
        }
    }
    yylval.expr = NULL;
    return T_new_id;
}

expr_t* push_var(const string& symbol, int lineno, expr_t* dim, cu_type type){
    if(type==voi_type){
        printf("Error, can not declare a variable with void type.\n");
        return NULL;
    }
    if(type==bad_type){
        printf("Error, declaring a variable without type.\n");
        exit(1);
    }
    scope_t::iterator itr;
    itr = scope_stack.back()->find(symbol);
    if(itr==scope_stack.back()->end()){
        if(dim){
            if(!dim->is_const()){
                sprintf(cu_err, "Error, on line %d, array size must be constant.\n", lineno);
                return NULL;
            }
            vector<int> dims;
            for(expr_t* d=dim; d; d=d->rhs){
                int sz=d->lhs_eval_int();
                if(sz<1){
                    sprintf(cu_err, "Error, on line %d, array size is not positive.\n", lineno);
                    return NULL;
                }
                dims.push_back(sz);
            }
            symbol_value_t* value = NULL; 
            if(!in_function_decl){
                switch(type){
                  case int_type: value = symbol_value_t::malloc_int(dims.size(), &(dims[0])); break;
                  case flt_type: value = symbol_value_t::malloc_flt(dims.size(), &(dims[0])); break;
                  case str_type: value = symbol_value_t::malloc_str(dims.size(), &(dims[0])); break;
                }
            }else{
                switch(type){
                  case int_type: value = symbol_value_t::malloc_int_stacked(dims.size(), &(dims[0])); break;
                  case flt_type: value = symbol_value_t::malloc_flt_stacked(dims.size(), &(dims[0])); break;
                  case str_type: value = symbol_value_t::malloc_str_stacked(dims.size(), &(dims[0])); break;
                }
            }
            (*scope_stack.back())[symbol] = value;
            return new expr_t(symbol.c_str(), value);
        }else{
            symbol_value_t* value = NULL;
            if(!in_function_decl){
                switch(type){
                  case int_type: value = symbol_value_t::malloc_int(); break;
                  case flt_type: value = symbol_value_t::malloc_flt(); break;
                  case str_type: value = symbol_value_t::malloc_str(); break;
                }
            }else{
                switch(type){
                  case int_type: value = symbol_value_t::malloc_int_stacked(); break;
                  case flt_type: value = symbol_value_t::malloc_flt_stacked(); break;
                  case str_type: value = symbol_value_t::malloc_str_stacked(); break;
                }
            }
            (*scope_stack.back())[symbol] = value;
            return new expr_t(symbol.c_str(), value);
        }
    }

    snprintf(cu_err, sizeof(cu_err), "Error, on line %d, name `%s' already used.\n", lineno, symbol.c_str());
    return NULL;
}

int verify_dimension(int* dims, expr_t* list){
    int sz = *dims;
    if(sz>-1){
        dims++;
        if(!list || list->eval_voi!=&expr_t::array_init_list_node){
            printf("array initialization list has inconsistent dimensions\n");
            return 0;
        }
        for(int i=0; i<sz; ++i){
            if(!list){
                printf("array initialization list is too short\n");
                return 0;        
            }
            if(!verify_dimension(dims, list->lhs)){
                return 0;
            }
            list = list->rhs;
        }
        if(list){
            printf("array initialization list is too long\n");
            return 0;
        }
        return 1;
    }else{
        if(list && list->eval_voi!=&expr_t::array_init_list_node){
            return 1;
        }
        printf("array initialization list has inconsistent dimensions\n");
        return 0;
    }
}

int dimension_consistent(expr_t* dim, expr_t* list){
    static vector<int> dims;
    dims.clear();
    for(expr_t* d=dim; d; d=d->rhs){
        int sz=d->lhs_eval_int();
        dims.push_back(sz);
    }
    dims.push_back(-1);
    return verify_dimension(&(dims[0]), list);
}

void yyerror(const char* msg){
    puts(msg);
}

int yyparse();

