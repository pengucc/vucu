/*
    hsv.h, Copyright (C) 2020 Peng-Hsien Lai

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

#ifndef __HSV_H__
#define __HSV_H__
#include<sys/types.h>
#include<sys/time.h>
#include<sys/resource.h>
#include<unistd.h>
#include<cstdio>
#include<cstdlib>
#include<vector>

using std::vector;

#define HSV_BLOCK_SIZE (1<<15)

class HsvStorage{

    friend class HsvBlock;
    friend class HsvFile;

    int   refc;
    char* data;

    HsvStorage* next;
    HsvStorage* prev;

static HsvStorage unused;

public:
    HsvStorage(){
        refc = 0;
        data = 0;
        next = this;
        prev = this;
    }
   ~HsvStorage(){
        if(refc!=0){
            printf("Error: deleting an HsvStorage with ref > 0\n"); exit(1);
        }
        if(data){
            printf("Error: deleting an HsvStorage with data not cleared\n"); exit(1);
        }
    }
    void unref(){
        if(!--refc){
            next = unused.next;
            next->prev  = this;
            unused.next = this;
            prev = &unused;
        }
    }
    void ref(){
        if(!refc++ && next){
            prev->next = next;
            next->prev = prev;
            prev = NULL;
            next = NULL;
        }
    }
};

class HsvLine{

    friend class HsvBlock;

    HsvStorage* storage; 
    const char* line;

public:
    HsvLine(){
        storage = NULL;
        line    = NULL;
    }
    HsvLine(HsvStorage* __storage, const char* __line){
        storage = __storage;
        line    = __line;
        storage->ref();
    }
    HsvLine(const HsvLine& copy){
        storage = copy.storage;
        line    = copy.line;
        if(storage) storage->ref();
    }
    HsvLine& operator=(const HsvLine& rhs){
        if(this!=&rhs){
            if(storage) storage->unref();
            storage = rhs.storage;
            line    = rhs.line;
            if(storage) storage->ref();
        }
        return *this;
    }
   ~HsvLine(){
        if(storage) storage->unref();
    }
    const char* c_str(){
        return line;
    }
};


struct raw_block_list_t{
    raw_block_list_t* next;
};
class HsvBlock : public HsvStorage{

    friend class HsvFile;
    
    char*       tail_line;
    vector<int> line_start_offset;
    int         number_of_newline;
    int         block_size;
    int         aligned;
    int         newline_terminated;

    static int   block_inuse;
    static raw_block_list_t* freed;
public:
    static int   block_limit;
    static char* get_raw_data_block(){
        if(freed){
            char* ptr = (char*) freed; 
            freed = freed->next;
            block_inuse++;
            return ptr;
        }
        struct rusage usage;
        if((getrusage(RUSAGE_SELF,&usage))){
            printf("Error, getrusage failed\n"); exit(1); 
        }
        if(block_limit<0 || usage.ru_maxrss<(HSV_BLOCK_SIZE>>10)*block_limit || block_inuse<100){
            block_inuse++;
            return (char*) malloc(HSV_BLOCK_SIZE);
        }
        HsvBlock* stg = (HsvBlock*)unused.prev;
        if(stg!=&unused){
            if(stg->refc){
                printf("Error: block stil has references\n"); exit(1);
            }
            stg->prev->next = stg->next;
            stg->next->prev = stg->prev;
            stg->next = NULL;
            stg->prev = NULL;
            char* borrowed = stg->data;
            stg->data = NULL;
            if(!stg->newline_terminated && stg->tail_line){
                free(stg->tail_line);
            }
            stg->tail_line = NULL;
            vector<int> tmp;
            stg->line_start_offset.swap(tmp);
            return borrowed;
        }
        printf("Error: out of memory. %ldMB. (%d/%d)\n", usage.ru_maxrss>>10, block_inuse, block_limit);
        exit(1);
    }
    static void return_raw_data_block(char* data){
        raw_block_list_t* ptr = (raw_block_list_t*) data; 
        ptr->next = freed;
        freed = ptr;
        block_inuse--;
    }

    HsvBlock(char* _data, int _block_size, int _aligned){
        data      = _data;
        tail_line = NULL;
        number_of_newline = 0; 
        block_size = _block_size;
        aligned = _aligned;
        newline_terminated = 0;
    }
   ~HsvBlock(){
        unload();
    }

    HsvLine get_line_of_block(long linenum){
        if(!data || !tail_line){
            printf("Error: data not loaded\n"); exit(1);
        }else if(linenum<0 || linenum>number_of_newline){
            printf("Error: bad linenum %ld\n", linenum); exit(1);
        }
        if((linenum+1)==number_of_newline){
            return HsvLine(this, tail_line);
        }else{
            return HsvLine(this, data+line_start_offset[linenum]);
        }
    }

    void unload(){
        if(refc){
            printf("Error: block stil has references\n"); exit(1);
        }
        if(!data) return;
        if(next){
            prev->next = next;
            next->prev = prev;
            prev = NULL;
            next = NULL;
        }
        HsvBlock::return_raw_data_block(data);
        data = NULL;
        if(!newline_terminated && tail_line){
            free(tail_line);
        }
        tail_line = NULL;
        vector<int> tmp;
        line_start_offset.swap(tmp);
    }

    static void free_pool(){
        for(;freed;){
            raw_block_list_t* ptr = freed;
            freed = freed->next; 
            free(ptr);
        }
    }
};

class HsvFile{
    vector<long>      first_line_of_block;
    vector<long>      file_offset_of_block;
    vector<HsvBlock*> blocks;
    long              number_of_lines;
    int               f;
    HsvFile(int f){
        this->f = f;
    }
public:
   ~HsvFile(){
       close(f);
       for(int i=0; i<blocks.size(); ++i)
         delete blocks[i];
    }

    int find_block_of_line_imp(long linenum){
        long l_idx = 0;
        long l = 0;
        long r_idx = first_line_of_block.size()-1;
        long r     = first_line_of_block.back();
        while(1){
            if(l_idx==(r_idx-1)) return l_idx;
            long m_idx = ((l_idx+r_idx)>>1);
            long m = first_line_of_block[m_idx];
            if(linenum<m){
                r_idx = m_idx;
                r = m;
            }else{
                l_idx = m_idx;
                l = m;
            }
        }
    }

    int find_block_of_line(long linenum){
        int block = find_block_of_line_imp(linenum);
        if(block<0){
            printf("Error block < 0\n");
            exit(1); 
        }
        if(block>=file_offset_of_block.size()){
            printf("Error while finding line %ld\n", linenum);
            printf("Got block %d, but block index must between [0,%ld)\n", block, file_offset_of_block.size());
            exit(1);
        }
        if(first_line_of_block[block]>linenum){
            printf("Error while finding line %ld\n", linenum);
            printf("Error block %d\n", block);
            printf("first line of block[%d] is %ld\n", block, first_line_of_block[block]);
            exit(1);
        }
        if(linenum>=first_line_of_block[block+1]){
            printf("Error while finding line %ld\n", linenum);
            printf("Error block %d\n", block);
            printf("first line of block[%d+1] is %ld\n", block, first_line_of_block[block+1]);
            exit(1);
        }
        return block;
    }

    HsvBlock* load_block(int index, bool load_tail_line=true){
        HsvBlock* block = blocks[index];
        if(!block->data){
            block->data = HsvBlock::get_raw_data_block();
            if(lseek64(f, file_offset_of_block[index], SEEK_SET)<0){
                printf("Error: lseek64 failed\n"); exit(1);
            }
            int sz = 0;
            while(sz<HSV_BLOCK_SIZE){
                int status = read(f, block->data+sz, HSV_BLOCK_SIZE-sz);
                if(status>0){
                    sz += status;
                }else if(status==0){
                    break;
                }else{
                    printf("Error: read failed\n"); exit(1);
                }
            }
            if(block->block_size != sz){
                printf("Error: block size mismatched (%d <-> %d)\n", block->block_size, sz);
                exit(1);
            }
            int line_started = block->aligned;
            for(int i=0; i<sz; ++i){
                char c;
                if( line_started ){
                    line_started = 0;
                    block->line_start_offset.push_back(i); 
                }
                c = block->data[i];
                if(!c || c=='\n'){
                    block->data[i] = 0;
                    line_started   = 1;
                }
            }
            if(sz<HSV_BLOCK_SIZE) block->data[sz] = 0;
            if(block->line_start_offset.size()!=block->number_of_newline){
                printf("Error: number of lines mismatched (%ld <-> %ld)\n", block->line_start_offset.size(), number_of_lines);
                exit(1);
            }
            if(block->newline_terminated){
                block->tail_line = block->data + block->line_start_offset.back(); 
            }
        }
        if(!block->tail_line && load_tail_line){
            if((index+1)==file_offset_of_block.size()){
                if(block->block_size==HSV_BLOCK_SIZE){
                    int len = HSV_BLOCK_SIZE - block->line_start_offset.back();
                    block->tail_line = (char*) malloc(len+1);
                    memcpy(block->tail_line,  block->data 
                                            + block->line_start_offset.back(), len);
                    block->tail_line[len] = 0;
                }else{
                }
            }else{
                HsvBlock* block2 = load_block(index+1, false);
                int length1 = HSV_BLOCK_SIZE - block->line_start_offset.back();
                int length2 = block2->line_start_offset.front();
                block->tail_line = (char*) malloc(length1+length2);
                memcpy(block->tail_line        , block->data + block->line_start_offset.back(), length1);
                memcpy(block->tail_line+length1, block2->data, length2);
                block2->unref();
            }
        }
        block->ref();
        return block;
    }

    HsvLine get_line(long linenum){
        int       index = find_block_of_line(linenum);
        HsvBlock* block = load_block(index);
        HsvLine line = block->get_line_of_block(linenum-first_line_of_block[index]);
        block->unref();
        return line;
    }
    long get_number_of_lines(){
        return number_of_lines;
    }

    void test(){
        FILE* f = fopen("dump.txt", "w");
        for(long i=0; i<number_of_lines; ++i){
            fprintf(f, "%s\n", get_line(i).c_str());
        }
        fclose(f);
    }

    static
    HsvFile* load_file(int f, long file_size);
};

HsvLine* hsv_app_get_line(long);

#endif

